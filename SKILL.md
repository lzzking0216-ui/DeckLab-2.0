---
name: decklab-2-0
description: >
  专为 Qt (C++) + 双通道工业通讯（UDP 直连国辰正域 Stewart 六自由度平台 + Modbus TCP 连汇川 PLC）的
  上位机控制界面开发技能。涵盖：六自由度动平台 UDP 协议控制、汇川伺服（SV660N/SVD60NR5I）SoftMotion
  运动控制、BRT38 SSI 拉线位移传感器数据读取、三栏工业 HMI 布局（MotorPanel / PlatformPanel / SensorPanel）。
  关键词触发：上位机、UDP 平台、Stewart、国辰正域、汇川、AC702、SV660N、Modbus TCP、SoftMotion、
  InProShop、BRT38、拉线传感器、动平台控制、EtherCAT 从站、实时曲线。
---

# DeckLab 2.0 上位机 HMI 开发技能

## 硬件配置总览

| 设备 | 型号 | 数量 | 角色 | 通信方式 |
|------|------|------|------|----------|
| 六自由度平台控制器 | 国辰正域 Stewart（内置汇川 PLC） | 1 | EtherCAT 主站 + UDP 服务端 | **UDP**（cmd:2000 / push:3001） |
| 伺服驱动器 | 汇川 InoSV660N | 6 | EtherCAT 从站，**动平台** 6 轴（PLC内部Axis_1~Axis_6） | EtherCAT（内部） |
| 用户扩展驱动器 | 汇川 SV660N | 1 | EtherCAT 从站，**卷筒电机**（PLC Axis_7 / Qt axis 6） | Modbus TCP |
| 用户扩展驱动器 | 汇川 SVD60NR5I | 1 | EtherCAT 从站，**RSD 丝杆**（PLC Axis_8 / Qt axis 7） | Modbus TCP |
| 拉线位移传感器 | 布瑞特 BRT38 SSI | 6 | RS422 差分 SSI 输出（12-bit 单圈 + 5-bit 多圈 = 17-bit），接 GL20 SSI 扩展模块 | EtherCAT（经 GL20） |
| **EtherCAT 远程 I/O 耦合器** | **汇川 GL20-RTU-ECT** | **1** | **EtherCAT 从站，GL20 扩展模块总线网关；BRT38 SSI 数据经此上传到 PLC，映射至 MW256~MW267** | **EtherCAT（内部）→ Modbus TCP** |
| **SSI 输入扩展模块** | **汇川 GL20 SSI 模块**（型号待确认） | **≥1** | **挂载于 GL20-RTU-ECT，读取 BRT38 SSI 差分信号，每路 2 个寄存器（32-bit 计数值）** | **EtherCAT（经 GL20-RTU-ECT）** |
| 上位机 | Qt C++ 程序 | 1 | UDP 客户端 + Modbus TCP 客户端 | 双通道 |

### 轴 ID 分配

> **重要**：PLC 侧轴号 1-based（Axis_1~Axis_6 为动平台，Axis_7=卷筒，Axis_8=RSD）；
> Qt 上位机 0-based（axis 1~6 对应 PLC Axis_1~6，axis 7=卷筒，axis 8=RSD）。

| Qt 轴 ID | PLC 轴号 | 名称 | 驱动器 | 控制方式 |
|----------|---------|------|--------|----------|
| 0~5 | Axis_1~6 | 平台轴 1~6 | InoSV660N ×6 | **UDP 直连**（PlatformPanel，全功能：上下电/模式/位姿目标/支链长度） |
| 6 | Axis_7 | 卷筒电机 | SV660N | Modbus TCP（MotorPanel，SoftMotion 控制字） |
| 7 | Axis_8 | RSD 丝杆 | SVD60NR5I | Modbus TCP（MotorPanel，SoftMotion 控制字） |

### 拉线传感器通道分配（BRT38 SSI，6路）

每路传感器占 **2 个 Modbus 寄存器**（低字在前，合成 32-bit 有符号计数值）：

| 通道 | 寄存器地址 | 默认名称 | 单位 | 换算参数（默认 5m BRT38） |
|------|-----------|---------|------|--------------------------|
| 0 | MW256~MW257 | 拉线 1 | mm | 周长 250mm / 单圈 4096 ≈ 0.061mm/count |
| 1 | MW258~MW259 | 拉线 2 | mm | 同上 |
| 2 | MW260~MW261 | 拉线 3 | mm | 同上 |
| 3 | MW262~MW263 | 拉线 4 | mm | 同上 |
| 4 | MW264~MW265 | 拉线 5 | mm | 同上 |
| 5 | MW266~MW267 | 拉线 6 | mm | 同上 |

