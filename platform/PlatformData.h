#pragma once
#include <QString>
#include <array>
#include <QtGlobal>

// -----------------------------------------------------------------------
// 国辰正域 Stewart 平台数据结构
// UDP 通讯：命令端口 2000，实时推送端口 3001
// -----------------------------------------------------------------------

// UDP 连接配置
struct PlatformConfig {
    QString ip       = "192.168.1.41";
    quint16 cmdPort  = 2000;   // 发送命令到设备
    quint16 pushPort = 3001;   // 接收设备实时推送
};

// 6 自由度位姿（平移 mm，转角 deg）
struct PlatformPose {
    double x  = 0.0;  // mm
    double y  = 0.0;  // mm
    double z  = 0.0;  // mm
    double rx = 0.0;  // deg
    double ry = 0.0;  // deg
    double rz = 0.0;  // deg
};

// 运动模式（func code 10010）
enum class MotionMode : int {
    Stop        = 0,   // 停止
    ChainJog    = 1,   // 支链点动（开环）
    PoseJog     = 2,   // 姿态点动
    PoseTarget  = 3,   // 姿态定点
    Sine        = 4,   // 正弦运动
    Increment   = 5,   // 增量运动
    Follow      = 6,   // 跟随
    ChainTarget = 10,  // 支链定点
};

// 实时推送的完整平台状态
struct PlatformState {
    // 设备状态（func code 20000）
    quint32 deviceStatus = 0;
    bool    powered      = false;   // 已上电
    bool    faultActive  = false;   // 故障
    int     motionMode   = 0;       // 当前运动模式

    // 当前位姿（func codes 21000-21005）
    PlatformPose currentPose;

    // 各支链实际长度 mm（func codes 20200-20205）
    std::array<double, 6> chainLengths = {};

    // 外部传感器（func codes 22100-22105）
    std::array<double, 6> extSensors   = {};

    // 是否收到过有效数据帧
    bool valid = false;
};

// -----------------------------------------------------------------------
// 协议功能码
// -----------------------------------------------------------------------
namespace FuncCode {
    // 写命令
    constexpr quint32 POWER       = 10000;  // 1=上电, 0=下电
    constexpr quint32 FAULT_RESET = 10001;  // 1=复位
    constexpr quint32 MOTION_MODE = 10010;  // 见 MotionMode 枚举

    // 目标位姿（double，mm/deg）
    constexpr quint32 TARGET_X  = 10300;
    constexpr quint32 TARGET_Y  = 10301;
    constexpr quint32 TARGET_Z  = 10302;
    constexpr quint32 TARGET_RX = 10303;
    constexpr quint32 TARGET_RY = 10304;
    constexpr quint32 TARGET_RZ = 10305;

    // 读状态
    constexpr quint32 DEVICE_STATE = 20000;  // 设备状态字

    // 当前位姿（double，mm/deg）
    constexpr quint32 POSE_X  = 21000;
    constexpr quint32 POSE_Y  = 21001;
    constexpr quint32 POSE_Z  = 21002;
    constexpr quint32 POSE_RX = 21003;
    constexpr quint32 POSE_RY = 21004;
    constexpr quint32 POSE_RZ = 21005;

    // 支链实际长度（double，mm）
    constexpr quint32 CHAIN_BASE = 20200;  // 20200~20205

    // 外部传感器（double）
    constexpr quint32 EXT_SENSOR_BASE = 22100;  // 22100~22105
} // namespace FuncCode
