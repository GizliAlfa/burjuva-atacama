#ifndef AIO20WIDGET_H
#define AIO20WIDGET_H

#include <QWidget>
#include <QLabel>
#include "moduletypes.h"

class SerialController;
class AIO20Channel;

class AIO20Widget : public QWidget
{
    Q_OBJECT
    
public:
    explicit AIO20Widget(int slot, SerialController *serial, QWidget *parent = nullptr);
    
    void updateState(const AIO20State &state);
    
private slots:
    void onChannelValueChanged(int channel, float value);
    void handleDataReceived(const QString &data);
    
private:
    void setupUI();
    void requestChannelState(int channel);
    void requestAllStates();
    
    int m_slot;
    SerialController *m_serial;
    
    // 12 input channels (0-11)
    AIO20Channel *m_inputChannels[12];
    
    // 8 output channels (12-19)
    AIO20Channel *m_outputChannels[8];
    
    QLabel *m_statusLabel;
    
    AIO20State m_state;
};

#endif // AIO20WIDGET_H
