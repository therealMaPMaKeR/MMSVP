#include "stateseditordialog.h"
#include "lightweightvideoplayer.h"
#include <QMessageBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QMenu>
#include <QTime>
#include <QPainter>
#include <QDebug>

// StatesEditorDialog implementation
StatesEditorDialog::StatesEditorDialog(LightweightVideoPlayer* player, QWidget *parent)
    : QDialog(parent)
    , m_player(player)
    , m_tabWidget(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
    , m_applyButton(nullptr)
    , m_instructionLabel(nullptr)
    , m_currentGroup(0)
    , m_initialGroup(0)
{
    setWindowTitle(tr("States Editor"));
    resize(700, 600);
    
    // Initialize change tracking
    for (int i = 0; i < 4; i++) {
        m_groupHasChanges[i] = false;
    }
    
    setupUI();
    loadStatesFromPlayer();
    
    // Open to current state group
    m_initialGroup = m_currentGroup;
    m_tabWidget->setCurrentIndex(m_currentGroup);
    populateStateList(m_currentGroup);
}

StatesEditorDialog::~StatesEditorDialog()
{
}

void StatesEditorDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Instruction label
    m_instructionLabel = new QLabel(tr("Double-click a state to edit. Right-click to delete. Changes are saved when you click Save or Apply."), this);
    m_instructionLabel->setWordWrap(true);
    m_instructionLabel->setStyleSheet("QLabel { color: #555; font-style: italic; margin-bottom: 10px; }");
    mainLayout->addWidget(m_instructionLabel);
    
    // Tab widget for 4 state groups
    m_tabWidget = new QTabWidget(this);
    
    for (int i = 0; i < 4; i++) {
        QWidget* tabPage = new QWidget();
        QVBoxLayout* tabLayout = new QVBoxLayout(tabPage);
        
        // Create list widget for this group
        m_stateLists[i] = new QListWidget(tabPage);
        m_stateLists[i]->setIconSize(QSize(100, 75));
        m_stateLists[i]->setContextMenuPolicy(Qt::CustomContextMenu);
        m_stateLists[i]->setSelectionMode(QAbstractItemView::SingleSelection);
        
        // Connect signals for this list
        connect(m_stateLists[i], &QListWidget::itemDoubleClicked, 
                this, &StatesEditorDialog::onStateItemDoubleClicked);
        connect(m_stateLists[i], &QListWidget::customContextMenuRequested,
                this, &StatesEditorDialog::onStateItemRightClicked);
        
        tabLayout->addWidget(m_stateLists[i]);
        
        m_tabWidget->addTab(tabPage, tr("Group %1").arg(i + 1));
    }
    
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &StatesEditorDialog::onTabChanged);
    
    mainLayout->addWidget(m_tabWidget);
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    buttonLayout->addStretch();
    
    m_applyButton = new QPushButton(tr("Apply"), this);
    m_applyButton->setToolTip(tr("Apply changes without closing"));
    connect(m_applyButton, &QPushButton::clicked, this, &StatesEditorDialog::onApplyClicked);
    buttonLayout->addWidget(m_applyButton);
    
    m_saveButton = new QPushButton(tr("Save"), this);
    m_saveButton->setDefault(true);
    m_saveButton->setToolTip(tr("Save changes and close"));
    connect(m_saveButton, &QPushButton::clicked, this, &StatesEditorDialog::onSaveClicked);
    buttonLayout->addWidget(m_saveButton);
    
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_cancelButton->setToolTip(tr("Discard changes and close"));
    connect(m_cancelButton, &QPushButton::clicked, this, &StatesEditorDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    setLayout(mainLayout);
}

void StatesEditorDialog::loadStatesFromPlayer()
{
    if (!m_player) {
        qDebug() << "StatesEditorDialog: No player reference";
        return;
    }
    
    // Get current state group from player
    m_currentGroup = m_player->currentStateGroup();
    
    // Copy all states from player to temporary storage
    for (int g = 0; g < 4; g++) {
        for (int s = 0; s < 12; s++) {
            const auto& playerState = m_player->getPlaybackState(g, s);
            m_tempStates[g][s].startPosition = playerState.startPosition;
            m_tempStates[g][s].endPosition = playerState.endPosition;
            m_tempStates[g][s].playbackSpeed = playerState.playbackSpeed;
            m_tempStates[g][s].isValid = playerState.isValid;
            m_tempStates[g][s].hasEndPosition = playerState.hasEndPosition;
            m_tempStates[g][s].previewImage = playerState.previewImage;
        }
    }
    
    qDebug() << "StatesEditorDialog: Loaded states from player, current group:" << (m_currentGroup + 1);
}

