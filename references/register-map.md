# 寄存器地址映射 — 完整参考

## 概述

上位机通过 Modbus TCP 读写 AC702 PLC 的保持寄存器（MW 区）。
寄存器地址需与 InProShop 中的变量映射配置**严格一致**。
本文件定义地址规范，是 `communication/RegisterMap.h` 的详细说明。

---

## RegisterMap.h 完整定义

```cpp
// communication/RegisterMap.h
#pragma once
#include <QtGlobal>

namespace Reg {

    // =========================================================
    // 轴控制区
    // 8轴（0~6: SV660N, 7: SVD60NR5I）
    // 每轴占 AXIS_STRIDE 个寄存器
    // 轴 n 基地址 = AXIS_BASE + n * AXIS_STRIDE
    // =========================================================
    constexpr quint16 AXIS_BASE   = 0x0000;  // MW0
    constexpr quint16 AXIS_STRIDE = 10;      // 每轴10个寄存器

    // 相对基地址的偏移（读/写方向如注释所示）
    constexpr quint16 AX_CTRL_WORD    = 0;  // [RW] 控制字 (DS402 Controlword)
    constexpr quint16 AX_TARGET_VEL   = 1;  // [RW] 目标速度（有符号，rpm × 10）
    constexpr quint16 AX_TARGET_POS   = 2;  // [RW] 目标位置低16位（pulse）
    constexpr quint16 AX_TARGET_POS_H = 3;  // [RW] 目标位置高16位
    constexpr quint16 AX_STATUS_WORD  = 4;  // [RO] 状态字 (DS402 Statusword)
    constexpr quint16 AX_ACT_VEL      = 5;  // [RO] 实际速度（有符号，rpm × 10）
    constexpr quint16 AX_ACT_POS      = 6;  // [RO] 实际位置低16位（pulse）
    constexpr quint16 AX_ACT_POS_H    = 7;  // [RO] 实际位置高16位
    constexpr quint16 AX_TORQUE       = 8;  // [RO] 实际力矩（有符号，0.1% 额定）
    constexpr quint16 AX_ERROR_CODE   = 9;  // [RO] 驱动器故障码（0 = 无故障）

    // 便捷函数：获取指定轴指定字段的绝对地址
    inline quint16 axisAddr(int axisId, quint16 field) {
        return AXIS_BASE + static_cast<quint16>(axisId) * AXIS_STRIDE + field;
    }

    // =========================================================
    // 传感器区（GL20-RTU-ECT）
    // 从 SENSOR_BASE 起，按接线顺序分配
    // 具体通道含义见 sensor_channels.json 配置文件
    // =========================================================
    constexpr quint16 SENSOR_BASE      = 0x0100;  // MW256
    constexpr quint16 SENSOR_AI_COUNT  = 8;       // 模拟量输入通道数（按实际调整）
    constexpr quint16 SENSOR_DI_COUNT  = 8;       // 数字量输入通道数（按实际调整）
    // 模拟量：MW256 ~ MW263（每通道1个寄存器，有符号或无符号由换算系数决定）
    // 数字量：MW264 起，每个寄存器的每个 bit 代表一路 DI，或每路占一个寄存器
    // ↑ 以上具体规划在 InProShop 中确认后填入

    // =========================================================
    // 系统状态区
    // =========================================================
    constexpr quint16 SYS_BASE         = 0x0200;  // MW512
    constexpr quint16 SYS_STATUS       = 0x0200;  // [RO] 系统整体状态字
    constexpr quint16 SYS_ALARM_WORD   = 0x0201;  // [RO] PLC 侧报警字（位字段）
    constexpr quint16 SYS_ESTOP_STATUS = 0x0202;  // [RO] 急停状态（1=急停中）
    constexpr quint16 SYS_ESTOP_CMD    = 0x0203;  // [RW] 上位机发急停命令（写1触发）
    constexpr quint16 SYS_HEARTBEAT    = 0x0204;  // [RW] 心跳寄存器（上位机每秒递增）
    constexpr quint16 SYS_RUN_MODE     = 0x0205;  // [RW] 运行模式（0=手动,1=自动）

    // =========================================================
    // 地址范围汇总（用于 InProShop 映射配置参考）
    // =========================================================
    // MW0   ~ MW79  : 8轴控制/状态（Axis 0~7 各10个寄存器）
    // MW256 ~ MW279 : GL20 传感器区
    // MW512 ~ MW517 : 系统状态区
    // =========================================================

} // namespace Reg
```

---

## 批量读取示例（PlcWorker 中的最优读法）

