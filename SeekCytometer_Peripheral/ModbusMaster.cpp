// modbusmaster.cpp
// Complete implementation for ModbusMaster (paired with modbusmaster.h provided earlier).
// Comments in English.

#include "ModbusMaster.h"
#include <QEventLoop>
#include <QDebug>
#include <QMetaObject>

ModbusMaster::ModbusMaster(QObject *parent)
    : QObject(parent),
    m_currentTimer(nullptr)
{
    // connect serial readyRead to our handler
    connect(&m_serial, &QSerialPort::readyRead, this, &ModbusMaster::onSerialReadyRead);
}

ModbusMaster::~ModbusMaster()
{
    close();
}

/* ---------- singleton accessor ---------- */
ModbusMaster &ModbusMaster::instance()
{
    static ModbusMaster inst(nullptr);
    return inst;
}

/* ---------- simple setters ---------- */
void ModbusMaster::setPortName(const QString &name) { m_portName = name; }
void ModbusMaster::setBaudRate(qint32 baud) { m_baud = baud; }
void ModbusMaster::setDataBits(QSerialPort::DataBits bits) { m_dataBits = bits; }
void ModbusMaster::setParity(QSerialPort::Parity parity) { m_parity = parity; }
void ModbusMaster::setStopBits(QSerialPort::StopBits stopBits) { m_stopBits = stopBits; }
void ModbusMaster::setTimeout(int ms) { m_timeoutMs = ms; }

/* ---------- open / close ---------- */
bool ModbusMaster::open()
{
    if (m_serial.isOpen()) return true;
    m_serial.setPortName(m_portName);
    if (!m_serial.open(QIODevice::ReadWrite)) {
        emit errorOccurred(QStringLiteral("Open serial port failed: %1").arg(m_serial.errorString()));
        return false;
    }
    m_serial.setBaudRate(m_baud);
    m_serial.setDataBits(m_dataBits);
    m_serial.setParity(m_parity);
    m_serial.setStopBits(m_stopBits);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);
    return true;
}

void ModbusMaster::close()
{
    {
        QMutexLocker locker(&m_mutex);
        // clear queued requests
        while (!m_queue.isEmpty()) m_queue.dequeue();

        // stop and delete current timer if any
        if (m_currentTimer) {
            m_currentTimer->stop();
            m_currentTimer->deleteLater();
            m_currentTimer = nullptr;
        }

        m_hasCurrent = false;
        m_busy = false;
        m_rxBuffer.clear();
    }

    if (m_serial.isOpen()) m_serial.close();
}