void StatesEditorDialog::populateStateList(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= 4) {
        return;
    }
    
    QListWidget* list = m_stateLists[groupIndex];
    list->clear();
    
    // Populate list with 12 states
    for (int i = 0; i < 12; i++) {
        const TempStateStorage& state = m_tempStates[groupIndex][i];
        
        QListWidgetItem* item = new QListWidgetItem();
        
        // Set icon (use real preview or placeholder)
        item->setIcon(createIconFromPixmap(state.previewImage, state.isValid));
        
        // Set text
        QString text = tr("State %1: ").arg(i + 1);
        
        if (!state.isValid) {
            text += tr("Empty");
            item->setForeground(QBrush(QColor(150, 150, 150)));
        } else {
            text += formatTime(state.startPosition);
            
            if (state.hasEndPosition) {
                text += " - " + formatTime(state.endPosition);
            }
            
            // Add speed info if not 1.0x
            if (!qFuzzyCompare(state.playbackSpeed, 1.0)) {
                text += QString(" (%1x)").arg(state.playbackSpeed, 0, 'f', 1);
            }
        }
        
        item->setText(text);
        item->setData(Qt::UserRole, i);  // Store state index
        
        list->addItem(item);
    }
    
    qDebug() << "StatesEditorDialog: Populated list for group" << (groupIndex + 1);
}

void StatesEditorDialog::onTabChanged(int index)
{
    qDebug() << "StatesEditorDialog: Tab changed to" << (index + 1);
    
    // Check if current group has unsaved changes
    if (checkUnsavedChanges(m_currentGroup)) {
        // User wants to save or was cancelled
        // If cancelled, revert to previous tab
        if (m_groupHasChanges[m_currentGroup]) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                tr("Unsaved Changes"),
                tr("Group %1 has unsaved changes. Do you want to apply them before switching?")
                    .arg(m_currentGroup + 1),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
            );
            
            if (reply == QMessageBox::Cancel) {
                // Block the tab change
                m_tabWidget->blockSignals(true);
                m_tabWidget->setCurrentIndex(m_currentGroup);
                m_tabWidget->blockSignals(false);
                return;
            } else if (reply == QMessageBox::Yes) {
                // Apply changes for current group
                m_groupHasChanges[m_currentGroup] = false;
            } else {
                // Discard changes - reload from player
                // (We'll implement this when we add the getter method)
            }
        }
    }
    
    m_currentGroup = index;
    populateStateList(index);
}

void StatesEditorDialog::onStateItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) {
        return;
    }
    
    int stateIndex = item->data(Qt::UserRole).toInt();
    
    qDebug() << "StatesEditorDialog: Double-clicked state" << (stateIndex + 1) 
             << "in group" << (m_currentGroup + 1);
    
    showEditDialog(m_currentGroup, stateIndex);
}

void StatesEditorDialog::onStateItemRightClicked(const QPoint& pos)
{
    // Find which list widget sent the signal
    QListWidget* list = qobject_cast<QListWidget*>(sender());
    if (!list) {
        return;
    }
    
    QListWidgetItem* item = list->itemAt(pos);
    if (!item) {
        return;
    }
    
    int stateIndex = item->data(Qt::UserRole).toInt();
    
    // Find which group this list belongs to
    int groupIndex = -1;
    for (int i = 0; i < 4; i++) {
        if (m_stateLists[i] == list) {
            groupIndex = i;
            break;
        }
    }
    
    if (groupIndex < 0) {
        return;
    }
    
    // Check if state is valid (can only delete valid states)
    if (!m_tempStates[groupIndex][stateIndex].isValid) {
        return;
    }
    
    // Show context menu
    QMenu contextMenu(tr("State Actions"), this);
    
    QAction* editAction = contextMenu.addAction(tr("Edit State"));
    QAction* refreshAction = contextMenu.addAction(tr("Refresh Preview"));
    QAction* deleteAction = contextMenu.addAction(tr("Delete State"));
    
    // Disable refresh for invalid states
    if (!m_tempStates[groupIndex][stateIndex].isValid) {
        refreshAction->setEnabled(false);
    }
    
    QAction* selectedAction = contextMenu.exec(list->mapToGlobal(pos));
    
    if (selectedAction == editAction) {
        showEditDialog(groupIndex, stateIndex);
    } else if (selectedAction == refreshAction) {
        refreshPreview(groupIndex, stateIndex);
    } else if (selectedAction == deleteAction) {
        deleteState(groupIndex, stateIndex);
    }
}