换算公式：`length_mm = rawCounts × (绕线轮周长 / 单圈分辨率)`

> 修改传感器型号参数：`communication/PlcWorker.cpp` → `readSensors()` 中的
> `kSpoolCircumference[]`（绕线轮周长，mm）和 `kSingleTurnRes[]`（单圈分辨率）
>
> 常见型号参数参考（BRT38 系列）：
> | 量程 | 绕线轮周长 | 多圈位数 | 总数据位 |
> |------|-----------|---------|---------|
> | 1m   | 100mm     | 5-bit   | 17-bit  |
> | 3m   | 200mm     | 5-bit   | 17-bit  |
> | 5m   | 250mm     | 5-bit   | 17-bit  |
> | 6~7m | 225mm    | 7-bit   | 19-bit  |

### 系统通信架构（双通道）

```
[InProShop]  ← 开发阶段下载程序，运行时断开
      ↓
┌───────────────────────────────────────────────────────────┐
│           国辰正域 Stewart 平台控制器（汇川 PLC）            │
│  ┌──────────────┐  ┌────────────┐  ┌────────────────────┐ │
│  │ EtherCAT主站 │  │UDP服务端   │  │Modbus TCP 服务端   │ │
│  │  (内部总线)  │  │cmd:2000    │  │:502                │ │
│  │              │  │push:3001   │  │(MW区对外暴露)      │ │
│  └──────┬───────┘  └─────┬──────┘  └────────┬───────────┘ │
└─────────┼────────────────┼─────────────────  ┼─────────────┘
          │ EtherCAT总线   │ UDP               │ Modbus TCP
    ┌─────┴──────────────────────────┐         │
    │                                │         ▼
[InoSV660N×6] [SV660N] [SVD60NR5I]  │    [Qt 上位机]
[平台Axis_1~6] [卷筒]   [RSD]        │    ├─ PlatformWorker (UDP)
                                     │    │    UdpPlatformClient
[BRT38 SSI×6]                        │    │    → PlatformPanel
  RS422差分 SSI (17-bit绝对值)       │    │
  ↓                                  │    └─ PlcWorker (Modbus TCP)
[GL20 SSI模块] → [GL20-RTU-ECT]     │        ModbusTcpClient
  SSI扩展模块     EtherCAT从站网关   │        → MotorPanel (卷筒+RSD)
  PLC内映射至 MW256~MW267            │        → SensorPanel
  每路2寄存器(32-bit有符号计数值)    │            └ BRT38拉线 MW256~MW267
```

**关键原则**：
- 动平台 6 轴由 **UDP 协议直连**（PlatformWorker + UdpPlatformClient），**非** Modbus
- 卷筒/RSD 两轴通过 **Modbus TCP** 读写（PlcWorker + ModbusTcpClient），SoftMotion 控制字
- EtherCAT 总线由 PLC 内部管理，上位机不直接访问 EtherCAT
- 两条通讯路径各自跑独立 QThread，互不阻塞

---

## 技术栈

| 层次 | 技术 |
|------|------|
| UI 框架 | Qt 5/6 (C++) |
| PLC 通信 | Modbus TCP（libmodbus 或自实现） |
| 实时图表 | QCustomPlot 或 Qt Charts |
| 多线程 | QThread + 信号槽 |
| 配置持久化 | QSettings / JSON |
| 日志 | QFile + 自定义 Logger |

---

## 项目架构（实际文件结构）

```
DeckLab-2.0/
├── main.cpp
├── mainwindow.{h,cpp}                  # 主窗口：双连接栏 + 三列 QSplitter
├── communication/
│   ├── ModbusTcpClient.{h,cpp}        # Modbus TCP 客户端封装
│   ├── PlcWorker.{h,cpp}              # 后台线程：20ms 轮询（卷筒/RSD/BRT38）
│   └── RegisterMap.h                  # Reg:: 命名空间，MW 地址表
├── motor/
│   ├── MotorController.{h,cpp}        # SoftMotion 控制字封装（SV660N/SVD60NR5I）
│   └── MotorPanel.{h,cpp}             # 卷筒 + RSD 控制面板（axis 6 & 7）
├── platform/
│   ├── PlatformData.h                 # PlatformConfig/PlatformState/FuncCode 命名空间
│   ├── UdpPlatformClient.{h,cpp}      # UDP 底层：命令帧构建 + 推送帧解析
│   ├── PlatformWorker.{h,cpp}         # 后台线程：UDP 收发 + 看门狗（200ms）
│   └── PlatformPanel.{h,cpp}          # 动平台完整控制面板（上下电/模式/位姿目标/支链长）
├── sensor/
│   ├── SensorData.h                   # SensorChannel / SensorData（6路 BRT38 SSI）
│   └── SensorPanel.{h,cpp}            # 6 路拉线传感器数值显示（mm）
└── references/                        # 开发参考文档
```

