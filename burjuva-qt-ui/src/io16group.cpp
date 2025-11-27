#include "io16group.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFont>

IO16Group::IO16Group(int group, QWidget *parent)
    : QWidget(parent)
    , m_group(group)
    , m_isOutput(false)
{
    setupUI();
}

void IO16Group::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // Group box
    QGroupBox *groupBox = new QGroupBox(QString("Grup %1").arg(m_group), this);
    QVBoxLayout *boxLayout = new QVBoxLayout(groupBox);
    
    // Direction control
    QHBoxLayout *dirLayout = new QHBoxLayout();
    m_directionLabel = new QLabel("Yön:", this);
    m_directionBtn = new QPushButton("Input", this);
    m_directionBtn->setCheckable(true);
    m_directionBtn->setMinimumWidth(100);
    
    connect(m_directionBtn, &QPushButton::clicked, this, [this](bool checked) {
        setDirection(checked);
        emit directionChangeRequested(m_group, checked);
    });
    
    dirLayout->addWidget(m_directionLabel);
    dirLayout->addWidget(m_directionBtn);
    dirLayout->addStretch();
    
    boxLayout->addLayout(dirLayout);
    
    // Pin display (4 pins in a grid)
    QGridLayout *pinGrid = new QGridLayout();
    pinGrid->setSpacing(10);
    
    for (int i = 0; i < 4; i++) {
        int pinNum = m_group * 4 + i;
        
        m_pins[i].value = false;
        
        // Pin label
        m_pins[i].label = new QLabel(QString("Pin %1:").arg(pinNum), this);
        QFont labelFont = m_pins[i].label->font();
        labelFont.setBold(true);
        m_pins[i].label->setFont(labelFont);
        
        // Value display (for input mode)
        m_pins[i].valueLabel = new QLabel("0", this);
        m_pins[i].valueLabel->setAlignment(Qt::AlignCenter);
        m_pins[i].valueLabel->setMinimumWidth(60);
        m_pins[i].valueLabel->setStyleSheet("QLabel { background-color: #F0F0F0; padding: 5px; border: 1px solid #CCC; border-radius: 3px; }");
        
        // Toggle button (for output mode)
        m_pins[i].toggleBtn = new QPushButton("OFF", this);
        m_pins[i].toggleBtn->setCheckable(true);
        m_pins[i].toggleBtn->setMinimumWidth(60);
        m_pins[i].toggleBtn->hide();
        
        connect(m_pins[i].toggleBtn, &QPushButton::clicked, this, [this, i](bool checked) {
            m_pins[i].value = checked;
            m_pins[i].toggleBtn->setText(checked ? "ON" : "OFF");
            m_pins[i].toggleBtn->setStyleSheet(checked ? "background-color: #90EE90;" : "");
            emit pinToggled(m_group, i, checked);
        });
        
        // ACK display
        m_pins[i].ackLabel = new QLabel("-", this);
        m_pins[i].ackLabel->setStyleSheet("QLabel { color: #666; font-size: 9pt; }");
        
        // Add to grid
        pinGrid->addWidget(m_pins[i].label, i, 0);
        pinGrid->addWidget(m_pins[i].valueLabel, i, 1);
        pinGrid->addWidget(m_pins[i].toggleBtn, i, 1);
        pinGrid->addWidget(m_pins[i].ackLabel, i, 2);
    }
    
    boxLayout->addLayout(pinGrid);
    mainLayout->addWidget(groupBox);
}

void IO16Group::setDirection(bool isOutput)
{
    m_isOutput = isOutput;
    
    m_directionBtn->setChecked(isOutput);
    m_directionBtn->setText(isOutput ? "Output" : "Input");
    m_directionBtn->setStyleSheet(isOutput ? "background-color: #FFD700;" : "");
    
    updatePinWidgets();
}

void IO16Group::setPinValue(int pin, bool value)
{
    if (pin < 0 || pin >= 4)
        return;
    
    m_pins[pin].value = value;
    
    if (m_isOutput) {
        m_pins[pin].toggleBtn->setChecked(value);
        m_pins[pin].toggleBtn->setText(value ? "ON" : "OFF");
        m_pins[pin].toggleBtn->setStyleSheet(value ? "background-color: #90EE90;" : "");
    } else {
        m_pins[pin].valueLabel->setText(value ? "1" : "0");
        m_pins[pin].valueLabel->setStyleSheet(value ?
            "QLabel { background-color: #90EE90; padding: 5px; border: 1px solid #008000; border-radius: 3px; font-weight: bold; }" :
            "QLabel { background-color: #F0F0F0; padding: 5px; border: 1px solid #CCC; border-radius: 3px; }");
    }
}

void IO16Group::setPinAck(int pin, const QString &ack)
{
    if (pin < 0 || pin >= 4)
        return;
    
    m_pins[pin].ackLabel->setText(ack);
    
    // Color code ACK
    if (ack.contains("OK", Qt::CaseInsensitive)) {
        m_pins[pin].ackLabel->setStyleSheet("QLabel { color: green; font-size: 9pt; }");
    } else if (ack.contains("ERROR", Qt::CaseInsensitive)) {
        m_pins[pin].ackLabel->setStyleSheet("QLabel { color: red; font-size: 9pt; }");
    } else {
        m_pins[pin].ackLabel->setStyleSheet("QLabel { color: #666; font-size: 9pt; }");
    }
}

void IO16Group::updatePinWidgets()
{
    for (int i = 0; i < 4; i++) {
        if (m_isOutput) {
            m_pins[i].valueLabel->hide();
            m_pins[i].toggleBtn->show();
        } else {
            m_pins[i].toggleBtn->hide();
            m_pins[i].valueLabel->show();
        }
    }
}