void StatesEditorDialog::showEditDialog(int groupIndex, int stateIndex)
{
    if (groupIndex < 0 || groupIndex >= 4 || stateIndex < 0 || stateIndex >= 12) {
        return;
    }
    
    qint64 maxDuration = 0;
    if (m_player) {
        maxDuration = m_player->duration();
    }
    
    TempStateStorage& state = m_tempStates[groupIndex][stateIndex];
    
    // Convert to EditableState
    StateEditDialog::EditableState editState;
    editState.startPosition = state.startPosition;
    editState.endPosition = state.endPosition;
    editState.playbackSpeed = state.playbackSpeed;
    editState.isValid = state.isValid;
    editState.hasEndPosition = state.hasEndPosition;
    editState.previewImage = state.previewImage;
    
    StateEditDialog editDialog(editState, stateIndex, maxDuration, this);
    
    if (editDialog.exec() == QDialog::Accepted) {
        // Get modified state
        editState = editDialog.getState();
        
        // Copy back to temp storage
        state.startPosition = editState.startPosition;
        state.endPosition = editState.endPosition;
        state.playbackSpeed = editState.playbackSpeed;
        state.isValid = editState.isValid;
        state.hasEndPosition = editState.hasEndPosition;
        state.previewImage = editState.previewImage;
        
        // Mark this group as having changes
        m_groupHasChanges[groupIndex] = true;
        
        // Update the list display
        populateStateList(groupIndex);
        
        qDebug() << "StatesEditorDialog: Modified state" << (stateIndex + 1)
                 << "in group" << (groupIndex + 1);
    }
}

void StatesEditorDialog::deleteState(int groupIndex, int stateIndex)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Delete State"),
        tr("Are you sure you want to delete State %1 from Group %2?")
            .arg(stateIndex + 1).arg(groupIndex + 1),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Clear the state
        m_tempStates[groupIndex][stateIndex] = TempStateStorage();
        
        // Mark as changed
        m_groupHasChanges[groupIndex] = true;
        
        // Update display
        populateStateList(groupIndex);
        
        qDebug() << "StatesEditorDialog: Deleted state" << (stateIndex + 1)
                 << "from group" << (groupIndex + 1);
    }
}

void StatesEditorDialog::refreshPreview(int groupIndex, int stateIndex)
{
    if (groupIndex < 0 || groupIndex >= 4 || stateIndex < 0 || stateIndex >= 12) {
        return;
    }
    
    if (!m_player) {
        return;
    }
    
    TempStateStorage& state = m_tempStates[groupIndex][stateIndex];
    
    if (!state.isValid) {
        return;
    }
    
    qDebug() << "StatesEditorDialog: Refreshing preview for state" << (stateIndex + 1)
             << "in group" << (groupIndex + 1);
    
    // Capture new preview at the state's start position
    QPixmap newPreview = m_player->captureFrameAtPosition(state.startPosition);
    
    if (!newPreview.isNull()) {
        state.previewImage = newPreview;
        m_groupHasChanges[groupIndex] = true;
        
        // Update display
        populateStateList(groupIndex);
        
        qDebug() << "StatesEditorDialog: Preview refreshed successfully";
    } else {
        QMessageBox::warning(this, tr("Failed to Capture"),
                           tr("Failed to capture preview image. Make sure video is loaded."));
    }
}

bool StatesEditorDialog::checkUnsavedChanges(int fromGroup)
{
    if (fromGroup < 0 || fromGroup >= 4) {
        return false;
    }
    
    return m_groupHasChanges[fromGroup];
}

