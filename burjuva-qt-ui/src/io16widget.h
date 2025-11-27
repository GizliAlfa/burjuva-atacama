#ifndef IO16WIDGET_H
#define IO16WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include "moduletypes.h"

class SerialController;
class IO16Group;

class IO16Widget : public QWidget
{
    Q_OBJECT
    
public:
    explicit IO16Widget(int slot, SerialController *serial, QWidget *parent = nullptr);
    
    void updateState(const IO16State &state);
    
private slots:
    void onDirectionChanged(int group, bool isOutput);
    void onPinToggled(int group, int pin, bool value);
    void handleDataReceived(const QString &data);
    
private:
    void setupUI();
    void requestGroupState(int group);
    void requestAllStates();
    
    int m_slot;
    SerialController *m_serial;
    
    IO16Group *m_groups[4];
    QLabel *m_statusLabel;
    
    IO16State m_state;
};

#endif // IO16WIDGET_H
