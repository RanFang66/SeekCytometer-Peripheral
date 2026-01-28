#ifndef OPTICSCONTROLWIDGET_H
#define OPTICSCONTROLWIDGET_H

#include <QDockWidget>
#include <QObject>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include "ToggleSwitch.h"
#include "StatusLight.h"

class OpticsControlWidget : public QDockWidget
{
    Q_OBJECT
public:
    OpticsControlWidget(const QString &tilte, QWidget *parent = nullptr);

    void updateStatus(const QVector<uint16_t> &regs);

private slots:
    void onLaser1Toggled(bool checked);
    void onLaser2Toggled(bool checked);
    void onLedToggled(bool checked);
    void onLaser1IntensityChanged();
    void onLaser2IntensityChanged();
    void onLedIntensityChanged();

private:
    void initDockWidget();

    ToggleSwitch *btnLaser_1;
    ToggleSwitch *btnLaser_2;
    ToggleSwitch *btnLed;

    QSpinBox *spinLaserIntensity_1;
    QSpinBox *spinLaserIntensity_2;
    QSpinBox *spinLedIntensity;

    // QLabel  *lblLaser1Status;
    // QLabel  *lblLaser2Status;
    // QLabel  *lblLedStatus;

    StatusLight *statusLightLaser1;
    StatusLight *statusLightLaser2;
    StatusLight *statusLightLed;


    int     statusLaser1;
    int     statusLaser2;
    int     statusLed;
    int     intensityLaser1;
    int     intensityLaser2;
    int     intensityLed;
};

#endif // OPTICSCONTROLWIDGET_H
