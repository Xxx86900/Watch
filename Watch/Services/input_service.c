/**
 * @file    input_service.c
 * @brief   Watch input service implementation
 *
 * 本模块位于 board 与 UI / App 层之间。
 *
 * 数据流：
 *
 * Physical Key
 *      |
 *      v
 * board_key_is_pressed()
 *      |
 *      v
 * input_service
 *      |
 *      +-- debounce
 *      +-- press duration
 *      +-- short press
 *      +-- long press
 *      |
 *      v
 * InputEvent_t
 *      |
 *      v
 * UI / App
 *
 * 本模块不关心按键连接在哪个 GPIO，
 * 也不负责执行菜单移动、页面切换等 UI 行为。
 */

#include "input_service.h"

#include "board.h"

#include <stddef.h>


/**
 * @brief 软件消抖时间
 *
 * 机械按键发生状态变化后，需要连续稳定达到该时间，
 * 才认为真正发生了按下或释放。
 */
#define INPUT_DEBOUNCE_TIME_MS       20U


/**
 * @brief 长按判定时间
 *
 * 按键持续按住达到该时间后产生 LONG_PRESS 事件。
 */
#define INPUT_LONG_PRESS_TIME_MS     800U


/**
 * @brief 输入事件队列长度
 */
#define INPUT_EVENT_QUEUE_SIZE       8U


/**
 * @brief 单个按键内部运行状态
 */
typedef struct
{
    /**
     * 最近一次读取到的原始按键状态。
     */
    bool raw_pressed;

    /**
     * 完成消抖后的稳定按键状态。
     */
    bool stable_pressed;

    /**
     * 原始状态最后一次发生变化的时刻。
     */
    uint32_t raw_change_time_ms;

    /**
     * 按键正式确认按下的时刻。
     */
    uint32_t press_start_time_ms;

    /**
     * 当前这次按压是否已经产生过长按事件。
     */
    bool long_press_generated;

} InputKeyState_t;


/**
 * @brief 所有逻辑按键的内部状态
 */
static InputKeyState_t s_key_states[INPUT_KEY_COUNT];


/**
 * @brief 输入事件循环队列
 */
static InputEvent_t s_event_queue[INPUT_EVENT_QUEUE_SIZE];

static uint8_t s_queue_head;
static uint8_t s_queue_tail;
static uint8_t s_queue_count;


/**
 * @brief 读取指定逻辑按键当前的物理状态
 *
 * 本函数负责建立：
 *
 * Logical Key -> Physical Key
 *
 * 的映射关系。
 *
 * 如果以后需要交换实体按键功能，只需要修改这里，
 * 上层 UI 不需要改变。
 *
 * @param[in] key 逻辑按键
 *
 * @return bool
 * @retval true  按键当前被按下
 * @retval false 按键当前处于松开状态
 */
static bool input_read_key(InputKey_t key)
{
    switch (key)
    {
        case INPUT_KEY_PREV:
            return board_key_is_pressed(BOARD_KEY_PREV);

        case INPUT_KEY_NEXT:
            return board_key_is_pressed(BOARD_KEY_NEXT);

        case INPUT_KEY_OK:
            return board_key_is_pressed(BOARD_KEY_OK);

        default:
            return false;
    }
}


/**
 * @brief 向输入事件队列添加事件
 *
 * 如果队列已经满，则丢弃最新产生的事件，
 * 避免覆盖尚未被 UI 处理的旧事件。
 *
 * @param[in] key  产生事件的按键
 * @param[in] type 事件类型
 *
 * @return bool
 * @retval true  事件成功加入队列
 * @retval false 队列已满
 */
static bool input_push_event(InputKey_t key, InputEventType_t type)
{
    if (s_queue_count >= INPUT_EVENT_QUEUE_SIZE)
    {
        return false;
    }

    s_event_queue[s_queue_tail].key = key;
    s_event_queue[s_queue_tail].type = type;
    s_queue_tail++;

    if (s_queue_tail >= INPUT_EVENT_QUEUE_SIZE)
    {
        s_queue_tail = 0U;
    }

    s_queue_count++;

    return true;
}


/**
 * @brief 更新单个按键状态
 *
 * 完成：
 *
 * 1. 原始状态变化检测；
 * 2. 软件消抖；
 * 3. 按下时间记录；
 * 4. 长按判断；
 * 5. 松开后的短按判断。
 *
 * @param[in] key    逻辑按键
 * @param[in] now_ms 当前系统时间
 *
 * @return void
 */
