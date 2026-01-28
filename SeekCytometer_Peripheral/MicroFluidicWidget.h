#ifndef MICROFLUIDICWIDGET_H
#define MICROFLUIDICWIDGET_H

#include <QDockWidget>
#include <QObject>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

#include "ToggleSwitch.h"
#include "ToggleRunButton.h"

#include <QButtonGroup>

class MicroFluidicWidget : public QDockWidget
{
    Q_OBJECT
public:
    MicroFluidicWidget(const QString &tilte, QWidget *parent = nullptr);

    void updateStatus(const QVector<uint16_t> &regs);

private slots:
    void onBtnPressCtrlEnClicked(bool checked);

private:
    static constexpr int CHANNEL_NUM = 5;

    void initDockWidget();

    ToggleSwitch *btnSoleValve[CHANNEL_NUM];
    ToggleSwitch *togglePressChEn[CHANNEL_NUM];
    QLabel      *lblPress[CHANNEL_NUM];
    QLabel      *lblPressControl[CHANNEL_NUM];
    QLabel      *lblSourePress;
    QSpinBox    *spinTargetPress[CHANNEL_NUM];
    QSpinBox    *spinPropoValveValue[CHANNEL_NUM];
    ToggleRunButton *btnPressCtrlEn;

    bool        m_pressCtrlEnabled;
    uint16_t    m_enabledChannel;
    uint16_t    m_pressTarget[CHANNEL_NUM];
    float       m_inputPress;
    float       m_chPress[CHANNEL_NUM];
    uint16_t    m_propoValveValue[CHANNEL_NUM];
    uint16_t    m_soleVavleStatus[CHANNEL_NUM];
    uint16_t    m_pressCtrlStatus[CHANNEL_NUM];


    void onBtnSoleClicked(int index, bool checked);
    void onBtnPressChEnClicked(int index, bool checked);
    void onSpinTargetPressChanged(int index, int val);
    void onSpinPropoValveValueChanged(int index, int val);



};

#endif // MICROFLUIDICWIDGET_H
