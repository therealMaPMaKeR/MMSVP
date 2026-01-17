#include "stateseditordialog.h"
#include "lightweightvideoplayer.h"
#include <QMessageBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QMenu>
#include <QTime>
#include <QPainter>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

// StatesEditorDialog implementation
StatesEditorDialog::StatesEditorDialog(LightweightVideoPlayer* player, QWidget *parent)
    : QDialog(parent)
    , m_player(player)
    , m_tabWidget(nullptr)
    , m_saveButton(nullptr)
    , m_copyToButton(nullptr)
    , m_cancelButton(nullptr)
    , m_instructionLabel(nullptr)
    , m_currentGroup(0)
    , m_initialGroup(0)
{
    setWindowTitle(tr("States Editor"));
    resize(700, 600);
    
    // Initialize tracking
    for (int i = 0; i < 4; i++) {
        m_groupVisited[i] = false;
    }
    
    setupUI();
    loadStatesFromPlayer();
    
    // Mark initial group as visited
    m_initialGroup = m_currentGroup;
    m_groupVisited[m_currentGroup] = true;
    
    // Block signals during initial setup to prevent premature onTabChanged trigger
    m_tabWidget->blockSignals(true);
    m_tabWidget->setCurrentIndex(m_currentGroup);
    m_tabWidget->blockSignals(false);
    
    // Now populate the initial group
    populateStateList(m_currentGroup);
}

StatesEditorDialog::~StatesEditorDialog()
{
}

void StatesEditorDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Instruction label
    m_instructionLabel = new QLabel(tr("Double-click a state to edit. Right-click for options. Save button saves the current group to file."), this);
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
    
    m_saveButton = new QPushButton(tr("Save Group to File"), this);
    m_saveButton->setToolTip(tr("Save current group to file (like Ctrl+F1-F4)"));
    connect(m_saveButton, &QPushButton::clicked, this, &StatesEditorDialog::onSaveClicked);
    buttonLayout->addWidget(m_saveButton);
    
    m_copyToButton = new QPushButton(tr("Copy to:"), this);
    m_copyToButton->setToolTip(tr("Copy current group to another group"));
    connect(m_copyToButton, &QPushButton::clicked, this, &StatesEditorDialog::onCopyToClicked);
    buttonLayout->addWidget(m_copyToButton);
    
    m_cancelButton = new QPushButton(tr("Close"), this);
    m_cancelButton->setDefault(true);
    m_cancelButton->setToolTip(tr("Close dialog"));
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
    
    // Copy current group's states from player to temporary storage
    for (int s = 0; s < 12; s++) {
        const auto& playerState = m_player->getPlaybackState(s);
        m_tempStates[m_currentGroup][s].startPosition = playerState.startPosition;
        m_tempStates[m_currentGroup][s].endPosition = playerState.endPosition;
        m_tempStates[m_currentGroup][s].playbackSpeed = playerState.playbackSpeed;
        m_tempStates[m_currentGroup][s].isValid = playerState.isValid;
        m_tempStates[m_currentGroup][s].hasEndPosition = playerState.hasEndPosition;
        m_tempStates[m_currentGroup][s].previewImage = playerState.previewImage;
    }
    
    qDebug() << "StatesEditorDialog: Loaded states from player, current group:" << (m_currentGroup + 1);
}

void StatesEditorDialog::loadGroupFromDisk(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= 4 || !m_player) {
        return;
    }
    
    qDebug() << "StatesEditorDialog: Loading group" << (groupIndex + 1) << "from disk into temp storage";
    
    // Load the group from file without affecting player's current RAM state
    // We need to temporarily switch to load the data
    int originalGroup = m_player->currentStateGroup();
    
    // Switch to target group (this loads from disk into player's RAM)
    m_player->switchStateGroup(groupIndex);
    
    // Copy the disk-loaded data into our temp storage
    for (int s = 0; s < 12; s++) {
        const auto& playerState = m_player->getPlaybackState(s);
        m_tempStates[groupIndex][s].startPosition = playerState.startPosition;
        m_tempStates[groupIndex][s].endPosition = playerState.endPosition;
        m_tempStates[groupIndex][s].playbackSpeed = playerState.playbackSpeed;
        m_tempStates[groupIndex][s].isValid = playerState.isValid;
        m_tempStates[groupIndex][s].hasEndPosition = playerState.hasEndPosition;
        m_tempStates[groupIndex][s].previewImage = playerState.previewImage;
    }
    
    // Switch back to original group if needed
    if (originalGroup != groupIndex) {
        m_player->switchStateGroup(originalGroup);
    }
}

