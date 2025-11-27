#include "mainwindow.h"
#include "serialcontroller.h"
#include "moduledetector.h"
#include "io16widget.h"
#include "aio20widget.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSerialPortInfo>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_serial(new SerialController(this))
    , m_detector(new ModuleDetector(m_serial, this))
    , m_currentSlot(-1)
{
    setupUI();
    createMenuBar();
    createToolBar();
    createStatusBar();
    
    // Connect signals
    connect(m_detector, &ModuleDetector::detectionCompleted,
            this, &MainWindow::onDetectionCompleted);
    
    connect(m_serial, &SerialController::connected, this, [this]() {
        m_statusLabel->setText("Bağlantı başarılı");
        m_connectionLabel->setText("Bağlı: " + m_serial->portName());
        m_connectBtn->setEnabled(false);
        m_disconnectBtn->setEnabled(true);
        m_portCombo->setEnabled(false);
        
        // Auto-detect modules
        m_detector->startDetection();
    });
    
    connect(m_serial, &SerialController::disconnected, this, [this]() {
        m_statusLabel->setText("Bağlantı kesildi");
        m_connectionLabel->setText("Bağlı değil");
        m_connectBtn->setEnabled(true);
        m_disconnectBtn->setEnabled(false);
        m_portCombo->setEnabled(true);
    });
    
    // Populate serial ports
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        m_portCombo->addItem(port.portName() + " - " + port.description(), port.portName());
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("Burjuva-Atacama Kontrol Paneli");
    resize(1200, 800);
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Top section: Connection controls + Slot display
    QHBoxLayout *topLayout = new QHBoxLayout();
    
    // Connection controls (left)
    QGroupBox *connectionGroup = new QGroupBox("Bağlantı", this);
    QHBoxLayout *connectionLayout = new QHBoxLayout(connectionGroup);
    
    m_portCombo = new QComboBox(this);
    m_connectBtn = new QPushButton("Bağlan", this);
    m_disconnectBtn = new QPushButton("Bağlantıyı Kes", this);
    m_disconnectBtn->setEnabled(false);
    
    connectionLayout->addWidget(new QLabel("Port:", this));
    connectionLayout->addWidget(m_portCombo);
    connectionLayout->addWidget(m_connectBtn);
    connectionLayout->addWidget(m_disconnectBtn);
    
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    
    topLayout->addWidget(connectionGroup);
    
    // Cycle time control
    QGroupBox *cycleGroup = new QGroupBox("Güncelleme Hızı", this);
    QHBoxLayout *cycleLayout = new QHBoxLayout(cycleGroup);
    
    m_cycleTimeSpinBox = new QSpinBox(this);
    m_cycleTimeSpinBox->setRange(10, 5000);
    m_cycleTimeSpinBox->setValue(100);
    m_cycleTimeSpinBox->setSuffix(" ms");
    
    cycleLayout->addWidget(new QLabel("Cycle Time:", this));
    cycleLayout->addWidget(m_cycleTimeSpinBox);
    
    connect(m_cycleTimeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onCycleTimeChanged);
    
    topLayout->addWidget(cycleGroup);
    
    // Slot display (right)
    QGroupBox *slotGroup = new QGroupBox("Algılanan Modüller", this);
    QHBoxLayout *slotLayout = new QHBoxLayout(slotGroup);
    
    for (int i = 0; i < 4; i++) {
        QVBoxLayout *slotVLayout = new QVBoxLayout();
        
        m_slotLabels[i] = new QLabel("Slot " + QString::number(i), this);
        m_slotLabels[i]->setAlignment(Qt::AlignCenter);
        
        m_slotButtons[i] = new QPushButton("Empty", this);
        m_slotButtons[i]->setMinimumWidth(120);
        m_slotButtons[i]->setEnabled(false);
        
        connect(m_slotButtons[i], &QPushButton::clicked, this, [this, i]() {
            onSlotClicked(i);
        });
        
        slotVLayout->addWidget(m_slotLabels[i]);
        slotVLayout->addWidget(m_slotButtons[i]);
        
        slotLayout->addLayout(slotVLayout);
    }
    
    topLayout->addWidget(slotGroup);
    mainLayout->addLayout(topLayout);
    
    // Module content area (stacked widget)
    m_stackedWidget = new QStackedWidget(this);
    
    m_emptyLabel = new QLabel("Modül algılanmadı veya seçilmedi", this);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    QFont emptyFont = m_emptyLabel->font();
    emptyFont.setPointSize(16);
    m_emptyLabel->setFont(emptyFont);
    
    m_stackedWidget->addWidget(m_emptyLabel);
    
    mainLayout->addWidget(m_stackedWidget, 1);
}

