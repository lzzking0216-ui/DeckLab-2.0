#pragma once
#include <QtGlobal>

namespace Reg {

    // =========================================================
    // 轴控制区（8轴：//*1~6: SV660N动平台，7:SV660N,8: SVD60NR5I）
    // 轴 n 基地址 = AXIS_BASE + n * AXIS_STRIDE
    // =========================================================
    constexpr quint16 AXIS_BASE   = 0x0000;  // MW0
    constexpr quint16 AXIS_STRIDE = 10;

    constexpr quint16 AX_CTRL_WORD    = 0;  // [RW] DS402 控制字
    constexpr quint16 AX_TARGET_VEL   = 1;  // [RW] 目标速度（有符号，rpm×10）
    constexpr quint16 AX_TARGET_POS   = 2;  // [RW] 目标位置 低16位（pulse）
    constexpr quint16 AX_TARGET_POS_H = 3;  // [RW] 目标位置 高16位
    constexpr quint16 AX_STATUS_WORD  = 4;  // [RO] DS402 状态字
    constexpr quint16 AX_ACT_VEL      = 5;  // [RO] 实际速度（有符号，rpm×10）
    constexpr quint16 AX_ACT_POS      = 6;  // [RO] 实际位置 低16位
    constexpr quint16 AX_ACT_POS_H    = 7;  // [RO] 实际位置 高16位
    constexpr quint16 AX_TORQUE       = 8;  // [RO] 实际力矩（有符号，0.1% 额定）
    constexpr quint16 AX_ERROR_CODE   = 9;  // [RO] 驱动器故障码（0=无故障）

    inline quint16 axisAddr(int axisId, quint16 field) {
        return AXIS_BASE + static_cast<quint16>(axisId) * AXIS_STRIDE + field;
    }

    // =========================================================
    // 拉线位移传感器区（BRT38 SSI，6路，MW256 起）
    // 每路占 2 个 16-bit 寄存器（低字在前，组成 32-bit 有符号计数值）
    // 通道 n：低字 = SENSOR_BASE + n*2，高字 = SENSOR_BASE + n*2 + 1
    // =========================================================
    constexpr quint16 SENSOR_BASE       = 0x0100;  // MW256
    constexpr int     SENSOR_COUNT      = 6;        // 传感器路数
    constexpr int     SENSOR_REGS_PER_CH = 2;       // 每路寄存器数（32-bit）

    inline quint16 sensorAddr(int channel) {
        return SENSOR_BASE + static_cast<quint16>(channel) * SENSOR_REGS_PER_CH;
    }

    // =========================================================
    // 系统状态区
    // =========================================================
    constexpr quint16 SYS_BASE         = 0x0200;
    constexpr quint16 SYS_STATUS       = 0x0200;
    constexpr quint16 SYS_ALARM_WORD   = 0x0201;
    constexpr quint16 SYS_ESTOP_STATUS = 0x0202;
    constexpr quint16 SYS_ESTOP_CMD    = 0x0203;
    constexpr quint16 SYS_HEARTBEAT    = 0x0204;

} // namespace Reg
