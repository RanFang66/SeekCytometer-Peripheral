#ifndef MFCPARASSETWIDGET_H
#define MFCPARASSETWIDGET_H

#include <QDockWidget>
#include <QObject>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>

#include "database/HydraulicControlParamsDao.h"

class MFCParasSetWidget : public QDockWidget
{
    Q_OBJECT
public:
    MFCParasSetWidget(const QString &tilte, QWidget *parent = nullptr);

    static constexpr int CHANNEL_NUM = 5;

private slots:
    void onBtnSetParasClicked();
    void onBtnUpdateParasClicked();

    void onBtnDbLoadClicked();
    void onBtnDbSaveClicked();
    void onBtnDbDeleteClicked();
    void onBtnDbOverwriteClicked();

private:
    void initDockWidget();
    void refreshDbComboBox();
    HydraulicControlParams collectFromLabels();
    void applyToDevice(const HydraulicControlParams &params);

    // Set Parameter group
    QDoubleSpinBox  *spinKp;
    QDoubleSpinBox  *spinKi;
    QSpinBox        *spinFeed;
    QCheckBox       *chkCh[CHANNEL_NUM];
    QPushButton     *btnSetPara;
    QPushButton     *btnUpdatePara;
    QLabel          *lblKpVal[CHANNEL_NUM];
    QLabel          *lblKiVal[CHANNEL_NUM];
    QLabel          *lblFeedForward[CHANNEL_NUM];

    // Database management group
    QComboBox       *cmbDbConfigs;
    QPushButton     *btnDbLoad;
    QPushButton     *btnDbSave;
    QPushButton     *btnDbDelete;
    QPushButton     *btnDbOverwrite;

    HydraulicControlParamsDao m_dao;
};

#endif // MFCPARASSETWIDGET_H
