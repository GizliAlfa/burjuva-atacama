#ifndef SERIALCONTROLLER_H
#define SERIALCONTROLLER_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QQueue>

class SerialController : public QObject
{
    Q_OBJECT
    
public:
    explicit SerialController(QObject *parent = nullptr);
    ~SerialController();
    
    // Connection
    bool connectToPort(const QString &portName, qint32 baudRate = 115200);
    void disconnectFromPort();
    bool isConnected() const;
    QString portName() const;
    
    // Command sending
    void sendCommand(const QString &command);
    void sendCommandWithPriority(const QString &command); // Skip queue
    
    // Cycle time control
    void setCycleTime(int milliseconds);
    int cycleTime() const { return m_cycleTime; }
    
signals:
    void connected();
    void disconnected();
    void dataReceived(const QString &data);
    void ackReceived(const QString &ack);
    void errorOccurred(const QString &error);
    void commandCompleted(const QString &command);
    
private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    void processCommandQueue();
    
private:
    QSerialPort *m_serial;
    QByteArray m_buffer;
    QTimer *m_queueTimer;
    QQueue<QString> m_commandQueue;
    QString m_lastCommand;
    int m_cycleTime;
    bool m_waitingForResponse;
};

#endif // SERIALCONTROLLER_H
