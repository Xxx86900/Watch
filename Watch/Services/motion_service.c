/**
 * @file    motion_service.c
 * @brief   手表运动服务实现。
 *
 * 本模块基于 MPU6050 提供的加速度和角速度数据实现：
 * - 三轴运动数据采集；
 * - Pitch、Roll 姿态角估计；
 * - 加速度计与陀螺仪互补滤波；
 * - 抬腕动作检测；
 * - 放下手腕自动熄屏；
 * - 屏幕自动超时熄灭。
 *
 * 当前抬腕检测参数属于第一版实机测试参数，
 * 其角度范围、轴方向、动作阈值和时间参数需要根据
 * 实际 PCB 安装方向和佩戴效果进一步标定。
 */

#include "motion_service.h"

#include "mpu6050.h"

#include <math.h>
#include <stddef.h>
#include <string.h>


/* ============================================================================
 * 数学参数
 * ========================================================================== */

/**
 * @brief 弧度转角度系数。
 */
#define MOTION_RAD_TO_DEG                  57.2957795f


/**
 * @brief 互补滤波时间常数，单位 s。
 *
 * 时间常数越大，姿态结果越依赖陀螺仪；
 * 时间常数越小，姿态结果越依赖加速度计。
 */
#define MOTION_FILTER_TAU_S                0.50f


/**
 * @brief 允许进行陀螺仪积分的最大采样间隔。
 *
 * 如果主循环长时间阻塞导致采样间隔过大，则放弃本次
 * 陀螺仪积分，直接使用加速度计重新校正姿态，避免产生
 * 过大的积分误差。
 */
#define MOTION_MAX_INTEGRATION_DT_S        0.10f


/* ============================================================================
 * 抬腕检测测试参数
 *
 * 当前参数仅用于第一版功能测试。
 *
 * MPU6050 在 PCB 上的实际安装方向、手表佩戴方向以及不同用户
 * 的抬腕习惯都会影响这些参数，后续必须通过实机数据重新标定。
 * ========================================================================== */

/**
 * @brief 抬腕动作最低角速度，单位 degree/s。
 *
 * 当三轴合成角速度超过该值时，认为可能发生了抬腕动作，
 * 并进入抬腕候选状态。
 */
#define MOTION_RAISE_GYRO_THRESHOLD_DPS    45.0f


/**
 * @brief 抬腕动作完成的最大时间窗口，单位 ms。
 *
 * 检测到明显旋转后，需要在该时间内进入查看姿态。
 */
#define MOTION_RAISE_WINDOW_MS             700U


/**
 * @brief 查看姿态保持时间，单位 ms。
 *
 * 为避免姿态瞬间穿过阈值造成误触，需要连续保持一段时间
 * 后才真正产生亮屏请求。
 */
#define MOTION_VIEW_HOLD_MS                120U


/**
 * @brief 放下手腕姿态保持时间，单位 ms。
 *
 * 连续处于放下姿态超过该时间后产生熄屏请求。
 */
#define MOTION_LOWER_HOLD_MS               600U


/**
 * @brief 屏幕自动熄灭时间，单位 ms。
 *
 * 即使手腕一直保持查看姿态，也不会让屏幕无限保持点亮。
 */
#define MOTION_SCREEN_TIMEOUT_MS           8000U


/**
 * @brief 判断姿态时允许的最小合加速度，单位 g。
 *
 * 当合加速度严重偏离 1 g 时，说明手表正在明显运动，
 * 此时加速度计不能可靠代表重力方向。
 */
#define MOTION_ACCEL_STABLE_MIN_G          0.75f


/**
 * @brief 判断姿态时允许的最大合加速度，单位 g。
 */
#define MOTION_ACCEL_STABLE_MAX_G          1.25f


/**
 * @brief 抬腕查看状态允许的最小 Pitch 角。
 *
 * 当前为测试值。
 */
#define MOTION_VIEW_PITCH_MIN_DEG          (-65.0f)


/**
 * @brief 抬腕查看状态允许的最大 Pitch 角。
 *
 * 当前为测试值。
 */
#define MOTION_VIEW_PITCH_MAX_DEG          35.0f


/**
 * @brief 抬腕查看状态允许的最大 Roll 绝对值。
 *
 * 当前为测试值。
 */