bool StatesEditorDialog::compareGroupWithDisk(int groupIndex) const
{
    if (groupIndex < 0 || groupIndex >= 4 || !m_player) {
        return true;  // Assume no changes if invalid
    }
    
    qDebug() << "StatesEditorDialog: Comparing group" << (groupIndex + 1) << "temp data with disk";
    
    // Load disk data temporarily for comparison
    int originalGroup = m_player->currentStateGroup();
    
    // Switch to target group to load from disk
    m_player->switchStateGroup(groupIndex);
    
    // Compare each state
    bool isIdentical = true;
    for (int s = 0; s < 12; s++) {
        const auto& diskState = m_player->getPlaybackState(s);
        const auto& tempState = m_tempStates[groupIndex][s];
        
        if (diskState.isValid != tempState.isValid ||
            diskState.hasEndPosition != tempState.hasEndPosition ||
            diskState.startPosition != tempState.startPosition ||
            diskState.endPosition != tempState.endPosition ||
            !qFuzzyCompare(diskState.playbackSpeed, tempState.playbackSpeed)) {
            isIdentical = false;
            qDebug() << "StatesEditorDialog: Found difference in state" << (s + 1);
            break;
        }
    }
    
    // Switch back to original group
    if (originalGroup != groupIndex) {
        m_player->switchStateGroup(originalGroup);
    }
    
    if (isIdentical) {
        qDebug() << "StatesEditorDialog: Group" << (groupIndex + 1) << "matches disk - no unsaved changes";
    } else {
        qDebug() << "StatesEditorDialog: Group" << (groupIndex + 1) << "differs from disk - has unsaved changes";
    }
    
    return isIdentical;
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
    qDebug() << "StatesEditorDialog: Tab changed from" << (m_currentGroup + 1) << "to" << (index + 1);
    
    if (!m_player || index == m_currentGroup) {
        return;
    }
    
    // Step 1: Check if we're leaving a group with unsaved changes
    // Compare current group's temp data with what's on disk
    bool hasUnsavedChanges = false;
    
    if (m_groupVisited[m_currentGroup]) {
        // Check if temp data differs from disk
        if (!compareGroupWithDisk(m_currentGroup)) {
            hasUnsavedChanges = true;
        }
    }
    
    // Step 2: If there are unsaved changes, ask user what to do
    if (hasUnsavedChanges) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Unsaved Changes"));
        msgBox.setText(tr("Group %1 has unsaved changes.").arg(m_currentGroup + 1));
        msgBox.setInformativeText(tr("These changes differ from what's saved on disk. What would you like to do?"));
        
        QPushButton* saveButton = msgBox.addButton(tr("Save to Disk"), QMessageBox::AcceptRole);
        QPushButton* discardButton = msgBox.addButton(tr("Discard Changes"), QMessageBox::DestructiveRole);
        QPushButton* cancelButton = msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
        
        msgBox.setDefaultButton(saveButton);
        msgBox.exec();
        
        if (msgBox.clickedButton() == cancelButton) {
            // User cancelled - stay on current group
            qDebug() << "StatesEditorDialog: User cancelled group switch";
            m_tabWidget->blockSignals(true);
            m_tabWidget->setCurrentIndex(m_currentGroup);
            m_tabWidget->blockSignals(false);
            return;
        }
        else if (msgBox.clickedButton() == saveButton) {
            // Save changes to both RAM and disk for current group
            qDebug() << "StatesEditorDialog: Saving changes to RAM and disk before switching";
            
            // First, make sure player is on the current group
            if (m_player->currentStateGroup() != m_currentGroup) {
                m_player->switchStateGroup(m_currentGroup);
            }
            
            // Write temp data to player RAM
            for (int s = 0; s < 12; s++) {
                LightweightVideoPlayer::PlaybackState playerState;
                playerState.startPosition = m_tempStates[m_currentGroup][s].startPosition;
                playerState.endPosition = m_tempStates[m_currentGroup][s].endPosition;
                playerState.playbackSpeed = m_tempStates[m_currentGroup][s].playbackSpeed;
                playerState.isValid = m_tempStates[m_currentGroup][s].isValid;
                playerState.hasEndPosition = m_tempStates[m_currentGroup][s].hasEndPosition;
                playerState.previewImage = m_tempStates[m_currentGroup][s].previewImage;
                
                m_player->setPlaybackState(s, playerState);
            }
            
            // Save to disk
            m_player->saveStateGroup(m_currentGroup);
        }
        else if (msgBox.clickedButton() == discardButton) {
            // Discard changes - reload from disk into temp
            qDebug() << "StatesEditorDialog: Discarding changes, reloading from disk";
            loadGroupFromDisk(m_currentGroup);
        }
    }
    
    // Step 3: Load the target group
    if (m_groupVisited[index]) {
        // Group was already visited - just switch player to it and update display from temp
        qDebug() << "StatesEditorDialog: Switching to previously visited group" << (index + 1);
        
        // Switch player to target group
        m_player->switchStateGroup(index);
        
        // Restore temp data to player RAM (in case it was modified in dialog)
        for (int s = 0; s < 12; s++) {
            LightweightVideoPlayer::PlaybackState playerState;
            playerState.startPosition = m_tempStates[index][s].startPosition;
            playerState.endPosition = m_tempStates[index][s].endPosition;
            playerState.playbackSpeed = m_tempStates[index][s].playbackSpeed;
            playerState.isValid = m_tempStates[index][s].isValid;
            playerState.hasEndPosition = m_tempStates[index][s].hasEndPosition;
            playerState.previewImage = m_tempStates[index][s].previewImage;
            
            m_player->setPlaybackState(s, playerState);
        }
    }
    else {
        // First time visiting this group - load from disk
        qDebug() << "StatesEditorDialog: First visit to group" << (index + 1) << "- loading from disk";
        loadGroupFromDisk(index);
        m_groupVisited[index] = true;
    }
    
    // Step 4: Update current group and refresh display
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
    
    // Pause playback temporarily for preview capture
    bool wasPlaying = m_player->isPlaying();
    if (wasPlaying) {
        m_player->pause();
        // Wait a bit for VLC to settle
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(150);
        loop.exec();
    }
    
    // Capture new preview at the state's start position
    QPixmap newPreview = m_player->captureFrameAtPosition(state.startPosition);
    
    // Resume playback if it was playing
    if (wasPlaying) {
        m_player->play();
    }
    
    if (!newPreview.isNull()) {
        state.previewImage = newPreview;
        
        // Update display
        populateStateList(groupIndex);
        
        qDebug() << "StatesEditorDialog: Preview refreshed successfully";
    } else {
        QMessageBox::warning(this, tr("Failed to Capture"),
                           tr("Failed to capture preview image. Make sure video is loaded."));
    }
}