**三栏布局**（mainwindow.cpp QSplitter）：
- **左栏** `MotorPanel`：卷筒（axis 6）+ RSD（axis 7）完整控制，连接 PlcWorker
- **中栏** `PlatformPanel`：Stewart 动平台，全功能（上下电/模式切换/位姿目标/支链长度显示），连接 PlatformWorker
- **右栏** `SensorPanel`：6 路 BRT38 SSI 拉线传感器实时数值，连接 PlcWorker

---

## 模块一：Modbus TCP 通信层

### 核心原则
- 所有 Modbus 读写必须在**非 UI 线程**中执行（`PlcWorker` 跑在 `QThread`）
- 使用**信号槽**将数据推送到 UI，禁止跨线程直接操作 Widget
- 连接断开时自动重连，重连间隔指数退避（500ms → 1s → 2s → 上限 10s）
- PLC IP 地址和端口通过配置文件读取，不硬编码

### Modbus TCP 客户端封装

```cpp
// communication/ModbusTcpClient.h
#pragma once
#include <QObject>
#include <QTcpSocket>

class ModbusTcpClient : public QObject {
    Q_OBJECT
public:
    explicit ModbusTcpClient(QObject* parent = nullptr);
    bool connectToPlc(const QString& ip, quint16 port = 502);
    void disconnect();
    bool isConnected() const;

    // 读保持寄存器（功能码 0x03）
    bool readHoldingRegisters(quint16 startAddr, quint16 count, QVector<quint16>& values);
    // 写单个寄存器（功能码 0x06）
    bool writeSingleRegister(quint16 addr, quint16 value);
    // 写多个寄存器（功能码 0x10）
    bool writeMultipleRegisters(quint16 startAddr, const QVector<quint16>& values);
    // 读线圈（功能码 0x01）
    bool readCoils(quint16 startAddr, quint16 count, QVector<bool>& values);
    // 写单个线圈（功能码 0x05）
    bool writeSingleCoil(quint16 addr, bool value);

signals:
    void connectionStateChanged(bool connected);
    void errorOccurred(const QString& msg);

private:
    QTcpSocket* m_socket;
    quint8 m_transactionId = 0;
    quint8 buildMbapHeader(QByteArray& buf, quint8 funcCode, quint16 dataLen);
};
```

### 轮询工作线程

```cpp
// communication/PlcWorker.h
class PlcWorker : public QObject {
    Q_OBJECT
public slots:
    void start();
    void stop();
signals:
    void motorDataUpdated(int axisId, MotorData data);
    void sensorDataUpdated(SensorData data);
    void connectionStateChanged(bool connected);
    void alarmTriggered(AlarmEvent event);
private:
    void pollCycle();         // 每个控制周期调用
    ModbusTcpClient* m_client;
    QTimer* m_timer;
    int m_pollInterval = 20; // ms，默认 50Hz
};
```

---

## 模块二：寄存器地址映射

### InProShop 配置规范

在 InProShop 中，需将以下 PLC 变量映射到 Modbus 保持寄存器（MW 区）：

