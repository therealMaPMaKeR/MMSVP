#include "keybindeditordialog.h"
#include <QMessageBox>
#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPushButton>

// KeyCaptureWidget implementation
KeybindEditorDialog::KeyCaptureWidget::KeyCaptureWidget(QWidget *parent)
    : QWidget(parent)
    , m_modifiers(Qt::NoModifier)
{
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("QWidget { background-color: #e8f4f8; border: 2px solid #2196F3; padding: 5px; }");
}

void KeybindEditorDialog::KeyCaptureWidget::reset()
{
    m_capturedKey = QKeySequence();
    m_modifiers = Qt::NoModifier;
}

void KeybindEditorDialog::KeyCaptureWidget::keyPressEvent(QKeyEvent *event)
{
    // Ignore auto-repeat
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }
    
    Qt::Key key = static_cast<Qt::Key>(event->key());
    Qt::KeyboardModifiers modifiers = event->modifiers();
    
    // Handle Escape to cancel
    if (key == Qt::Key_Escape) {
        emit captureCancelled();
        event->accept();
        return;
    }
    
    // Forbidden keys
    if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Delete) {
        QMessageBox::warning(this, tr("Invalid Key"), 
                           tr("The keys Enter, Return, and Delete cannot be bound."));
        event->accept();
        return;
    }
    
    // Don't allow binding only modifier keys
    if (key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Shift ||
        key == Qt::Key_Meta || key == Qt::Key_AltGr) {
        QMessageBox::information(this, tr("Modifier Key"), 
                                tr("Modifier keys (Ctrl, Alt, Shift) can only be used in combination with other keys.\nPress a key while holding the modifier."));
        event->accept();
        return;
    }
    
    // Create key sequence with modifiers
    QKeyCombination combination(modifiers, key);
    m_capturedKey = QKeySequence(combination);
    
    qDebug() << "KeyCaptureWidget: Captured key:" << m_capturedKey.toString();
    
    emit keyCaptured(m_capturedKey);
    event->accept();
}

void KeybindEditorDialog::KeyCaptureWidget::focusOutEvent(QFocusEvent *event)
{
    emit captureCancelled();
    QWidget::focusOutEvent(event);
}

// KeybindEditorDialog implementation
KeybindEditorDialog::KeybindEditorDialog(KeybindManager* keybindManager, QWidget *parent)
    : QDialog(parent)
    , m_keybindManager(keybindManager)
    , m_tableWidget(nullptr)
    , m_resetButton(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
    , m_instructionLabel(nullptr)
    , m_editingRow(-1)
    , m_editingColumn(-1)
    , m_isEditingKeybind(false)
{
    setWindowTitle(tr("Edit Keybinds"));
    resize(600, 400);
    
    setupUI();
    populateTable();
}

KeybindEditorDialog::~KeybindEditorDialog()
{
}

void KeybindEditorDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Instruction label
    m_instructionLabel = new QLabel(tr("Click on a keybind cell to change it. Right-click to clear. Press Escape to cancel editing."), this);
    m_instructionLabel->setWordWrap(true);
    m_instructionLabel->setStyleSheet("QLabel { color: #555; font-style: italic; margin-bottom: 10px; }");
    mainLayout->addWidget(m_instructionLabel);
    
    // Table widget
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(3);
    m_tableWidget->setHorizontalHeaderLabels(QStringList() << tr("Action") << tr("Primary Keybind") << tr("Secondary Keybind"));
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    
    connect(m_tableWidget, &QTableWidget::cellClicked, this, &KeybindEditorDialog::onCellClicked);
    connect(m_tableWidget, &QTableWidget::customContextMenuRequested, this, &KeybindEditorDialog::onCellRightClicked);
    
    mainLayout->addWidget(m_tableWidget);
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_resetButton = new QPushButton(tr("Reset to Defaults"), this);
    connect(m_resetButton, &QPushButton::clicked, this, &KeybindEditorDialog::onResetToDefaultsClicked);
    buttonLayout->addWidget(m_resetButton);
    
    buttonLayout->addStretch();
    
    m_saveButton = new QPushButton(tr("Save"), this);
    m_saveButton->setDefault(true);
    connect(m_saveButton, &QPushButton::clicked, this, &KeybindEditorDialog::onSaveClicked);
    buttonLayout->addWidget(m_saveButton);
    
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, &KeybindEditorDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    setLayout(mainLayout);
}

