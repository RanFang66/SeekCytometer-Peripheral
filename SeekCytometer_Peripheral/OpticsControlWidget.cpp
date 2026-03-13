#include "OpticsControlWidget.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QMessageBox>
#include "ModbusRegistersTable.h"
#include "ModbusMaster.h"
#include "database/DatabaseManager.h"

#define MAX_LASER_INTENSITY 3105
#define MAX_LED_INTENSITY   100


OpticsControlWidget::OpticsControlWidget(const QString &tilte, QWidget *parent)
    : QDockWidget{tilte, parent}
{
    initDockWidget();

    connect(btnLaser_1, &ToggleSwitch::toggled, this, &OpticsControlWidget::onLaser1Toggled);
    connect(btnLaser_2, &ToggleSwitch::toggled, this, &OpticsControlWidget::onLaser2Toggled);
    connect(btnLed, &ToggleSwitch::toggled, this, &OpticsControlWidget::onLedToggled);
    connect(spinLaserIntensity_1, &QSpinBox::valueChanged, this, &OpticsControlWidget::onLaser1IntensityChanged);
    connect(spinLaserIntensity_2, &QSpinBox::valueChanged, this, &OpticsControlWidget::onLaser2IntensityChanged);
    connect(spinLedIntensity, &QSpinBox::valueChanged, this, &OpticsControlWidget::onLedIntensityChanged);

    connect(btnDbLoad, &QPushButton::clicked, this, &OpticsControlWidget::onBtnDbLoadClicked);
    connect(btnDbSave, &QPushButton::clicked, this, &OpticsControlWidget::onBtnDbSaveClicked);
    connect(btnDbDelete, &QPushButton::clicked, this, &OpticsControlWidget::onBtnDbDeleteClicked);
    connect(btnDbOverwrite, &QPushButton::clicked, this, &OpticsControlWidget::onBtnDbOverwriteClicked);
}

void OpticsControlWidget::updateStatus(const QVector<uint16_t> &regs)
{
    if (regs.length() < 6) {
         qDebug() << "Wrong status registers number! Need 6, but actually " << regs.length();
        return;
    }
    statusLaser1 = regs[0];
    intensityLaser1 = regs[1];
    statusLaser2 = regs[2];
    intensityLaser2 = regs[3];
    statusLed = regs[4];
    intensityLed = regs[5];

    if (statusLaser1 == 0) {
        statusLightLaser1->setState(StatusLight::State::IDLE);
    } else {
        statusLightLaser1->setState(StatusLight::State::RUNNING);
    }

    if (statusLaser2 == 0) {
        statusLightLaser2->setState(StatusLight::State::IDLE);
    } else {
        statusLightLaser2->setState(StatusLight::State::RUNNING);
    }

    if (statusLed == 0) {
        statusLightLed->setState(StatusLight::State::IDLE);
    } else {
        statusLightLed->setState(StatusLight::State::RUNNING);
    }
}

void OpticsControlWidget::onLaser1Toggled(bool checked)
{
    uint16_t intensity = spinLaserIntensity_1->value() * MAX_LASER_INTENSITY / 100;

    QVector<uint16_t> laserCmd(2, 0);

    if (checked) {
        laserCmd[0] = (LASER_1_ID << 8) | LASER_CMD_SWITCH_ON;
    } else {
        laserCmd[0] = (LASER_1_ID << 8) | LASER_CMD_SWITCH_OFF;
    }

    laserCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LASER_CMD, laserCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LASER_BIT);
}

void OpticsControlWidget::onLaser2Toggled(bool checked)
{
    uint16_t intensity = spinLaserIntensity_2->value() * MAX_LASER_INTENSITY / 100;

    QVector<uint16_t> laserCmd(2, 0);

    if (checked) {
        laserCmd[0] = (LASER_2_ID << 8) | LASER_CMD_SWITCH_ON;
    } else {
        laserCmd[0] = (LASER_2_ID << 8) | LASER_CMD_SWITCH_OFF;
    }

    laserCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LASER_CMD, laserCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LASER_BIT);
}

void OpticsControlWidget::onLedToggled(bool checked)
{
    uint16_t intensity = spinLedIntensity->value() * MAX_LED_INTENSITY / 100;

    QVector<uint16_t> ledCmd(2, 0);

    if (checked) {
        ledCmd[0] = LED_CMD_SWITCH_ON;
    } else {
        ledCmd[0] = LED_CMD_SWITCH_OFF;
    }

    ledCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LED_CMD, ledCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LED_BIT);
}