#define MOTION_VIEW_ROLL_MAX_DEG           50.0f


/**
 * @brief 明确判定手腕已放下的最小 Pitch。
 *
 * 与查看姿态使用不同阈值形成迟滞区间，
 * 避免姿态位于边界附近时频繁亮灭屏。
 */
#define MOTION_LOWER_PITCH_MIN_DEG         (-80.0f)


/**
 * @brief 明确判定手腕已放下的最大 Pitch。
 */
#define MOTION_LOWER_PITCH_MAX_DEG         55.0f


/**
 * @brief 明确判定手腕已放下的 Roll 绝对值。
 */
#define MOTION_LOWER_ROLL_DEG              70.0f


/**
 * @brief Pitch 对应陀螺仪轴方向修正。
 *
 * 当前按照 Gyro Y 为 Pitch 正方向处理。
 * 如果实机角度方向相反，可将该值修改为 -1.0f。
 */
#define MOTION_PITCH_GYRO_SIGN             1.0f


/**
 * @brief Roll 对应陀螺仪轴方向修正。
 *
 * 当前按照 Gyro X 为 Roll 正方向处理。
 * 如果实机角度方向相反，可将该值修改为 -1.0f。
 */
#define MOTION_ROLL_GYRO_SIGN              1.0f


/* ============================================================================
 * 内部状态
 * ========================================================================== */

/**
 * @brief Motion Service 内部上下文。
 */
typedef struct
{
    bool initialized;
    bool sample_valid;

    bool screen_on;
    bool screen_woken_by_motion;

    bool raise_candidate_active;
    bool view_hold_active;
    bool lower_hold_active;

    uint32_t last_sample_ms;

    uint32_t raise_candidate_start_ms;
    uint32_t view_hold_start_ms;
    uint32_t lower_hold_start_ms;
    uint32_t last_screen_activity_ms;

    MotionServiceData_t data;
} MotionServiceContext_t;


static MotionServiceContext_t s_motion;


/* ============================================================================
 * 私有函数
 * ========================================================================== */

/**
 * @brief 根据加速度计计算 Pitch。
 *
 * 当设备仅受重力作用时，加速度方向可以作为姿态的长期参考。
 *
 * @param accel_x X 轴加速度，单位 g。
 * @param accel_y Y 轴加速度，单位 g。
 * @param accel_z Z 轴加速度，单位 g。
 *
 * @return Pitch 角，单位 degree。
 */
static float motion_calc_accel_pitch(float accel_x,
                                     float accel_y,
                                     float accel_z)
{
    float yz = sqrtf(accel_y * accel_y + accel_z * accel_z);

    return atan2f(-accel_x, yz) * MOTION_RAD_TO_DEG;
}


/**
 * @brief 根据加速度计计算 Roll。
 *
 * @param accel_y Y 轴加速度，单位 g。
 * @param accel_z Z 轴加速度，单位 g。
 *
 * @return Roll 角，单位 degree。
 */
static float motion_calc_accel_roll(float accel_y, float accel_z)
{
    return atan2f(accel_y, accel_z) * MOTION_RAD_TO_DEG;
}


/**
 * @brief 判断当前加速度是否适合用于姿态参考。
 *
 * 静止或低动态运动时，三轴合加速度应接近 1 g。
 * 当合加速度明显偏离 1 g 时，不应完全相信加速度计计算的姿态。
 *
 * @param accel_norm_g 三轴合加速度，单位 g。
 *
 * @return true  当前加速度适合用于姿态判断；
 *         false 当前处于较强动态运动状态。
 */
static bool motion_is_accel_stable(float accel_norm_g)
{
    return (accel_norm_g >= MOTION_ACCEL_STABLE_MIN_G) &&
           (accel_norm_g <= MOTION_ACCEL_STABLE_MAX_G);
}


/**
 * @brief 判断当前姿态是否属于可能的查看手表姿态。
 *
 * 当前角度范围仅为第一版测试值，后续需要根据实机佩戴数据调整。
 *
 * @param pitch Pitch 角，单位 degree。
 * @param roll  Roll 角，单位 degree。
 *
 * @return true 处于查看姿态；
 *         false 不处于查看姿态。
 */
