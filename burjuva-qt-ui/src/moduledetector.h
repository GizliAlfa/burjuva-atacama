#ifndef MODULEDETECTOR_H
#define MODULEDETECTOR_H

#include <QObject>
#include <QList>
#include "moduletypes.h"

class SerialController;

class ModuleDetector : public QObject
{
    Q_OBJECT
    
public:
    explicit ModuleDetector(SerialController *serial, QObject *parent = nullptr);
    
    void startDetection();
    QList<ModuleInfo> getDetectedModules() const { return m_modules; }
    ModuleInfo getModuleAtSlot(int slot) const;
    bool isDetectionComplete() const { return m_detectionComplete; }
    
signals:
    void detectionStarted();
    void moduleDetected(const ModuleInfo &module);
    void detectionCompleted(const QList<ModuleInfo> &modules);
    void detectionFailed(const QString &error);
    
private slots:
    void handleDataReceived(const QString &data);
    void handleCommandCompleted(const QString &command);
    
private:
    void parseModuleData(const QString &data);
    ModuleType parseModuleType(const QString &typeStr);
    
    SerialController *m_serial;
    QList<ModuleInfo> m_modules;
    bool m_detectionComplete;
    QString m_currentData;
};

#endif // MODULEDETECTOR_H
