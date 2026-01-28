#include "MicroFluidicWidget.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QVBoxLayout>
#include "ModbusRegistersTable.h"
#include "ModbusMaster.h"

MicroFluidicWidget::MicroFluidicWidget(const QString &tilte, QWidget *parent)
    : QDockWidget{tilte, parent}, m_pressCtrlEnabled(false), m_enabledChannel(0)
{
    initDockWidget();
}

void MicroFluidicWidget::updateStatus(const QVector<uint16_t> &regs)
{
    if (regs.length() < 13) {
        qDebug() << "Wrong MFC status registers number! Need 13, but actually " << regs.length();
    }

    for (int i = 0; i < CHANNEL_NUM; i++) {
        m_pressCtrlStatus[i] = (regs.at(0) >> (i*3)) & 0x0007;
        m_soleVavleStatus[i] = (regs.at(1) >> (i*3)) & 0x0007;
        m_chPress[i] = (float)regs[3+i] / 10.0;
        m_propoValveValue[i] = regs[8+i];

        lblPress[i]->setText(QString::number(m_chPress[i], 'f', 1));
        lblPressControl[i]->setText(QString::number(m_chPress[i], 'f', 1));
        if (m_pressCtrlEnabled) {
            btnSoleValve[i]->setChecked(m_soleVavleStatus[i] == 0x0002);
            spinPropoValveValue[i]->setValue(m_propoValveValue[i]);
        }
    }
    m_inputPress = (float)static_cast<int>(regs[2]) / 10.0;

    lblSourePress->setText(QString::number(m_inputPress, 'f', 1));
}


void MicroFluidicWidget::initDockWidget()
{
    QGridLayout *debugLayout = new QGridLayout();
    QGroupBox *groupBoxDebug = new QGroupBox("Device Debug", this);

    QGridLayout *controlLayout = new QGridLayout();
    QGroupBox *groupBoxControl = new QGroupBox("Press Control", this);

    auto addLabel = [&](QGridLayout* lay, int row, int col, const QString &text){
        QLabel *hdr = new QLabel(text, this);
        hdr->setAlignment(Qt::AlignCenter);
        hdr->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        hdr->setFixedHeight(20); // 你可以调整到合适值，例如 18~24
        lay->addWidget(hdr, row, col);
    };

    addLabel(debugLayout, 0, 0, tr("Channel"));
    addLabel(debugLayout, 0, 1, tr("SoleVavle"));
    addLabel(debugLayout, 0, 2, tr("PropoVavle"));
    addLabel(debugLayout, 0, 3, tr("Press(mbar)"));

    addLabel(controlLayout, 0, 0, tr("Channel"));
    addLabel(controlLayout, 0, 1, tr("Target(mbar)"));
    addLabel(controlLayout, 0, 2, tr("Channel Enable"));
    addLabel(controlLayout, 0, 3, tr("Press(mbar)"));


    for (int i = 0; i < CHANNEL_NUM; i++) {
        btnSoleValve[i] = new ToggleSwitch(this);
        togglePressChEn[i] = new ToggleSwitch(this);


        lblPress[i] = new QLabel(QString("0.0"), this);
        lblPress[i]->setAlignment(Qt::AlignCenter);

        lblPressControl[i] = new QLabel(QString("0.0"), this);
        lblPressControl[i]->setAlignment(Qt::AlignCenter);
        spinTargetPress[i] = new QSpinBox(this);
        spinTargetPress[i]->setRange(0, 1000);
        spinTargetPress[i]->setValue(0);

        spinPropoValveValue[i] = new QSpinBox(this);
        spinPropoValveValue[i]->setRange(0, 65535);
        spinPropoValveValue[i]->setValue(0);

        connect(btnSoleValve[i], &ToggleSwitch::toggled, this, [=](bool checked){
            // bool checked = btnSoleValve[i]->isChecked();
            onBtnSoleClicked(i, checked);
        });
        connect(togglePressChEn[i], &ToggleSwitch::toggled, this, [=](bool checked){
            // bool checked = togglePressChEn[i]->isChecked();
            onBtnPressChEnClicked(i, checked);
        });

        connect(spinPropoValveValue[i], &QSpinBox::editingFinished, this, [=](){
            int value = spinPropoValveValue[i]->value();
            onSpinPropoValveValueChanged(i, value);
        });

        connect(spinTargetPress[i], &QSpinBox::editingFinished, this, [=](){
            int value = spinTargetPress[i]->value();
            onSpinTargetPressChanged(i, value);
        });

        // debugLayout->addWidget(new QLabel(QString("Ch-%1").arg(i+1), this), i+1, 0);
        addLabel(debugLayout, i+1, 0, QString("Ch-%1").arg(i+1));
        debugLayout->addWidget(btnSoleValve[i], i+1, 1);
        debugLayout->addWidget(spinPropoValveValue[i], i+1, 2);
        debugLayout->addWidget(lblPress[i], i+1, 3);

        // controlLayout->addWidget(new QLabel(QString("Ch-%1").arg(i+1), this), i+1, 0);
        addLabel(controlLayout, i+1, 0, QString("Ch-%1").arg(i+1));
        controlLayout->addWidget(spinTargetPress[i], i+1, 1);
        controlLayout->addWidget(togglePressChEn[i], i+1, 2);
        controlLayout->addWidget(lblPressControl[i], i+1, 3);
    }
    btnPressCtrlEn = new ToggleRunButton(this);
    controlLayout->addWidget(btnPressCtrlEn, 6, 1, 2, 2);
    connect(btnPressCtrlEn, &ToggleRunButton::toggled, this, &MicroFluidicWidget::onBtnPressCtrlEnClicked);

    lblSourePress = new QLabel(QString("Source Press: 1000 mbar"), this);
    groupBoxDebug->setLayout(debugLayout);
    groupBoxControl->setLayout(controlLayout);

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(mainWidget);
    layout->addWidget(lblSourePress, 1);
    layout->addWidget(groupBoxDebug, 4);
    layout->addWidget(groupBoxControl, 4);

    mainWidget->setLayout(layout);
    setWidget(mainWidget);
}