/* ---------- CRC16 (Modbus RTU) ---------- */
uint16_t ModbusMaster::crc16(const QByteArray &data)
{
    uint16_t crc = 0xFFFF;
    for (unsigned char ch : data) {
        crc ^= static_cast<uint8_t>(ch);
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

/* ---------- Frame builders ---------- */
QByteArray ModbusMaster::buildRead03(uint8_t slaveAddr, uint16_t startAddr, uint16_t qty)
{
    QByteArray frame;
    frame.append(static_cast<char>(slaveAddr));
    frame.append(static_cast<char>(0x03));
    frame.append(static_cast<char>((startAddr >> 8) & 0xFF));
    frame.append(static_cast<char>(startAddr & 0xFF));
    frame.append(static_cast<char>((qty >> 8) & 0xFF));
    frame.append(static_cast<char>(qty & 0xFF));
    uint16_t c = crc16(frame);
    frame.append(static_cast<char>(c & 0xFF));        // CRC low
    frame.append(static_cast<char>((c >> 8) & 0xFF)); // CRC high
    return frame;
}

QByteArray ModbusMaster::buildWrite06(uint8_t slaveAddr, uint16_t addr, uint16_t value)
{
    QByteArray frame;
    frame.append(static_cast<char>(slaveAddr));
    frame.append(static_cast<char>(0x06));
    frame.append(static_cast<char>((addr >> 8) & 0xFF));
    frame.append(static_cast<char>(addr & 0xFF));
    frame.append(static_cast<char>((value >> 8) & 0xFF));
    frame.append(static_cast<char>(value & 0xFF));
    uint16_t c = crc16(frame);
    frame.append(static_cast<char>(c & 0xFF));
    frame.append(static_cast<char>((c >> 8) & 0xFF));
    return frame;
}

QByteArray ModbusMaster::buildWrite16(uint8_t slaveAddr, uint16_t startAddr, const QVector<uint16_t> &values)
{
    QByteArray frame;
    frame.append(static_cast<char>(slaveAddr));
    frame.append(static_cast<char>(0x10));
    frame.append(static_cast<char>((startAddr >> 8) & 0xFF));
    frame.append(static_cast<char>(startAddr & 0xFF));
    uint16_t qty = static_cast<uint16_t>(values.size());
    frame.append(static_cast<char>((qty >> 8) & 0xFF));
    frame.append(static_cast<char>((qty) & 0xFF));
    frame.append(static_cast<char>((qty * 2) & 0xFF)); // byte count
    for (uint16_t v : values) {
        frame.append(static_cast<char>((v >> 8) & 0xFF));
        frame.append(static_cast<char>(v & 0xFF));
    }
    uint16_t c = crc16(frame);
    frame.append(static_cast<char>(c & 0xFF));
    frame.append(static_cast<char>((c >> 8) & 0xFF));
    return frame;
}

/* ---------- Queueing & processing ---------- */

void ModbusMaster::enqueueRequest(const PendingRequest &req)
{
    bool needStart = false;
    {
        QMutexLocker locker(&m_mutex);
        m_queue.enqueue(req);
        // we should start processing if nothing is running
        if (!m_busy && !m_hasCurrent) {
            needStart = true;
        }
    } // release lock quickly

    if (needStart) {
        // queued invoke to avoid re-entry and nested lock acquisition
        QMetaObject::invokeMethod(this, "processNextRequest", Qt::QueuedConnection);
    }
}

void ModbusMaster::processNextRequest()
{
    PendingRequest req;
    {
        QMutexLocker locker(&m_mutex);
        // if some request is active, return
        if (m_busy || m_hasCurrent) return;
        if (m_queue.isEmpty()) return;
        // dequeue and mark current
        req = m_queue.dequeue();
        m_currentRequest = req;
        m_hasCurrent = true;
        m_busy = true;
    } // unlock before doing IO

    // prepare to receive response for this request
    m_rxBuffer.clear();

    // create timer for this request
    if (m_currentTimer) {
        m_currentTimer->stop();
        m_currentTimer->deleteLater();
        m_currentTimer = nullptr;
    }
    m_currentTimer = new QTimer(this);
    m_currentTimer->setSingleShot(true);
    connect(m_currentTimer, &QTimer::timeout, this, &ModbusMaster::onTimeout);
    m_currentTimer->start(m_timeoutMs);

    // write frame to serial
    qint64 written = m_serial.write(m_currentRequest.frame);
    if (written == -1 || written != m_currentRequest.frame.size()) {
        // write failed: cleanup timer and current state, then schedule next
        if (m_currentTimer) {
            m_currentTimer->stop();
            m_currentTimer->deleteLater();
            m_currentTimer = nullptr;
        }

        {
            QMutexLocker locker(&m_mutex);
            m_hasCurrent = false;
            m_busy = false;
        }

        emit errorOccurred(QStringLiteral("Serial write failed: %1").arg(m_serial.errorString()));
        QMetaObject::invokeMethod(this, "processNextRequest", Qt::QueuedConnection);
        return;
    }
    // m_serial.flush();
    // optional small wait for bytes to be written
    m_serial.waitForBytesWritten(100);
    // now wait for readyRead or timeout
}

/* ---------- incoming data parsing ---------- */

void ModbusMaster::onSerialReadyRead()
{
    QByteArray chunk = m_serial.readAll();
    if (chunk.isEmpty()) return;
    m_rxBuffer.append(chunk);

    // try to parse complete frames; multiple frames may be concatenated
    while (m_rxBuffer.size() >= 5) {
        uint8_t slave = static_cast<uint8_t>(m_rxBuffer.at(0));
        uint8_t func = static_cast<uint8_t>(m_rxBuffer.at(1));
        int needed = -1;

        if (func == 0x03) {
            if (m_rxBuffer.size() < 3) break;
            int byteCount = static_cast<uint8_t>(m_rxBuffer.at(2));
            needed = 1 + 1 + 1 + byteCount + 2;
            if (m_rxBuffer.size() < needed) break;
        } else if (func == 0x06 || func == 0x10) {
            needed = 8;
            if (m_rxBuffer.size() < needed) break;
        } else if (func & 0x80) {
            needed = 5;
            if (m_rxBuffer.size() < needed) break;
        } else {
            // unknown func code; discard first byte to resync
            m_rxBuffer.remove(0, 1);
            continue;
        }

        QByteArray candidate = m_rxBuffer.left(needed);
        uint16_t calc = crc16(candidate.left(candidate.size() - 2));
        uint8_t crcLo = static_cast<uint8_t>(candidate.at(candidate.size() - 2));
        uint8_t crcHi = static_cast<uint8_t>(candidate.at(candidate.size() - 1));
        uint16_t frameCrc = static_cast<uint16_t>(crcLo) | (static_cast<uint16_t>(crcHi) << 8);

        if (calc == frameCrc) {
            handleFrame(candidate);
            m_rxBuffer.remove(0, needed);
        } else {
            // CRC mismatch -> drop first byte and try to resync
            m_rxBuffer.remove(0, 1);
        }
    }
}

void ModbusMaster::handleFrame(const QByteArray &frame)
{
    if (frame.size() < 5) return;

    // stop and delete timer for current request
    if (m_currentTimer) {
        m_currentTimer->stop();
        m_currentTimer->deleteLater();
        m_currentTimer = nullptr;
    }

    uint8_t slave = static_cast<uint8_t>(frame.at(0));
    uint8_t func = static_cast<uint8_t>(frame.at(1));

    if (func == 0x03) {
        int byteCount = static_cast<uint8_t>(frame.at(2));
        QVector<uint16_t> regs;
        regs.reserve(byteCount / 2);
        for (int i = 0; i < byteCount / 2; ++i) {
            uint8_t hi = static_cast<uint8_t>(frame.at(3 + i * 2));
            uint8_t lo = static_cast<uint8_t>(frame.at(3 + i * 2 + 1));
            uint16_t val = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
            regs.append(val);
        }
        emit responseReady(slave, func, regs);
    } else if (func == 0x06) {
        uint16_t addr = (static_cast<uint8_t>(frame.at(2)) << 8) | static_cast<uint8_t>(frame.at(3));
        uint16_t val = (static_cast<uint8_t>(frame.at(4)) << 8) | static_cast<uint8_t>(frame.at(5));
        emit writeAck(slave, func, addr, val);
    } else if (func == 0x10) {
        uint16_t addr = (static_cast<uint8_t>(frame.at(2)) << 8) | static_cast<uint8_t>(frame.at(3));
        uint16_t qty = (static_cast<uint8_t>(frame.at(4)) << 8) | static_cast<uint8_t>(frame.at(5));
        emit writeAck(slave, func, addr, qty);
    } else if ((func & 0x80) != 0) {
        uint8_t exc = static_cast<uint8_t>(frame.at(2));
        emit errorOccurred(QStringLiteral("Slave exception response: slave=%1 func=0x%2 exc=%3").arg(slave).arg(func, 0, 16).arg(exc));
    } else {
        emit errorOccurred(QStringLiteral("Unknown function code in response: 0x%1").arg(func, 0, 16));
    }

    // clear current state and schedule next queued request
    {
        QMutexLocker locker(&m_mutex);
        m_hasCurrent = false;
        m_busy = false;
    }
    QMetaObject::invokeMethod(this, "processNextRequest", Qt::QueuedConnection);
}

/* ---------- timeout handling ---------- */

void ModbusMaster::onTimeout()
{
    // current request timed out
    if (m_currentTimer) {
        m_currentTimer->stop();
        m_currentTimer->deleteLater();
        m_currentTimer = nullptr;
    }

    {
        QMutexLocker locker(&m_mutex);
        m_hasCurrent = false;
        m_busy = false;
    }

    emit errorOccurred(QStringLiteral("Modbus request timeout (%1 ms)").arg(m_timeoutMs));
    QMetaObject::invokeMethod(this, "processNextRequest", Qt::QueuedConnection);
}

/* ---------- blocking APIs (short-lived event loop) ---------- */

QVector<uint16_t> ModbusMaster::blockingReadHoldingRegisters(uint8_t slaveAddr, uint16_t startAddr, uint16_t quantity, QString *err)
{
    QVector<uint16_t> result;
    QEventLoop loop;
    QString lastErr;

    auto onResp = [&](uint8_t s, uint8_t f, const QVector<uint16_t> &regs){
        if (s == slaveAddr && f == 0x03) {
            result = regs;
            loop.quit();
        }
    };
    auto onErr = [&](const QString &e){
        lastErr = e;
        loop.quit();
    };

    QMetaObject::Connection c1 = connect(this, &ModbusMaster::responseReady, this, onResp);
    QMetaObject::Connection c2 = connect(this, &ModbusMaster::errorOccurred, this, onErr);

    PendingRequest req;
    req.frame = buildRead03(slaveAddr, startAddr, quantity);
    req.slaveAddr = slaveAddr;
    req.function = 0x03;
    enqueueRequest(req);

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, [&](){ lastErr = "blocking call timeout"; loop.quit(); });
    timer.start(m_timeoutMs + 200);
    loop.exec();

    disconnect(c1);
    disconnect(c2);

    if (!lastErr.isEmpty() && err) *err = lastErr;
    return result;
}

bool ModbusMaster::blockingWriteSingleRegister(uint8_t slaveAddr, uint16_t addr, uint16_t value, QString *err)
{
    bool ok = false;
    QEventLoop loop;
    QString lastErr;

    auto onAck = [&](uint8_t s, uint8_t f, uint16_t a, uint16_t v){
        if (s == slaveAddr && f == 0x06 && a == addr) {
            ok = true;
            loop.quit();
        }
    };
    auto onErr = [&](const QString &e){ lastErr = e; loop.quit(); };

    QMetaObject::Connection c1 = connect(this, &ModbusMaster::writeAck, this, onAck);
    QMetaObject::Connection c2 = connect(this, &ModbusMaster::errorOccurred, this, onErr);

    PendingRequest req;
    req.frame = buildWrite06(slaveAddr, addr, value);
    req.slaveAddr = slaveAddr;
    req.function = 0x06;
    enqueueRequest(req);

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, [&](){ lastErr = "blocking call timeout"; loop.quit(); });
    timer.start(m_timeoutMs + 200);
    loop.exec();

    disconnect(c1);
    disconnect(c2);

    if (!lastErr.isEmpty() && err) *err = lastErr;
    return ok;
}