static void input_update_key(InputKey_t key, uint32_t now_ms)
{
    InputKeyState_t *state = &s_key_states[key];

    bool current_raw_pressed = input_read_key(key);


    /*
     * --------------------------------------------------------
     * Step 1：检测原始 GPIO 状态是否发生变化
     * --------------------------------------------------------
     *
     * 每次发现原始状态改变，都重新开始消抖计时。
     */
    if (current_raw_pressed != state->raw_pressed)
    {
        state->raw_pressed = current_raw_pressed;
        state->raw_change_time_ms = now_ms;
    }


    /*
     * --------------------------------------------------------
     * Step 2：软件消抖
     * --------------------------------------------------------
     *
     * 如果原始状态已经连续保持 INPUT_DEBOUNCE_TIME_MS，
     * 并且与当前稳定状态不同，则确认按键真正发生状态变化。
     */
    if (state->raw_pressed != state->stable_pressed)
    {
        if ((uint32_t)(now_ms - state->raw_change_time_ms) >=
            INPUT_DEBOUNCE_TIME_MS)
        {
            state->stable_pressed = state->raw_pressed;


            /*
             * ------------------------------------------------
             * 稳定状态：松开 -> 按下
             * ------------------------------------------------
             */
            if (state->stable_pressed)
            {
                state->press_start_time_ms = now_ms;
                state->long_press_generated = false;
            }


            /*
             * ------------------------------------------------
             * 稳定状态：按下 -> 松开
             * ------------------------------------------------
             */
            else
            {
                /*
                 * 如果这一轮按压没有触发过长按，
                 * 那么释放时将它解释为一次短按。
                 */
                if (!state->long_press_generated)
                {
                    (void)input_push_event(
                        key,
                        INPUT_EVENT_SHORT_PRESS
                    );
                }
            }
        }
    }


    /*
     * --------------------------------------------------------
     * Step 3：长按判断
     * --------------------------------------------------------
     *
     * 只有：
     *
     * - 按键稳定处于按下状态
     * - 当前按压尚未产生长按事件
     *
     * 才进行长按时间判断。
     */
    if (state->stable_pressed &&
        !state->long_press_generated)
    {
        if ((uint32_t)(now_ms - state->press_start_time_ms) >=
            INPUT_LONG_PRESS_TIME_MS)
        {
            (void)input_push_event(
                key,
                INPUT_EVENT_LONG_PRESS
            );

            /*
             * 当前这一次按住只允许产生一次长按事件。
             */
            state->long_press_generated = true;
        }
    }
}


/**
 * @brief 初始化输入服务
 *
 * @return void
 */
void input_service_init(void)
{
    s_queue_head = 0U;
    s_queue_tail = 0U;
    s_queue_count = 0U;

    for (uint8_t i = 0U; i < (uint8_t)INPUT_KEY_COUNT; ++i)
    {
        InputKey_t key = (InputKey_t)i;

        bool pressed = input_read_key(key);

        /*
         * 初始化时让 raw 和 stable 保持一致，
         * 避免刚开机时凭空产生一次按键事件。
         */
        s_key_states[i].raw_pressed = pressed;
        s_key_states[i].stable_pressed = pressed;

        s_key_states[i].raw_change_time_ms = 0U;
        s_key_states[i].press_start_time_ms = 0U;
        s_key_states[i].long_press_generated = false;
    }
}


/**
 * @brief 更新所有输入按键
 *
 * @param[in] now_ms 当前系统运行时间，单位 ms
 *
 * @return void
 */
void input_service_update(uint32_t now_ms)
{
    for (uint8_t i = 0U; i < (uint8_t)INPUT_KEY_COUNT; ++i)
    {
        input_update_key((InputKey_t)i, now_ms);
    }
}


/**
 * @brief 获取一个输入事件
 *
 * @param[out] event 输入事件
 *
 * @return bool
 */
bool input_service_get_event(InputEvent_t *event)
{
    if (event == NULL)
    {
        return false;
    }

    if (s_queue_count == 0U)
    {
        event->key = INPUT_KEY_PREV;
        event->type = INPUT_EVENT_NONE;

        return false;
    }

    *event = s_event_queue[s_queue_head];

    s_queue_head++;

    if (s_queue_head >= INPUT_EVENT_QUEUE_SIZE)
    {
        s_queue_head = 0U;
    }

    s_queue_count--;

    return true;
}


/**
 * @brief 清空输入事件队列
 *
 * @return void
 */
void input_service_clear_events(void)
{
    s_queue_head = 0U;
    s_queue_tail = 0U;
    s_queue_count = 0U;
}