#include "SampleChipWidget.h"
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSpacerItem>
#include <QInputDialog>
#include <QMessageBox>
#include "ModbusMaster.h"
#include "ModbusRegistersTable.h"
#include "database/DatabaseManager.h"
#include <QThread>


#define MOTOR_CMD(id, cmdType) ((id << 8) | (cmdType))

const QStringList coverStatusStr = {
    "Idle", "Opening", "Closing", "Fault",
};

const QStringList sealStatusStr = {
    "Idle", "Pushing", "Releasing", "Fault",
};

const QStringList churnStatusStr = {
    "Idle", "RuningCW", "RuningCCW", "Fault",
};

const QStringList tempStatusStr = {
    "Idle", "Running", "Fault",
};

SampleChipWidget::SampleChipWidget(const QString &tilte, QWidget *parent)
    : QDockWidget{tilte, parent}
{
    initSampleChipWidget();

    connect(btnOpenCover, &QPushButton::clicked, this, &SampleChipWidget::onOpenCoverClicked);
    connect(btnCloseCover, &QPushButton::clicked, this, &SampleChipWidget::onCloseCoverClicked);
    connect(btnPressSample, &QPushButton::clicked, this, &SampleChipWidget::onPressSampleClicked);
    connect(btnReleaseSample, &QPushButton::clicked, this, &SampleChipWidget::onReleaseSampleClicked);
    connect(btnResetSampleChip, &QPushButton::clicked, this, &SampleChipWidget::onResetSampleClicked);
    connect(btnChurnRunCW, &QPushButton::clicked, this, &SampleChipWidget::onChurnCWClicked);
    connect(btnChurnRunCCW, &QPushButton::clicked, this, &SampleChipWidget::onChurnCCWClicked);
    connect(btnChurnStop, &QPushButton::clicked, this, &SampleChipWidget::onChurnStopClicked);
    connect(btnMoveXBackward, &ArrowButton::clicked, this, &SampleChipWidget::onMoveLeftClicked);
    connect(btnMoveXForward, &ArrowButton::clicked, this, &SampleChipWidget::onMoveRightClicked);
    connect(btnMoveYBackward, &ArrowButton::clicked, this, &SampleChipWidget::onMoveBackwardClicked);
    connect(btnMoveYForward, &ArrowButton::clicked, this, &SampleChipWidget::onMoveForwardClicked);
    connect(btnMoveLenDown, &ArrowButton::clicked, this, &SampleChipWidget::onMoveDownClicked);
    connect(btnMoveLenUp, &ArrowButton::clicked, this, &SampleChipWidget::onMoveUpClicked);
    connect(btnXGoZero, &QPushButton::clicked, this, &SampleChipWidget::onXReturnZeroClicked);
    connect(btnYGoZero, &QPushButton::clicked, this, &SampleChipWidget::onYReturnZeroClicked);
    connect(btnZGoZero, &QPushButton::clicked, this, &SampleChipWidget::onZReturnZeroClicked);
    connect(btnXGoPos, &QPushButton::clicked, this, &SampleChipWidget::onXRunToPosClicked);
    connect(btnYGoPos, &QPushButton::clicked, this, &SampleChipWidget::onYRunToPosClicked);
    connect(btnZGoPos, &QPushButton::clicked, this, &SampleChipWidget::onZRunToPosClicked);

    // Chip position DB
    connect(btnChipDbLoad, &QPushButton::clicked, this, &SampleChipWidget::onBtnChipDbLoadClicked);
    connect(btnChipDbSave, &QPushButton::clicked, this, &SampleChipWidget::onBtnChipDbSaveClicked);
    connect(btnChipDbDelete, &QPushButton::clicked, this, &SampleChipWidget::onBtnChipDbDeleteClicked);
    connect(btnChipDbOverwrite, &QPushButton::clicked, this, &SampleChipWidget::onBtnChipDbOverwriteClicked);

    // Lens position DB
    connect(btnLensDbLoad, &QPushButton::clicked, this, &SampleChipWidget::onBtnLensDbLoadClicked);
    connect(btnLensDbSave, &QPushButton::clicked, this, &SampleChipWidget::onBtnLensDbSaveClicked);
    connect(btnLensDbDelete, &QPushButton::clicked, this, &SampleChipWidget::onBtnLensDbDeleteClicked);
    connect(btnLensDbOverwrite, &QPushButton::clicked, this, &SampleChipWidget::onBtnLensDbOverwriteClicked);
}

