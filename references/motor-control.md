# 电机运动控制 — 完整参考（SV660N / SVD60NR5I）

## 硬件说明

| 轴 ID | 驱动器型号 | 备注 |
|-------|-----------|------|
| 0 ~ 6 | 汇川 SV660N | 7台 |
| 7     | 汇川 SVD60NR5I | 1台 |

所有驱动器均通过 EtherCAT 连接到 AC702，**全部使用 SoftMotion 自动 PDO 映射**，控制字/状态字采用 SoftMotion 自定义格式（见下方说明），上位机代码统一处理。

---

## SoftMotion 控制字 / 状态字

**控制字（写）：**

| 操作 | Controlword 值 | 说明 |
|------|----------------|------|
| 去使能/停止 | 0x0000 | 清所有位 |
| 使能 | 0x0001 | Bit0=使能 |
| 速度运行 | 0x0003 | Bit0=使能 + Bit1=运行 |
| 故障复位 | 0x0009 | Bit0=使能 + Bit3=复位，100ms 后回 0x0001 |
| 急停 | 0x0000 | 清控制字，同时写 SYS_ESTOP_CMD=1 |

**状态字（读，Bit定义）：**

| 位 | 含义 |
|----|------|
| Bit0 | 已使能 |
| Bit1 | 运行中（已到速）|
| Bit2 | 故障 |
| Bit3 | 已停止 |

---

## 数据结构

```cpp
// motor/MotorController.h

struct MotorData {
    int     axisId;
    quint16 statusWord;
    qint16  actualVel;       // rpm（有符号）
    qint32  actualPos;       // pulse（32位，由高低寄存器合并）
    qint16  torque;          // 0.1% 额定力矩（有符号）
    quint16 errorCode;       // 驱动器故障码
    bool    enabled;         // Operation Enabled 位
    bool    faultActive;     // Fault 位
    QString stateName;       // 状态文字描述
};

struct MotorCommand {
    enum Type { Enable, Disable, EmergencyStop, SetVelocity, SetPosition, FaultReset };
    Type    type;
    double  velocityRpm  = 0.0;
    qint32  positionPulse = 0;
};

// DS402 状态名解析
inline QString ds402StateName(quint16 sw) {
    if (sw & 0x0008) return "故障";
    quint16 masked = sw & 0x006F;
    if (masked == 0x0040) return "未就绪";
    if (masked == 0x0060) return "禁止使能";
    if (masked == 0x0021) return "准备使能";
    if (masked == 0x0023) return "已上电";
    if (masked == 0x0027) return "运行使能";
    if (masked == 0x0007) return "快停激活";
    return QString("未知(0x%1)").arg(sw, 4, 16, QChar('0'));
}
```

---

## MotorController 类

```cpp
// motor/MotorController.h
class MotorController : public QObject {
    Q_OBJECT
public:
    // axisId: 0~6 = SV660N, 7 = SVD60NR5I
    explicit MotorController(int axisId, QObject* parent = nullptr);

    // 由 PlcWorker 调用（工作线程中），更新缓存状态
    void updateFromData(const MotorData& data);

    // 生成写寄存器指令（发射信号到 PlcWorker）
    void enable();
    void disable();
    void faultReset();
    void setVelocity(double rpm);
    void setPosition(qint32 pulses, double approachRpm);
    void emergencyStop();

    MotorData currentData() const { return m_data; }

signals:
    // 上层连接到 PlcWorker::writeRegister / writeRegisters
    void writeRegisterRequested(quint16 addr, quint16 value);
    void writeRegistersRequested(quint16 addr, QVector<quint16> values);
    void statusChanged(MotorData data);
    void faultOccurred(int axisId, quint16 errorCode);

private:
    int       m_axisId;
    MotorData m_data;

    quint16 baseAddr() const {
        return Reg::AXIS_BASE + m_axisId * Reg::AXIS_STRIDE;
    }
    void writeCtrlWord(quint16 cw) {
        emit writeRegisterRequested(baseAddr() + Reg::AX_CTRL_WORD, cw);
    }
};
```

