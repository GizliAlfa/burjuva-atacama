#include "aio20channel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFont>

AIO20Channel::AIO20Channel(int channel, bool isOutput, QWidget *parent)
    : QWidget(parent)
    , m_channel(channel)
    , m_isOutput(isOutput)
    , m_value(0.0f)
    , m_mode("Unknown")
{
    setupUI();
}

void AIO20Channel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    QGroupBox *groupBox = new QGroupBox(
        QString("Kanal %1 (%2)").arg(m_channel).arg(m_isOutput ? "Output" : "Input"),
        this
    );
    QVBoxLayout *boxLayout = new QVBoxLayout(groupBox);
    
    // Channel info
    QHBoxLayout *infoLayout = new QHBoxLayout();
    
    m_channelLabel = new QLabel(QString("CH%1:").arg(m_channel), this);
    QFont labelFont = m_channelLabel->font();
    labelFont.setBold(true);
    m_channelLabel->setFont(labelFont);
    
    m_valueLabel = new QLabel("0.00 V", this);
    m_valueLabel->setAlignment(Qt::AlignRight);
    QFont valueFont = m_valueLabel->font();
    valueFont.setPointSize(12);
    m_valueLabel->setFont(valueFont);
    m_valueLabel->setStyleSheet("QLabel { background-color: #F0F0F0; padding: 5px; border: 1px solid #CCC; border-radius: 3px; }");
    
    infoLayout->addWidget(m_channelLabel);
    infoLayout->addWidget(m_valueLabel, 1);
    
    boxLayout->addLayout(infoLayout);
    
    // Mode display
    m_modeLabel = new QLabel("Mod: -", this);
    m_modeLabel->setStyleSheet("QLabel { color: #666; font-size: 9pt; }");
    boxLayout->addWidget(m_modeLabel);
    
    // Output controls (only for output channels)
    if (m_isOutput) {
        QHBoxLayout *controlLayout = new QHBoxLayout();
        
        m_valueSpinBox = new QDoubleSpinBox(this);
        m_valueSpinBox->setRange(0.0, 10.0);
        m_valueSpinBox->setSingleStep(0.1);
        m_valueSpinBox->setDecimals(2);
        m_valueSpinBox->setSuffix(" V");
        m_valueSpinBox->setValue(0.0);
        
        m_slider = new QSlider(Qt::Horizontal, this);
        m_slider->setRange(0, 1000);  // 0.01V precision
        m_slider->setValue(0);
        
        m_applyBtn = new QPushButton("Uygula", this);
        m_applyBtn->setMaximumWidth(80);
        
        controlLayout->addWidget(m_valueSpinBox);
        controlLayout->addWidget(m_slider, 1);
        controlLayout->addWidget(m_applyBtn);
        
        boxLayout->addLayout(controlLayout);
        
        // Connect signals
        connect(m_valueSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [this](double value) {
            m_slider->setValue(static_cast<int>(value * 100));
        });
        
        connect(m_slider, &QSlider::valueChanged, this, [this](int value) {
            m_valueSpinBox->setValue(value / 100.0);
        });
        
        connect(m_applyBtn, &QPushButton::clicked, this, [this]() {
            emit valueChanged(m_channel, m_valueSpinBox->value());
        });
    }
    
    // ACK display
    m_ackLabel = new QLabel("-", this);
    m_ackLabel->setStyleSheet("QLabel { color: #666; font-size: 8pt; font-style: italic; }");
    m_ackLabel->setWordWrap(true);
    boxLayout->addWidget(m_ackLabel);
    
    mainLayout->addWidget(groupBox);
}

void AIO20Channel::setValue(float value)
{
    m_value = value;
    updateDisplay();
}

void AIO20Channel::setAck(const QString &ack)
{
    m_ackLabel->setText(ack);
    
    // Color code ACK
    if (ack.contains("OK", Qt::CaseInsensitive)) {
        m_ackLabel->setStyleSheet("QLabel { color: green; font-size: 8pt; font-style: italic; }");
    } else if (ack.contains("ERROR", Qt::CaseInsensitive)) {
        m_ackLabel->setStyleSheet("QLabel { color: red; font-size: 8pt; font-style: italic; }");
    } else {
        m_ackLabel->setStyleSheet("QLabel { color: #666; font-size: 8pt; font-style: italic; }");
    }
}

void AIO20Channel::setMode(const QString &mode)
{
    m_mode = mode;
    m_modeLabel->setText("Mod: " + mode);
}

void AIO20Channel::updateDisplay()
{
    QString unit = " V";
    
    // Check mode for unit
    if (m_mode.contains("mA", Qt::CaseInsensitive) || 
        m_mode.contains("4-20", Qt::CaseInsensitive)) {
        unit = " mA";
    }
    
    m_valueLabel->setText(QString::number(m_value, 'f', 2) + unit);
    
    // Color coding for value display
    if (m_isOutput) {
        m_valueLabel->setStyleSheet("QLabel { background-color: #FFE4B5; padding: 5px; border: 1px solid #FFA500; border-radius: 3px; }");
    } else {
        // Input channel - color based on value
        if (m_value > 0.1) {
            m_valueLabel->setStyleSheet("QLabel { background-color: #90EE90; padding: 5px; border: 1px solid #008000; border-radius: 3px; }");
        } else {
            m_valueLabel->setStyleSheet("QLabel { background-color: #F0F0F0; padding: 5px; border: 1px solid #CCC; border-radius: 3px; }");
        }
    }
}