void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    QMenu *fileMenu = menuBar->addMenu("Dosya");
    fileMenu->addAction("Çıkış", this, &QWidget::close);
    
    QMenu *toolsMenu = menuBar->addMenu("Araçlar");
    toolsMenu->addAction("Modülleri Yeniden Algıla", this, [this]() {
        if (m_serial->isConnected()) {
            m_detector->startDetection();
        }
    });
}

void MainWindow::createToolBar()
{
    QToolBar *toolbar = new QToolBar(this);
    addToolBar(toolbar);
    
    toolbar->addAction("Bağlan", this, &MainWindow::onConnectClicked);
    toolbar->addAction("Kes", this, &MainWindow::onDisconnectClicked);
    toolbar->addSeparator();
    toolbar->addAction("Yenile", this, [this]() {
        if (m_serial->isConnected()) {
            m_detector->startDetection();
        }
    });
}

void MainWindow::createStatusBar()
{
    QStatusBar *status = statusBar();
    
    m_statusLabel = new QLabel("Bağlı değil", this);
    m_connectionLabel = new QLabel("Port: -", this);
    
    status->addWidget(m_statusLabel, 1);
    status->addPermanentWidget(m_connectionLabel);
}

void MainWindow::onConnectClicked()
{
    QString portName = m_portCombo->currentData().toString();
    
    if (portName.isEmpty()) {
        QMessageBox::warning(this, "Hata", "Lütfen bir port seçin");
        return;
    }
    
    if (!m_serial->connectToPort(portName, 115200)) {
        QMessageBox::critical(this, "Bağlantı Hatası", "Port açılamadı: " + portName);
    }
}

void MainWindow::onDisconnectClicked()
{
    m_serial->disconnectFromPort();
}

void MainWindow::onDetectionCompleted(const QList<ModuleInfo> &modules)
{
    m_modules = modules;
    updateSlotDisplay();
    
    m_statusLabel->setText(QString("Algılama tamamlandı: %1 modül bulundu").arg(modules.size()));
}

void MainWindow::onCycleTimeChanged(int value)
{
    m_serial->setCycleTime(value);
}

void MainWindow::onSlotClicked(int slot)
{
    switchToModule(slot);
}

void MainWindow::updateSlotDisplay()
{
    for (int i = 0; i < 4; i++) {
        ModuleInfo module = m_detector->getModuleAtSlot(i);
        
        m_slotButtons[i]->setText(module.name);
        
        if (module.type != ModuleType::None) {
            m_slotButtons[i]->setEnabled(true);
            m_slotButtons[i]->setStyleSheet("background-color: #90EE90;");
        } else {
            m_slotButtons[i]->setEnabled(false);
            m_slotButtons[i]->setStyleSheet("");
        }
    }
}

void MainWindow::switchToModule(int slot)
{
    if (slot == m_currentSlot)
        return;
    
    m_currentSlot = slot;
    
    ModuleInfo module = m_detector->getModuleAtSlot(slot);
    
    if (module.type == ModuleType::None) {
        m_stackedWidget->setCurrentWidget(m_emptyLabel);
        return;
    }
    
    // Create widget if not exists
    if (module.type == ModuleType::IO16) {
        if (!m_io16Widgets.contains(slot)) {
            IO16Widget *widget = new IO16Widget(slot, m_serial, this);
            m_io16Widgets[slot] = widget;
            m_stackedWidget->addWidget(widget);
        }
        m_stackedWidget->setCurrentWidget(m_io16Widgets[slot]);
    } else if (module.type == ModuleType::AIO20) {
        if (!m_aio20Widgets.contains(slot)) {
            AIO20Widget *widget = new AIO20Widget(slot, m_serial, this);
            m_aio20Widgets[slot] = widget;
            m_stackedWidget->addWidget(widget);
        }
        m_stackedWidget->setCurrentWidget(m_aio20Widgets[slot]);
    }
}