void StatesEditorDialog::onSaveClicked()
{
    qDebug() << "StatesEditorDialog: Save clicked";
    
    if (!m_player) {
        QMessageBox::warning(this, tr("Error"), tr("No player reference."));
        return;
    }
    
    // Make sure player is on the current group
    if (m_player->currentStateGroup() != m_currentGroup) {
        m_player->switchStateGroup(m_currentGroup);
    }
    
    // Apply current group's temp data to player memory
    for (int s = 0; s < 12; s++) {
        LightweightVideoPlayer::PlaybackState playerState;
        playerState.startPosition = m_tempStates[m_currentGroup][s].startPosition;
        playerState.endPosition = m_tempStates[m_currentGroup][s].endPosition;
        playerState.playbackSpeed = m_tempStates[m_currentGroup][s].playbackSpeed;
        playerState.isValid = m_tempStates[m_currentGroup][s].isValid;
        playerState.hasEndPosition = m_tempStates[m_currentGroup][s].hasEndPosition;
        playerState.previewImage = m_tempStates[m_currentGroup][s].previewImage;
        
        m_player->setPlaybackState(s, playerState);
    }
    
    // Save current group to file (like Ctrl+F1-F4)
    m_player->saveStateGroup(m_currentGroup);
    
    QMessageBox::information(this, tr("Saved"), 
                           tr("Group %1 has been saved to file.").arg(m_currentGroup + 1));
}

