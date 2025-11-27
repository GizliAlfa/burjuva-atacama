#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QSpinBox>
#include <QStackedWidget>
#include <QComboBox>
#include <QPushButton>
#include "moduletypes.h"

class SerialController;
class ModuleDetector;
class IO16Widget;
class AIO20Widget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onDetectionCompleted(const QList<ModuleInfo> &modules);
    void onCycleTimeChanged(int value);
    void onSlotClicked(int slot);
    
private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void updateSlotDisplay();
    void switchToModule(int slot);
    
    // Serial communication
    SerialController *m_serial;
    ModuleDetector *m_detector;
    
    // UI components
    QComboBox *m_portCombo;
    QPushButton *m_connectBtn;
    QPushButton *m_disconnectBtn;
    QSpinBox *m_cycleTimeSpinBox;
    
    // Slot display (top-right)
    QLabel *m_slotLabels[4];
    QPushButton *m_slotButtons[4];
    
    // Module widgets
    QStackedWidget *m_stackedWidget;
    QLabel *m_emptyLabel;
    
    // Status bar
    QLabel *m_statusLabel;
    QLabel *m_connectionLabel;
    
    // Module data
    QList<ModuleInfo> m_modules;
    int m_currentSlot;
    
    // Module widget instances
    QMap<int, IO16Widget*> m_io16Widgets;
    QMap<int, AIO20Widget*> m_aio20Widgets;
};

#endif // MAINWINDOW_H