```cpp
// 策略：尽量合并连续地址范围，减少 Modbus 请求次数

void PlcWorker::readAllMotors() {
    // 一次读 8轴 × 10寄存器 = 80个寄存器（1次 Modbus 请求）
    QVector<quint16> axisRegs;
    if (!m_client->readHoldingRegisters(Reg::AXIS_BASE, 8 * Reg::AXIS_STRIDE, axisRegs))
        return;

    for (int ax = 0; ax < 8; ++ax) {
        int b = ax * Reg::AXIS_STRIDE;
        MotorData d;
        d.axisId     = ax;
        d.statusWord = axisRegs[b + Reg::AX_STATUS_WORD];
        d.actualVel  = static_cast<qint16>(axisRegs[b + Reg::AX_ACT_VEL]);
        d.actualPos  = static_cast<qint32>(
                           (static_cast<quint32>(axisRegs[b + Reg::AX_ACT_POS_H]) << 16)
                           | axisRegs[b + Reg::AX_ACT_POS]);
        d.torque     = static_cast<qint16>(axisRegs[b + Reg::AX_TORQUE]);
        d.errorCode  = axisRegs[b + Reg::AX_ERROR_CODE];
        d.enabled    = (d.statusWord & 0x0004) != 0;
        d.faultActive= (d.statusWord & 0x0008) != 0;
        d.stateName  = ds402StateName(d.statusWord);
        emit motorDataUpdated(ax, d);
    }
}

void PlcWorker::readSensors() {
    // 一次读传感器区（1次 Modbus 请求）
    QVector<quint16> sensorRegs;
    int totalCount = Reg::SENSOR_AI_COUNT + Reg::SENSOR_DI_COUNT;
    if (!m_client->readHoldingRegisters(Reg::SENSOR_BASE, totalCount, sensorRegs))
        return;
    SensorData d;
    d.timestamp = QDateTime::currentMSecsSinceEpoch();
    m_sensorManager->processRawRegisters(sensorRegs, d.timestamp);
}

void PlcWorker::checkSystemAlarms() {
    QVector<quint16> sysRegs;
    if (!m_client->readHoldingRegisters(Reg::SYS_STATUS, 4, sysRegs)) return;
    if (sysRegs[Reg::SYS_ESTOP_STATUS - Reg::SYS_BASE])
        emit alarmTriggered({.level = AlarmLevel::Critical,
                             .source = "系统", .description = "急停信号激活"});
}
```

---

## InProShop 配置步骤（给开发者的备忘）

1. **打开变量表**：在 InProShop 的全局变量表中，找到需要映射的变量
2. **配置 Modbus 映射**：
   - 进入 `通信配置 → Modbus TCP Server`
   - 为每个变量指定 MW 地址（与本文件 `Reg::` 常量一致）
   - 确认读写方向（只读 / 读写）
3. **启用 Modbus Server**：配置以太网口 IP（与上位机同网段），启用端口 502
4. **验证**：用 Modbus Poll 等工具先验证寄存器可读，再接入上位机
5. **记录偏差**：若实际地址与本规范有差异，优先修改本文件，保持单一数据源

---

## 地址查询工具函数

```cpp
// 调试用：打印某轴的完整地址表
void printAxisAddrMap(int axisId) {
    quint16 base = Reg::AXIS_BASE + axisId * Reg::AXIS_STRIDE;
    qDebug() << QString("=== 轴 %1 寄存器地址表 ===").arg(axisId);
    qDebug() << QString("控制字:      MW%1 (0x%2)").arg(base+Reg::AX_CTRL_WORD).arg(base+Reg::AX_CTRL_WORD, 4, 16);
    qDebug() << QString("目标速度:    MW%1").arg(base+Reg::AX_TARGET_VEL);
    qDebug() << QString("目标位置:    MW%1~%2 (32bit)").arg(base+Reg::AX_TARGET_POS).arg(base+Reg::AX_TARGET_POS_H);
    qDebug() << QString("状态字:      MW%1").arg(base+Reg::AX_STATUS_WORD);
    qDebug() << QString("实际速度:    MW%1").arg(base+Reg::AX_ACT_VEL);
    qDebug() << QString("实际位置:    MW%1~%2 (32bit)").arg(base+Reg::AX_ACT_POS).arg(base+Reg::AX_ACT_POS_H);
    qDebug() << QString("实际力矩:    MW%1").arg(base+Reg::AX_TORQUE);
    qDebug() << QString("故障码:      MW%1").arg(base+Reg::AX_ERROR_CODE);
}
```
