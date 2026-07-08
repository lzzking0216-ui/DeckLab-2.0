#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QToolButton>
#include <array>
#include "PlatformData.h"

class QHBoxLayout;
class QRadioButton;

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
    struct ValueRow {
        QLabel* caption = nullptr;
        QLabel* value = nullptr;
        QLabel* unit = nullptr;
    };

    struct TargetRow {
        QLabel* caption = nullptr;
        QDoubleSpinBox* spin = nullptr;
        QLabel* unit = nullptr;
    };

    void setupUi();
    QWidget* buildSystemPane(QWidget* parent);
    QWidget* buildMotionPane(QWidget* parent);
    QWidget* buildHeader(const QString& title, QWidget* parent);
    QWidget* buildFixedPointPage(QWidget* parent);
    QWidget* buildIncrementPage(QWidget* parent);
    QWidget* buildSinePage(QWidget* parent);
    QWidget* buildTrajectoryPage(QWidget* parent);
    void addModeButton(const QString& text, int pageIndex, QHBoxLayout* row);
    void selectMotionPage(int pageIndex);
    void updateButtonStates();
    void updateTargetRanges();
    void updatePoseReadouts();
    void sendFixedTarget();
    void sendIncrementStep(int axis, double direction);
    static QString modeName(int mode);

    QLabel* m_connIndicator = nullptr;
    QLabel* m_powerState = nullptr;
    QLabel* m_modeDisplay = nullptr;
    QLabel* m_faultIndicator = nullptr;
    QPushButton* m_powerOnBtn = nullptr;
    QPushButton* m_powerOffBtn = nullptr;
    QPushButton* m_faultResetBtn = nullptr;

    std::array<ValueRow, 6> m_poseRows;
    std::array<TargetRow, 6> m_targetRows;
    std::array<QLabel*, 6> m_incrementValueLabels = {};
    std::array<QPushButton*, 6> m_incrementMinusBtns = {};
    std::array<QPushButton*, 6> m_incrementPlusBtns = {};

    QComboBox* m_coordCombo = nullptr;
    QStackedWidget* m_motionStack = nullptr;
    QButtonGroup* m_modeButtons = nullptr;
    QPushButton* m_fixedExecuteBtn = nullptr;
    QPushButton* m_fixedResetBtn = nullptr;
    QDoubleSpinBox* m_translateStepSpin = nullptr;
    QDoubleSpinBox* m_rotateStepSpin = nullptr;
    QRadioButton* m_incrementSingleRadio = nullptr;
    QRadioButton* m_incrementContinuousRadio = nullptr;
    QPushButton* m_incrementStopBtn = nullptr;
    QPushButton* m_incrementResetBtn = nullptr;
    QPushButton* m_sineExecuteBtn = nullptr;
    QPushButton* m_sineResetBtn = nullptr;
    QPushButton* m_trajectoryStartBtn = nullptr;
    QPushButton* m_trajectoryStopBtn = nullptr;

    bool m_online = false;
    PlatformPose m_currentPose;
};