void SampleChipWidget::updateStatus(const QVector<uint16_t> &regs)
{
    if (regs.length() < 13) {
        qDebug() << "Wrong status registers number! Need 13, but actually " << regs.length();
        return;
    }
    coverStatus = regs[0];
    sealStatus = regs[1];
    churnStatus= regs[2];
    motorXStatus = regs[3];
    motorYStatus = regs[4];
    motorZStatus = regs[5];

    motorXLimit = regs[6] & 0x0003;
    motorYLimit = (regs[6] >> 2) & 0x0003;
    motorZLimit = (regs[6] >> 4) & 0x0003;
    motorXPos = ((uint32_t)regs[7] << 16 | (uint32_t)regs[8]);
    motorYPos = ((uint32_t)regs[9] << 16 | (uint32_t)regs[10]);
    motorZPos = ((uint32_t)regs[11] << 16 | (uint32_t)regs[12]);

    lblCoverStatus->setText(coverStatusStr[coverStatus]);
    lblPressStatus->setText(sealStatusStr[sealStatus]);
    lblChurnStatus->setText(churnStatusStr[churnStatus]);
    lblMotorXStatus->setText(QString::asprintf("X Status: 0x%4x", motorXStatus));
    lblMotorYStatus->setText(QString::asprintf("Y Status: 0x%4x", motorYStatus));
    lblMotorZStatus->setText(QString::asprintf("Len Status: 0x%4x", motorZStatus));
    lblChipPos->setText(QString("X: %1, Y: %2").arg(motorXPos).arg(motorYPos));
    lblLenPos->setText(QString("Len(Z) Pos: %1").arg(motorZPos));
}


void SampleChipWidget::onOpenCoverClicked()
{
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_COVER_CMD, COVER_CMD_OPEN);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_COVER_BIT);
}

void SampleChipWidget::onCloseCoverClicked()
{
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_COVER_CMD, COVER_CMD_CLOSE);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_COVER_BIT);
}

void SampleChipWidget::onPressSampleClicked()
{
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_SEAL_CMD, SEAL_CMD_PUSH);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_SEAL_BIT);
}

void SampleChipWidget::onReleaseSampleClicked()
{
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_SEAL_CMD, SEAL_CMD_RELEASE);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_SEAL_BIT);
}

void SampleChipWidget::onResetSampleClicked()
{
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_SEAL_CMD, SEAL_CMD_RESET);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_SEAL_BIT);
}

void SampleChipWidget::onChurnCWClicked()
{
    uint16_t speed = spinChurnSpeed->value() * 160 / 60;

    const QVector<uint16_t> churnCmd = {CHURN_CMD_RUN_CW, speed};
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_CHURN_CMD, churnCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_CHURN_BIT);
}

void SampleChipWidget::onChurnCCWClicked()
{
    uint16_t speed = spinChurnSpeed->value() * 160 / 60;

    const QVector<uint16_t> churnCmd = {CHURN_CMD_RUN_CCW, speed};
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_CHURN_CMD, churnCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_CHURN_BIT);
}

void SampleChipWidget::onChurnStopClicked()
{
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CHURN_CMD, CHURN_CMD_STOP);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_CHURN_BIT);
}