void KeybindEditorDialog::populateTable()
{
    // List of all actions
    QList<KeybindManager::Action> actions = {
        KeybindManager::Action::PlayPause,
        KeybindManager::Action::Stop,
        KeybindManager::Action::SeekForward,
        KeybindManager::Action::SeekBackward,
        KeybindManager::Action::VolumeUp,
        KeybindManager::Action::VolumeDown
    };
    
    m_tableWidget->setRowCount(actions.size());
    
    // Load current keybinds into temporary storage
    m_tempKeybinds.clear();
    for (KeybindManager::Action action : actions) {
        m_tempKeybinds[action] = m_keybindManager->getKeybinds(action);
    }
    
    // Populate table
    for (int i = 0; i < actions.size(); ++i) {
        KeybindManager::Action action = actions[i];
        
        // Action name column (read-only)
        QTableWidgetItem* actionItem = new QTableWidgetItem(KeybindManager::actionToString(action));
        actionItem->setFlags(actionItem->flags() & ~Qt::ItemIsEditable);
        actionItem->setData(Qt::UserRole, static_cast<int>(action));
        m_tableWidget->setItem(i, 0, actionItem);
        
        // Primary keybind column
        QTableWidgetItem* primaryItem = new QTableWidgetItem();
        primaryItem->setFlags(primaryItem->flags() & ~Qt::ItemIsEditable);
        primaryItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(i, 1, primaryItem);
        
        // Secondary keybind column
        QTableWidgetItem* secondaryItem = new QTableWidgetItem();
        secondaryItem->setFlags(secondaryItem->flags() & ~Qt::ItemIsEditable);
        secondaryItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(i, 2, secondaryItem);
        
        // Update keybind display
        updateKeybindDisplay(i);
    }
}

void KeybindEditorDialog::updateKeybindDisplay(int row)
{
    QTableWidgetItem* actionItem = m_tableWidget->item(row, 0);
    if (!actionItem) return;
    
    KeybindManager::Action action = static_cast<KeybindManager::Action>(actionItem->data(Qt::UserRole).toInt());
    QList<QKeySequence> keybinds = m_tempKeybinds.value(action);
    
    // Update primary keybind
    QTableWidgetItem* primaryItem = m_tableWidget->item(row, 1);
    if (primaryItem) {
        if (keybinds.size() > 0 && !keybinds[0].isEmpty()) {
            primaryItem->setText(getKeybindDisplayText(keybinds[0]));
            primaryItem->setBackground(QBrush(QColor(240, 240, 240)));
        } else {
            primaryItem->setText(tr("[Not Set]"));
            primaryItem->setBackground(QBrush(QColor(255, 250, 240)));
        }
    }
    
    // Update secondary keybind
    QTableWidgetItem* secondaryItem = m_tableWidget->item(row, 2);
    if (secondaryItem) {
        if (keybinds.size() > 1 && !keybinds[1].isEmpty()) {
            secondaryItem->setText(getKeybindDisplayText(keybinds[1]));
            secondaryItem->setBackground(QBrush(QColor(240, 240, 240)));
        } else {
            secondaryItem->setText(tr("[Not Set]"));
            secondaryItem->setBackground(QBrush(QColor(255, 250, 240)));
        }
    }
}

void KeybindEditorDialog::onCellClicked(int row, int column)
{
    // Only allow clicking on keybind columns (1 and 2)
    if (column < 1 || column > 2) {
        return;
    }
    
    qDebug() << "KeybindEditorDialog: Cell clicked - row:" << row << "column:" << column;
    
    // Determine which keybind index (0 = primary, 1 = secondary)
    int keybindIndex = column - 1;
    
    startEditingKeybind(row, keybindIndex);
}