static bool motion_is_view_pose(float pitch, float roll)
{
    return (pitch >= MOTION_VIEW_PITCH_MIN_DEG) &&
           (pitch <= MOTION_VIEW_PITCH_MAX_DEG) &&
           (fabsf(roll) <= MOTION_VIEW_ROLL_MAX_DEG);
}


/**
 * @brief 判断手腕是否已经明显离开查看姿态。
 *
 * 本函数使用比查看姿态更宽的阈值形成迟滞，
 * 防止在边界附近产生屏幕频繁亮灭。
 *
 * @param pitch Pitch 角，单位 degree。
 * @param roll  Roll 角，单位 degree。
 *
 * @return true 手腕已经明显放下；
 *         false 尚未满足明确放下条件。
 */
static bool motion_is_lowered_pose(float pitch, float roll)
{
    return (pitch <= MOTION_LOWER_PITCH_MIN_DEG) ||
           (pitch >= MOTION_LOWER_PITCH_MAX_DEG) ||
           (fabsf(roll) >= MOTION_LOWER_ROLL_DEG);
}


/**
 * @brief 清除当前抬腕候选状态。
 */
static void motion_reset_raise_candidate(void)
{
    s_motion.raise_candidate_active = false;
    s_motion.view_hold_active = false;
}


/**
 * @brief 产生亮屏请求并更新内部状态。
 *
 * @param now_ms         当前系统时间，单位 ms。
 * @param screen_request 屏幕请求输出。
 */
static void motion_request_screen_on(uint32_t now_ms,
                                     MotionScreenRequest_t *screen_request)
{
    s_motion.screen_on = true;
    s_motion.screen_woken_by_motion = true;
    s_motion.last_screen_activity_ms = now_ms;

    s_motion.lower_hold_active = false;

    motion_reset_raise_candidate();

    s_motion.data.screen_on = true;

    *screen_request = MOTION_SCREEN_REQUEST_ON;
}


/**
 * @brief 产生熄屏请求并更新内部状态。
 *
 * @param screen_request 屏幕请求输出。
 */
static void motion_request_screen_off(MotionScreenRequest_t *screen_request)
{
    s_motion.screen_on = false;
    s_motion.screen_woken_by_motion = false;

    s_motion.lower_hold_active = false;

    motion_reset_raise_candidate();

    s_motion.data.screen_on = false;

    *screen_request = MOTION_SCREEN_REQUEST_OFF;
}


/**
 * @brief 执行屏幕状态机。
 *
 * 屏幕关闭时：
 * 1. 检测明显手腕旋转；
 * 2. 在规定时间内进入查看姿态；
 * 3. 查看姿态稳定保持一段时间；
 * 4. 产生亮屏请求。
 *
 * 屏幕开启时：
 * - 如果由抬腕动作唤醒，检测放下手腕并自动熄屏；
 * - 无论通过何种方式打开屏幕，都执行超时熄屏。
 *
 * @param now_ms         当前系统时间，单位 ms。
 * @param screen_request 屏幕请求输出。
 */