```
// RegisterMap.h — 与 InProShop 中的地址配置严格对应
namespace Reg {
    // ===== 轴控制区（每轴占 10 个寄存器，轴0~轴7）=====
    // 轴 n 基地址 = AXIS_BASE + n * AXIS_STRIDE
    constexpr quint16 AXIS_BASE   = 0x0000;  // MW0
    constexpr quint16 AXIS_STRIDE = 10;

    // 相对基地址偏移
    constexpr quint16 AX_CTRL_WORD   = 0;   // 控制字（DS402）
    constexpr quint16 AX_TARGET_VEL  = 1;   // 目标速度（rpm * 10）
    constexpr quint16 AX_TARGET_POS  = 2;   // 目标位置低16位（pulse）
    constexpr quint16 AX_TARGET_POS_H= 3;   // 目标位置高16位
    constexpr quint16 AX_STATUS_WORD = 4;   // 状态字（只读）
    constexpr quint16 AX_ACT_VEL    = 5;   // 实际速度（只读）
    constexpr quint16 AX_ACT_POS    = 6;   // 实际位置低16位（只读）
    constexpr quint16 AX_ACT_POS_H  = 7;   // 实际位置高16位（只读）
    constexpr quint16 AX_TORQUE     = 8;   // 实际力矩（只读，0.1%）
    constexpr quint16 AX_ERROR_CODE = 9;   // 驱动器故障码（只读）

    // ===== 拉线位移传感器区（BRT38 SSI，6路）=====
    constexpr quint16 SENSOR_BASE        = 0x0100; // MW256 起
    constexpr int     SENSOR_COUNT       = 6;       // 传感器路数
    constexpr int     SENSOR_REGS_PER_CH = 2;       // 每路2个16-bit寄存器（低字在前，合成32-bit）
    // 通道 n：低字 = SENSOR_BASE + n*2，高字 = SENSOR_BASE + n*2 + 1
    // 通道0: MW256~257，通道1: MW258~259，...，通道5: MW266~267

    // ===== 系统状态区 =====
    constexpr quint16 SYS_STATUS    = 0x0200; // MW512 系统整体状态
    constexpr quint16 SYS_ALARM     = 0x0201; // MW513 报警字
    constexpr quint16 SYS_ESTOP     = 0x0202; // MW514 急停状态
}
```

**注意**：以上地址为示例，需与你在 InProShop 中实际配置的地址保持一致。

---

## 模块三：电机运动控制（SV660N / SVD60NR5I）

### SoftMotion 控制字（汇川 SoftMotion 自定义，非标准 DS402）

> **重要**：本项目 PLC 使用汇川 SoftMotion 轴控，控制字与标准 DS402 不同，
> 不可套用 DS402 的 Shutdown/SwitchOn/EnableOperation 序列。

```cpp
// PlcWorker.cpp 实际使用的 SoftMotion 控制字
namespace SoftMotion {
    // 控制字（写 AX_CTRL_WORD）
    constexpr quint16 CTRL_STOP       = 0x0000;  // 去使能/停止
    constexpr quint16 CTRL_ENABLE     = 0x0001;  // bit0=使能
    constexpr quint16 CTRL_RUN        = 0x0003;  // bit0=使能 + bit1=运行（触发速度/位置）
    constexpr quint16 CTRL_FAULT_RST  = 0x0009;  // bit0=使能 + bit3=故障复位
    // 状态字掩码（读 AX_STATUS_WORD）
    constexpr quint16 STAT_ENABLED    = 0x0001;  // bit0：当前已使能
    constexpr quint16 STAT_FAULT      = 0x0004;  // bit2：故障激活
}

class MotorController : public QObject {
    Q_OBJECT
public:
    // 轴 ID: 0~6 → SV660N x7, 7 → SVD60NR5I
    explicit MotorController(int axisId, ModbusTcpClient* client, QObject* parent = nullptr);

    void enable();
    void disable();
    void faultReset();
    void setVelocity(double rpm);           // 速度模式
    void setPosition(qint32 pulses, double rpm); // 位置模式
    void emergencyStop();

    MotorData currentData() const { return m_data; }
    void updateFromRegisters(const QVector<quint16>& regs); // 由 PlcWorker 调用

signals:
    void statusChanged(MotorData data);
    void faultOccurred(int axisId, quint16 errorCode);

private:
    int m_axisId;
    quint16 baseAddr() const { return Reg::AXIS_BASE + m_axisId * Reg::AXIS_STRIDE; }
    ModbusTcpClient* m_client;
    MotorData m_data;
};
```

### UI 面板规范

- 8 轴统一面板，Tab 页或列表区分
- 每轴必须包含：
  - 使能/去使能按钮（带绿/灰状态指示灯）
  - 目标速度/位置输入框（带范围校验，超限则拒绝发送）
  - 当前位置、速度、力矩实时数显
  - 驱动器故障码显示 + 一键复位按钮
  - **急停按钮**（红色，全局可见，任何状态下有效）
  - SoftMotion 状态文字（未使能/使能中/运行/故障）

**安全约束（必须实现）**：
1. 发送运动指令前检查使能状态，未使能则弹窗提示
2. 位置/速度超限时 UI 层拒绝发送，不依赖 PLC 保护
3. 急停后写 `SoftMotion::CTRL_STOP`（0x0000）到所有轴 + 写 `Reg::SYS_ESTOP_CMD`
4. SVD60NR5I 与 SV660N 使用相同 SoftMotion 控制字，可复用逻辑

