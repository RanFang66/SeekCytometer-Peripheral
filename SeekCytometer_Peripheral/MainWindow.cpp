#include "MainWindow.h"
#include "SerialPortConfigWidget.h"
#include <QMessageBox>
#include "ModbusMaster.h"
#include "ModbusRegistersTable.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_count(0)
{

    setWindowIcon(QIcon(":/fr_icon.ico"));
    setWindowState(Qt::WindowState::WindowMaximized);
    initDockWidgets();
    statusUpdateTimer = new QTimer(this);

    statusUpdateTimer->stop();
    statusUpdateTimer->setInterval(400);
    connect(statusUpdateTimer, &QTimer::timeout, this, &MainWindow::onNeedUpdateStatus);
    connect(serialConfigWidget, &SerialPortConfigWidget::commPortConnectedOk, this, [this](){
        statusUpdateTimer->start();
    });
    connect(serialConfigWidget, &SerialPortConfigWidget::commPortDisconnected, this, [this](){
        statusUpdateTimer->stop();
    });

    ModbusMaster &master = ModbusMaster::instance();
    connect(&master, &ModbusMaster::responseReady, this, &MainWindow::handleResponse);
    connect(&master, &ModbusMaster::writeAck, this, &MainWindow::handleWriteAck);
    connect(&master, &ModbusMaster::errorOccurred, this, &MainWindow::handleError);


}

MainWindow::~MainWindow()
{
    statusUpdateTimer->stop();
}


void MainWindow::initDockWidgets()
{
    QWidget *p = takeCentralWidget();
    if (p) {
        delete p;
    }
    setDockNestingEnabled(true);
    setStyleSheet(
        "QDockWidget {"
        "   border: 3px solid #0a0a0a;"
        "}"
        "QDockWidget::title {"
        "   background: #e0e0e0;"
        "   padding: 3px;"
        "}"
    );

    serialConfigWidget = new SerialPortConfigWidget("Communication Setting", this);
    addDockWidget(Qt::TopDockWidgetArea, serialConfigWidget);

    sampleChipWidget = new SampleChipWidget("Chip Control", this);
    splitDockWidget(serialConfigWidget, sampleChipWidget, Qt::Vertical);

    opticsControlWidget = new OpticsControlWidget("Optics Control", this);
    splitDockWidget(sampleChipWidget, opticsControlWidget, Qt::Horizontal);

    tempCtrlWidget = new TempControlWidget("Temp Control", this);
    splitDockWidget(sampleChipWidget, tempCtrlWidget, Qt::Vertical);



    sampleParaWidget = new SampleParaWidget("Sample Parameters", this);
    splitDockWidget(opticsControlWidget, sampleParaWidget, Qt::Vertical);

    microFluidicWidget = new MicroFluidicWidget("MicroFluidic Control", this);
    splitDockWidget(sampleParaWidget, microFluidicWidget, Qt::Horizontal);

    pztCtrlWidget = new PZTCtrlWidget("PZT Control", this);
    splitDockWidget(microFluidicWidget, pztCtrlWidget, Qt::Horizontal);

    mfcParasWidget = new MFCParasSetWidget("MFC Paras Set", this);
    tabifyDockWidget(microFluidicWidget, mfcParasWidget);

    microFluidicWidget->raise();

}

void MainWindow::handleResponse(uint8_t slave, uint8_t func, const QVector<uint16_t> &regs)
{
    // Q_UNUSED(func);
    // QString s = QStringLiteral("收到读响应，从站%1:").arg(slave);
    // for (int i = 0; i < regs.size(); ++i) {
    //     s += QString(" R%1=0x%2").arg(i).arg(regs[i], 0, 16);
    // }
    // qDebug() << s;

    if (m_count == 1 && regs.size() == 21) {
        tempCtrlWidget->updateStatus(regs.first(2));
        sampleChipWidget->updateStatus(regs.mid(2, 13));
        opticsControlWidget->updateStatus(regs.last(6));
    } else if (m_count == 2 && regs.size() == 13) {
        microFluidicWidget->updateStatus(regs);
    } else if (m_count == 3 && regs.size() == 11) {
        pztCtrlWidget->updateStatus(regs);
    }
}

void MainWindow::handleWriteAck(uint8_t slave, uint8_t func, uint16_t addr, uint16_t qtyOrVal)
{
    QString s;
    if (func == 0x06) {
        s = QStringLiteral("写单寄存器应答，从站%1 地址0x%2 值0x%3").arg(slave).arg(addr,0,16).arg(qtyOrVal,0,16);
    } else {
        s = QStringLiteral("写多寄存器应答，从站%1 起始地址0x%2 数量%3").arg(slave).arg(addr,0,16).arg(qtyOrVal);
    }
    qDebug() << s;
}

void MainWindow::handleError(const QString &err)
{
    // QMessageBox::warning(this, QStringLiteral("Modbus 错误"), err);
    qWarning() << "Modbus 错误:" << err;
}

void MainWindow::onNeedUpdateStatus()
{
    if (m_count == 0) {
        // Update motor control board status
        ModbusMaster::instance().asyncReadHoldingRegisters(SLAVE_ADDR, MOTOR_CTRL_TEMP_STATUS, 21);
        m_count = 1;
    } else if (m_count == 1) {
        ModbusMaster::instance().asyncReadHoldingRegisters(SLAVE_ADDR, MFC_PRESS_CTRL_STATUS, 13);
        m_count = 2;
    } else if (m_count == 2) {
        // Update PZT board status
        ModbusMaster::instance().asyncReadHoldingRegisters(SLAVE_ADDR, PZT_STATUS_WORD, 11);
        m_count = 3;
    } else {
        // Update AD board status
        m_count = 0;
    }
}
