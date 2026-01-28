#ifndef MODBUSMASTER_H
#define MODBUSMASTER_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QVector>

class ModbusMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusMaster(QObject *parent = nullptr);
    ~ModbusMaster();

    // Singleton accessor
    static ModbusMaster &instance();

    void setPortName(const QString &name);
    void setBaudRate(qint32 baud);
    void setDataBits(QSerialPort::DataBits bits);
    void setParity(QSerialPort::Parity parity);
    void setStopBits(QSerialPort::StopBits stopBits);
    void setTimeout(int ms);

    bool open();
    void close();

    // Blocking API
    QVector<uint16_t> blockingReadHoldingRegisters(uint8_t slaveAddr, uint16_t startAddr, uint16_t quantity, QString *err = nullptr);
    bool blockingWriteSingleRegister(uint8_t slaveAddr, uint16_t addr, uint16_t value, QString *err = nullptr);
    bool blockingWriteMultipleRegisters(uint8_t slaveAddr, uint16_t startAddr, const QVector<uint16_t> &values, QString *err = nullptr);

    // Async API
    void asyncReadHoldingRegisters(uint8_t slaveAddr, uint16_t startAddr, uint16_t quantity);
    void asyncWriteSingleRegister(uint8_t slaveAddr, uint16_t addr, uint16_t value);
    void asyncWriteMultipleRegisters(uint8_t slaveAddr, uint16_t startAddr, const QVector<uint16_t> &values);

signals:
    void responseReady(uint8_t slave, uint8_t func, const QVector<uint16_t> &regs);
    void writeAck(uint8_t slave, uint8_t func, uint16_t addr, uint16_t qtyOrValue);
    void errorOccurred(const QString &msg);

private slots:
    void onSerialReadyRead();
    void onTimeout();
    void processNextRequest();

private:
    struct PendingRequest {
        QByteArray frame;
        uint8_t slaveAddr = 0;
        uint8_t function = 0;
    };

    static uint16_t crc16(const QByteArray &data);
    static QByteArray buildRead03(uint8_t slaveAddr, uint16_t startAddr, uint16_t qty);
    static QByteArray buildWrite06(uint8_t slaveAddr, uint16_t addr, uint16_t value);
    static QByteArray buildWrite16(uint8_t slaveAddr, uint16_t startAddr, const QVector<uint16_t> &values);

    void enqueueRequest(const PendingRequest &req);
    void handleFrame(const QByteArray &frame);

private:
    QSerialPort m_serial;
    QString m_portName;
    qint32 m_baud = QSerialPort::Baud19200;
    QSerialPort::DataBits m_dataBits = QSerialPort::Data8;
    QSerialPort::Parity m_parity = QSerialPort::NoParity;
    QSerialPort::StopBits m_stopBits = QSerialPort::OneStop;
    int m_timeoutMs = 1000;

    QByteArray m_rxBuffer;
    QQueue<PendingRequest> m_queue;
    QMutex m_mutex;

    bool m_busy = false;
    bool m_hasCurrent = false;
    PendingRequest m_currentRequest;
    QTimer *m_currentTimer = nullptr;
};

#endif