void KeybindEditorDialog::onCellRightClicked(const QPoint& pos)
{
    QTableWidgetItem* item = m_tableWidget->itemAt(pos);
    if (!item) return;
    
    int row = item->row();
    int column = item->column();
    
    // Only allow right-clicking on keybind columns (1 and 2)
    if (column < 1 || column > 2) {
        return;
    }
    
    qDebug() << "KeybindEditorDialog: Right-click on row:" << row << "column:" << column;
    
    // Determine which keybind index (0 = primary, 1 = secondary)
    int keybindIndex = column - 1;
    
    clearKeybind(row, keybindIndex);
}

void KeybindEditorDialog::startEditingKeybind(int row, int keybindIndex)
{
    qDebug() << "KeybindEditorDialog: Starting keybind edit for row" << row << "index" << keybindIndex;
    
    m_editingRow = row;
    m_editingColumn = keybindIndex + 1; // +1 because column 0 is action name
    m_isEditingKeybind = true;
    
    // Get the action
    QTableWidgetItem* actionItem = m_tableWidget->item(row, 0);
    if (!actionItem) return;
    
    KeybindManager::Action action = static_cast<KeybindManager::Action>(actionItem->data(Qt::UserRole).toInt());
    
    // Create key capture widget
    KeyCaptureWidget* captureWidget = new KeyCaptureWidget(this);
    captureWidget->setMinimumHeight(30);
    
    // Connect signals
    connect(captureWidget, &KeyCaptureWidget::keyCaptured, this, [this, row, keybindIndex, action, captureWidget](const QKeySequence& keySeq) {
        qDebug() << "KeybindEditorDialog: Key captured:" << keySeq.toString();
        
        // Validate the keybind
        if (!m_keybindManager->isValidKeybind(keySeq)) {
            // Clean up first
            m_tableWidget->removeCellWidget(row, keybindIndex + 1);
            captureWidget->deleteLater();
            m_isEditingKeybind = false;
            resetInstructionLabel();
            
            // Then show error
            QMessageBox::warning(this, tr("Invalid Keybind"), 
                               tr("This key combination cannot be bound."));
            return;
        }
        
        // Check if keybind is already in use
        QList<QKeySequence> currentKeybinds = m_tempKeybinds.value(action);
        
        // Check against other actions
        for (auto it = m_tempKeybinds.constBegin(); it != m_tempKeybinds.constEnd(); ++it) {
            if (it.key() == action) continue; // Skip current action
            
            if (it.value().contains(keySeq)) {
                QString actionName = KeybindManager::actionToString(it.key());
                
                // Clean up first
                m_tableWidget->removeCellWidget(row, keybindIndex + 1);
                captureWidget->deleteLater();
                m_isEditingKeybind = false;
                resetInstructionLabel();
                
                // Then show error
                QMessageBox::warning(this, tr("Keybind In Use"), 
                                   tr("This keybind is already assigned to: %1").arg(actionName));
                return;
            }
        }
        
        // Check if it's already assigned to another slot of the same action
        for (int i = 0; i < currentKeybinds.size(); ++i) {
            if (i != keybindIndex && currentKeybinds[i] == keySeq) {
                // Clean up first
                m_tableWidget->removeCellWidget(row, keybindIndex + 1);
                captureWidget->deleteLater();
                m_isEditingKeybind = false;
                resetInstructionLabel();
                
                // Then show error
                QMessageBox::warning(this, tr("Duplicate Keybind"), 
                                   tr("This keybind is already assigned to another slot of this action."));
                return;
            }
        }
        
        // Update the temporary keybind storage
        QList<QKeySequence> newKeybinds = currentKeybinds;
        
        // Ensure the list is the right size
        while (newKeybinds.size() <= keybindIndex) {
            newKeybinds.append(QKeySequence());
        }
        
        newKeybinds[keybindIndex] = keySeq;
        m_tempKeybinds[action] = newKeybinds;
        
        // Update display
        updateKeybindDisplay(row);
        
        // Remove capture widget
        m_tableWidget->removeCellWidget(row, keybindIndex + 1);
        captureWidget->deleteLater();
        m_isEditingKeybind = false;
        
        // Reset instruction label after successful capture
        resetInstructionLabel();
    });
    
    connect(captureWidget, &KeyCaptureWidget::captureCancelled, this, [this, row, keybindIndex, captureWidget]() {
        qDebug() << "KeybindEditorDialog: Key capture cancelled";
        m_tableWidget->removeCellWidget(row, keybindIndex + 1);
        captureWidget->deleteLater();
        m_isEditingKeybind = false;
        
        // Reset instruction label after cancellation
        resetInstructionLabel();
    });
    
    // Set the capture widget in the table cell
    m_tableWidget->setCellWidget(row, keybindIndex + 1, captureWidget);
    captureWidget->setFocus();
    
    // Show instruction
    m_instructionLabel->setText(tr("Press a key combination (or Escape to cancel)..."));
    m_instructionLabel->setStyleSheet("QLabel { color: #2196F3; font-weight: bold; margin-bottom: 10px; }");
}

