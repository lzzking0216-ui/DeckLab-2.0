#pragma once
#include <QString>
#include <QtGlobal>
#include <array>

// 单个拉线传感器通道描述（BRT38 SSI/Modbus）
struct SensorChannel {
    QString name;              // 传感器名称（如 "拉线 1"）
    QString unit;              // 工程单位（固定 "mm"）
    qint32  rawCounts  = 0;   // SSI 原始编码计数（32位有符号）
    float   displayValue = 0.0f; // 换算后的位移（mm）
    bool    valid      = false;  // 数据是否有效（未连接时 false）
};

// 所有拉线传感器的快照（由 PlcWorker::sensorDataUpdated 信号携带）
struct SensorData {
    static constexpr int COUNT = 6;  // 6 个拉线位移传感器
    std::array<SensorChannel, COUNT> channels;
};
