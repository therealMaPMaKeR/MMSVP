#include "keybindmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

KeybindManager::KeybindManager(QObject *parent)
    : QObject(parent)
{
}

KeybindManager::~KeybindManager()
{
}

bool KeybindManager::initialize()
{
    qDebug() << "KeybindManager: Initializing";
    
    QString filePath = getKeybindsFilePath();
    QFile file(filePath);
    
    // Check if keybinds file exists
    if (!file.exists()) {
        qDebug() << "KeybindManager: Keybinds file doesn't exist, creating default";
        return createDefaultKeybindsFile();
    }
    
    // Try to load keybinds
    if (!loadKeybinds()) {
        qWarning() << "KeybindManager: Failed to load keybinds, recreating with defaults";
        
        // Delete the corrupted file
        if (file.exists() && !file.remove()) {
            qWarning() << "KeybindManager: Failed to delete corrupted keybinds file";
        }
        
        // Create new default file
        return createDefaultKeybindsFile();
    }
    
    qDebug() << "KeybindManager: Initialization successful";
    return true;
}

QList<QKeySequence> KeybindManager::getKeybinds(Action action) const
{
    return m_keybinds.value(action, QList<QKeySequence>());
}

bool KeybindManager::setKeybinds(Action action, const QList<QKeySequence>& keybinds)
{
    // Validate that we have at most 2 keybinds
    if (keybinds.size() > 2) {
        qWarning() << "KeybindManager: Cannot set more than 2 keybinds per action";
        return false;
    }
    
    // Validate each keybind
    for (const QKeySequence& keySeq : keybinds) {
        if (!isValidKeybind(keySeq)) {
            qWarning() << "KeybindManager: Invalid keybind:" << keySeq.toString();
            return false;
        }
        
        // Check if keybind is already in use by another action
        if (isKeybindInUse(keySeq, action)) {
            qWarning() << "KeybindManager: Keybind already in use:" << keySeq.toString();
            return false;
        }
    }
    
    // Check for duplicates within the same action
    if (keybinds.size() == 2 && keybinds[0] == keybinds[1]) {
        qWarning() << "KeybindManager: Cannot assign the same keybind twice to one action";
        return false;
    }
    
    // Set the keybinds
    m_keybinds[action] = keybinds;
    emit keybindsChanged();
    
    qDebug() << "KeybindManager: Set keybinds for action" << actionToString(action);
    return true;
}

bool KeybindManager::isValidKeybind(const QKeySequence& keySequence) const
{
    if (keySequence.isEmpty()) {
        return false;
    }
    
    return isKeySequenceValid(keySequence);
}

bool KeybindManager::isKeybindInUse(const QKeySequence& keySequence, Action excludeAction) const
{
    for (auto it = m_keybinds.constBegin(); it != m_keybinds.constEnd(); ++it) {
        // Skip the excluded action
        if (it.key() == excludeAction) {
            continue;
        }
        
        // Check if this action uses the keybind
        if (it.value().contains(keySequence)) {
            return true;
        }
    }
    
    return false;
}

KeybindManager::Action KeybindManager::findActionForKey(const QKeySequence& keySequence) const
{
    for (auto it = m_keybinds.constBegin(); it != m_keybinds.constEnd(); ++it) {
        if (it.value().contains(keySequence)) {
            return it.key();
        }
    }
    
    // Return a default action if not found (this shouldn't happen in normal use)
    return Action::PlayPause;
}

QString KeybindManager::actionToString(Action action)
{
    switch (action) {
        case Action::PlayPause:
            return "Play/Pause";
        case Action::Stop:
            return "Stop";
        case Action::SeekForward:
            return "Seek Forward";
        case Action::SeekBackward:
            return "Seek Backward";
        case Action::VolumeUp:
            return "Volume Up";
        case Action::VolumeDown:
            return "Volume Down";
        case Action::SpeedUp:
            return "Speed Up";
        case Action::SpeedDown:
            return "Speed Down";
        case Action::SaveState1:
            return "Save State 1";
        case Action::SaveState2:
            return "Save State 2";
        case Action::SaveState3:
            return "Save State 3";
        case Action::SaveState4:
            return "Save State 4";
        case Action::SaveState5:
            return "Save State 5";
        case Action::SaveState6:
            return "Save State 6";
        case Action::SaveState7:
            return "Save State 7";
        case Action::SaveState8:
            return "Save State 8";
        case Action::LoadState1:
            return "Load State 1";
        case Action::LoadState2:
            return "Load State 2";
        case Action::LoadState3:
            return "Load State 3";
        case Action::LoadState4:
            return "Load State 4";
        case Action::LoadState5:
            return "Load State 5";
        case Action::LoadState6:
            return "Load State 6";
        case Action::LoadState7:
            return "Load State 7";
        case Action::LoadState8:
            return "Load State 8";
        default:
            return "Unknown";
    }
}

