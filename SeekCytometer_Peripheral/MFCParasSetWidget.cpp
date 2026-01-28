#include "MFCParasSetWidget.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include "ModbusMaster.h"
#include "ModbusRegistersTable.h"

MFCParasSetWidget::MFCParasSetWidget(const QString &tilte, QWidget *parent)
    : QDockWidget{tilte, parent}
{
    initDockWidget();

    connect(btnSetPara, &QPushButton::clicked, this, &MFCParasSetWidget::onBtnSetParasClicked);
    connect(btnUpdatePara, &QPushButton::clicked, this, &MFCParasSetWidget::onBtnUpdateParasClicked);
}

void MFCParasSetWidget::onBtnSetParasClicked()
{
    uint16_t ch = 0;
    for (int i = 0; i < CHANNEL_NUM; i++) {
        if (chkCh[i]->isChecked()) {
            ch |= (0x01 << i);
        }
    }
    uint16_t kpx100 = (uint16_t)(spinKp->value() * 100.0);
    uint16_t kix100 = (uint16_t)(spinKi->value() * 100.0);
    uint16_t feed = spinFeed->value();

    QVector<uint16_t> paras = {kpx100, kix100, feed};
    QVector<uint16_t> cmd = {MFC_PRESS_CTRL_SET_PI, ch};
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MFC_KP_SET, paras);
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MFC_PRESS_CMD, cmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, MFC_CW_PRESS_BIT);
}

void MFCParasSetWidget::onBtnUpdateParasClicked()
{
    QVector<uint16_t> paras;
    paras = ModbusMaster::instance().blockingReadHoldingRegisters(SLAVE_ADDR, MFC_KP_CH1, 15);

    if (paras.size() != 15) {
        qDebug() << "Update MFC control paras failed!";
        return;
    }
    for (int i = 0; i < CHANNEL_NUM; i++) {
        lblKpVal[i]->setText(QString::number((float)paras[i * 3] / 100.0, 'f', 2));
        lblKiVal[i]->setText(QString::number((float)paras[i * 3+1] / 100.0, 'f', 2));
        lblFeedForward[i]->setText(QString::number(paras[i * 3 + 2]));
    }
}

void MFCParasSetWidget::initDockWidget()
{
    QHBoxLayout *chkLayout = new QHBoxLayout;
    QGridLayout *paraLayout = new QGridLayout;
    QVBoxLayout *setLayout = new QVBoxLayout;

    btnSetPara = new QPushButton("Set Paras", this);
    btnUpdatePara = new QPushButton("Update Para", this);
    spinKi = new QDoubleSpinBox(this);
    spinKp = new QDoubleSpinBox(this);
    spinFeed = new QSpinBox(this);

    spinKp->setRange(0, 100.0);
    spinKi->setRange(0, 100.0);
    spinFeed->setRange(0, 40000);

    spinKp->setSingleStep(0.1);
    spinKi->setSingleStep(0.1);
    spinFeed->setSingleStep(100);
    spinKp->setValue(1.0);
    spinKi->setValue(1.0);
    spinFeed->setValue(13000);

    auto addSpin = [&](QVBoxLayout *lay, const QString &prompt, QAbstractSpinBox *spinbox) {
        QLabel *lblPrompt = new QLabel(prompt, this);
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(lblPrompt, 0, Qt::AlignCenter);
        layout->addWidget(spinbox, 0, Qt::AlignCenter);
        lay->addLayout(layout);
    };

    auto addLabel = [&](QGridLayout* lay, int row, int col, const QString &text){
        QLabel *hdr = new QLabel(text, this);
        hdr->setAlignment(Qt::AlignCenter);
        hdr->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        hdr->setFixedHeight(20); // 你可以调整到合适值，例如 18~24
        lay->addWidget(hdr, row, col);
    };

    addLabel(paraLayout, 0, 0, QString("Channel"));
    addLabel(paraLayout, 0, 1, QString("Kp"));
    addLabel(paraLayout, 0, 2, QString("Ki"));
    addLabel(paraLayout, 0, 3, QString("FeedForward"));
    for (int ch = 0; ch < CHANNEL_NUM; ch++) {
        chkCh[ch] = new QCheckBox(QString("CH-%1").arg(ch+1), this);
        chkLayout->addWidget(chkCh[ch]);

        lblKpVal[ch] = new QLabel("0.00", this);
        lblKiVal[ch] = new QLabel("0.00", this);
        lblFeedForward[ch] = new QLabel("0.00", this);
        paraLayout->addWidget(new QLabel(QString("CH-%1").arg(ch+1), this), ch+1, 0);
        paraLayout->addWidget(lblKpVal[ch], ch+1, 1);
        paraLayout->addWidget(lblKiVal[ch], ch+1, 2);
        paraLayout->addWidget(lblFeedForward[ch], ch+1, 3);
    }

    setLayout->addLayout(chkLayout);
    addSpin(setLayout, QString("Kp:"), spinKp);
    addSpin(setLayout, QString("Ki:"), spinKi);
    addSpin(setLayout, QString("FeedForward:"), spinFeed);
    setLayout->addWidget(btnSetPara);

    paraLayout->addWidget(btnUpdatePara, 6, 1, 1, 2, Qt::AlignCenter);

    QGroupBox *grpSet = new QGroupBox("Set Parameter", this);
    QGroupBox *grpDisp = new QGroupBox("Realtime Parameters", this);
    QVBoxLayout *layout = new QVBoxLayout;
    grpSet->setLayout(setLayout);
    grpDisp->setLayout(paraLayout);
    layout->addWidget(grpSet);
    layout->addWidget(grpDisp);
    QWidget *mainWidget = new QWidget(this);
    mainWidget->setLayout(layout);
    setWidget(mainWidget);
}
