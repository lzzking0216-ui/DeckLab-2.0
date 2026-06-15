#pragma once
#include <QWidget>
#include <QLabel>
#include <QVector>
#include "SensorData.h"

// -----------------------------------------------------------------------
// SensorPanel — 右侧拉线位移传感器显示面板
// 显示 6 路 BRT38 SSI 传感器的实时位移（mm），不含控制功能
// -----------------------------------------------------------------------
class SensorPanel : public QWidget {
    Q_OBJECT
public:
    explicit SensorPanel(QWidget* parent = nullptr);

public slots:
    void onSensorDataUpdated(const SensorData& data);
    void onConnectionChanged(bool connected);

private:
    struct ChannelRow {
        QLabel* nameLabel  = nullptr;
        QLabel* valueLabel = nullptr;
        QLabel* unitLabel  = nullptr;
    };

    void setupUi();

    QVector<ChannelRow> m_rows;
};