void KeybindEditorDialog::clearKeybind(int row, int keybindIndex)
{
    qDebug() << "KeybindEditorDialog: Clearing keybind at row" << row << "index" << keybindIndex;
    
    // Get the action
    QTableWidgetItem* actionItem = m_tableWidget->item(row, 0);
    if (!actionItem) return;
    
    KeybindManager::Action action = static_cast<KeybindManager::Action>(actionItem->data(Qt::UserRole).toInt());
    
    // Get current keybinds
    QList<QKeySequence> currentKeybinds = m_tempKeybinds.value(action);
    
    // Clear the specified keybind
    if (keybindIndex < currentKeybinds.size()) {
        currentKeybinds[keybindIndex] = QKeySequence();
        m_tempKeybinds[action] = currentKeybinds;
        
        // Update display
        updateKeybindDisplay(row);
        
        qDebug() << "KeybindEditorDialog: Keybind cleared successfully";
    }
}

void KeybindEditorDialog::resetInstructionLabel()
{
    m_instructionLabel->setText(tr("Click on a keybind cell to change it. Right-click to clear. Press Escape to cancel editing."));
    m_instructionLabel->setStyleSheet("QLabel { color: #555; font-style: italic; margin-bottom: 10px; }");
}

void KeybindEditorDialog::onResetToDefaultsClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        tr("Reset to Defaults"), 
        tr("Are you sure you want to reset all keybinds to their default values?"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        qDebug() << "KeybindEditorDialog: Resetting to defaults";
        
        // Reset temporary keybinds to defaults
        QList<KeybindManager::Action> actions = {
            KeybindManager::Action::PlayPause,
            KeybindManager::Action::Stop,
            KeybindManager::Action::SeekForward,
            KeybindManager::Action::SeekBackward,
            KeybindManager::Action::VolumeUp,
            KeybindManager::Action::VolumeDown
        };
        
        for (KeybindManager::Action action : actions) {
            m_tempKeybinds[action] = KeybindManager::getDefaultKeybinds(action);
        }
        
        // Update display
        for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
            updateKeybindDisplay(i);
        }
    }
}

void KeybindEditorDialog::onSaveClicked()
{
    qDebug() << "KeybindEditorDialog: Saving keybinds";
    
    // Apply all temporary keybinds to the manager
    for (auto it = m_tempKeybinds.constBegin(); it != m_tempKeybinds.constEnd(); ++it) {
        if (!m_keybindManager->setKeybinds(it.key(), it.value())) {
            QMessageBox::warning(this, tr("Save Failed"), 
                               tr("Failed to save keybinds for action: %1")
                               .arg(KeybindManager::actionToString(it.key())));
            return;
        }
    }
    
    // Save to file
    if (!m_keybindManager->saveKeybinds()) {
        QMessageBox::warning(this, tr("Save Failed"), 
                           tr("Failed to save keybinds to file."));
        return;
    }
    
    QMessageBox::information(this, tr("Success"), tr("Keybinds saved successfully!"));
    accept();
}

void KeybindEditorDialog::onCancelClicked()
{
    qDebug() << "KeybindEditorDialog: Cancelled";
    reject();
}

QString KeybindEditorDialog::getKeybindDisplayText(const QKeySequence& keySeq) const
{
    if (keySeq.isEmpty()) {
        return tr("[Not Set]");
    }
    
    return keySeq.toString(QKeySequence::NativeText);
}