QList<QKeySequence> KeybindManager::getDefaultKeybinds(Action action)
{
    QList<QKeySequence> defaults;
    
    switch (action) {
        case Action::PlayPause:
            defaults << QKeySequence(Qt::Key_Space);
            break;
        case Action::Stop:
            // No default keybind for stop
            break;
        case Action::SeekForward:
            defaults << QKeySequence(Qt::Key_Right);
            break;
        case Action::SeekBackward:
            defaults << QKeySequence(Qt::Key_Left);
            break;
        case Action::VolumeUp:
            defaults << QKeySequence(Qt::Key_Up);
            break;
        case Action::VolumeDown:
            defaults << QKeySequence(Qt::Key_Down);
            break;
        case Action::SpeedUp:
            defaults << QKeySequence(Qt::CTRL | Qt::Key_Right);
            break;
        case Action::SpeedDown:
            defaults << QKeySequence(Qt::CTRL | Qt::Key_Left);
            break;
        case Action::SaveState1:
            defaults << QKeySequence(Qt::CTRL | Qt::Key_F1);
            break;
        case Action::SaveState2:
            defaults << QKeySequence(Qt::CTRL | Qt::Key_F2);
            break;
        case Action::SaveState3:
            defaults << QKeySequence(Qt::CTRL | Qt::Key_F3);
            break;
        case Action::SaveState4:
            defaults << QKeySequence(Qt::CTRL | Qt::Key_F4);
            break;
        case Action::SaveState5:
            defaults << QKeySequence(Qt::CTRL | Qt::Key_F5);
            break;
        case Action::SaveState6:
            defaults << QKeySequence(Qt::CTRL | Qt::Key_F6);
            break;
        case Action::SaveState7:
            defaults << QKeySequence(Qt::CTRL | Qt::Key_F7);
            break;
        case Action::SaveState8:
            defaults << QKeySequence(Qt::CTRL | Qt::Key_F8);
            break;
        case Action::LoadState1:
            defaults << QKeySequence(Qt::Key_F1);
            break;
        case Action::LoadState2:
            defaults << QKeySequence(Qt::Key_F2);
            break;
        case Action::LoadState3:
            defaults << QKeySequence(Qt::Key_F3);
            break;
        case Action::LoadState4:
            defaults << QKeySequence(Qt::Key_F4);
            break;
        case Action::LoadState5:
            defaults << QKeySequence(Qt::Key_F5);
            break;
        case Action::LoadState6:
            defaults << QKeySequence(Qt::Key_F6);
            break;
        case Action::LoadState7:
            defaults << QKeySequence(Qt::Key_F7);
            break;
        case Action::LoadState8:
            defaults << QKeySequence(Qt::Key_F8);
            break;
    }
    
    return defaults;
}

