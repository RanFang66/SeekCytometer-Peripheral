#include "MFCParasSetWidget.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QMessageBox>
#include "ModbusMaster.h"
#include "ModbusRegistersTable.h"
#include "database/DatabaseManager.h"

MFCParasSetWidget::MFCParasSetWidget(const QString &tilte, QWidget *parent)
    : QDockWidget{tilte, parent}
{
    initDockWidget();

    connect(btnSetPara, &QPushButton::clicked, this, &MFCParasSetWidget::onBtnSetParasClicked);
    connect(btnUpdatePara, &QPushButton::clicked, this, &MFCParasSetWidget::onBtnUpdateParasClicked);
    connect(btnDbLoad, &QPushButton::clicked, this, &MFCParasSetWidget::onBtnDbLoadClicked);
    connect(btnDbSave, &QPushButton::clicked, this, &MFCParasSetWidget::onBtnDbSaveClicked);
    connect(btnDbDelete, &QPushButton::clicked, this, &MFCParasSetWidget::onBtnDbDeleteClicked);
    connect(btnDbOverwrite, &QPushButton::clicked, this, &MFCParasSetWidget::onBtnDbOverwriteClicked);
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

// ---- Database management slots ----

void MFCParasSetWidget::onBtnDbLoadClicked()
{
    QString name = cmbDbConfigs->currentText();
    if (name.isEmpty())
        return;

    HydraulicControlParams params = m_dao.queryByName(name);
    if (params.id < 0) {
        QMessageBox::warning(this, tr("Load Failed"), tr("Parameter \"%1\" not found in database.").arg(name));
        return;
    }

    // Update display labels with loaded values
    auto setChLabels = [&](int ch, float kp, float ki, int ff) {
        lblKpVal[ch]->setText(QString::number(kp, 'f', 2));
        lblKiVal[ch]->setText(QString::number(ki, 'f', 2));
        lblFeedForward[ch]->setText(QString::number(ff));
    };
    setChLabels(0, params.ch1Kp, params.ch1Ki, params.ch1Feedforward);
    setChLabels(1, params.ch2Kp, params.ch2Ki, params.ch2Feedforward);
    setChLabels(2, params.ch3Kp, params.ch3Ki, params.ch3Feedforward);
    setChLabels(3, params.ch4Kp, params.ch4Ki, params.ch4Feedforward);
    setChLabels(4, params.ch5Kp, params.ch5Ki, params.ch5Feedforward);

    // Write all channel parameters to device
    applyToDevice(params);
}

void MFCParasSetWidget::onBtnDbSaveClicked()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Save Parameters"),
                                         tr("Enter parameter name:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    name = name.trimmed();

    // Check if name already exists
    HydraulicControlParams existing = m_dao.queryByName(name);
    if (existing.id >= 0) {
        QMessageBox::warning(this, tr("Save Failed"),
                             tr("Parameter name \"%1\" already exists. Use \"Overwrite\" to update.").arg(name));
        return;
    }

    HydraulicControlParams params = collectFromLabels();
    params.name = name;

    if (m_dao.insert(params)) {
        refreshDbComboBox();
        cmbDbConfigs->setCurrentText(name);
    } else {
        QMessageBox::warning(this, tr("Save Failed"), tr("Failed to save parameters to database."));
    }
}

void MFCParasSetWidget::onBtnDbDeleteClicked()
{
    QString name = cmbDbConfigs->currentText();
    if (name.isEmpty())
        return;

    int ret = QMessageBox::question(this, tr("Delete Parameters"),
                                    tr("Delete parameter \"%1\"?").arg(name),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    HydraulicControlParams params = m_dao.queryByName(name);
    if (params.id >= 0 && m_dao.remove(params.id)) {
        refreshDbComboBox();
    } else {
        QMessageBox::warning(this, tr("Delete Failed"), tr("Failed to delete parameters."));
    }
}

void MFCParasSetWidget::onBtnDbOverwriteClicked()
{
    QString name = cmbDbConfigs->currentText();
    if (name.isEmpty())
        return;

    HydraulicControlParams existing = m_dao.queryByName(name);
    if (existing.id < 0) {
        QMessageBox::warning(this, tr("Overwrite Failed"),
                             tr("Parameter \"%1\" not found in database.").arg(name));
        return;
    }

    int ret = QMessageBox::question(this, tr("Overwrite Parameters"),
                                    tr("Overwrite parameter \"%1\" with current values?").arg(name),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    HydraulicControlParams params = collectFromLabels();
    params.id = existing.id;
    params.name = name;

    if (!m_dao.update(params)) {
        QMessageBox::warning(this, tr("Overwrite Failed"), tr("Failed to update parameters in database."));
    }
}

// ---- Helper methods ----

void MFCParasSetWidget::refreshDbComboBox()
{
    cmbDbConfigs->clear();
    if (DatabaseManager::instance().isConnected()) {
        cmbDbConfigs->addItems(m_dao.queryAllNames());
    }
}

HydraulicControlParams MFCParasSetWidget::collectFromLabels()
{
    HydraulicControlParams p;
    auto readCh = [&](int ch, float &kp, float &ki, int &ff) {
        kp = lblKpVal[ch]->text().toFloat();
        ki = lblKiVal[ch]->text().toFloat();
        ff = lblFeedForward[ch]->text().toInt();
    };
    readCh(0, p.ch1Kp, p.ch1Ki, p.ch1Feedforward);
    readCh(1, p.ch2Kp, p.ch2Ki, p.ch2Feedforward);
    readCh(2, p.ch3Kp, p.ch3Ki, p.ch3Feedforward);
    readCh(3, p.ch4Kp, p.ch4Ki, p.ch4Feedforward);
    readCh(4, p.ch5Kp, p.ch5Ki, p.ch5Feedforward);
    return p;
}

void MFCParasSetWidget::applyToDevice(const HydraulicControlParams &params)
{
    // Write per-channel Kp/Ki/FeedForward to device via Modbus
    float kps[CHANNEL_NUM]  = {params.ch1Kp, params.ch2Kp, params.ch3Kp, params.ch4Kp, params.ch5Kp};
    float kis[CHANNEL_NUM]  = {params.ch1Ki, params.ch2Ki, params.ch3Ki, params.ch4Ki, params.ch5Ki};
    int   ffs[CHANNEL_NUM]  = {params.ch1Feedforward, params.ch2Feedforward, params.ch3Feedforward,
                               params.ch4Feedforward, params.ch5Feedforward};

    for (int i = 0; i < CHANNEL_NUM; i++) {
        uint16_t kpx100 = (uint16_t)(kps[i] * 100.0f);
        uint16_t kix100 = (uint16_t)(kis[i] * 100.0f);
        uint16_t feed   = (uint16_t)ffs[i];

        QVector<uint16_t> paraVals = {kpx100, kix100, feed};
        uint16_t chBit = (uint16_t)(0x01 << i);
        QVector<uint16_t> cmd = {MFC_PRESS_CTRL_SET_PI, chBit};

        ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MFC_KP_SET, paraVals);
        ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MFC_PRESS_CMD, cmd);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, 0x0000);
        ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MFC_CW, MFC_CW_PRESS_BIT);
    }
}