void StatesEditorDialog::onCopyToClicked()
{
    qDebug() << "StatesEditorDialog: Copy to clicked";
    
    if (!m_player) {
        QMessageBox::warning(this, tr("Error"), tr("No player reference."));
        return;
    }
    
    // Create a dialog to select target group
    QDialog selectDialog(this);
    selectDialog.setWindowTitle(tr("Copy Group %1").arg(m_currentGroup + 1));
    
    QVBoxLayout* layout = new QVBoxLayout(&selectDialog);
    
    QLabel* label = new QLabel(tr("Select which group to overwrite with Group %1:").arg(m_currentGroup + 1), &selectDialog);
    label->setWordWrap(true);
    layout->addWidget(label);
    
    // Create buttons for each group (excluding current)
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    int targetGroup = -1;
    
    for (int i = 0; i < 4; i++) {
        if (i == m_currentGroup) {
            continue; // Skip current group
        }
        
        QPushButton* groupButton = new QPushButton(tr("Group %1").arg(i + 1), &selectDialog);
        connect(groupButton, &QPushButton::clicked, [&selectDialog, i, &targetGroup]() {
            targetGroup = i;
            selectDialog.accept();
        });
        buttonLayout->addWidget(groupButton);
    }
    
    layout->addLayout(buttonLayout);
    
    // Add cancel button
    QHBoxLayout* cancelLayout = new QHBoxLayout();
    cancelLayout->addStretch();
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &selectDialog);
    connect(cancelButton, &QPushButton::clicked, &selectDialog, &QDialog::reject);
    cancelLayout->addWidget(cancelButton);
    
    layout->addLayout(cancelLayout);
    
    selectDialog.setLayout(layout);
    selectDialog.resize(400, 150);
    
    // Show selection dialog
    if (selectDialog.exec() == QDialog::Rejected || targetGroup < 0) {
        qDebug() << "StatesEditorDialog: Copy cancelled";
        return;
    }
    
    qDebug() << "StatesEditorDialog: Target group selected:" << (targetGroup + 1);
    
    // Show confirmation dialog
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Confirm Copy"),
        tr("Overwrite Group %1 with Group %2?\n\nThis will replace all states in Group %1 with the states from Group %2.")
            .arg(targetGroup + 1).arg(m_currentGroup + 1),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        qDebug() << "StatesEditorDialog: Copy confirmation declined";
        return;
    }
    
    // Copy all states from current group to target group in temp storage
    for (int s = 0; s < 12; s++) {
        m_tempStates[targetGroup][s] = m_tempStates[m_currentGroup][s];
    }
    
    // Mark target group as visited since we modified it
    m_groupVisited[targetGroup] = true;
    
    // Save target group to player memory and then to file
    // First switch to target group
    m_player->switchStateGroup(targetGroup);
    
    // Apply copied data to player memory
    for (int s = 0; s < 12; s++) {
        LightweightVideoPlayer::PlaybackState playerState;
        playerState.startPosition = m_tempStates[targetGroup][s].startPosition;
        playerState.endPosition = m_tempStates[targetGroup][s].endPosition;
        playerState.playbackSpeed = m_tempStates[targetGroup][s].playbackSpeed;
        playerState.isValid = m_tempStates[targetGroup][s].isValid;
        playerState.hasEndPosition = m_tempStates[targetGroup][s].hasEndPosition;
        playerState.previewImage = m_tempStates[targetGroup][s].previewImage;
        
        m_player->setPlaybackState(s, playerState);
    }
    
    // Save target group to file
    m_player->saveStateGroup(targetGroup);
    
    // Switch back to original group
    m_player->switchStateGroup(m_currentGroup);
    
    // Restore current group's data to player memory
    for (int s = 0; s < 12; s++) {
        LightweightVideoPlayer::PlaybackState playerState;
        playerState.startPosition = m_tempStates[m_currentGroup][s].startPosition;
        playerState.endPosition = m_tempStates[m_currentGroup][s].endPosition;
        playerState.playbackSpeed = m_tempStates[m_currentGroup][s].playbackSpeed;
        playerState.isValid = m_tempStates[m_currentGroup][s].isValid;
        playerState.hasEndPosition = m_tempStates[m_currentGroup][s].hasEndPosition;
        playerState.previewImage = m_tempStates[m_currentGroup][s].previewImage;
        
        m_player->setPlaybackState(s, playerState);
    }
    
    qDebug() << "StatesEditorDialog: Successfully copied Group" << (m_currentGroup + 1) 
             << "to Group" << (targetGroup + 1);
    
    QMessageBox::information(this, tr("Copy Complete"),
                           tr("Group %1 has been copied to Group %2 and saved to file.")
                           .arg(m_currentGroup + 1).arg(targetGroup + 1));
    
    // If the target group tab is currently visible, refresh it
    if (m_tabWidget->currentIndex() == targetGroup) {
        populateStateList(targetGroup);
    }
}

