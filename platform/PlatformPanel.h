#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <array>
#include "PlatformData.h"

// -----------------------------------------------------------------------
// PlatformPanel — 国辰正域 Stewart 六自由度动平台控制面板（UDP）
//
// 功能：设备连接状态显示、上下电、故障复位、运动模式选择、
//       姿态目标输入、当前位姿实时显示、支链长度显示
// -----------------------------------------------------------------------
class PlatformPanel : public QWidget {
    Q_OBJECT
public:
    explicit PlatformPanel(QWidget* parent = nullptr);

public slots:
    void onPlatformStateUpdated(const PlatformState& state);
    void onConnectionChanged(bool online);

signals:
    void powerOnRequested();
    void powerOffRequested();
    void faultResetRequested();
    void setMotionModeRequested(int mode);
    void sendPoseTargetRequested(double x, double y, double z,
                                 double rx, double ry, double rz);
    void stopMotionRequested();

private:
    struct PoseDisplay {
        QLabel* label = nullptr;
        QLabel* value = nullptr;
        QLabel* unit  = nullptr;
    };

    struct PoseInput {
        QLabel*         label = nullptr;
        QDoubleSpinBox* spin  = nullptr;
        QLabel*         unit  = nullptr;
    };

    void setupUi();
    void buildStatusSection(QWidget* parent, class QVBoxLayout* vbox);
    void buildCurrentPoseSection(QWidget* parent, QVBoxLayout* vbox);
    void buildControlSection(QWidget* parent, QVBoxLayout* vbox);
    void buildTargetPoseSection(QWidget* parent, QVBoxLayout* vbox);
    void buildChainSection(QWidget* parent, QVBoxLayout* vbox);

    QLabel*      m_connIndicator  = nullptr;
    QLabel*      m_powerState     = nullptr;
    QLabel*      m_modeDisplay    = nullptr;
    QLabel*      m_faultIndicator = nullptr;
    QPushButton* m_powerOnBtn     = nullptr;
    QPushButton* m_powerOffBtn    = nullptr;
    QPushButton* m_faultResetBtn  = nullptr;

    QComboBox*   m_modeCombo  = nullptr;
    QPushButton* m_modeApply  = nullptr;
    QPushButton* m_stopBtn    = nullptr;

    std::array<PoseDisplay, 6> m_poseDisplay;
    std::array<PoseInput, 6>   m_poseInput;
    QPushButton* m_sendTargetBtn = nullptr;

    std::array<QLabel*, 6> m_chainLabels = {};

    bool m_online = false;

    void updateButtonStates();
    static QString modeName(int mode);
};
