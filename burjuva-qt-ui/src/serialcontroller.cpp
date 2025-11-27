#include "serialcontroller.h"
#include <QDebug>
#include <QDateTime>

SerialController::SerialController(QObject *parent)
    : QObject(parent)
    , m_serial(new QSerialPort(this))
    , m_queueTimer(new QTimer(this))
    , m_cycleTime(100)  // Default 100ms cycle time
    , m_waitingForResponse(false)
{
    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialController::handleReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred,
            this, &SerialController::handleError);
    
    connect(m_queueTimer, &QTimer::timeout,
            this, &SerialController::processCommandQueue);
}

SerialController::~SerialController()
{
    disconnectFromPort();
}

bool SerialController::connectToPort(const QString &portName, qint32 baudRate)
{
    if (m_serial->isOpen())
        disconnectFromPort();
    
    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    
    if (m_serial->open(QIODevice::ReadWrite)) {
        qDebug() << "Connected to" << portName << "at" << baudRate << "baud";
        
        // Start command queue processing
        m_queueTimer->start(m_cycleTime);
        
        emit connected();
        return true;
    }
    
    QString error = m_serial->errorString();
    qWarning() << "Failed to connect:" << error;
    emit errorOccurred(error);
    return false;
}

void SerialController::disconnectFromPort()
{
    if (m_serial->isOpen()) {
        m_queueTimer->stop();
        m_commandQueue.clear();
        m_serial->close();
        qDebug() << "Disconnected from serial port";
        emit disconnected();
    }
}

bool SerialController::isConnected() const
{
    return m_serial->isOpen();
}

QString SerialController::portName() const
{
    return m_serial->portName();
}

void SerialController::sendCommand(const QString &command)
{
    if (!m_serial->isOpen())
        return;
    
    // Add to queue
    m_commandQueue.enqueue(command);
}

void SerialController::sendCommandWithPriority(const QString &command)
{
    if (!m_serial->isOpen())
        return;
    
    // Send immediately (bypass queue)
    QString cmd = command + "\r\n";
    m_serial->write(cmd.toUtf8());
    m_serial->flush();
    
    m_lastCommand = command;
    m_waitingForResponse = true;
    
    qDebug() << "TX [PRIORITY]:" << command;
}

void SerialController::setCycleTime(int milliseconds)
{
    if (milliseconds < 10)
        milliseconds = 10;  // Minimum 10ms
    
    m_cycleTime = milliseconds;
    
    if (m_queueTimer->isActive()) {
        m_queueTimer->setInterval(milliseconds);
    }
    
    qDebug() << "Cycle time set to" << milliseconds << "ms";
}

void SerialController::processCommandQueue()
{
    // Don't process if waiting for response
    if (m_waitingForResponse)
        return;
    
    // Check if queue has commands
    if (m_commandQueue.isEmpty())
        return;
    
    // Get next command
    QString command = m_commandQueue.dequeue();
    
    // Send command
    QString cmd = command + "\r\n";
    m_serial->write(cmd.toUtf8());
    m_serial->flush();
    
    m_lastCommand = command;
    m_waitingForResponse = true;
    
    qDebug() << "TX:" << command;
}

void SerialController::handleReadyRead()
{
    m_buffer.append(m_serial->readAll());
    
    // Process complete lines
    while (m_buffer.contains('\n')) {
        int idx = m_buffer.indexOf('\n');
        QByteArray line = m_buffer.left(idx);
        m_buffer.remove(0, idx + 1);
        
        QString data = QString::fromUtf8(line).trimmed();
        
        if (data.isEmpty())
            continue;
        
        qDebug() << "RX:" << data;
        
        // Check for ACK
        if (data.startsWith("[ACK]")) {
            emit ackReceived(data);
        }
        
        // Check for command completion
        if (data.contains("Komut tamamlandi:")) {
            m_waitingForResponse = false;
            emit commandCompleted(m_lastCommand);
        }
        
        // Emit all received data
        emit dataReceived(data);
    }
}

void SerialController::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;
    
    QString errorString = m_serial->errorString();
    qWarning() << "Serial error:" << errorString;
    
    emit errorOccurred(errorString);
    
    // Disconnect on critical errors
    if (error == QSerialPort::ResourceError ||
        error == QSerialPort::PermissionError) {
        disconnectFromPort();
    }
}
