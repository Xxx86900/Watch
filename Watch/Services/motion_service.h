/**
 * @file    motion_service.h
 * @brief   手表运动服务接口。
 *
 * 本模块基于 MPU6050 六轴数据实现手表运动状态处理，
 * 主要负责：
 * - 周期读取加速度计和陀螺仪数据；
 * - 通过互补滤波估算 Pitch、Roll 姿态角；
 * - 判断当前手表是否处于抬腕查看姿态；
 * - 根据手腕动作产生亮屏和熄屏请求；
 * - 为上层 UI 或其他业务模块提供运动状态数据。
 *
 * 本模块只负责产生屏幕状态请求，不直接操作 OLED，
 * OLED 的实际开关应由上层应用根据请求进行处理。
 */

#ifndef MOTION_SERVICE_H
#define MOTION_SERVICE_H

#include <stdbool.h>
#include <stdint.h>


/**
 * @brief Motion Service 推荐更新周期，单位 ms。
 *
 * MPU6050 当前配置为 100 Hz，因此推荐每 10 ms 更新一次。
 * motion_service_update() 内部已进行周期限制，可以直接在主循环中调用。
 */
#define MOTION_SERVICE_UPDATE_PERIOD_MS    10U


/**
 * @brief Motion Service 运行状态。
 */
typedef enum
{
    MOTION_SERVICE_STATUS_OK = 0,
    MOTION_SERVICE_STATUS_ERROR_PARAM,
    MOTION_SERVICE_STATUS_ERROR_SENSOR,
    MOTION_SERVICE_STATUS_NOT_INITIALIZED,
    MOTION_SERVICE_STATUS_NOT_READY
} MotionServiceStatus_t;


/**
 * @brief Motion Service 向上层产生的屏幕控制请求。
 */
typedef enum
{
    MOTION_SCREEN_REQUEST_NONE = 0,
    MOTION_SCREEN_REQUEST_ON,
    MOTION_SCREEN_REQUEST_OFF
} MotionScreenRequest_t;


/**
 * @brief Motion Service 当前运动数据。
 */
typedef struct
{
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;

    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;

    float pitch_deg;
    float roll_deg;

    float accel_norm_g;
    float gyro_norm_dps;

    bool is_view_pose;
    bool screen_on;
} MotionServiceData_t;


/**
 * @brief 初始化 Motion Service。
 *
 * 初始化过程中会初始化 MPU6050，并清空姿态滤波、
 * 抬腕检测和屏幕状态机内部状态。
 *
 * 初始化完成后 Motion Service 默认认为屏幕处于关闭状态。
 * 如果系统启动时屏幕实际处于开启状态，应调用
 * motion_service_set_screen_state() 进行同步。
 *
 * @return MOTION_SERVICE_STATUS_OK 初始化成功；
 *         其他值表示初始化失败。
 */
MotionServiceStatus_t motion_service_init(void);


/**
 * @brief 更新 Motion Service。
 *
 * 本函数应在裸机主循环中周期调用。
 *
 * 函数内部会：
 * - 按约 100 Hz 周期读取 MPU6050；
 * - 更新姿态角；
 * - 执行抬腕检测；
 * - 判断是否需要亮屏或熄屏。
 *
 * 如果距离上一次采样不足 MOTION_SERVICE_UPDATE_PERIOD_MS，
 * 本次调用不会访问 MPU6050，并直接返回成功。
 *
 * @param now_ms         当前系统时间，单位 ms。
 * @param screen_request 用于返回本次产生的屏幕控制请求。
 *
 * @return MOTION_SERVICE_STATUS_OK 更新成功；
 *         其他值表示更新失败。
 */
MotionServiceStatus_t motion_service_update(uint32_t now_ms,
                                            MotionScreenRequest_t *screen_request);


/**
 * @brief 获取 Motion Service 当前数据。
 *
 * @param data 用于保存当前运动数据。
 *
 * @return MOTION_SERVICE_STATUS_OK 获取成功；
 *         MOTION_SERVICE_STATUS_NOT_READY 尚未获得有效传感器数据；
 *         其他值表示参数或状态错误。
 */
MotionServiceStatus_t motion_service_get_data(MotionServiceData_t *data);


/**
 * @brief 同步实际屏幕状态到 Motion Service。
 *
 * 当屏幕由按键、UI 或其他模块主动开启或关闭时，
 * 上层应调用本函数，使 Motion Service 内部状态与实际屏幕状态一致。
 *
 * @param screen_on 当前实际屏幕状态。
 * @param now_ms    当前系统时间，单位 ms。
 */
void motion_service_set_screen_state(bool screen_on, uint32_t now_ms);


/**
 * @brief 通知 Motion Service 用户发生了交互操作。
 *
 * 可在按键操作等用户交互发生时调用，用于刷新自动熄屏计时。
 *
 * @param now_ms 当前系统时间，单位 ms。
 */
void motion_service_notify_user_activity(uint32_t now_ms);


/**
 * @brief 获取 Motion Service 当前记录的屏幕状态。
 *
 * @return true  屏幕开启；
 *         false 屏幕关闭。
 */
bool motion_service_is_screen_on(void);


#endif /* MOTION_SERVICE_H */