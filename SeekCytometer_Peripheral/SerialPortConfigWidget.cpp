#include "SerialPortConfigWidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include "ModbusMaster.h"

SerialPortConfigWidget::SerialPortConfigWidget(const QString &tilte, QWidget *parent)
    :QDockWidget{tilte, parent}
{
    initDockWidget();

    // 初始填充串口与波特率
    populatePortList();
    populateBaudrateList();

    // 初始状态：未连接
    setUiEnabledWhenConnected(false);
}

void SerialPortConfigWidget::onRefreshPortsClicked()
{
    populatePortList();
    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("已刷新串口列表"));
}

void SerialPortConfigWidget::onConnectClicked()
{
    ModbusMaster &master = ModbusMaster::instance();
    bool currentlyOpen = false;
    // 我们通过 try 查询 ModbusMaster 的串口是否打开来判断（没有公开接口时可尝试通过 QSerialPort 直接检查）
    // 假设 ModbusMaster 提供 open()/close()，但没有 isOpen()，我们可以尝试检验内部 m_serial（如果你愿意暴露接口）
    // 这里以简单方式：按钮文字表示当前意图（Connect -> 建立连接；Disconnect -> 断开连接）
    if (btnConnect->text() == QStringLiteral("Connect") || btnConnect->text() == QStringLiteral("连接")) {
        // 建立连接
        QString portName = comboPort->currentText();
        if (portName.isEmpty() || portName == QStringLiteral("无可用串口")) {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请选择有效的串口"));
            return;
        }
        // 设置端口名与波特率
        qint32 baud = comboBaudrate->currentData().toInt();
        master.setPortName(portName);
        master.setBaudRate(baud);
        // 其余参数使用默认（DataBits=8, Parity=NoParity, StopBits=OneStop）
        bool ok = master.open();
        if (ok) {
            QMessageBox::information(this, QStringLiteral("连接成功"), QStringLiteral("Modbus 已成功连接"));
            btnConnect->setText(QStringLiteral("Disconnect"));
            setUiEnabledWhenConnected(true);
            statusLabel->setText(QStringLiteral("状态: 已连接 %1 @ %2").arg(portName).arg(baud));
            emit commPortConnectedOk();
        } else {
            QMessageBox::critical(this, QStringLiteral("连接失败"), QStringLiteral("无法打开串口，请检查端口与权限"));
            statusLabel->setText(QStringLiteral("状态: 连接失败"));
        }
    } else {
        // 断开连接
        master.close();
        QMessageBox::information(this, QStringLiteral("断开连接"), QStringLiteral("Modbus 已断开"));
        btnConnect->setText(QStringLiteral("Connect"));
        setUiEnabledWhenConnected(false);
        statusLabel->setText(QStringLiteral("状态: 未连接"));
        emit commPortDisconnected();
    }
}

void SerialPortConfigWidget::initDockWidget()
{
    comboPort = new QComboBox(this);
    comboBaudrate = new QComboBox(this);
    btnRefreshPorts = new QPushButton(QStringLiteral("刷新端口"), this);
    btnConnect = new QPushButton(QStringLiteral("Connect"), this);
    statusLabel = new QLabel(QStringLiteral("状态: 未连接"), this);

    QHBoxLayout *settingsLayout = new QHBoxLayout;
    settingsLayout->addWidget(new QLabel(QStringLiteral("串口:"), this));
    settingsLayout->addWidget(comboPort);
    settingsLayout->addWidget(btnRefreshPorts);
    settingsLayout->addSpacing(10);
    settingsLayout->addWidget(new QLabel(QStringLiteral("波特率:"), this));
    settingsLayout->addWidget(comboBaudrate);
    settingsLayout->addStretch();
    settingsLayout->addWidget(btnConnect);
    settingsLayout->addWidget(statusLabel);

    QWidget *mainWidget = new QWidget(this);
    mainWidget->setLayout(settingsLayout);
    setWidget(mainWidget);


    connect(btnRefreshPorts, &QPushButton::clicked, this, &SerialPortConfigWidget::onRefreshPortsClicked);
    connect(btnConnect, &QPushButton::clicked, this, &SerialPortConfigWidget::onConnectClicked);
}

void SerialPortConfigWidget::populatePortList()
{
    comboPort->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        QString display = info.portName();
#if defined(Q_OS_WIN)
        // Windows 上显示 COM 名称（例如 COM3）
        comboPort->addItem(display);
#else
        // Linux/macOS 显示完整路径，例如 /dev/ttyUSB0
        comboPort->addItem(info.systemLocation());
#endif
    }
    if (comboPort->count() == 0) {
        comboPort->addItem(QStringLiteral("无可用串口"));
        comboPort->setEnabled(false);
        btnConnect->setEnabled(false);
    } else {
        comboPort->setEnabled(true);
        btnConnect->setEnabled(true);
    }
}

void SerialPortConfigWidget::populateBaudrateList()
{
    comboBaudrate->clear();
    // 常见波特率
    QList<qint32> common = {9600, 19200, 38400, 57600, 115200, 230400};
    for (qint32 b : common) {
        comboBaudrate->addItem(QString::number(b), b);
    }
    // 默认选择 19200（若存在）
    int idx = comboBaudrate->findText("115200");
    if (idx >= 0) comboBaudrate->setCurrentIndex(idx);
}

void SerialPortConfigWidget::setUiEnabledWhenConnected(bool connected)
{
    // 连接后不能修改端口（防止误操作），但可以刷新（若你需要也可禁用刷新）
    comboPort->setEnabled(!connected);
    comboBaudrate->setEnabled(!connected);
    btnRefreshPorts->setEnabled(!connected);
}