void StatesEditorDialog::saveChangesToPlayer()
{
    if (!m_player) {
        qDebug() << "StatesEditorDialog: No player reference";
        return;
    }
    
    // Copy modified states back to player
    for (int g = 0; g < 4; g++) {
        for (int s = 0; s < 12; s++) {
            LightweightVideoPlayer::PlaybackState playerState;
            playerState.startPosition = m_tempStates[g][s].startPosition;
            playerState.endPosition = m_tempStates[g][s].endPosition;
            playerState.playbackSpeed = m_tempStates[g][s].playbackSpeed;
            playerState.isValid = m_tempStates[g][s].isValid;
            playerState.hasEndPosition = m_tempStates[g][s].hasEndPosition;
            playerState.previewImage = m_tempStates[g][s].previewImage;
            
            m_player->setPlaybackState(g, s, playerState);
        }
    }
    
    // Mark all groups as saved
    for (int i = 0; i < 4; i++) {
        m_groupHasChanges[i] = false;
    }
    
    qDebug() << "StatesEditorDialog: Saved all changes to player";
}

void StatesEditorDialog::onSaveClicked()
{
    qDebug() << "StatesEditorDialog: Save clicked";
    
    saveChangesToPlayer();
    accept();
}

void StatesEditorDialog::onCancelClicked()
{
    qDebug() << "StatesEditorDialog: Cancel clicked";
    
    // Check if any group has unsaved changes
    bool hasAnyChanges = false;
    for (int i = 0; i < 4; i++) {
        if (m_groupHasChanges[i]) {
            hasAnyChanges = true;
            break;
        }
    }
    
    if (hasAnyChanges) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Unsaved Changes"),
            tr("You have unsaved changes. Are you sure you want to discard them?"),
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::No) {
            return;
        }
    }
    
    reject();
}

void StatesEditorDialog::onApplyClicked()
{
    qDebug() << "StatesEditorDialog: Apply clicked";
    
    saveChangesToPlayer();
    
    QMessageBox::information(this, tr("Changes Applied"), 
                           tr("All changes have been applied to the player."));
}

QString StatesEditorDialog::formatTime(qint64 milliseconds) const
{
    if (milliseconds < 0) {
        return "00:00";
    }
    
    int hours = milliseconds / 3600000;
    int minutes = (milliseconds % 3600000) / 60000;
    int seconds = (milliseconds % 60000) / 1000;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
}

qint64 StatesEditorDialog::parseTime(const QTime& time) const
{
    return time.msecsSinceStartOfDay();
}

QIcon StatesEditorDialog::createIconFromPixmap(const QPixmap& pixmap, bool hasState) const
{
    if (!pixmap.isNull()) {
        // Use the actual preview image
        return QIcon(pixmap);
    }
    
    // Create a placeholder icon
    QPixmap placeholder(100, 75);
    
    if (hasState) {
        // Blue placeholder for valid states without preview
        placeholder.fill(QColor(100, 150, 200));
        
        QPainter painter(&placeholder);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 20, QFont::Bold));
        painter.drawText(placeholder.rect(), Qt::AlignCenter, "▶");
    } else {
        // Gray placeholder for empty states
        placeholder.fill(QColor(180, 180, 180));
        
        QPainter painter(&placeholder);
        painter.setPen(Qt::darkGray);
        painter.setFont(QFont("Arial", 12));
        painter.drawText(placeholder.rect(), Qt::AlignCenter, "Empty");
    }
    
    return QIcon(placeholder);
}

// StateEditDialog implementation
StateEditDialog::StateEditDialog(const EditableState& state,
                                 int stateIndex,
                                 qint64 maxDuration,
                                 QWidget* parent)
    : QDialog(parent)
    , m_stateIndex(stateIndex)
    , m_maxDuration(maxDuration)
    , m_startTimeEdit(nullptr)
    , m_endTimeEdit(nullptr)
    , m_hasEndCheckBox(nullptr)
    , m_warningLabel(nullptr)
    , m_state(state)
{
    setWindowTitle(tr("Edit State %1").arg(stateIndex + 1));
    setupUI();
}

void StateEditDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create form layout
    QGridLayout* formLayout = new QGridLayout();
    
    // Start time
    QLabel* startLabel = new QLabel(tr("Start Time:"), this);
    m_startTimeEdit = new QTimeEdit(this);
    m_startTimeEdit->setDisplayFormat("HH:mm:ss");
    
    // Convert milliseconds to QTime
    QTime startTime = QTime::fromMSecsSinceStartOfDay(m_state.startPosition);
    m_startTimeEdit->setTime(startTime);
    
    connect(m_startTimeEdit, &QTimeEdit::timeChanged, 
            this, &StateEditDialog::onStartTimeChanged);
    
    formLayout->addWidget(startLabel, 0, 0);
    formLayout->addWidget(m_startTimeEdit, 0, 1);
    
    // Has end position checkbox
    m_hasEndCheckBox = new QCheckBox(tr("Set Loop End Position"), this);
    m_hasEndCheckBox->setChecked(m_state.hasEndPosition);
    
    connect(m_hasEndCheckBox, &QCheckBox::stateChanged,
            this, &StateEditDialog::onHasEndChanged);
    
    formLayout->addWidget(m_hasEndCheckBox, 1, 0, 1, 2);
    
    // End time
    QLabel* endLabel = new QLabel(tr("End Time:"), this);
    m_endTimeEdit = new QTimeEdit(this);
    m_endTimeEdit->setDisplayFormat("HH:mm:ss");
    
    QTime endTime = QTime::fromMSecsSinceStartOfDay(m_state.endPosition);
    m_endTimeEdit->setTime(endTime);
    
    connect(m_endTimeEdit, &QTimeEdit::timeChanged,
            this, &StateEditDialog::onEndTimeChanged);
    
    formLayout->addWidget(endLabel, 2, 0);
    formLayout->addWidget(m_endTimeEdit, 2, 1);
    
    // Playback speed
    QLabel* speedLabel = new QLabel(tr("Playback Speed:"), this);
    m_speedSpinBox = new QDoubleSpinBox(this);
    m_speedSpinBox->setRange(0.1, 5.0);
    m_speedSpinBox->setSingleStep(0.1);
    m_speedSpinBox->setValue(m_state.playbackSpeed);
    m_speedSpinBox->setSuffix("x");
    m_speedSpinBox->setDecimals(1);
    
    connect(m_speedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &StateEditDialog::onSpeedChanged);
    
    formLayout->addWidget(speedLabel, 3, 0);
    formLayout->addWidget(m_speedSpinBox, 3, 1);
    
    mainLayout->addLayout(formLayout);
    
    // Warning label
    m_warningLabel = new QLabel(this);
    m_warningLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    m_warningLabel->setVisible(false);
    m_warningLabel->setWordWrap(true);
    mainLayout->addWidget(m_warningLabel);
    
    mainLayout->addStretch();
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    QPushButton* okButton = new QPushButton(tr("OK"), this);
    okButton->setDefault(true);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), this);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    setLayout(mainLayout);
    
    // Update end time enabled state
    updateEndTimeEnabled();
    
    resize(400, 250);
}

void StateEditDialog::onStartTimeChanged(const QTime& time)
{
    qint64 startMs = time.msecsSinceStartOfDay();
    m_state.startPosition = startMs;
    
    // Mark as valid if it wasn't before
    if (!m_state.isValid) {
        m_state.isValid = true;
    }
    
    // Validate that start < end
    if (m_state.hasEndPosition && m_state.endPosition <= m_state.startPosition) {
        m_warningLabel->setText(tr("Warning: End time must be after start time!"));
        m_warningLabel->setVisible(true);
    } else {
        m_warningLabel->setVisible(false);
    }
}

void StateEditDialog::onEndTimeChanged(const QTime& time)
{
    qint64 endMs = time.msecsSinceStartOfDay();
    m_state.endPosition = endMs;
    
    // Validate that start < end
    if (m_state.hasEndPosition && m_state.endPosition <= m_state.startPosition) {
        m_warningLabel->setText(tr("Warning: End time must be after start time!"));
        m_warningLabel->setVisible(true);
    } else {
        m_warningLabel->setVisible(false);
    }
}

void StateEditDialog::onHasEndChanged(int state)
{
    m_state.hasEndPosition = (state == Qt::Checked);
    updateEndTimeEnabled();
    
    // Clear warning if end is disabled
    if (!m_state.hasEndPosition) {
        m_warningLabel->setVisible(false);
    }
}

void StateEditDialog::onSpeedChanged(double value)
{
    m_state.playbackSpeed = value;
}

void StateEditDialog::updateEndTimeEnabled()
{
    m_endTimeEdit->setEnabled(m_hasEndCheckBox->isChecked());
}

StateEditDialog::EditableState StateEditDialog::getState() const
{
    return m_state;
}