void OpticsControlWidget::onLaser1IntensityChanged()
{
    uint16_t intensity = spinLaserIntensity_1->value() * MAX_LASER_INTENSITY / 100;

    QVector<uint16_t> laserCmd(2, 0);
    laserCmd[0] = (LASER_1_ID << 8) | LASER_CMD_SET_INTENSITY;
    laserCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LASER_CMD, laserCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LASER_BIT);
}

void OpticsControlWidget::onLaser2IntensityChanged()
{
    uint16_t intensity = spinLaserIntensity_2->value() * MAX_LASER_INTENSITY / 100;

    QVector<uint16_t> laserCmd(2, 0);
    laserCmd[0] = (LASER_2_ID << 8) | LASER_CMD_SET_INTENSITY;
    laserCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LASER_CMD, laserCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LASER_BIT);
}

void OpticsControlWidget::onLedIntensityChanged()
{
    uint16_t intensity = spinLedIntensity->value() * MAX_LED_INTENSITY / 100;

    QVector<uint16_t> ledCmd(2, 0);
    ledCmd[0] = LED_CMD_SET_INTENSITY;
    ledCmd[1] = intensity;

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_LED_CMD, ledCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_LED_BIT);
}

// ---- DB management slots ----

void OpticsControlWidget::onBtnDbLoadClicked()
{
    QString name = cmbDbConfigs->currentText();
    if (name.isEmpty())
        return;

    LaserConfig cfg = m_dao.queryByName(name);
    if (cfg.id < 0) {
        QMessageBox::warning(this, tr("Load Failed"),
                             tr("Laser config \"%1\" not found in database.").arg(name));
        return;
    }

    applyLaserConfig(cfg);
}

void OpticsControlWidget::onBtnDbSaveClicked()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Save Laser Config"),
                                         tr("Enter config name:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    name = name.trimmed();

    LaserConfig existing = m_dao.queryByName(name);
    if (existing.id >= 0) {
        QMessageBox::warning(this, tr("Save Failed"),
                             tr("Name \"%1\" already exists. Use \"Overwrite\" to update.").arg(name));
        return;
    }

    LaserConfig cfg;
    cfg.name = name;
    cfg.laser638nmEnable = btnLaser_1->isChecked();
    cfg.laser448nmEnable = btnLaser_2->isChecked();
    cfg.whiteLedEnable = btnLed->isChecked();
    cfg.laser638nmPower = spinLaserIntensity_1->value();
    cfg.laser448nmPower = spinLaserIntensity_2->value();
    cfg.whiteLedPower = spinLedIntensity->value();

    if (m_dao.insert(cfg)) {
        refreshDbComboBox();
        cmbDbConfigs->setCurrentText(name);
    } else {
        QMessageBox::warning(this, tr("Save Failed"), tr("Failed to save laser config to database."));
    }
}

