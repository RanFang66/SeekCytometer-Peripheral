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
#include <QComboBox>
#include "ToggleSwitch.h"

#include "database/ChipPositionDao.h"
#include "database/LensPositionDao.h"

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

    // Chip position DB slots
    void onBtnChipDbLoadClicked();
    void onBtnChipDbSaveClicked();
    void onBtnChipDbDeleteClicked();
    void onBtnChipDbOverwriteClicked();

    // Lens position DB slots
    void onBtnLensDbLoadClicked();
    void onBtnLensDbSaveClicked();
    void onBtnLensDbDeleteClicked();
    void onBtnLensDbOverwriteClicked();

private:
    void initSampleChipWidget();
    void refreshChipDbComboBox();
    void refreshLensDbComboBox();

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

    QLabel *lblCoverStatus;
    QLabel *lblPressStatus;
    QLabel *lblChurnStatus;
    QLabel *lblTempContorlStatus;
    QLabel *lblChipPos;
    QLabel *lblLenPos;
    QLabel *lblMotorXStatus;
    QLabel *lblMotorYStatus;
    QLabel *lblMotorZStatus;

    QSpinBox *spinChurnSpeed;

    // Chip position DB management
    QComboBox   *cmbChipDbConfigs;
    QPushButton *btnChipDbLoad;
    QPushButton *btnChipDbSave;
    QPushButton *btnChipDbDelete;
    QPushButton *btnChipDbOverwrite;
    ChipPositionDao m_chipDao;

    // Lens position DB management
    QComboBox   *cmbLensDbConfigs;
    QPushButton *btnLensDbLoad;
    QPushButton *btnLensDbSave;
    QPushButton *btnLensDbDelete;
    QPushButton *btnLensDbOverwrite;
    LensPositionDao m_lensDao;

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
