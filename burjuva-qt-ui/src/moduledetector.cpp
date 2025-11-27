#include "moduledetector.h"
#include "serialcontroller.h"
#include <QDebug>
#include <QRegularExpression>

ModuleDetector::ModuleDetector(SerialController *serial, QObject *parent)
    : QObject(parent)
    , m_serial(serial)
    , m_detectionComplete(false)
{
    connect(m_serial, &SerialController::dataReceived,
            this, &ModuleDetector::handleDataReceived);
    connect(m_serial, &SerialController::commandCompleted,
            this, &ModuleDetector::handleCommandCompleted);
}

void ModuleDetector::startDetection()
{
    if (!m_serial || !m_serial->isConnected()) {
        emit detectionFailed("Serial port not connected");
        return;
    }
    
    qDebug() << "Starting module detection...";
    
    m_modules.clear();
    m_currentData.clear();
    m_detectionComplete = false;
    
    emit detectionStarted();
    
    // Send detection command
    m_serial->sendCommandWithPriority("modul-algila");
}

ModuleInfo ModuleDetector::getModuleAtSlot(int slot) const
{
    for (const ModuleInfo &module : m_modules) {
        if (module.slot == slot) {
            return module;
        }
    }
    
    // Return empty module
    ModuleInfo empty;
    empty.slot = slot;
    empty.type = ModuleType::None;
    empty.name = "Empty";
    empty.initialized = false;
    return empty;
}

void ModuleDetector::handleDataReceived(const QString &data)
{
    if (!data.startsWith("Slot "))
        return;
    
    // Parse module line: "Slot X: MODUL_TIPI (UID: XXXXXXXX)"
    parseModuleData(data);
}

void ModuleDetector::handleCommandCompleted(const QString &command)
{
    if (command != "modul-algila")
        return;
    
    // Detection completed
    m_detectionComplete = true;
    
    qDebug() << "Module detection completed:" << m_modules.size() << "modules found";
    
    // Fill empty slots
    for (int slot = 0; slot < 4; slot++) {
        bool found = false;
        for (const ModuleInfo &module : m_modules) {
            if (module.slot == slot) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            ModuleInfo empty;
            empty.slot = slot;
            empty.type = ModuleType::None;
            empty.name = "Empty";
            empty.initialized = false;
            m_modules.append(empty);
        }
    }
    
    // Sort by slot
    std::sort(m_modules.begin(), m_modules.end(),
              [](const ModuleInfo &a, const ModuleInfo &b) {
                  return a.slot < b.slot;
              });
    
    emit detectionCompleted(m_modules);
}

void ModuleDetector::parseModuleData(const QString &data)
{
    // Example: "Slot 0: IO16_DIJITAL (UID: 5A3B1C9D)"
    // Example: "Slot 1: AIO20_ANALOG (UID: 8F2E4A6C)"
    // Example: "Slot 2: BOS"
    
    QRegularExpression slotRegex(R"(Slot\s+(\d+):\s+(\w+)(?:\s+\(UID:\s+([0-9A-F]+)\))?)");
    QRegularExpressionMatch match = slotRegex.match(data);
    
    if (!match.hasMatch()) {
        qWarning() << "Failed to parse module data:" << data;
        return;
    }
    
    int slot = match.captured(1).toInt();
    QString typeStr = match.captured(2);
    QString uid = match.captured(3);
    
    ModuleInfo module;
    module.slot = slot;
    module.type = parseModuleType(typeStr);
    module.uid = uid;
    module.initialized = true;
    
    // Set module name
    switch (module.type) {
        case ModuleType::IO16:
            module.name = "IO16 Digital";
            break;
        case ModuleType::AIO20:
            module.name = "AIO20 Analog";
            break;
        case ModuleType::None:
            module.name = "Empty";
            break;
        default:
            module.name = "Unknown";
            break;
    }
    
    // Only add non-empty modules during parsing
    if (module.type != ModuleType::None) {
        m_modules.append(module);
        qDebug() << "Detected module at slot" << slot << ":" << module.name << "UID:" << uid;
        emit moduleDetected(module);
    }
}

ModuleType ModuleDetector::parseModuleType(const QString &typeStr)
{
    QString upper = typeStr.toUpper();
    
    if (upper.contains("IO16") || upper.contains("DIJITAL")) {
        return ModuleType::IO16;
    } else if (upper.contains("AIO20") || upper.contains("ANALOG")) {
        return ModuleType::AIO20;
    } else if (upper == "BOS" || upper == "EMPTY") {
        return ModuleType::None;
    } else {
        return ModuleType::Unknown;
    }
}