static void motion_update_screen_state(uint32_t now_ms,
                                       MotionScreenRequest_t *screen_request)
{
    bool accel_stable = motion_is_accel_stable(s_motion.data.accel_norm_g);

    bool view_pose = accel_stable &&
                     motion_is_view_pose(
                         s_motion.data.pitch_deg,
                         s_motion.data.roll_deg
                     );

    bool lowered_pose = motion_is_lowered_pose(
        s_motion.data.pitch_deg,
        s_motion.data.roll_deg
    );

    s_motion.data.is_view_pose = view_pose;

    if (!s_motion.screen_on)
    {
        /*
         * 首先检测一次明显的旋转动作。
         * 仅仅处于查看姿态不会直接亮屏，可以降低手表平放在桌面时
         * 因姿态本身满足条件而误亮屏的概率。
         */
        if (!s_motion.raise_candidate_active &&
            (s_motion.data.gyro_norm_dps >= MOTION_RAISE_GYRO_THRESHOLD_DPS))
        {
            s_motion.raise_candidate_active = true;
            s_motion.raise_candidate_start_ms = now_ms;
            s_motion.view_hold_active = false;
        }

        if (!s_motion.raise_candidate_active)
        {
            return;
        }

        /*
         * 抬腕动作需要在规定时间窗口内完成。
         */
        if ((uint32_t)(now_ms - s_motion.raise_candidate_start_ms) >
            MOTION_RAISE_WINDOW_MS)
        {
            motion_reset_raise_candidate();
            return;
        }

        /*
         * 检测动作结束后是否进入稳定的查看姿态。
         */
        if (view_pose)
        {
            if (!s_motion.view_hold_active)
            {
                s_motion.view_hold_active = true;
                s_motion.view_hold_start_ms = now_ms;
            }
            else if ((uint32_t)(now_ms - s_motion.view_hold_start_ms) >=
                     MOTION_VIEW_HOLD_MS)
            {
                motion_request_screen_on(now_ms, screen_request);
            }
        }
        else
        {
            s_motion.view_hold_active = false;
        }

        return;
    }

    /*
     * 屏幕已经打开后不再处理新的抬腕候选动作。
     */
    motion_reset_raise_candidate();

    /*
     * 只有由抬腕动作唤醒的屏幕才根据放下手腕自动熄灭。
     *
     * 如果屏幕由按键或其他 UI 操作打开，则主要依赖超时熄屏，
     * 防止用户在非佩戴状态下操作手表时被姿态逻辑强制关闭。
     */
    if (s_motion.screen_woken_by_motion)
    {
        if (lowered_pose)
        {
            if (!s_motion.lower_hold_active)
            {
                s_motion.lower_hold_active = true;
                s_motion.lower_hold_start_ms = now_ms;
            }
            else if ((uint32_t)(now_ms - s_motion.lower_hold_start_ms) >=
                     MOTION_LOWER_HOLD_MS)
            {
                motion_request_screen_off(screen_request);
                return;
            }
        }
        else
        {
            s_motion.lower_hold_active = false;
        }
    }

    /*
     * 屏幕自动超时关闭。
     */
    if ((uint32_t)(now_ms - s_motion.last_screen_activity_ms) >=
        MOTION_SCREEN_TIMEOUT_MS)
    {
        motion_request_screen_off(screen_request);
    }
}


/* ============================================================================
 * 公共接口
 * ========================================================================== */

MotionServiceStatus_t motion_service_init(void)
{
    memset(&s_motion, 0, sizeof(s_motion));

    if (mpu6050_init() != MPU6050_STATUS_OK)
    {
        return MOTION_SERVICE_STATUS_ERROR_SENSOR;
    }

    s_motion.initialized = true;

    return MOTION_SERVICE_STATUS_OK;
}