void MicroFluidicWidget::onBtnSoleClicked(int index, bool checked)
{
    QVector<uint16_t> soleCmd(2, 0);
    if (checked) {
        soleCmd[0] = SOL_CTRL_OPEN;
    } else {
        soleCmd[0] = SOL_CTRL_CLOSE;
    }
    soleCmd[1] = index;
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MFC_SOL_CMD, soleCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, MFC_CW_SOLE_BIT);
}

void MicroFluidicWidget::onBtnPressChEnClicked(int index, bool checked)
{
    uint16_t ch = (uint16_t)0x0001 << index;
    if (m_pressCtrlEnabled) {
        if (checked) {
            m_enabledChannel |= ch;
            QVector<uint16_t> pressCmd = {MFC_PRESS_CTRL_START, ch};
            ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MFC_PRESS_CMD, pressCmd);
            ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_PRESS_SET_CH1+index, m_pressTarget[index]);
            ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, 0x0000);
            ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, MFC_CW_PRESS_BIT);
        } else {
            m_enabledChannel &= ~ch;
            QVector<uint16_t> pressCmd = {MFC_PRESS_CTRL_STOP, ch};
            ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MFC_PRESS_CMD, pressCmd);
            ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, 0x0000);
            ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, MFC_CW_PRESS_BIT);
        }
    }
}

void MicroFluidicWidget::onSpinTargetPressChanged(int index, int val)
{
    m_pressTarget[index] = val;
    uint16_t ch = (uint16_t)0x0001 << index;
    QVector<uint16_t> pressCmd = {MFC_PRESS_CTRL_SET_TARGET, ch};
    if (m_pressCtrlEnabled) {
        ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MFC_PRESS_CMD, pressCmd);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_PRESS_SET_CH1+index, m_pressTarget[index]);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, 0x0000);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, MFC_CW_PRESS_BIT);
    }
}

void MicroFluidicWidget::onSpinPropoValveValueChanged(int index, int val)
{
    QVector<uint16_t> propoCmd(3, 0);
    if (val > 0) {
        propoCmd[0] = PROPO_CTRL_OPEN;
    } else {
        propoCmd[0] = PROPO_CTRL_CLOSE;
    }
    propoCmd[1] = index;
    propoCmd[2] = val;
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MFC_PROPO_CMD, propoCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, MFC_CW_PROPO_BIT);
}

void MicroFluidicWidget::onBtnPressCtrlEnClicked(bool checked)
{
    if (checked) {
        QVector<uint16_t> pressCmd(7, 0);
        m_pressCtrlEnabled = true;
        pressCmd[0] = MFC_PRESS_CTRL_START;
        for (int i = 0; i < CHANNEL_NUM; i++) {
            if (togglePressChEn[i]->isChecked()) {
                m_enabledChannel |= ((uint16_t)0x01 << i);
            } else {
                m_enabledChannel &= (~((uint16_t)0x01 << i));
            }

            m_pressTarget[i] = spinTargetPress[i]->value();
            pressCmd[i+2] = (m_pressTarget[i]);
        }

        pressCmd[1] = m_enabledChannel;
        ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MFC_PRESS_CMD, pressCmd);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, 0x0000);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, MFC_CW_PRESS_BIT);
    } else {
        QVector<uint16_t> pressCmd(2, 0);
        m_pressCtrlEnabled = false;
        m_enabledChannel = 0;
        pressCmd[0] = MFC_PRESS_CTRL_STOP;
        pressCmd[1] = 0x001F;

        ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MFC_PRESS_CMD, pressCmd);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, 0x0000);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, MFC_CW_PRESS_BIT);

        for (int i = 0; i < CHANNEL_NUM; i++) {
            togglePressChEn[i]->setChecked(false);
        }
    }
}
