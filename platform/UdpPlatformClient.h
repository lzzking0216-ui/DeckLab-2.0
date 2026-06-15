#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QByteArray>
#include <QString>
#include <QVector>
#include "PlatformData.h"

// -----------------------------------------------------------------------
// UdpPlatformClient — 国辰正域 Stewart 平台 UDP 通讯底层
//
// 协议帧格式（命令帧，500 字节，小端序）：
//   [0..1]   0xAA 0x55           帧头
//   [2..15]  7×uint16            时间戳（年月日时分秒毫秒）
//   [16..17] uint16              序列号
//   [18..19] uint16              请求类型（0x03E9=1001读 / 0x07D1=2001写）
//   [20..21] uint16              数据区条目数
//   [22..497] 476 字节           数据区（每条：uint16 功能码 + uint16 保留 + double 值 = 12 字节）
//   [498..499] 0x66 0xBB         帧尾
//
// 实时推送帧（1578 字节，设备→本机 3001 端口）：
//   [0..1]   0xAA 0x55           帧头
//   [2..15]  7×uint16            时间戳
//   [16..375] 180×uint16         功能码索引（0=未使用）
//   [376..1575] 150×double       数据区（对应功能码索引中前 150 个非零条目）
//   [1576..1577] 0x66 0xBB       帧尾
// -----------------------------------------------------------------------
class UdpPlatformClient : public QObject {
    Q_OBJECT
public:
    static constexpr int CMD_FRAME_SIZE  = 500;
    static constexpr int PUSH_FRAME_SIZE = 1578;
    static constexpr quint16 REQ_READ  = 1001;
    static constexpr quint16 REQ_WRITE = 2001;
    static constexpr int MAX_ENTRIES_PER_FRAME = 39;  // 476 / 12

    explicit UdpPlatformClient(const PlatformConfig& cfg, QObject* parent = nullptr);

    bool bindSockets();
    void close();

    bool writeValue(quint32 funcCode, double value);
    bool writeValues(const QVector<QPair<quint32, double>>& entries);

signals:
    void platformStateReceived(const PlatformState& state);
    void errorOccurred(const QString& msg);

private slots:
    void onPushDataReady();

private:
    QByteArray buildWriteFrame(const QVector<QPair<quint32, double>>& entries);
    void       parsePushFrame(const QByteArray& data);

    PlatformConfig m_cfg;
    QUdpSocket*    m_cmdSocket  = nullptr;
    QUdpSocket*    m_pushSocket = nullptr;
    quint16        m_seqNo      = 0;
};
