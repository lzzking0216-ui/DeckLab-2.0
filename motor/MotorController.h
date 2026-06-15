#pragma once
#include <QObject>
#include <QString>
#include <QMap>
#include <QtGlobal>

// -----------------------------------------------------------------------
// DS402 状态字解析
// -----------------------------------------------------------------------
inline QString ds402StateName(quint16 sw)
{
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

// -----------------------------------------------------------------------
// SVD60NR5I SoftMotion 自定义状态字解析（MW74位定义）
// Bit0=已使能  Bit1=运行到速  Bit2=故障  Bit3=已停止
// -----------------------------------------------------------------------
inline QString smStateName(quint16 sw)
{
    if (sw & 0x0004) return "故障";
    if (sw & 0x0001) {
        if (sw & 0x0002) return "运行中";
        if (sw & 0x0008) return "已停止";
        return "已使能";
    }
    return "未使能";
}

// -----------------------------------------------------------------------
// 驱动器故障码（SV660N / SVD60NR5I 通用）
// -----------------------------------------------------------------------
inline QString faultCodeToString(quint16 code)
{
    static const QMap<quint16, QString> table = {
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
    };
    if (code == 0) return "正常";
    return table.value(code, QString("未知故障(0x%1)").arg(code, 4, 16, QChar('0')));
}

// -----------------------------------------------------------------------
// 轴数据结构
// -----------------------------------------------------------------------
struct MotorData {
    int     axisId     = 0;
    quint16 statusWord = 0;
    qint16  actualVel  = 0;    // rpm（有符号，值 / 10 = 实际 rpm）
    qint32  actualPos  = 0;    // pulse（32位）
    qint16  torque     = 0;    // 0.1% 额定力矩
    quint16 errorCode  = 0;
    bool    enabled    = false;
    bool    faultActive= false;
    QString stateName;
};

// -----------------------------------------------------------------------
// MotorController — 轻量数据缓存，由 PlcWorker 更新
// -----------------------------------------------------------------------
class MotorController : public QObject {
    Q_OBJECT
public:
    explicit MotorController(int axisId, QObject* parent = nullptr)
        : QObject(parent) { m_data.axisId = axisId; }

    int axisId() const { return m_data.axisId; }
    MotorData currentData() const { return m_data; }

    void updateFromData(const MotorData& data) {
        m_data = data;
        emit statusChanged(data);
        if (data.faultActive && data.errorCode != 0)
            emit faultOccurred(data.axisId, data.errorCode);
    }

signals:
    void statusChanged(MotorData data);
    void faultOccurred(int axisId, quint16 errorCode);

private:
    MotorData m_data;
};