void KeybindManager::resetToDefaults()
{
    qDebug() << "KeybindManager: Resetting to defaults";
    
    m_keybinds.clear();
    
    // Set default keybinds for all actions
    m_keybinds[Action::PlayPause] = getDefaultKeybinds(Action::PlayPause);
    m_keybinds[Action::Stop] = getDefaultKeybinds(Action::Stop);
    m_keybinds[Action::SeekForward] = getDefaultKeybinds(Action::SeekForward);
    m_keybinds[Action::SeekBackward] = getDefaultKeybinds(Action::SeekBackward);
    m_keybinds[Action::VolumeUp] = getDefaultKeybinds(Action::VolumeUp);
    m_keybinds[Action::VolumeDown] = getDefaultKeybinds(Action::VolumeDown);
    m_keybinds[Action::SpeedUp] = getDefaultKeybinds(Action::SpeedUp);
    m_keybinds[Action::SpeedDown] = getDefaultKeybinds(Action::SpeedDown);
    m_keybinds[Action::SaveState1] = getDefaultKeybinds(Action::SaveState1);
    m_keybinds[Action::SaveState2] = getDefaultKeybinds(Action::SaveState2);
    m_keybinds[Action::SaveState3] = getDefaultKeybinds(Action::SaveState3);
    m_keybinds[Action::SaveState4] = getDefaultKeybinds(Action::SaveState4);
    m_keybinds[Action::SaveState5] = getDefaultKeybinds(Action::SaveState5);
    m_keybinds[Action::SaveState6] = getDefaultKeybinds(Action::SaveState6);
    m_keybinds[Action::SaveState7] = getDefaultKeybinds(Action::SaveState7);
    m_keybinds[Action::SaveState8] = getDefaultKeybinds(Action::SaveState8);
    m_keybinds[Action::LoadState1] = getDefaultKeybinds(Action::LoadState1);
    m_keybinds[Action::LoadState2] = getDefaultKeybinds(Action::LoadState2);
    m_keybinds[Action::LoadState3] = getDefaultKeybinds(Action::LoadState3);
    m_keybinds[Action::LoadState4] = getDefaultKeybinds(Action::LoadState4);
    m_keybinds[Action::LoadState5] = getDefaultKeybinds(Action::LoadState5);
    m_keybinds[Action::LoadState6] = getDefaultKeybinds(Action::LoadState6);
    m_keybinds[Action::LoadState7] = getDefaultKeybinds(Action::LoadState7);
    m_keybinds[Action::LoadState8] = getDefaultKeybinds(Action::LoadState8);
    
    emit keybindsChanged();
}

bool KeybindManager::saveKeybinds()
{
    QString filePath = getKeybindsFilePath();
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "KeybindManager: Failed to open keybinds file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    // Write header
    out << "# Video Player Keybinds Configuration\n";
    out << "# Format: ActionName=Key1,Key2\n";
    out << "# Each action can have up to 2 keybinds separated by comma\n";
    out << "# Use modifier keys as combos: Ctrl+Key, Alt+Key, Shift+Key\n";
    out << "\n";
    
    // Write each action's keybinds
    QList<Action> actions = {
        Action::PlayPause,
        Action::Stop,
        Action::SeekForward,
        Action::SeekBackward,
        Action::VolumeUp,
        Action::VolumeDown,
        Action::SpeedUp,
        Action::SpeedDown,
        Action::SaveState1,
        Action::SaveState2,
        Action::SaveState3,
        Action::SaveState4,
        Action::SaveState5,
        Action::SaveState6,
        Action::SaveState7,
        Action::SaveState8,
        Action::LoadState1,
        Action::LoadState2,
        Action::LoadState3,
        Action::LoadState4,
        Action::LoadState5,
        Action::LoadState6,
        Action::LoadState7,
        Action::LoadState8
    };
    
    for (Action action : actions) {
        QString actionName = actionToString(action).replace("/", "").replace(" ", "");
        QList<QKeySequence> keybinds = getKeybinds(action);
        
        out << actionName << "=";
        
        QStringList keybindStrings;
        for (const QKeySequence& keySeq : keybinds) {
            keybindStrings << keySequenceToString(keySeq);
        }
        
        out << keybindStrings.join(",") << "\n";
    }
    
    file.close();
    qDebug() << "KeybindManager: Saved keybinds to" << filePath;
    return true;
}