```cpp
// motor/MotorController.cpp — 关键实现

void MotorController::enable() {
    // DS402 上电序列：Shutdown → Switch On → Enable Operation
    writeCtrlWord(0x0006);
    QThread::msleep(30);   // 等待 PLC 扫描周期响应
    writeCtrlWord(0x0007);
    QThread::msleep(30);
    writeCtrlWord(0x000F);
}

void MotorController::disable() {
    writeCtrlWord(0x0006);  // Shutdown
}

void MotorController::faultReset() {
    writeCtrlWord(0x0000);  // 先写0确保上升沿
    QThread::msleep(20);
    writeCtrlWord(0x0080);  // 上升沿触发清故障
    QThread::msleep(50);
    writeCtrlWord(0x0006);  // 回到 Shutdown 状态
}

void MotorController::setVelocity(double rpm) {
    // 写目标速度（rpm × 10，整数形式）
    quint16 addr = baseAddr() + Reg::AX_TARGET_VEL;
    qint16 val = static_cast<qint16>(rpm * 10);
    emit writeRegisterRequested(addr, static_cast<quint16>(val));
}

void MotorController::setPosition(qint32 pulses, double approachRpm) {
    quint16 base = baseAddr();
    // 先写速度
    qint16 rpmVal = static_cast<qint16>(approachRpm * 10);
    emit writeRegisterRequested(base + Reg::AX_TARGET_VEL, static_cast<quint16>(rpmVal));
    // 写目标位置（32位拆成高低16位）
    QVector<quint16> posRegs = {
        static_cast<quint16>((quint32)pulses >> 16),   // 高位
        static_cast<quint16>(pulses & 0xFFFF)           // 低位
    };
    emit writeRegistersRequested(base + Reg::AX_TARGET_POS, posRegs);
    // New Setpoint 上升沿触发
    QThread::msleep(10);
    writeCtrlWord(0x001F);
    QThread::msleep(10);
    writeCtrlWord(0x000F);
}

void MotorController::emergencyStop() {
    writeCtrlWord(0x000B);  // Quick Stop
}
```

---

## MotorPanel Widget

```cpp
// motor/MotorPanel.h
class MotorPanel : public QWidget {
    Q_OBJECT
public:
    // 创建时传入8个 MotorController 指针
    explicit MotorPanel(QVector<MotorController*> controllers, QWidget* parent = nullptr);

public slots:
    void onDataUpdated(int axisId, const MotorData& data);

signals:
    void emergencyStopAll();  // 全局急停，连接到所有轴

private:
    void setupUi();
    void buildAxisWidget(int axisId);   // 为每个轴创建子面板

    QVector<MotorController*> m_controllers;

    // 每个轴的控件组（用结构体管理）
    struct AxisWidgets {
        QLabel*         statusLabel;
        QPushButton*    enableBtn;
        QDoubleSpinBox* velInput;
        QSpinBox*       posInput;
        QLabel*         actVelLabel;
        QLabel*         actPosLabel;
        QLabel*         actTorqueLabel;
        QLabel*         errorCodeLabel;
        QPushButton*    faultResetBtn;
    };
    QVector<AxisWidgets> m_axisWidgets;

    QPushButton* m_globalEStopBtn;  // 固定在顶栏，全局可见
};
```

**急停按钮样式（必须醒目）：**
```cpp
void MotorPanel::setupGlobalEStop() {
    m_globalEStopBtn = new QPushButton("⚠ 全局急停", this);
    m_globalEStopBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #CC0000; color: white;"
        "  font-size: 14pt; font-weight: bold;"
        "  border-radius: 6px; min-height: 44px; min-width: 140px;"
        "}"
        "QPushButton:pressed { background-color: #880000; }"
    );
    // 急停：无需使能检查，直接发所有轴 Quick Stop
    connect(m_globalEStopBtn, &QPushButton::clicked, this, [this]{
        for (auto* ctrl : m_controllers)
            ctrl->emergencyStop();
        emit emergencyStopAll();
    });
}
```

---

## 安全联锁检查

```cpp
void MotorPanel::onSetVelocityClicked(int axisId) {
    const auto& d = m_controllers[axisId]->currentData();

    // 1. 检查使能状态
    if (!d.enabled) {
        QMessageBox::warning(this, "操作无效",
            QString("轴%1 未使能，请先使能再设置速度").arg(axisId));
        return;
    }

    double vel = m_axisWidgets[axisId].velInput->value();

    // 2. UI 层速度范围检查（不依赖 PLC 保护）
    double maxVel = Config::instance()->get(
        QString("axis%1/maxVelocityRpm").arg(axisId), 3000.0);
    if (qAbs(vel) > maxVel) {
        QMessageBox::warning(this, "超限",
            QString("轴%1 速度 %2 rpm 超过最大值 %3 rpm")
            .arg(axisId).arg(vel).arg(maxVel));
        return;
    }

    m_controllers[axisId]->setVelocity(vel);
}
```

---

## SV660N 故障码对照表（常见）

```cpp
// motor/MotorController.h
static const QMap<quint16, QString> SV660N_FAULT_CODES = {
    {0x2310, "过流（A相）"},
    {0x2320, "过流（B相）"},
    {0x3210, "母线过压"},
    {0x3220, "母线欠压"},
    {0x4210, "驱动器过温"},
    {0x4310, "电机过温"},
    {0x5210, "EEPROM 故障"},
    {0x6320, "参数设置错误"},
    {0x7300, "编码器断线"},
    {0x7301, "编码器通信故障"},
    {0x8400, "速度偏差过大"},
    {0x8611, "位置跟随误差超限"},
    {0x8612, "位置超程"},
    {0xFF01, "STO 安全停止"},
    // 完整列表参考《SV660N 用户手册》附录 A
};

inline QString faultCodeToString(quint16 code) {
    return SV660N_FAULT_CODES.value(code,
        code == 0 ? "正常" : QString("未知故障(0x%1)").arg(code, 4, 16, QChar('0')));
}
```