void SampleChipWidget::onMoveLeftClicked()
{
    uint16_t cmd = (MOTOR_X << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinXSteps->value() * -1;

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onMoveRightClicked()
{
    uint16_t cmd = (MOTOR_X << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinXSteps->value();

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onMoveForwardClicked()
{
    uint16_t cmd = (MOTOR_Y << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinYSteps->value() * -1;

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onMoveBackwardClicked()
{
    uint16_t cmd = (MOTOR_Y << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinYSteps->value();

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);
    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onMoveUpClicked()
{
    uint16_t cmd = (MOTOR_Z << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinZSteps->value() * -1;

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onMoveDownClicked()
{
    uint16_t cmd = (MOTOR_Z << 8) | MOTOR_CMD_RUN_STEPS;
    int32_t steps = spinZSteps->value();

    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((steps >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(steps & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}



void SampleChipWidget::onXReturnZeroClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_X, MOTOR_CMD_RETURN_HOME);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, cmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onYReturnZeroClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_Y, MOTOR_CMD_RETURN_HOME);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, cmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onZReturnZeroClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_Z, MOTOR_CMD_RETURN_HOME);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, cmd);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onXRunToPosClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_X, MOTOR_CMD_RUN_POS);
    int32_t pos = spinXPos->value();
    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((pos >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(pos & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onYRunToPosClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_Y, MOTOR_CMD_RUN_POS);
    int32_t pos = spinYPos->value();
    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((pos >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(pos & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

void SampleChipWidget::onZRunToPosClicked()
{
    uint16_t cmd = MOTOR_CMD(MOTOR_Z, MOTOR_CMD_RUN_POS);
    int32_t pos = spinZPos->value();
    QVector<uint16_t> motorCmd(3, 0);

    motorCmd[0] = cmd;
    motorCmd[1] = (uint16_t)((pos >> 16) & 0x0000FFFF);
    motorCmd[2] = (uint16_t)(pos & 0x0000FFFF);

    ModbusMaster::instance().asyncWriteMultipleRegisters(SLAVE_ADDR, MOTOR_CTRL_MOTOR_CMD, motorCmd);

    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, 0x0000);
    ModbusMaster::instance().asyncWriteSingleRegister(SLAVE_ADDR, MOTOR_CTRL_CW, CW_STEPPER_MOTOR_BIT);
}

// ---- Chip Position DB slots ----

void SampleChipWidget::onBtnChipDbLoadClicked()
{
    QString name = cmbChipDbConfigs->currentText();
    if (name.isEmpty())
        return;

    ChipPosition pos = m_chipDao.queryByName(name);
    if (pos.id < 0) {
        QMessageBox::warning(this, tr("Load Failed"),
                             tr("Chip position \"%1\" not found in database.").arg(name));
        return;
    }

    // Set spinboxes and move to position
    spinXPos->setValue(pos.xPosition);
    spinYPos->setValue(pos.yPosition);
    onXRunToPosClicked();
    onYRunToPosClicked();
}

void SampleChipWidget::onBtnChipDbSaveClicked()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Save Chip Position"),
                                         tr("Enter position name:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    name = name.trimmed();

    ChipPosition existing = m_chipDao.queryByName(name);
    if (existing.id >= 0) {
        QMessageBox::warning(this, tr("Save Failed"),
                             tr("Name \"%1\" already exists. Use \"Overwrite\" to update.").arg(name));
        return;
    }

    ChipPosition pos;
    pos.name = name;
    pos.xPosition = motorXPos;
    pos.yPosition = motorYPos;

    if (m_chipDao.insert(pos)) {
        refreshChipDbComboBox();
        cmbChipDbConfigs->setCurrentText(name);
    } else {
        QMessageBox::warning(this, tr("Save Failed"), tr("Failed to save chip position to database."));
    }
}

void SampleChipWidget::onBtnChipDbDeleteClicked()
{
    QString name = cmbChipDbConfigs->currentText();
    if (name.isEmpty())
        return;

    int ret = QMessageBox::question(this, tr("Delete Chip Position"),
                                    tr("Delete position \"%1\"?").arg(name),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    ChipPosition pos = m_chipDao.queryByName(name);
    if (pos.id >= 0 && m_chipDao.remove(pos.id)) {
        refreshChipDbComboBox();
    } else {
        QMessageBox::warning(this, tr("Delete Failed"), tr("Failed to delete chip position."));
    }
}

void SampleChipWidget::onBtnChipDbOverwriteClicked()
{
    QString name = cmbChipDbConfigs->currentText();
    if (name.isEmpty())
        return;

    ChipPosition existing = m_chipDao.queryByName(name);
    if (existing.id < 0) {
        QMessageBox::warning(this, tr("Overwrite Failed"),
                             tr("Position \"%1\" not found in database.").arg(name));
        return;
    }

    int ret = QMessageBox::question(this, tr("Overwrite Chip Position"),
                                    tr("Overwrite \"%1\" with current position (X:%2, Y:%3)?")
                                        .arg(name).arg(motorXPos).arg(motorYPos),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    existing.xPosition = motorXPos;
    existing.yPosition = motorYPos;

    if (!m_chipDao.update(existing)) {
        QMessageBox::warning(this, tr("Overwrite Failed"), tr("Failed to update chip position in database."));
    }
}

// ---- Lens Position DB slots ----

void SampleChipWidget::onBtnLensDbLoadClicked()
{
    QString name = cmbLensDbConfigs->currentText();
    if (name.isEmpty())
        return;

    LensPosition pos = m_lensDao.queryByName(name);
    if (pos.id < 0) {
        QMessageBox::warning(this, tr("Load Failed"),
                             tr("Lens position \"%1\" not found in database.").arg(name));
        return;
    }

    spinZPos->setValue(pos.zPosition);
    onZRunToPosClicked();
}

void SampleChipWidget::onBtnLensDbSaveClicked()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Save Lens Position"),
                                         tr("Enter position name:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    name = name.trimmed();

    LensPosition existing = m_lensDao.queryByName(name);
    if (existing.id >= 0) {
        QMessageBox::warning(this, tr("Save Failed"),
                             tr("Name \"%1\" already exists. Use \"Overwrite\" to update.").arg(name));
        return;
    }

    LensPosition pos;
    pos.name = name;
    pos.zPosition = motorZPos;

    if (m_lensDao.insert(pos)) {
        refreshLensDbComboBox();
        cmbLensDbConfigs->setCurrentText(name);
    } else {
        QMessageBox::warning(this, tr("Save Failed"), tr("Failed to save lens position to database."));
    }
}

void SampleChipWidget::onBtnLensDbDeleteClicked()
{
    QString name = cmbLensDbConfigs->currentText();
    if (name.isEmpty())
        return;

    int ret = QMessageBox::question(this, tr("Delete Lens Position"),
                                    tr("Delete position \"%1\"?").arg(name),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    LensPosition pos = m_lensDao.queryByName(name);
    if (pos.id >= 0 && m_lensDao.remove(pos.id)) {
        refreshLensDbComboBox();
    } else {
        QMessageBox::warning(this, tr("Delete Failed"), tr("Failed to delete lens position."));
    }
}

void SampleChipWidget::onBtnLensDbOverwriteClicked()
{
    QString name = cmbLensDbConfigs->currentText();
    if (name.isEmpty())
        return;

    LensPosition existing = m_lensDao.queryByName(name);
    if (existing.id < 0) {
        QMessageBox::warning(this, tr("Overwrite Failed"),
                             tr("Position \"%1\" not found in database.").arg(name));
        return;
    }

    int ret = QMessageBox::question(this, tr("Overwrite Lens Position"),
                                    tr("Overwrite \"%1\" with current Z position (%2)?")
                                        .arg(name).arg(motorZPos),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    existing.zPosition = motorZPos;

    if (!m_lensDao.update(existing)) {
        QMessageBox::warning(this, tr("Overwrite Failed"), tr("Failed to update lens position in database."));
    }
}

// ---- Helper methods ----

void SampleChipWidget::refreshChipDbComboBox()
{
    cmbChipDbConfigs->clear();
    if (DatabaseManager::instance().isConnected()) {
        cmbChipDbConfigs->addItems(m_chipDao.queryAllNames());
    }
}

void SampleChipWidget::refreshLensDbComboBox()
{
    cmbLensDbConfigs->clear();
    if (DatabaseManager::instance().isConnected()) {
        cmbLensDbConfigs->addItems(m_lensDao.queryAllNames());
    }
}

// ---- UI initialization ----

void SampleChipWidget::initSampleChipWidget()
{
    QGroupBox *groupChipMove = new QGroupBox("Chip Move", this);
    QGroupBox *groupLenMove = new QGroupBox("Len Move", this);
    QGroupBox *groupCover = new QGroupBox("Cover Control", this);
    QGroupBox *groupSeal = new QGroupBox("Seal Control", this);
    QGroupBox *groupChurn = new QGroupBox("Churn Control", this);

    auto addSpin = [&](QGridLayout *lay, int row, int col, const QString &prompt, QAbstractSpinBox *spinbox) {
        QLabel *lblPrompt = new QLabel(prompt, this);
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(lblPrompt, 0, Qt::AlignCenter);
        layout->addWidget(spinbox, 0, Qt::AlignCenter);
        lay->addLayout(layout, row, col, Qt::AlignCenter);
    };

    auto addSpinButton = [&](QGridLayout *lay, int row, int col, QPushButton *button, QSpinBox *spinbox) {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(button, 0, Qt::AlignCenter);
        layout->addWidget(spinbox, 0, Qt::AlignCenter);
        lay->addLayout(layout, row, col, Qt::AlignCenter);
    };


    btnMoveXBackward = new ArrowButton(ArrowButton::Direction::Left, this);
    btnMoveXForward = new ArrowButton(ArrowButton::Direction::Right, this);
    btnMoveYBackward = new ArrowButton(ArrowButton::Direction::Down, this);
    btnMoveYForward = new ArrowButton(ArrowButton::Direction::Up, this);

    btnXGoZero = new QPushButton("Go X Zero", this);
    btnYGoZero = new QPushButton("Go Y Zero", this);
    btnZGoZero = new QPushButton("Go Z Zero", this);

    btnXGoPos = new QPushButton("Go X Pos", this);
    btnYGoPos = new QPushButton("Go Y Pos", this);
    btnZGoPos = new QPushButton("Go Z Pos", this);

    spinXSteps = new QSpinBox(this);
    spinYSteps = new QSpinBox(this);
    spinZSteps = new QSpinBox(this);

    spinXSteps->setRange(1, 50000);
    spinYSteps->setRange(1, 100000);
    spinZSteps->setRange(1, 10000);
    spinXSteps->setSingleStep(100);
    spinYSteps->setSingleStep(100);
    spinZSteps->setSingleStep(100);
    spinXSteps->setValue(10000);
    spinYSteps->setValue(10000);
    spinZSteps->setValue(5000);

    spinXPos = new QSpinBox(this);
    spinYPos = new QSpinBox(this);
    spinZPos = new QSpinBox(this);
    spinXPos->setRange(-100000, 70000);
    spinXPos->setSingleStep(1000);
    spinXPos->setValue(-24000);
    spinYPos->setRange(-30000, 700000);
    spinYPos->setSingleStep(1000);
    spinYPos->setValue(7885);
    spinZPos->setRange(-10000, 35000);
    spinZPos->setSingleStep(1000);
    spinZPos->setValue(-7600);

    btnMoveLenUp = new ArrowButton(ArrowButton::Direction::Up, this);
    btnMoveLenDown = new ArrowButton(ArrowButton::Direction::Down, this);

    btnOpenCover = new QPushButton("Open Cover", this);
    btnCloseCover = new QPushButton("Close Cover", this);

    btnPressSample = new QPushButton("Press Sample", this);
    btnReleaseSample = new QPushButton("Release Sample", this);
    btnResetSampleChip = new QPushButton("Reset Sample", this);

    btnChurnRunCW = new QPushButton("Churn CW", this);
    btnChurnRunCCW = new QPushButton("Churn CCW", this);
    btnChurnStop = new QPushButton("Churn Stop", this);

    lblCoverStatus = new QLabel("Cover: Opened", this);
    lblPressStatus = new QLabel("Presser: Released", this);
    lblChurnStatus = new QLabel("Churn: IDLE", this);
    lblChipPos = new QLabel("X Pos: 100, \nY Pos: 10000", this);
    lblMotorXStatus = new QLabel("X Status: 0");
    lblMotorYStatus = new QLabel("Y Status: 0");
    lblMotorZStatus = new QLabel("Len Status: 0");
    lblLenPos = new QLabel("Len(Z) Pos: 10000", this);

    spinChurnSpeed = new QSpinBox(this);
    spinChurnSpeed->setMinimum(10);
    spinChurnSpeed->setMaximum(2000);
    spinChurnSpeed->setValue(120);

    QHBoxLayout *coverLayout = new QHBoxLayout();
    coverLayout->addWidget(btnOpenCover);
    coverLayout->addWidget(btnCloseCover);
    coverLayout->addWidget(lblCoverStatus);

    QHBoxLayout *sealLayout = new QHBoxLayout();
    sealLayout->addWidget(btnPressSample);
    sealLayout->addWidget(btnReleaseSample);
    sealLayout->addWidget(btnResetSampleChip);
    sealLayout->addWidget(lblPressStatus);


    QHBoxLayout *churnLayout = new QHBoxLayout();
    churnLayout->addWidget(btnChurnRunCW);
    churnLayout->addWidget(btnChurnRunCCW);
    churnLayout->addWidget(btnChurnStop);
    churnLayout->addWidget(spinChurnSpeed);
    churnLayout->addWidget(lblChurnStatus);

    QGridLayout *chipMoveLayout = new QGridLayout();
    chipMoveLayout->addWidget(lblMotorXStatus, 0, 0, Qt::AlignCenter);
    chipMoveLayout->addWidget(lblMotorYStatus, 0, 2, Qt::AlignCenter);
    chipMoveLayout->addWidget(btnXGoZero, 1, 0, Qt::AlignCenter);
    chipMoveLayout->addWidget(btnYGoZero, 1, 2, Qt::AlignCenter);
    addSpin(chipMoveLayout, 2, 0, "X Steps:", spinXSteps);
    addSpin(chipMoveLayout, 2, 2, "Y Steps:", spinYSteps);
    chipMoveLayout->addWidget(btnMoveYForward, 3, 1);
    chipMoveLayout->addWidget(btnMoveXBackward, 4, 0);
    chipMoveLayout->addWidget(lblChipPos, 4, 1, Qt::AlignCenter);
    chipMoveLayout->addWidget(btnMoveXForward, 4, 2);
    chipMoveLayout->addWidget(btnMoveYBackward, 5, 1);
    addSpinButton(chipMoveLayout, 6, 0, btnXGoPos, spinXPos);
    addSpinButton(chipMoveLayout, 6, 2, btnYGoPos, spinYPos);

    // ---- Chip Position DB management (embedded in Chip Move group) ----
    cmbChipDbConfigs = new QComboBox(this);
    cmbChipDbConfigs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnChipDbLoad = new QPushButton(tr("Load"), this);
    btnChipDbSave = new QPushButton(tr("Save As"), this);
    btnChipDbOverwrite = new QPushButton(tr("Overwrite"), this);
    btnChipDbDelete = new QPushButton(tr("Delete"), this);

    QHBoxLayout *chipDbBtnLayout = new QHBoxLayout;
    chipDbBtnLayout->addWidget(btnChipDbLoad);
    chipDbBtnLayout->addWidget(btnChipDbSave);
    chipDbBtnLayout->addWidget(btnChipDbOverwrite);
    chipDbBtnLayout->addWidget(btnChipDbDelete);

    QVBoxLayout *chipDbLayout = new QVBoxLayout;
    chipDbLayout->addWidget(cmbChipDbConfigs);
    chipDbLayout->addLayout(chipDbBtnLayout);

    QGroupBox *grpChipDb = new QGroupBox(tr("Chip Position Management"), this);
    grpChipDb->setLayout(chipDbLayout);

    chipMoveLayout->addWidget(grpChipDb, 7, 0, 1, 3);


    QVBoxLayout *lenMoveLayout = new QVBoxLayout();
    lenMoveLayout->addWidget(lblMotorZStatus, 0, Qt::AlignCenter);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(new QLabel("Len Steps:", this), 0, Qt::AlignCenter);
    layout->addWidget(spinZSteps, 0, Qt::AlignCenter);
    lenMoveLayout->addWidget(btnZGoZero);
    lenMoveLayout->addLayout(layout);
    lenMoveLayout->addWidget(btnMoveLenUp);
    lenMoveLayout->addWidget(lblLenPos);
    lenMoveLayout->addWidget(btnMoveLenDown);
    QHBoxLayout *layout2 = new QHBoxLayout();
    layout2->addWidget(btnZGoPos);
    layout2->addWidget(spinZPos);
    lenMoveLayout->addLayout(layout2);

    // ---- Lens Position DB management (embedded in Len Move group) ----
    cmbLensDbConfigs = new QComboBox(this);
    cmbLensDbConfigs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnLensDbLoad = new QPushButton(tr("Load"), this);
    btnLensDbSave = new QPushButton(tr("Save As"), this);
    btnLensDbOverwrite = new QPushButton(tr("Overwrite"), this);
    btnLensDbDelete = new QPushButton(tr("Delete"), this);

    QVBoxLayout *lensDbLayout = new QVBoxLayout;
    lensDbLayout->addWidget(cmbLensDbConfigs);
    QHBoxLayout *lensDbBtnRow1 = new QHBoxLayout;
    lensDbBtnRow1->addWidget(btnLensDbLoad);
    lensDbBtnRow1->addWidget(btnLensDbSave);
    lensDbLayout->addLayout(lensDbBtnRow1);
    QHBoxLayout *lensDbBtnRow2 = new QHBoxLayout;
    lensDbBtnRow2->addWidget(btnLensDbOverwrite);
    lensDbBtnRow2->addWidget(btnLensDbDelete);
    lensDbLayout->addLayout(lensDbBtnRow2);

    QGroupBox *grpLensDb = new QGroupBox(tr("Lens Position Management"), this);
    grpLensDb->setLayout(lensDbLayout);

    lenMoveLayout->addWidget(grpLensDb);


    groupChipMove->setLayout(chipMoveLayout);
    groupLenMove->setLayout(lenMoveLayout);
    groupChurn->setLayout(churnLayout);
    groupCover->setLayout(coverLayout);
    groupSeal->setLayout(sealLayout);

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    QHBoxLayout *moveLayout = new QHBoxLayout();
    moveLayout->addWidget(groupChipMove, 3);
    moveLayout->addWidget(groupLenMove, 1);
    mainLayout->addWidget(groupCover, 1);
    mainLayout->addWidget(groupSeal, 1);
    mainLayout->addWidget(groupChurn, 1);
    mainLayout->addLayout(moveLayout, 6);

    setWidget(mainWidget);

    // Load saved configs from database
    refreshChipDbComboBox();
    refreshLensDbComboBox();
}