bool KeybindManager::loadKeybinds()
{
    QString filePath = getKeybindsFilePath();
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "KeybindManager: Failed to open keybinds file for reading:" << filePath;
        return false;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    
    m_keybinds.clear();
    
    // Map action names to Action enum
    QMap<QString, Action> actionMap;
    actionMap["PlayPause"] = Action::PlayPause;
    actionMap["Stop"] = Action::Stop;
    actionMap["SeekForward"] = Action::SeekForward;
    actionMap["SeekBackward"] = Action::SeekBackward;
    actionMap["VolumeUp"] = Action::VolumeUp;
    actionMap["VolumeDown"] = Action::VolumeDown;
    actionMap["SpeedUp"] = Action::SpeedUp;
    actionMap["SpeedDown"] = Action::SpeedDown;
    actionMap["SaveState1"] = Action::SaveState1;
    actionMap["SaveState2"] = Action::SaveState2;
    actionMap["SaveState3"] = Action::SaveState3;
    actionMap["SaveState4"] = Action::SaveState4;
    actionMap["SaveState5"] = Action::SaveState5;
    actionMap["SaveState6"] = Action::SaveState6;
    actionMap["SaveState7"] = Action::SaveState7;
    actionMap["SaveState8"] = Action::SaveState8;
    actionMap["LoadState1"] = Action::LoadState1;
    actionMap["LoadState2"] = Action::LoadState2;
    actionMap["LoadState3"] = Action::LoadState3;
    actionMap["LoadState4"] = Action::LoadState4;
    actionMap["LoadState5"] = Action::LoadState5;
    actionMap["LoadState6"] = Action::LoadState6;
    actionMap["LoadState7"] = Action::LoadState7;
    actionMap["LoadState8"] = Action::LoadState8;
    
    int lineNumber = 0;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        lineNumber++;
        
        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith("#")) {
            continue;
        }
        
        // Parse line: ActionName=Key1,Key2
        QStringList parts = line.split("=");
        if (parts.size() != 2) {
            qWarning() << "KeybindManager: Invalid line format at line" << lineNumber << ":" << line;
            return false;
        }
        
        QString actionName = parts[0].trimmed();
        QString keybindsStr = parts[1].trimmed();
        
        // Find the action
        if (!actionMap.contains(actionName)) {
            qWarning() << "KeybindManager: Unknown action at line" << lineNumber << ":" << actionName;
            return false;
        }
        
        Action action = actionMap[actionName];
        
        // Parse keybinds
        QList<QKeySequence> keybinds;
        if (!keybindsStr.isEmpty()) {
            QStringList keybindList = keybindsStr.split(",");
            
            for (const QString& keyStr : keybindList) {
                QString trimmedKey = keyStr.trimmed();
                if (!trimmedKey.isEmpty()) {
                    QKeySequence keySeq = parseKeySequence(trimmedKey);
                    if (!keySeq.isEmpty()) {
                        keybinds << keySeq;
                    } else {
                        qWarning() << "KeybindManager: Failed to parse keybind:" << trimmedKey;
                        return false;
                    }
                }
            }
        }
        
        m_keybinds[action] = keybinds;
    }
    
    file.close();
    
    // Verify that all actions have been loaded
    if (m_keybinds.size() != 24) {
        qWarning() << "KeybindManager: Not all actions were loaded from file";
        return false;
    }
    
    qDebug() << "KeybindManager: Loaded keybinds from" << filePath;
    return true;
}

bool KeybindManager::createDefaultKeybindsFile()
{
    resetToDefaults();
    return saveKeybinds();
}

QString KeybindManager::getKeybindsFilePath() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    return QDir::cleanPath(appDir + "/keybinds.txt");
}

bool KeybindManager::isKeySequenceValid(const QKeySequence& keySequence) const
{
    if (keySequence.isEmpty() || keySequence.count() == 0) {
        return false;
    }
    
    // Get the key combination
    QKeyCombination keyCombination = keySequence[0];
    Qt::Key key = keyCombination.key();
    Qt::KeyboardModifiers modifiers = keyCombination.keyboardModifiers();
    
    // Forbidden keys that cannot be bound at all
    if (key == Qt::Key_Escape || key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Delete) {
        return false;
    }
    
    // Modifier-only keys must be used as combos
    if (key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Shift ||
        key == Qt::Key_Meta || key == Qt::Key_AltGr) {
        return false;
    }
    
    // If the key is Ctrl, Alt, or Shift without another key, it's invalid
    // This is already handled above, but we also need to check that these
    // modifiers are only used WITH another key
    bool hasModifier = (modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier)) != 0;
    bool isModifierKey = (key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Shift);
    
    // If this is trying to bind ONLY a modifier key, reject it
    if (isModifierKey && !hasModifier) {
        return false;
    }
    
    return true;
}

QKeySequence KeybindManager::parseKeySequence(const QString& str) const
{
    if (str.isEmpty()) {
        return QKeySequence();
    }
    
    QKeySequence keySeq = QKeySequence::fromString(str, QKeySequence::PortableText);
    
    // Validate the parsed sequence
    if (!isKeySequenceValid(keySeq)) {
        return QKeySequence();
    }
    
    return keySeq;
}

QString KeybindManager::keySequenceToString(const QKeySequence& keySequence) const
{
    return keySequence.toString(QKeySequence::PortableText);
}
