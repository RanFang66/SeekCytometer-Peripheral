#ifndef PZTCTRLWIDGET_H
#define PZTCTRLWIDGET_H

#include <QDockWidget>
#include <QObject>
#include "ArrowButton.h"
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include "StatusLight.h"
#include <QGroupBox>

#define PZT_CMD_IDLE            0
#define PZT_CMD_MOVE_ABS        1
#define PZT_CMD_MOVE_REL        2
#define PZT_CMD_PRESS           3
#define PZT_CMD_UPDATE_ORGIN    4
#define PZT_CMD_UPDATE_RC       5
#define PZT_CMD_RESET           6

#define MOTOR_NUM   2


class PZTMotorWidget : public QGroupBox
{
    Q_OBJECT

public:
    PZTMotorWidget(const QString &title, int id, QWidget *parent=nullptr);

    void updateStatus(const QVector<uint16_t> &regs);

private slots:
    void onBtnMotorUpClicked();
    void onBtnMotorDownClicked();
    void onBtnMotorRunToPos();
    void onBtnMotorPressClicked();
    void onMotorResetClicked();
    void onBtnMotorStopClicked();
    void onBtnMotorUpdateOriginClicked();
    void onBtnMotorUpdateRCClicked();
    void onSpinMotorStepsSet();
    void onSpinMotorTargetPosSet();
    void onSpinMotorSpeedSet();
    void onSpinMotorPeriodSet();
    void onSpinTriggerSet();

private:
    void initWidget();

    ArrowButton *btnMotorUp;
    ArrowButton *btnMotorDown;
    QPushButton *btnMotorRunToPos;
    QPushButton *btnMotorPress;
    QPushButton *btnMotorReset;
    QPushButton *btnMotorStop;
    QPushButton *btnMotorUpdateOrigin;
    QPushButton *btnMotorUpdateRC;


    QSpinBox    *spinMotorSteps;
    QSpinBox    *spinMotorPeriod;
    QSpinBox    *spinMotorPosTarget;
    QSpinBox    *spinMotorSpeed;
    QSpinBox    *spinMotorTrigger;

    StatusLight *statusLightMotor;
    QLabel      *lblMotorPos;
    QLabel      *lblAdcInfo;
    QLabel      *lblRCInfo;

    int         m_id;
    int         m_status;
    int         m_pos;
    int         m_adcBaseLine;
    int         m_adcThreshold;
    int         m_rc;
};


class PZTCtrlWidget : public QDockWidget
{
    Q_OBJECT
public:
    PZTCtrlWidget(const QString &tilte, QWidget *parent = nullptr);
    void updateStatus(const QVector<uint16_t> &regs);

private:
    void initDockWidget();
    PZTMotorWidget *motor1;
    PZTMotorWidget *motor2;
};

#endif // PZTCTRLWIDGET_H
