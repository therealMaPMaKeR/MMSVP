#ifndef KEYBINDMANAGER_H
#define KEYBINDMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QKeySequence>
#include <QList>

/**
 * @class KeybindManager
 * @brief Manages keybindings for the video player
 * 
 * Handles loading, saving, and validating keybindings from/to a file.
 * Each action can have up to 2 different keybinds assigned.
 */
class KeybindManager : public QObject
{
    Q_OBJECT

public:
    // Action identifiers
    enum class Action {
        PlayPause,
        Stop,
        SeekForward,
        SeekBackward,
        VolumeUp,
        VolumeDown,
        SpeedUp,
        SpeedDown,
        SaveState1,
        SaveState2,
        SaveState3,
        SaveState4,
        SaveState5,
        SaveState6,
        SaveState7,
        SaveState8,
        LoadState1,
        LoadState2,
        LoadState3,
        LoadState4,
        LoadState5,
        LoadState6,
        LoadState7,
        LoadState8
    };

    explicit KeybindManager(QObject *parent = nullptr);
    ~KeybindManager();

    // Initialize and load keybinds from file
    bool initialize();
    
    // Get keybinds for an action (returns up to 2 keybinds)
    QList<QKeySequence> getKeybinds(Action action) const;
    
    // Set keybinds for an action (up to 2)
    bool setKeybinds(Action action, const QList<QKeySequence>& keybinds);
    
    // Check if a key sequence is valid for binding
    bool isValidKeybind(const QKeySequence& keySequence) const;
    
    // Check if a key sequence is already bound to another action
    bool isKeybindInUse(const QKeySequence& keySequence, Action excludeAction) const;
    
    // Find which action a key sequence is bound to
    Action findActionForKey(const QKeySequence& keySequence) const;
    
    // Get action name as string
    static QString actionToString(Action action);
    
    // Get default keybinds for an action
    static QList<QKeySequence> getDefaultKeybinds(Action action);
    
    // Reset all keybinds to defaults
    void resetToDefaults();
    
    // Save keybinds to file
    bool saveKeybinds();

signals:
    void keybindsChanged();

private:
    // Load keybinds from file
    bool loadKeybinds();
    
    // Create default keybinds file
    bool createDefaultKeybindsFile();
    
    // Get the keybinds file path
    QString getKeybindsFilePath() const;
    
    // Validate a key sequence (check for forbidden keys)
    bool isKeySequenceValid(const QKeySequence& keySequence) const;
    
    // Parse a string into a QKeySequence
    QKeySequence parseKeySequence(const QString& str) const;
    
    // Convert QKeySequence to string for saving
    QString keySequenceToString(const QKeySequence& keySequence) const;

    // Storage for keybinds: Action -> List of up to 2 QKeySequences
    QMap<Action, QList<QKeySequence>> m_keybinds;
};

#endif // KEYBINDMANAGER_H