void OpticsControlWidget::onBtnDbDeleteClicked()
{
    QString name = cmbDbConfigs->currentText();
    if (name.isEmpty())
        return;

    int ret = QMessageBox::question(this, tr("Delete Laser Config"),
                                    tr("Delete config \"%1\"?").arg(name),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    LaserConfig cfg = m_dao.queryByName(name);
    if (cfg.id >= 0 && m_dao.remove(cfg.id)) {
        refreshDbComboBox();
    } else {
        QMessageBox::warning(this, tr("Delete Failed"), tr("Failed to delete laser config."));
    }
}

void OpticsControlWidget::onBtnDbOverwriteClicked()
{
    QString name = cmbDbConfigs->currentText();
    if (name.isEmpty())
        return;

    LaserConfig existing = m_dao.queryByName(name);
    if (existing.id < 0) {
        QMessageBox::warning(this, tr("Overwrite Failed"),
                             tr("Config \"%1\" not found in database.").arg(name));
        return;
    }

    int ret = QMessageBox::question(this, tr("Overwrite Laser Config"),
                                    tr("Overwrite \"%1\" with current settings?").arg(name),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    existing.laser638nmEnable = btnLaser_1->isChecked();
    existing.laser448nmEnable = btnLaser_2->isChecked();
    existing.whiteLedEnable = btnLed->isChecked();
    existing.laser638nmPower = spinLaserIntensity_1->value();
    existing.laser448nmPower = spinLaserIntensity_2->value();
    existing.whiteLedPower = spinLedIntensity->value();

    if (!m_dao.update(existing)) {
        QMessageBox::warning(this, tr("Overwrite Failed"), tr("Failed to update laser config in database."));
    }
}

// ---- Helper methods ----

void OpticsControlWidget::refreshDbComboBox()
{
    cmbDbConfigs->clear();
    if (DatabaseManager::instance().isConnected()) {
        cmbDbConfigs->addItems(m_dao.queryAllNames());
    }
}

void OpticsControlWidget::applyLaserConfig(const LaserConfig &cfg)
{
    // Block signals to avoid triggering Modbus commands during batch update
    btnLaser_1->blockSignals(true);
    btnLaser_2->blockSignals(true);
    btnLed->blockSignals(true);
    spinLaserIntensity_1->blockSignals(true);
    spinLaserIntensity_2->blockSignals(true);
    spinLedIntensity->blockSignals(true);

    // Set UI values
    spinLaserIntensity_1->setValue(cfg.laser638nmPower);
    spinLaserIntensity_2->setValue(cfg.laser448nmPower);
    spinLedIntensity->setValue(cfg.whiteLedPower);
    btnLaser_1->setChecked(cfg.laser638nmEnable);
    btnLaser_2->setChecked(cfg.laser448nmEnable);
    btnLed->setChecked(cfg.whiteLedEnable);

    // Restore signals
    btnLaser_1->blockSignals(false);
    btnLaser_2->blockSignals(false);
    btnLed->blockSignals(false);
    spinLaserIntensity_1->blockSignals(false);
    spinLaserIntensity_2->blockSignals(false);
    spinLedIntensity->blockSignals(false);

    // Send commands to device
    onLaser1Toggled(cfg.laser638nmEnable);
    onLaser2Toggled(cfg.laser448nmEnable);
    onLedToggled(cfg.whiteLedEnable);
}

// ---- UI initialization ----

void OpticsControlWidget::initDockWidget()
{
    btnLaser_1 = new ToggleSwitch(this);
    btnLaser_2 = new ToggleSwitch(this);
    btnLed = new ToggleSwitch(this);

    spinLaserIntensity_1 = new QSpinBox(this);
    spinLaserIntensity_2 = new QSpinBox(this);
    spinLedIntensity = new QSpinBox(this);

    statusLightLaser1 = new StatusLight(this);
    statusLightLaser2 = new StatusLight(this);
    statusLightLed = new StatusLight(this);


    QGridLayout *laserLayout = new QGridLayout();

    auto addLabel = [&](QGridLayout* lay, int row, int col, const QString &text){
        QLabel *hdr = new QLabel(text, this);
        hdr->setAlignment(Qt::AlignCenter);
        hdr->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        hdr->setFixedHeight(20);
        lay->addWidget(hdr, row, col);
    };


    addLabel(laserLayout, 0, 0, tr("Device"));
    addLabel(laserLayout, 0, 1, tr("Switch On/Off"));
    addLabel(laserLayout, 0, 2, tr("Intensity Set"));
    addLabel(laserLayout, 0, 3, tr("Status"));


    addLabel(laserLayout, 1, 0, tr("Laser-638nm"));
    laserLayout->addWidget(btnLaser_1, 1, 1);
    laserLayout->addWidget(spinLaserIntensity_1, 1, 2);
    laserLayout->addWidget(statusLightLaser1, 1, 3);


    addLabel(laserLayout, 2, 0, tr("Laser-448nm"));
    laserLayout->addWidget(btnLaser_2, 2, 1);
    laserLayout->addWidget(spinLaserIntensity_2, 2, 2);
    laserLayout->addWidget(statusLightLaser2, 2, 3);

    addLabel(laserLayout, 3, 0, tr("LED"));
    laserLayout->addWidget(btnLed, 3, 1);
    laserLayout->addWidget(spinLedIntensity, 3, 2);
    laserLayout->addWidget(statusLightLed, 3, 3);

    // ---- DB parameter management group ----
    cmbDbConfigs = new QComboBox(this);
    cmbDbConfigs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnDbLoad = new QPushButton(tr("Load"), this);
    btnDbSave = new QPushButton(tr("Save As"), this);
    btnDbOverwrite = new QPushButton(tr("Overwrite"), this);
    btnDbDelete = new QPushButton(tr("Delete"), this);

    QHBoxLayout *dbBtnLayout = new QHBoxLayout;
    dbBtnLayout->addWidget(btnDbLoad);
    dbBtnLayout->addWidget(btnDbSave);
    dbBtnLayout->addWidget(btnDbOverwrite);
    dbBtnLayout->addWidget(btnDbDelete);

    QVBoxLayout *dbLayout = new QVBoxLayout;
    dbLayout->addWidget(cmbDbConfigs);
    dbLayout->addLayout(dbBtnLayout);

    QGroupBox *grpDb = new QGroupBox(tr("Laser Config Management"), this);
    grpDb->setLayout(dbLayout);

    // ---- Assemble ----
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->addLayout(laserLayout);
    mainLayout->addWidget(grpDb);

    mainWidget->setLayout(mainLayout);
    setWidget(mainWidget);

    // Load saved configs from database
    refreshDbComboBox();
}
