#pragma once
#include <QWidget>
#include <QVector>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QGroupBox>
#include "MotorController.h"

// -----------------------------------------------------------------------
// MotorPanel — 左侧本地电机控制面板
//
// 轴分配：
//   index 0 → 物理轴 7 → 卷筒电机
//   index 1 → 物理轴 8 → RSD 丝杆
//
// 提供完整控制：使能/去使能、速度设定、位置定位、故障复位
// -----------------------------------------------------------------------
class MotorPanel : public QWidget {
    Q_OBJECT
public:
    static constexpr int LOCAL_AXIS_COUNT = 2;
    // Qt 0-based 轴 ID：卷筒=6（PLC Axis_7），RSD=7（PLC Axis_8）
    static constexpr int AXIS_IDS[LOCAL_AXIS_COUNT] = {6, 7};

    explicit MotorPanel(QWidget* parent = nullptr);

public slots:
    void onMotorDataUpdated(int axisId, const MotorData& data);
    void onConnectionChanged(bool connected);

signals:
    void enableAxisRequested(int axisId);
    void disableAxisRequested(int axisId);
    void faultResetRequested(int axisId);
    void setVelocityRequested(int axisId, double rpm);
    void setPositionRequested(int axisId, qint32 pulses, double approachRpm);
    void emergencyStopAllRequested();

private:
    struct AxisWidgets {
        int             physicalId       = 0;   // Modbus 轴 ID（6 或 7）
        QGroupBox*      group            = nullptr;
        QLabel*         stateLabel       = nullptr;
        QLabel*         actVelLabel      = nullptr;
        QLabel*         actPosLabel      = nullptr;
        QLabel*         torqueLabel      = nullptr;
        QLabel*         errorLabel       = nullptr;
        QPushButton*    enableBtn        = nullptr;
        QPushButton*    disableBtn       = nullptr;
        QPushButton*    faultResetBtn    = nullptr;
        QDoubleSpinBox* velInput         = nullptr;
        QPushButton*    velApplyBtn      = nullptr;
        QSpinBox*       posInput         = nullptr;
        QDoubleSpinBox* approachVelInput = nullptr;
        QPushButton*    posApplyBtn      = nullptr;
    };

    void setupUi();
    void buildAxisWidget(int index, int physicalAxisId, const QString& name);

    QVector<AxisWidgets> m_axisWidgets;
    QPushButton*         m_eStopBtn  = nullptr;
    QLabel*              m_connLabel = nullptr;
};
