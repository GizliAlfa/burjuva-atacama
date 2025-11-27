#include "io16widget.h"
#include "io16group.h"
#include "serialcontroller.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRegularExpression>

IO16Widget::IO16Widget(int slot, SerialController *serial, QWidget *parent)
    : QWidget(parent)
    , m_slot(slot)
    , m_serial(serial)
{
    setupUI();
    
    connect(m_serial, &SerialController::dataReceived,
            this, &IO16Widget::handleDataReceived);
    
    // Request initial states
    requestAllStates();
}

void IO16Widget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Title
    QLabel *titleLabel = new QLabel(QString("IO16 Modülü - Slot %1").arg(m_slot), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // Status
    m_statusLabel = new QLabel("Durum: Bekleniyor...", this);
    mainLayout->addWidget(m_statusLabel);
    
    mainLayout->addSpacing(10);
    
    // Groups (2x2 grid)
    QHBoxLayout *topRowLayout = new QHBoxLayout();
    QHBoxLayout *bottomRowLayout = new QHBoxLayout();
    
    for (int i = 0; i < 4; i++) {
        m_groups[i] = new IO16Group(i, this);
        
        connect(m_groups[i], &IO16Group::directionChangeRequested,
                this, &IO16Widget::onDirectionChanged);
        connect(m_groups[i], &IO16Group::pinToggled,
                this, &IO16Widget::onPinToggled);
        
        if (i < 2) {
            topRowLayout->addWidget(m_groups[i]);
        } else {
            bottomRowLayout->addWidget(m_groups[i]);
        }
    }
    
    mainLayout->addLayout(topRowLayout);
    mainLayout->addLayout(bottomRowLayout);
    mainLayout->addStretch();
}

void IO16Widget::updateState(const IO16State &state)
{
    m_state = state;
    
    for (int group = 0; group < 4; group++) {
        IO16GroupState groupState = state.groups[group];
        
        m_groups[group]->setDirection(groupState.isOutput);
        
        for (int pin = 0; pin < 4; pin++) {
            m_groups[group]->setPinValue(pin, groupState.pins[pin].value);
            m_groups[group]->setPinAck(pin, groupState.pins[pin].lastAck);
        }
    }
}

void IO16Widget::onDirectionChanged(int group, bool isOutput)
{
    QString cmd = QString("io16:slot%1:grup%2:direction:%3")
                      .arg(m_slot)
                      .arg(group)
                      .arg(isOutput ? "output" : "input");
    
    m_serial->sendCommand(cmd);
    m_statusLabel->setText(QString("Grup %1 yönü değiştiriliyor...").arg(group));
}

void IO16Widget::onPinToggled(int group, int pin, bool value)
{
    int pinNum = group * 4 + pin;
    
    QString cmd = QString("io16:slot%1:pin%2:%3")
                      .arg(m_slot)
                      .arg(pinNum)
                      .arg(value ? "1" : "0");
    
    m_serial->sendCommand(cmd);
    m_statusLabel->setText(QString("Pin %1 = %2").arg(pinNum).arg(value));
}

void IO16Widget::handleDataReceived(const QString &data)
{
    // Parse IO16 responses
    // Example: "[ACK] IO16 Slot 0 Pin 5 = 1"
    // Example: "[ACK] IO16 Slot 0 Grup 1 Direction = Output"
    
    if (!data.contains(QString("Slot %1").arg(m_slot)))
        return;
    
    // Parse pin value
    QRegularExpression pinRegex(R"(Pin\s+(\d+)\s+=\s+(\d+))");
    QRegularExpressionMatch pinMatch = pinRegex.match(data);
    
    if (pinMatch.hasMatch()) {
        int pinNum = pinMatch.captured(1).toInt();
        bool value = pinMatch.captured(2).toInt() == 1;
        
        int group = pinNum / 4;
        int pin = pinNum % 4;
        
        if (group >= 0 && group < 4) {
            m_groups[group]->setPinValue(pin, value);
            m_groups[group]->setPinAck(pin, data);
            m_statusLabel->setText(QString("Pin %1 güncellendi").arg(pinNum));
        }
        return;
    }
    
    // Parse direction
    QRegularExpression dirRegex(R"(Grup\s+(\d+)\s+Direction\s+=\s+(\w+))");
    QRegularExpressionMatch dirMatch = dirRegex.match(data);
    
    if (dirMatch.hasMatch()) {
        int group = dirMatch.captured(1).toInt();
        QString direction = dirMatch.captured(2);
        bool isOutput = direction.contains("output", Qt::CaseInsensitive);
        
        if (group >= 0 && group < 4) {
            m_groups[group]->setDirection(isOutput);
            m_statusLabel->setText(QString("Grup %1 yönü: %2").arg(group).arg(direction));
        }
        return;
    }
}

void IO16Widget::requestGroupState(int group)
{
    QString cmd = QString("io16:slot%1:grup%2:oku").arg(m_slot).arg(group);
    m_serial->sendCommand(cmd);
}

void IO16Widget::requestAllStates()
{
    for (int group = 0; group < 4; group++) {
        requestGroupState(group);
    }
}