---

## 模块三·B：UDP 平台通讯（国辰正域 Stewart）

### UDP 帧格式（小端序）

| 帧类型 | 长度 | 方向 | 说明 |
|--------|------|------|------|
| 命令帧 | 500 字节 | Qt→平台 | 写命令，目标端口 2000 |
| 推送帧 | 1578 字节 | 平台→Qt | 实时状态，本地绑定端口 3001 |

**命令帧结构**：帧头(0xAA55) + 时间戳(7×uint16) + 序列号(uint16) + 请求类型(1001读/2001写) + 条目数(uint16) + 数据区(每条12字节: uint16功能码+uint16保留+double值) + 帧尾(0x66BB)

**推送帧结构**：帧头(0xAA55) + 时间戳(7×uint16) + 180个功能码索引(uint16) + 150个double数据 + 帧尾(0x66BB)

### 关键功能码（FuncCode 命名空间）

| 功能码 | 方向 | 含义 |
|--------|------|------|
| 10000 | 写 | 上下电（1=上电，0=下电） |
| 10001 | 写 | 故障复位（1=复位） |
| 10010 | 写 | 运动模式（0=停止/1=支链点动/2=姿态点动/3=姿态定点/5=增量/6=跟随） |
| 10300~10305 | 写 | 目标位姿 X/Y/Z/Rx/Ry/Rz（mm/deg） |
| 20000 | 读/推送 | 设备状态字 |
| 21000~21005 | 推送 | 当前位姿 X/Y/Z/Rx/Ry/Rz |
| 20200~20205 | 推送 | 支链实际长度 1~6（mm） |
| 22100~22105 | 推送 | 外部传感器 1~6 |

### 连接状态检测
- `PlatformWorker` 启动 200ms QTimer 看门狗
- 连续 5 次（1s）未收到推送帧 → `connectionChanged(false)`
- 不需要主动心跳，平台持续推送状态帧

---

## 模块四：拉线位移传感器（BRT38 SSI）

### BRT38 SSI 传感器特性
- RS422 差分接口（SSI 协议），时钟频率 100kHz~5MHz
- 默认输出：二进制编码，多圈（M位）+ 单圈（12位=4096分辨率）
- 总数据位 = 多圈位数 + 12，例如：5m型 = 5+12 = 17-bit，最大计数 131072
- PLC 通过 SSI 主站模块读取后，将 32-bit 计数值拆为 2 个 16-bit Modbus 寄存器（低字在前）
- 电气寿命 >100000h，重复性精度 ±0.01%

### 数据读取与换算

```cpp
// PlcWorker.cpp — readSensors()
const int totalRegs = Reg::SENSOR_COUNT * Reg::SENSOR_REGS_PER_CH; // 6×2=12
QVector<quint16> regs;
m_client->readHoldingRegisters(Reg::SENSOR_BASE, totalRegs, regs);

for (int i = 0; i < Reg::SENSOR_COUNT; ++i) {
    int base = i * Reg::SENSOR_REGS_PER_CH;
    quint32 raw = (quint32(regs[base + 1]) << 16) | quint32(regs[base]); // 高字在后
    qint32 counts = qint32(raw);
    float mm = float(counts) * (kSpoolCircumference[i] / kSingleTurnRes[i]);
}
```

### SensorChannel 数据结构

```cpp
// sensor/SensorData.h
struct SensorChannel {
    QString name;           // "拉线 1" ~ "拉线 6"
    QString unit;           // 固定 "mm"
    qint32  rawCounts = 0;  // SSI 原始 32-bit 计数（有符号）
    float   displayValue = 0.0f; // 换算后位移（mm）
    bool    valid = false;
};

struct SensorData {
    static constexpr int COUNT = 6;
    std::array<SensorChannel, COUNT> channels;
};
```

### 显示规范（SensorPanel）
- 6 行数值显示，Consolas 等宽字体，精度 2 位小数
- 未连接时显示 "---"
- 预留位姿解算接口：后续根据 6 路长度计算被测平台的 6-DOF 位姿

### 后续：位姿解算
6 路拉线读数 → 正向运动学（位置+姿态）：

```
[L1, L2, L3, L4, L5, L6] → FK算法 → [X, Y, Z, Roll, Pitch, Yaw]
```

位姿解算模块建议独立为 `sensor/PoseCalculator.{h,cpp}`，由 `SensorPanel` 接收
`SensorData` 后调用，结果通过信号发出供显示用。

