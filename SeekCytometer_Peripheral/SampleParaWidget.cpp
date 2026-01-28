#include "SampleParaWidget.h"
#include <QGridLayout>

#include "ModbusRegistersTable.h"
#include "ModbusMaster.h"


SampleParaWidget::SampleParaWidget(const QString &tilte, QWidget *parent)
    : QDockWidget{tilte, parent}
{
    initDockWidget();
}




void SampleParaWidget::initDockWidget()
{
    QGridLayout *layout = new QGridLayout;

    auto addLabel = [&](QGridLayout* lay, int row, int col, const QString &text){
        QLabel *hdr = new QLabel(text, this);
        hdr->setAlignment(Qt::AlignCenter);
        hdr->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        hdr->setFixedHeight(20); // 你可以调整到合适值，例如 18~24
        lay->addWidget(hdr, row, col);
    };

    addLabel(layout, 0, 0, tr("Channel"));
    addLabel(layout, 0, 1, tr("Gain"));
    addLabel(layout, 0, 2, tr("Reference"));
    for (int i = 0; i < SAMPLE_CH_NUM; i++) {
        spinGains[i] = new QSpinBox(this);
        spinRefs[i] = new QSpinBox(this);

        spinGains[i]->setRange(0, 11000);
        spinGains[i]->setValue(6000);
        spinRefs[i]->setRange(0, 32768);
        spinRefs[i]->setValue(10000);

        addLabel(layout, i+1, 0, QString("Channel-%1").arg(i+1));
        layout->addWidget(spinGains[i], i+1, 1);
        layout->addWidget(spinRefs[i], i+1, 2);

        connect(spinGains[i], &QSpinBox::valueChanged, this, [=](){
            int val = spinGains[i]->value();
            onGainSetChanged(i, val);
        });

        connect(spinRefs[i], &QSpinBox::valueChanged, this, [=](){
            int val = spinRefs[i]->value();
            onRefSetChanged(i, val);
        });
    }

    QWidget *mainWidget = new QWidget(this);
    mainWidget->setLayout(layout);
    setWidget(mainWidget);
}

void SampleParaWidget::onGainSetChanged(int ch, int val)
{
    QVector<uint16_t> cmd(2, 0);
    cmd[0] = SAMPLE_UPDATE_GAIN;
    cmd[1] = (uint16_t)0x01 << ch;

    uint16_t gainVal = val * 19859 / 10000;
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, AD_SAMPLE_CMD, cmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, AD_SAMPLE_GAIN_SET_1+ch, gainVal);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, AD_COMM_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, AD_COMM_CW, 0x0001);
}

void SampleParaWidget::onRefSetChanged(int ch, int val)
{
    QVector<uint16_t> cmd(2, 0);
    cmd[0] = SAMPLE_UPDATE_REF;
    cmd[1] = (uint16_t)0x01 << ch;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, AD_SAMPLE_CMD, cmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, AD_SAMPLE_REF_SET_1+ch, (uint16_t)val);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, AD_COMM_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, AD_COMM_CW, 0x0001);
}
