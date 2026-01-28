#ifndef TEMPCONTROLWIDGET_H
#define TEMPCONTROLWIDGET_H

#include <QDockWidget>
#include <QObject>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTimer>
#include "ToggleSwitch.h"
#include "ToggleRunButton.h"
#include "StatusLight.h"

#define TEMP_CTRL_IDLE      0x0000
#define TEMP_CTRL_RUNNING   0x0001
#define TEMP_CTRL_FAULT     0x0002


class TempControlWidget : public QDockWidget
{
    Q_OBJECT
public:
    TempControlWidget(const QString &tilte, QWidget *parent = nullptr);

    void updateStatus(const QVector<uint16_t> &regs);

private slots:
    void onTempControlToggled(bool checked);
    void onTempTargetChanged(double value);
    void onFanChToggled(uint16_t ch, bool checked);
    void onFanSpeedChanged(uint16_t ch, int value);
    void onFanControlToggled(bool checked);


private:
    static constexpr int FAN_NUM = 4;

    void initTempControlWidget();

    ToggleSwitch *chkFans[FAN_NUM];
    QSpinBox *spinFanSpeed[FAN_NUM];
    ToggleRunButton *btnFanEnable;
    ToggleRunButton *btnTempControl;
    StatusLight *lblTempControlStatus;
    QDoubleSpinBox *spinTargetTemp;
    QLabel          *lblCurrentTemp;

    uint16_t    tempCtrlStatus;
    float       currentTemp;

};

#endif // TEMPCONTROLWIDGET_H
