#ifndef MFCPARASSETWIDGET_H
#define MFCPARASSETWIDGET_H

#include <QDockWidget>
#include <QObject>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>

class MFCParasSetWidget : public QDockWidget
{
    Q_OBJECT
public:
    MFCParasSetWidget(const QString &tilte, QWidget *parent = nullptr);

    static constexpr int CHANNEL_NUM = 5;

private slots:
    void onBtnSetParasClicked();
    void onBtnUpdateParasClicked();

private:
    void initDockWidget();

    QDoubleSpinBox  *spinKp;
    QDoubleSpinBox  *spinKi;
    QSpinBox        *spinFeed;
    QCheckBox       *chkCh[CHANNEL_NUM];
    QPushButton     *btnSetPara;
    QPushButton     *btnUpdatePara;
    QLabel          *lblKpVal[CHANNEL_NUM];
    QLabel          *lblKiVal[CHANNEL_NUM];
    QLabel          *lblFeedForward[CHANNEL_NUM];
};

#endif // MFCPARASSETWIDGET_H
