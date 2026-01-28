#ifndef SAMPLECHIPWIDGET_H
#define SAMPLECHIPWIDGET_H

#include <QDockWidget>
#include <QObject>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTimer>
#include "ArrowButton.h"
#include <QCheckBox>
#include "ToggleSwitch.h"

class SampleChipWidget : public QDockWidget
{
    Q_OBJECT
public:
    SampleChipWidget(const QString &tilte, QWidget *parent = nullptr);
    void updateStatus(const QVector<uint16_t> &regs);
    static constexpr int FAN_NUM = 4;
private slots:
    void onOpenCoverClicked();
    void onCloseCoverClicked();
    void onPressSampleClicked();
    void onReleaseSampleClicked();
    void onChurnCWClicked();
    void onChurnCCWClicked();
    void onChurnStopClicked();
    // void onTempControlClicked();


    void onMoveLeftClicked();
    void onMoveRightClicked();
    void onMoveForwardClicked();
    void onMoveBackwardClicked();
    void onMoveUpClicked();
    void onMoveDownClicked();

    void onXReturnZeroClicked();
    void onYReturnZeroClicked();
    void onZReturnZeroClicked();

    void onXRunToPosClicked();
    void onYRunToPosClicked();
    void onZRunToPosClicked();


private:
    void initSampleChipWidget();


    ArrowButton *btnMoveXForward;
    ArrowButton *btnMoveXBackward;
    ArrowButton *btnMoveYForward;
    ArrowButton *btnMoveYBackward;
    ArrowButton *btnMoveLenUp;
    ArrowButton *btnMoveLenDown;

    QPushButton *btnXGoZero;
    QPushButton *btnYGoZero;
    QPushButton *btnZGoZero;
    QPushButton *btnXGoPos;
    QPushButton *btnYGoPos;
    QPushButton *btnZGoPos;

    QSpinBox    *spinXSteps;
    QSpinBox    *spinYSteps;
    QSpinBox    *spinZSteps;

    QSpinBox    *spinXPos;
    QSpinBox    *spinYPos;
    QSpinBox    *spinZPos;

    QPushButton *btnOpenCover;
    QPushButton *btnCloseCover;
    QPushButton *btnPressSample;
    QPushButton *btnReleaseSample;

    QPushButton *btnChurnRunCW;
    QPushButton *btnChurnRunCCW;
    QPushButton *btnChurnStop;

    // QPushButton *btnTempControl;

    QLabel *lblCoverStatus;
    QLabel *lblPressStatus;
    QLabel *lblChurnStatus;
    QLabel *lblTempContorlStatus;
    QLabel *lblChipPos;
    QLabel *lblLenPos;
    QLabel *lblMotorXStatus;
    QLabel *lblMotorYStatus;
    QLabel *lblMotorZStatus;
    // ToggleSwitch *chkFans[FAN_NUM];
    // QSpinBox *spinFanSpeed[FAN_NUM];
    // QPushButton *btnFanEnable;

    QSpinBox *spinChurnSpeed;
    // QDoubleSpinBox *spinTargetTemp;



    uint16_t coverStatus;
    uint16_t sealStatus;
    uint16_t churnStatus;
    uint16_t tempCtrlStatus;
    uint16_t motorXStatus;
    uint16_t motorYStatus;
    uint16_t motorZStatus;
    uint16_t motorXLimit;
    uint16_t motorYLimit;
    uint16_t motorZLimit;
    int32_t motorXPos;
    int32_t motorYPos;
    int32_t motorZPos;
};


#endif // SAMPLECHIPWIDGET_H