void StatesEditorDialog::onCancelClicked()
{
    qDebug() << "StatesEditorDialog: Close clicked";
    
    // Check if current group has unsaved changes by comparing with disk
    bool hasUnsavedChanges = false;
    if (m_player && m_groupVisited[m_currentGroup]) {
        if (!compareGroupWithDisk(m_currentGroup)) {
            hasUnsavedChanges = true;
        }
    }
    
    if (hasUnsavedChanges) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Unsaved Changes"));
        msgBox.setText(tr("Group %1 has unsaved changes.").arg(m_currentGroup + 1));
        msgBox.setInformativeText(tr("These changes differ from what's saved on disk. Do you want to save them before closing?"));
        
        QPushButton* saveButton = msgBox.addButton(tr("Save to Disk"), QMessageBox::AcceptRole);
        QPushButton* discardButton = msgBox.addButton(tr("Discard Changes"), QMessageBox::DestructiveRole);
        QPushButton* cancelButton = msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
        
        msgBox.setDefaultButton(saveButton);
        msgBox.exec();
        
        if (msgBox.clickedButton() == cancelButton) {
            // User cancelled - don't close
            qDebug() << "StatesEditorDialog: User cancelled close";
            return;
        }
        else if (msgBox.clickedButton() == saveButton) {
            // Save changes to both RAM and disk
            qDebug() << "StatesEditorDialog: Saving changes to RAM and disk before closing";
            
            // Make sure player is on the current group
            if (m_player->currentStateGroup() != m_currentGroup) {
                m_player->switchStateGroup(m_currentGroup);
            }
            
            // Write temp data to player RAM
            for (int s = 0; s < 12; s++) {
                LightweightVideoPlayer::PlaybackState playerState;
                playerState.startPosition = m_tempStates[m_currentGroup][s].startPosition;
                playerState.endPosition = m_tempStates[m_currentGroup][s].endPosition;
                playerState.playbackSpeed = m_tempStates[m_currentGroup][s].playbackSpeed;
                playerState.isValid = m_tempStates[m_currentGroup][s].isValid;
                playerState.hasEndPosition = m_tempStates[m_currentGroup][s].hasEndPosition;
                playerState.previewImage = m_tempStates[m_currentGroup][s].previewImage;
                
                m_player->setPlaybackState(s, playerState);
            }
            
            // Save to disk
            m_player->saveStateGroup(m_currentGroup);
        }
        // If discard was clicked, just proceed to close without saving
    }
    
    accept();
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