MotionServiceStatus_t motion_service_update(
    uint32_t now_ms,
    MotionScreenRequest_t *screen_request)
{
    if (screen_request == NULL)
    {
        return MOTION_SERVICE_STATUS_ERROR_PARAM;
    }

    *screen_request = MOTION_SCREEN_REQUEST_NONE;

    if (!s_motion.initialized)
    {
        return MOTION_SERVICE_STATUS_NOT_INITIALIZED;
    }

    /*
     * 限制传感器读取频率。
     *
     * 即使本函数在主循环中被高频调用，也只会按照约 100 Hz
     * 的周期真正访问 MPU6050。
     */
    if (s_motion.sample_valid &&
        ((uint32_t)(now_ms - s_motion.last_sample_ms) <
         MOTION_SERVICE_UPDATE_PERIOD_MS))
    {
        return MOTION_SERVICE_STATUS_OK;
    }

    Mpu6050Data_t imu;

    if (mpu6050_read(&imu) != MPU6050_STATUS_OK)
    {
        return MOTION_SERVICE_STATUS_ERROR_SENSOR;
    }

    float accel_pitch = motion_calc_accel_pitch(
        imu.accel_x_g,
        imu.accel_y_g,
        imu.accel_z_g
    );

    float accel_roll = motion_calc_accel_roll(
        imu.accel_y_g,
        imu.accel_z_g
    );

    /*
     * 第一次获得数据时无法进行陀螺仪积分，
     * 因此直接使用加速度计建立初始姿态。
     */
    if (!s_motion.sample_valid)
    {
        s_motion.data.pitch_deg = accel_pitch;
        s_motion.data.roll_deg = accel_roll;

        s_motion.sample_valid = true;
    }
    else
    {
        float dt_s =
            (float)((uint32_t)(now_ms - s_motion.last_sample_ms)) / 1000.0f;

        if ((dt_s > 0.0f) && (dt_s <= MOTION_MAX_INTEGRATION_DT_S))
        {
            /*
             * 根据实际采样周期动态计算互补滤波系数：
             *
             * alpha = tau / (tau + dt)
             *
             * 在 10 ms 更新周期、tau = 0.5 s 时，
             * alpha 约为 0.98。
             */
            float alpha =
                MOTION_FILTER_TAU_S / (MOTION_FILTER_TAU_S + dt_s);

            float gyro_pitch =
                s_motion.data.pitch_deg +
                MOTION_PITCH_GYRO_SIGN * imu.gyro_y_dps * dt_s;

            float gyro_roll =
                s_motion.data.roll_deg +
                MOTION_ROLL_GYRO_SIGN * imu.gyro_x_dps * dt_s;

            s_motion.data.pitch_deg =
                alpha * gyro_pitch +
                (1.0f - alpha) * accel_pitch;

            s_motion.data.roll_deg =
                alpha * gyro_roll +
                (1.0f - alpha) * accel_roll;
        }
        else
        {
            /*
             * 采样间隔异常时放弃陀螺仪积分，
             * 防止长时间积分产生明显角度跳变。
             */
            s_motion.data.pitch_deg = accel_pitch;
            s_motion.data.roll_deg = accel_roll;
        }
    }

    s_motion.last_sample_ms = now_ms;

    /*
     * 保存本周期传感器数据。
     */
    s_motion.data.accel_x_g = imu.accel_x_g;
    s_motion.data.accel_y_g = imu.accel_y_g;
    s_motion.data.accel_z_g = imu.accel_z_g;

    s_motion.data.gyro_x_dps = imu.gyro_x_dps;
    s_motion.data.gyro_y_dps = imu.gyro_y_dps;
    s_motion.data.gyro_z_dps = imu.gyro_z_dps;

    s_motion.data.accel_norm_g = sqrtf(
        imu.accel_x_g * imu.accel_x_g +
        imu.accel_y_g * imu.accel_y_g +
        imu.accel_z_g * imu.accel_z_g
    );

    s_motion.data.gyro_norm_dps = sqrtf(
        imu.gyro_x_dps * imu.gyro_x_dps +
        imu.gyro_y_dps * imu.gyro_y_dps +
        imu.gyro_z_dps * imu.gyro_z_dps
    );

    s_motion.data.screen_on = s_motion.screen_on;

    motion_update_screen_state(now_ms, screen_request);

    return MOTION_SERVICE_STATUS_OK;
}


MotionServiceStatus_t motion_service_get_data(MotionServiceData_t *data)
{
    if (data == NULL)
    {
        return MOTION_SERVICE_STATUS_ERROR_PARAM;
    }

    if (!s_motion.initialized)
    {
        return MOTION_SERVICE_STATUS_NOT_INITIALIZED;
    }

    if (!s_motion.sample_valid)
    {
        return MOTION_SERVICE_STATUS_NOT_READY;
    }

    *data = s_motion.data;

    return MOTION_SERVICE_STATUS_OK;
}


void motion_service_set_screen_state(bool screen_on, uint32_t now_ms)
{
    s_motion.screen_on = screen_on;
    s_motion.data.screen_on = screen_on;

    s_motion.lower_hold_active = false;

    if (screen_on)
    {
        /*
         * 由外部模块主动打开屏幕时，不将其认为是一次抬腕唤醒。
         */
        s_motion.screen_woken_by_motion = false;
        s_motion.last_screen_activity_ms = now_ms;
    }
    else
    {
        s_motion.screen_woken_by_motion = false;
        motion_reset_raise_candidate();
    }
}


void motion_service_notify_user_activity(uint32_t now_ms)
{
    if (!s_motion.screen_on)
    {
        return;
    }

    s_motion.last_screen_activity_ms = now_ms;

    /*
     * 用户已经主动操作手表后，不再根据放下手腕立即关闭屏幕，
     * 后续交由超时机制处理。
     */
    s_motion.screen_woken_by_motion = false;
    s_motion.lower_hold_active = false;
}


bool motion_service_is_screen_on(void)
{
    return s_motion.screen_on;
}