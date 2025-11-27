#ifndef AIO20CHANNEL_H
#define AIO20CHANNEL_H

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QPushButton>

class AIO20Channel : public QWidget
{
    Q_OBJECT
    
public:
    explicit AIO20Channel(int channel, bool isOutput, QWidget *parent = nullptr);
    
    void setValue(float value);
    void setAck(const QString &ack);
    void setMode(const QString &mode);
    
    int getChannel() const { return m_channel; }
    float getValue() const { return m_value; }
    bool isOutput() const { return m_isOutput; }
    
signals:
    void valueChanged(int channel, float value);
    
private:
    void setupUI();
    void updateDisplay();
    
    int m_channel;
    bool m_isOutput;
    float m_value;
    QString m_mode;
    
    QLabel *m_channelLabel;
    QLabel *m_valueLabel;
    QLabel *m_modeLabel;
    QLabel *m_ackLabel;
    
    // Output controls
    QDoubleSpinBox *m_valueSpinBox;
    QSlider *m_slider;
    QPushButton *m_applyBtn;
};

#endif // AIO20CHANNEL_H