---

## 模块五：报警与故障处理

### 报警分级

| 级别 | 颜色 | 触发条件 | 行为 |
|------|------|----------|------|
| INFO | 蓝 | 通信统计、状态切换 | 记录日志 |
| WARNING | 黄 | 速度/位置接近限位 | 状态栏提示 |
| ERROR | 橙 | 驱动器故障码非零 | 弹出通知，需手动确认 |
| CRITICAL | 红 | 急停触发 / Modbus 通信中断 | 弹窗 + 自动发急停 + 声音 |

### SV660N 常见故障码映射（部分）

```cpp
static const QMap<quint16, QString> SV660N_ERRORS = {
    {0x2310, "过流"},      {0x3210, "过压"},
    {0x3220, "欠压"},      {0x4210, "过温"},
    {0x7300, "编码器异常"}, {0x8612, "跟随误差超限"},
    // ... 参考《SV660N 用户手册》附录错误码表
};
```

报警列表 Widget 必须包含：
- 时间戳、级别、轴号/模块、描述、确认状态
- 未确认报警数量徽章
- 一键全部确认 / 单条确认
- 导出报警历史为 CSV

---

## UI/UX 设计规范

1. **主色调**：深色工业风（`#2B2B2B` 背景，白色文字）
2. **字体**：等宽字体显示数值（Consolas），正文用系统默认
3. **状态指示灯**：自定义 `QLed` Widget（圆形，绿/黄/红/灰四态）
4. **布局**：主窗口用 `QDockWidget` 可拖拽面板
5. **刷新率解耦**：Modbus 轮询 20ms（50Hz），UI 最多 50ms 刷新一次
6. **急停按钮**：固定在主窗口顶栏，任何页面均可见，不可被遮挡

---

## Qt .pro 文件关键配置

```qmake
QT += core gui widgets charts network
CONFIG += c++17

# QCustomPlot（直接加源码）
SOURCES += qcustomplot.cpp
HEADERS += qcustomplot.h

# libmodbus（可选，也可自实现 Modbus TCP）
# INCLUDEPATH += /usr/include/modbus
# LIBS += -lmodbus
```

---

## InProShop 配置要点（提示给用户）

1. **EtherCAT 配置**：在 InProShop 中扫描从站，确认 SV660N×7、SVD60NR5I×1、GL20-RTU-ECT×1 均在线
2. **变量映射**：将需要上位机读写的 PLC 变量（轴控制字、速度设定、状态字、传感器值等）映射到 MW 区（Modbus 保持寄存器），记录每个变量的寄存器地址
3. **Modbus TCP 使能**：在 AC702 以太网口配置中启用 Modbus TCP Server，确认 IP 地址和端口（默认 502）
4. **扫描周期**：PLC 扫描周期建议 ≤ 4ms，确保伺服控制实时性

---

## 开发工作流

当用户提出具体需求时，按以下顺序思考并输出：

1. **确认需求** — 明确是哪个模块、哪个轴（SV660N / SVD60NR5I）或哪个 IO 通道（GL20）
2. **定位寄存器地址** — 对应 `RegisterMap.h` 中的地址，确认读/写方向
3. **定位架构位置** — 代码属于哪个类/文件
4. **考虑线程安全** — 是否涉及跨线程数据访问（所有 Modbus 操作在 PlcWorker 线程）
5. **编写代码** — 头文件先于实现，信号槽优于直接调用
6. **给出集成说明** — 如何与现有模块连接，以及 InProShop 端需要配置什么

---

## 参考文件索引

| 文件 | 内容 |
|------|------|
| `references/modbus-communication.md` | ModbusTcpClient 完整实现、PlcWorker 轮询线程、重连机制、错误码 |
| `references/register-map.md` | RegisterMap.h 完整定义、8轴地址表、BRT38传感器区（MW256~MW267）、InProShop 配置步骤 |
| `references/motor-control.md` | SoftMotion 控制字（非DS402）、SV660N/SVD60NR5I 控制指令、安全联锁、故障码对照 |
| `references/sensor-display.md` | BRT38 SSI 换算参数、SensorPanel 显示规范、位姿解算接口预留 |
| `references/alarm-system.md` | 报警规则引擎、SV660N 故障码、Modbus 断线报警、AlarmPanel Widget |
| `BRT38拉线盒SSI通信.pdf` | BRT38 系列拉线传感器官方手册：SSI/BISS协议、接线图、指示灯说明、量程参数表 |