bool ModbusMaster::blockingWriteMultipleRegisters(uint8_t slaveAddr, uint16_t startAddr, const QVector<uint16_t> &values, QString *err)
{
    bool ok = false;
    QEventLoop loop;
    QString lastErr;

    auto onAck = [&](uint8_t s, uint8_t f, uint16_t a, uint16_t qty){
        if (s == slaveAddr && f == 0x10 && a == startAddr && qty == values.size()) {
            ok = true;
            loop.quit();
        }
    };
    auto onErr = [&](const QString &e){ lastErr = e; loop.quit(); };

    QMetaObject::Connection c1 = connect(this, &ModbusMaster::writeAck, this, onAck);
    QMetaObject::Connection c2 = connect(this, &ModbusMaster::errorOccurred, this, onErr);

    PendingRequest req;
    req.frame = buildWrite16(slaveAddr, startAddr, values);
    req.slaveAddr = slaveAddr;
    req.function = 0x10;
    enqueueRequest(req);

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, [&](){ lastErr = "blocking call timeout"; loop.quit(); });
    timer.start(m_timeoutMs + 200);
    loop.exec();

    disconnect(c1);
    disconnect(c2);

    if (!lastErr.isEmpty() && err) *err = lastErr;
    return ok;
}

/* ---------- asynchronous APIs ---------- */

void ModbusMaster::asyncReadHoldingRegisters(uint8_t slaveAddr, uint16_t startAddr, uint16_t quantity)
{
    PendingRequest req;
    req.frame = buildRead03(slaveAddr, startAddr, quantity);
    req.slaveAddr = slaveAddr;
    req.function = 0x03;
    enqueueRequest(req);
}

void ModbusMaster::asyncWriteSingleRegister(uint8_t slaveAddr, uint16_t addr, uint16_t value)
{
    PendingRequest req;
    req.frame = buildWrite06(slaveAddr, addr, value);
    req.slaveAddr = slaveAddr;
    req.function = 0x06;
    enqueueRequest(req);
}

void ModbusMaster::asyncWriteMultipleRegisters(uint8_t slaveAddr, uint16_t startAddr, const QVector<uint16_t> &values)
{
    PendingRequest req;
    req.frame = buildWrite16(slaveAddr, startAddr, values);
    req.slaveAddr = slaveAddr;
    req.function = 0x10;
    enqueueRequest(req);
}
