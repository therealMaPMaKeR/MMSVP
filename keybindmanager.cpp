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
    // Check if action is editable
    if (!isActionEditable(action)) {
        qWarning() << "KeybindManager: Action is not editable:" << actionToString(action);
        return false;
    }
    
    // StateKeys action can have up to 12 keybinds, others can have at most 2
    int maxKeybinds = (action == Action::StateKeys) ? 12 : 2;
    if (keybinds.size() > maxKeybinds) {
        qWarning() << "KeybindManager: Cannot set more than" << maxKeybinds << "keybinds for action" << actionToString(action);
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
    QSet<QKeySequence> uniqueKeys;
    for (const QKeySequence& keySeq : keybinds) {
        if (!keySeq.isEmpty() && uniqueKeys.contains(keySeq)) {
            qWarning() << "KeybindManager: Cannot assign the same keybind twice to one action";
            return false;
        }
        if (!keySeq.isEmpty()) {
            uniqueKeys.insert(keySeq);
        }
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
        case Action::SaveState:
            return "Save State (Ctrl+Num)";
        case Action::SetLoopEnd:
            return "Set Loop End (Alt+Num)";
        case Action::DeleteState:
            return "Delete State (Shift+Num)";
        case Action::ToggleLoadSpeed:
            return "Toggle Load Speed";
        case Action::CycleLoopMode:
            return "Cycle Loop Mode";
        case Action::StateKeys:
            return "State Keys (1-12)";
        case Action::StateGroup1:
            return "State Group 1";
        case Action::StateGroup2:
            return "State Group 2";
        case Action::StateGroup3:
            return "State Group 3";
        case Action::StateGroup4:
            return "State Group 4";
        default:
            return "Unknown";
    }
}

bool KeybindManager::isActionEditable(Action action)
{
    // SaveState, SetLoopEnd, and DeleteState are display-only
    return action != Action::SaveState &&
           action != Action::SetLoopEnd &&
           action != Action::DeleteState;
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
        case Action::SaveState:
            defaults << QKeySequence(Qt::CTRL | Qt::Key_1);  // Example, user uses Ctrl+any number
            break;
        case Action::SetLoopEnd:
            defaults << QKeySequence(Qt::ALT | Qt::Key_1);   // Example, user uses Alt+any number
            break;
        case Action::DeleteState:
            defaults << QKeySequence(Qt::SHIFT | Qt::Key_1); // Example, user uses Shift+any number
            break;
        case Action::ToggleLoadSpeed:
            defaults << QKeySequence(Qt::Key_F5);
            break;
        case Action::CycleLoopMode:
            defaults << QKeySequence(Qt::Key_F9);
            break;
        case Action::StateKeys:
            // Default 12 state keys: 1,2,3,4,5,6,7,8,9,0,-,=
            defaults << QKeySequence(Qt::Key_1)
                    << QKeySequence(Qt::Key_2)
                    << QKeySequence(Qt::Key_3)
                    << QKeySequence(Qt::Key_4)
                    << QKeySequence(Qt::Key_5)
                    << QKeySequence(Qt::Key_6)
                    << QKeySequence(Qt::Key_7)
                    << QKeySequence(Qt::Key_8)
                    << QKeySequence(Qt::Key_9)
                    << QKeySequence(Qt::Key_0)
                    << QKeySequence(Qt::Key_Minus)
                    << QKeySequence(Qt::Key_Equal);
            break;
        case Action::StateGroup1:
            defaults << QKeySequence(Qt::Key_F1);
            break;
        case Action::StateGroup2:
            defaults << QKeySequence(Qt::Key_F2);
            break;
        case Action::StateGroup3:
            defaults << QKeySequence(Qt::Key_F3);
            break;
        case Action::StateGroup4:
            defaults << QKeySequence(Qt::Key_F4);
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
    m_keybinds[Action::SaveState] = getDefaultKeybinds(Action::SaveState);
    m_keybinds[Action::SetLoopEnd] = getDefaultKeybinds(Action::SetLoopEnd);
    m_keybinds[Action::DeleteState] = getDefaultKeybinds(Action::DeleteState);
    m_keybinds[Action::ToggleLoadSpeed] = getDefaultKeybinds(Action::ToggleLoadSpeed);
    m_keybinds[Action::CycleLoopMode] = getDefaultKeybinds(Action::CycleLoopMode);
    m_keybinds[Action::StateKeys] = getDefaultKeybinds(Action::StateKeys);
    m_keybinds[Action::StateGroup1] = getDefaultKeybinds(Action::StateGroup1);
    m_keybinds[Action::StateGroup2] = getDefaultKeybinds(Action::StateGroup2);
    m_keybinds[Action::StateGroup3] = getDefaultKeybinds(Action::StateGroup3);
    m_keybinds[Action::StateGroup4] = getDefaultKeybinds(Action::StateGroup4);
    
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
        Action::SaveState,
        Action::SetLoopEnd,
        Action::DeleteState,
        Action::ToggleLoadSpeed,
        Action::CycleLoopMode,
        Action::StateKeys
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
    actionMap["SaveState(Ctrl+Num)"] = Action::SaveState;
    actionMap["SetLoopEnd(Alt+Num)"] = Action::SetLoopEnd;
    actionMap["DeleteState(Shift+Num)"] = Action::DeleteState;
    actionMap["ToggleLoadSpeed"] = Action::ToggleLoadSpeed;
    actionMap["CycleLoopMode"] = Action::CycleLoopMode;
    actionMap["StateKeys(1-12)"] = Action::StateKeys;
    actionMap["StateGroup1"] = Action::StateGroup1;
    actionMap["StateGroup2"] = Action::StateGroup2;
    actionMap["StateGroup3"] = Action::StateGroup3;
    actionMap["StateGroup4"] = Action::StateGroup4;
    
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
    if (m_keybinds.size() != 18) {
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
