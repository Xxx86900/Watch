/**
 * @file    input_service.h
 * @brief   Watch input service interface
 *
 * 本模块负责处理手表物理按键产生的输入行为。
 *
 * 主要功能：
 * 1. 对物理按键进行软件消抖；
 * 2. 检测按键按下和释放过程；
 * 3. 区分短按和长按；
 * 4. 将硬件按键转换为统一的输入事件；
 * 5. 向 UI / App 层提供输入事件。
 *
 * 本模块不直接操作 GPIO。
 * GPIO 电平读取统一由 board 模块负责。
 */

#ifndef INPUT_SERVICE_H
#define INPUT_SERVICE_H

#include <stdbool.h>
#include <stdint.h>


/**
 * @brief Logical key definition
 *
 * 这里描述的是用户看到的“逻辑按键”，
 * 而不是具体 GPIO 或 KEY1/KEY2/KEY3。
 */
typedef enum
{
    INPUT_KEY_PREV = 0,
    INPUT_KEY_NEXT,
    INPUT_KEY_OK,

    INPUT_KEY_COUNT
} InputKey_t;


/**
 * @brief Input event type
 */
typedef enum
{
    INPUT_EVENT_NONE = 0,

    /**
     * 按键短按。
     *
     * 按下时间未达到长按阈值，
     * 并在释放时产生该事件。
     */
    INPUT_EVENT_SHORT_PRESS,

    /**
     * 按键长按。
     *
     * 按住超过长按阈值后产生一次，
     * 持续按住不会重复产生。
     */
    INPUT_EVENT_LONG_PRESS
} InputEventType_t;


/**
 * @brief Input event structure
 */
typedef struct
{
    InputKey_t key;
    InputEventType_t type;
} InputEvent_t;


/**
 * @brief 初始化输入服务
 *
 * 初始化每个按键的内部状态和事件队列。
 *
 * 调用本函数之前，应确保 GPIO 和 board 模块已经完成初始化。
 *
 * @return void
 */
void input_service_init(void);


/**
 * @brief 更新输入服务状态
 *
 * 本函数需要周期性调用，用于进行：
 *
 * - 按键扫描
 * - 软件消抖
 * - 按压时间计算
 * - 短按 / 长按判断
 * - 输入事件生成
 *
 * 建议每 5~10 ms 调用一次。
 *
 * 本函数不会阻塞程序运行。
 *
 * @param[in] now_ms 当前系统运行时间，单位 ms
 *
 * @return void
 */
void input_service_update(uint32_t now_ms);


/**
 * @brief 获取一个待处理的输入事件
 *
 * 如果事件队列中存在事件，则取出最早产生的一个事件。
 *
 * @param[out] event 输入事件输出地址
 *
 * @return bool
 * @retval true  成功取得一个事件
 * @retval false 当前没有事件或参数为空
 */
bool input_service_get_event(InputEvent_t *event);


/**
 * @brief 清空所有待处理输入事件
 *
 * 可用于页面切换等场景，防止旧页面残留的按键事件
 * 被新页面继续处理。
 *
 * @return void
 */
void input_service_clear_events(void);


#endif /* INPUT_SERVICE_H */