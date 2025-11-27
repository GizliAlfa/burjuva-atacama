#ifndef IO16GROUP_H
#define IO16GROUP_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

class IO16Group : public QWidget
{
    Q_OBJECT
    
public:
    explicit IO16Group(int group, QWidget *parent = nullptr);
    
    void setDirection(bool isOutput);
    void setPinValue(int pin, bool value);
    void setPinAck(int pin, const QString &ack);
    
    bool isOutput() const { return m_isOutput; }
    
signals:
    void directionChangeRequested(int group, bool isOutput);
    void pinToggled(int group, int pin, bool value);
    
private:
    void setupUI();
    void updatePinWidgets();
    
    int m_group;
    bool m_isOutput;
    
    QPushButton *m_directionBtn;
    QLabel *m_directionLabel;
    
    // Pin widgets (4 pins per group)
    struct PinWidget {
        QLabel *label;
        QLabel *valueLabel;
        QPushButton *toggleBtn;
        QLabel *ackLabel;
        bool value;
    };
    
    PinWidget m_pins[4];
};

#endif // IO16GROUP_H