// ---- UI initialization ----

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
        hdr->setFixedHeight(20);
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

    // ---- Database parameter management group ----
    cmbDbConfigs = new QComboBox(this);
    cmbDbConfigs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    btnDbLoad = new QPushButton(tr("Load"), this);
    btnDbSave = new QPushButton(tr("Save As"), this);
    btnDbDelete = new QPushButton(tr("Delete"), this);
    btnDbOverwrite = new QPushButton(tr("Overwrite"), this);

    QHBoxLayout *dbBtnLayout = new QHBoxLayout;
    dbBtnLayout->addWidget(btnDbLoad);
    dbBtnLayout->addWidget(btnDbSave);
    dbBtnLayout->addWidget(btnDbOverwrite);
    dbBtnLayout->addWidget(btnDbDelete);

    QVBoxLayout *dbLayout = new QVBoxLayout;
    dbLayout->addWidget(cmbDbConfigs);
    dbLayout->addLayout(dbBtnLayout);

    QGroupBox *grpDb = new QGroupBox(tr("Parameter Management"), this);
    grpDb->setLayout(dbLayout);

    // ---- Assemble all groups ----
    QGroupBox *grpSet = new QGroupBox("Set Parameter", this);
    QGroupBox *grpDisp = new QGroupBox("Realtime Parameters", this);
    QVBoxLayout *layout = new QVBoxLayout;
    grpSet->setLayout(setLayout);
    grpDisp->setLayout(paraLayout);
    layout->addWidget(grpDb);
    layout->addWidget(grpSet);
    layout->addWidget(grpDisp);
    QWidget *mainWidget = new QWidget(this);
    mainWidget->setLayout(layout);
    setWidget(mainWidget);

    // Load saved configs from database
    refreshDbComboBox();
}
