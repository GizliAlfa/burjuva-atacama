#include "aio20widget.h"
#include "aio20channel.h"
#include "serialcontroller.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QRegularExpression>

AIO20Widget::AIO20Widget(int slot, SerialController *serial, QWidget *parent)
    : QWidget(parent)
    , m_slot(slot)
    , m_serial(serial)
{
    setupUI();
    
    connect(m_serial, &SerialController::dataReceived,
            this, &AIO20Widget::handleDataReceived);
    
    // Request initial states
    requestAllStates();
}

void AIO20Widget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Title
    QLabel *titleLabel = new QLabel(QString("AIO20 Modülü - Slot %1").arg(m_slot), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // Status
    m_statusLabel = new QLabel("Durum: Bekleniyor...", this);
    mainLayout->addWidget(m_statusLabel);
    
    mainLayout->addSpacing(10);
    
    // Scroll area for channels
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    QWidget *scrollContent = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    
    // Input channels (0-11)
    QGroupBox *inputGroup = new QGroupBox("Giriş Kanalları (0-11)", this);
    QGridLayout *inputGrid = new QGridLayout(inputGroup);
    inputGrid->setSpacing(5);
    
    for (int i = 0; i < 12; i++) {
        m_inputChannels[i] = new AIO20Channel(i, false, this);
        
        int row = i / 3;
        int col = i % 3;
        inputGrid->addWidget(m_inputChannels[i], row, col);
    }
    
    scrollLayout->addWidget(inputGroup);
    
    // Output channels (12-19)
    QGroupBox *outputGroup = new QGroupBox("Çıkış Kanalları (12-19)", this);
    QGridLayout *outputGrid = new QGridLayout(outputGroup);
    outputGrid->setSpacing(5);
    
    for (int i = 0; i < 8; i++) {
        int channelNum = i + 12;
        m_outputChannels[i] = new AIO20Channel(channelNum, true, this);
        
        connect(m_outputChannels[i], &AIO20Channel::valueChanged,
                this, &AIO20Widget::onChannelValueChanged);
        
        int row = i / 2;
        int col = i % 2;
        outputGrid->addWidget(m_outputChannels[i], row, col);
    }
    
    scrollLayout->addWidget(outputGroup);
    scrollLayout->addStretch();
    
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);
}

void AIO20Widget::updateState(const AIO20State &state)
{
    m_state = state;
    
    // Update input channels
    for (int i = 0; i < 12; i++) {
        AIO20ChannelState channelState = state.inputChannels[i];
        m_inputChannels[i]->setValue(channelState.value);
        m_inputChannels[i]->setMode(channelState.mode);
        m_inputChannels[i]->setAck(channelState.lastAck);
    }
    
    // Update output channels
    for (int i = 0; i < 8; i++) {
        AIO20ChannelState channelState = state.outputChannels[i];
        m_outputChannels[i]->setValue(channelState.value);
        m_outputChannels[i]->setMode(channelState.mode);
        m_outputChannels[i]->setAck(channelState.lastAck);
    }
}

void AIO20Widget::onChannelValueChanged(int channel, float value)
{
    QString cmd = QString("aio20:slot%1:kanal%2:set:%3")
                      .arg(m_slot)
                      .arg(channel)
                      .arg(value, 0, 'f', 2);
    
    m_serial->sendCommand(cmd);
    m_statusLabel->setText(QString("Kanal %1 = %2V").arg(channel).arg(value, 0, 'f', 2));
}

void AIO20Widget::handleDataReceived(const QString &data)
{
    // Parse AIO20 responses
    // Example: "[ACK] AIO20 Slot 1 Kanal 5 = 3.25V"
    // Example: "[ACK] AIO20 Slot 1 Kanal 15 Output = 5.00V"
    
    if (!data.contains(QString("Slot %1").arg(m_slot)))
        return;
    
    // Parse channel value
    QRegularExpression channelRegex(R"(Kanal\s+(\d+)\s+(?:Output\s+)?=\s+([\d.]+)\s*([VmA]+))");
    QRegularExpressionMatch match = channelRegex.match(data);
    
    if (match.hasMatch()) {
        int channel = match.captured(1).toInt();
        float value = match.captured(2).toFloat();
        QString unit = match.captured(3);
        
        // Convert mA to voltage for display (4-20mA range)
        if (unit.contains("mA", Qt::CaseInsensitive)) {
            // Keep as mA value
        }
        
        // Update appropriate channel
        if (channel >= 0 && channel < 12) {
            m_inputChannels[channel]->setValue(value);
            m_inputChannels[channel]->setAck(data);
            m_statusLabel->setText(QString("Kanal %1 güncellendi: %2%3")
                                       .arg(channel).arg(value, 0, 'f', 2).arg(unit));
        } else if (channel >= 12 && channel < 20) {
            int idx = channel - 12;
            m_outputChannels[idx]->setValue(value);
            m_outputChannels[idx]->setAck(data);
            m_statusLabel->setText(QString("Kanal %1 güncellendi: %2%3")
                                       .arg(channel).arg(value, 0, 'f', 2).arg(unit));
        }
        return;
    }
    
    // Parse mode configuration
    QRegularExpression modeRegex(R"(Kanal\s+(\d+)\s+Mode:\s+(\w+))");
    QRegularExpressionMatch modeMatch = modeRegex.match(data);
    
    if (modeMatch.hasMatch()) {
        int channel = modeMatch.captured(1).toInt();
        QString mode = modeMatch.captured(2);
        
        if (channel >= 0 && channel < 12) {
            m_inputChannels[channel]->setMode(mode);
        } else if (channel >= 12 && channel < 20) {
            int idx = channel - 12;
            m_outputChannels[idx]->setMode(mode);
        }
    }
}

void AIO20Widget::requestChannelState(int channel)
{
    QString cmd = QString("aio20:slot%1:kanal%2:oku").arg(m_slot).arg(channel);
    m_serial->sendCommand(cmd);
}

void AIO20Widget::requestAllStates()
{
    // Request all input channels
    for (int i = 0; i < 12; i++) {
        requestChannelState(i);
    }
    
    // Request all output channels
    for (int i = 12; i < 20; i++) {
        requestChannelState(i);
    }
}
