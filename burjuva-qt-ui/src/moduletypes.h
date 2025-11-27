#ifndef MODULETYPES_H
#define MODULETYPES_H

#include <QString>
#include <QVector>

// Module types
enum class ModuleType {
    None,
    IO16,
    AIO20,
    Unknown
};

// Module info structure
struct ModuleInfo {
    int slot;                   // 0-3
    ModuleType type;
    QString name;               // "io16", "aio20", etc.
    QString uid;                // 1-Wire UID
    bool initialized;
    
    ModuleInfo() 
        : slot(-1)
        , type(ModuleType::None)
        , initialized(false)
    {}
};

// IO16 pin state
struct IO16PinState {
    int pin;                    // 0-15
    bool isOutput;              // true=output, false=input
    bool value;                 // HIGH/LOW
    QString lastAck;            // Last ACK message
    qint64 lastUpdateTime;      // Timestamp
    
    IO16PinState()
        : pin(0)
        , isOutput(false)
        , value(false)
        , lastUpdateTime(0)
    {}
};

// IO16 group state (4 pins per group)
struct IO16GroupState {
    int group;                  // 0-3
    bool isOutput;              // Direction for this group
    IO16PinState pins[4];       // 4 pins in this group
    
    IO16GroupState()
        : group(0)
        , isOutput(false)
    {}
};

// AIO20 channel state
struct AIO20ChannelState {
    int channel;                // 0-19
    bool isOutput;              // true=output (12-19), false=input (0-11)
    float value;                // Voltage/current value
    int rawValue;               // ADC/DAC raw (12-bit)
    QString mode;               // "0-10V", "±10V", "4-20mA"
    QString lastAck;            // Last ACK message
    qint64 lastUpdateTime;      // Timestamp
    
    AIO20ChannelState()
        : channel(0)
        , isOutput(false)
        , value(0.0f)
        , rawValue(0)
        , lastUpdateTime(0)
    {}
};

// Module state container
struct IO16State {
    int slot;
    IO16GroupState groups[4];   // 4 groups
    
    IO16State() : slot(-1) {}
};

struct AIO20State {
    int slot;
    AIO20ChannelState inputs[12];   // AI0-AI11
    AIO20ChannelState outputs[8];   // AO0-AO7
    
    AIO20State() : slot(-1) {}
};

// Utility functions
inline QString moduleTypeToString(ModuleType type) {
    switch (type) {
        case ModuleType::IO16: return "IO16";
        case ModuleType::AIO20: return "AIO20";
        case ModuleType::None: return "Empty";
        default: return "Unknown";
    }
}

inline ModuleType stringToModuleType(const QString& str) {
    QString lower = str.toLower();
    if (lower.contains("io16")) return ModuleType::IO16;
    if (lower.contains("aio20")) return ModuleType::AIO20;
    return ModuleType::None;
}

#endif // MODULETYPES_H
