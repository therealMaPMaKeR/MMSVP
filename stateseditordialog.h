#ifndef STATESEDITORDIALOG_H
#define STATESEDITORDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimeEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QMap>
#include <QPixmap>

// Forward declaration
class LightweightVideoPlayer;

/**
 * @class StatesEditorDialog
 * @brief Dialog for editing playback states with visual previews
 * 
 * Allows users to view and edit all saved playback states across 4 groups.
 * Displays thumbnail previews and time information for each state.
 */
class StatesEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatesEditorDialog(LightweightVideoPlayer* player, QWidget *parent = nullptr);
    ~StatesEditorDialog();

private slots:
    void onTabChanged(int index);
    void onStateItemDoubleClicked(QListWidgetItem* item);
    void onStateItemRightClicked(const QPoint& pos);
    void onSaveClicked();
    void onCancelClicked();
    void onApplyClicked();

private:
    void setupUI();
    void loadStatesFromPlayer();
    void populateStateList(int groupIndex);
    void showEditDialog(int groupIndex, int stateIndex);
    bool checkUnsavedChanges(int fromGroup);
    void saveChangesToPlayer();
    void deleteState(int groupIndex, int stateIndex);
    void refreshPreview(int groupIndex, int stateIndex);
    QString formatTime(qint64 milliseconds) const;
    qint64 parseTime(const QTime& time) const;
    QIcon createIconFromPixmap(const QPixmap& pixmap, bool hasState) const;
    
    // Reference to video player
    LightweightVideoPlayer* m_player;
    
    // UI components
    QTabWidget* m_tabWidget;
    QListWidget* m_stateLists[4];  // One list per group
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    QLabel* m_instructionLabel;
    
    // Forward declare the PlaybackState type from LightweightVideoPlayer
    // We'll use it via the player's public typedef
    // State data (temporary storage for editing) - using player's PlaybackState type
    struct TempStateStorage {
        qint64 startPosition;
        qint64 endPosition;
        qreal playbackSpeed;
        bool isValid;
        bool hasEndPosition;
        QPixmap previewImage;
        
        TempStateStorage() : startPosition(0), endPosition(0), playbackSpeed(1.0), isValid(false), hasEndPosition(false) {}
    };
    
    TempStateStorage m_tempStates[4][12];  // 4 groups x 12 states
    
    // Track changes
    bool m_groupHasChanges[4];
    int m_currentGroup;
    int m_initialGroup;
};

/**
 * @class StateEditDialog
 * @brief Dialog for editing individual state properties
 */
class StateEditDialog : public QDialog
{
    Q_OBJECT

public:
    // Local state structure for editing
    struct EditableState {
        qint64 startPosition;
        qint64 endPosition;
        qreal playbackSpeed;
        bool isValid;
        bool hasEndPosition;
        QPixmap previewImage;
        
        EditableState() : startPosition(0), endPosition(0), playbackSpeed(1.0), isValid(false), hasEndPosition(false) {}
    };
    
    explicit StateEditDialog(const EditableState& state, 
                            int stateIndex, 
                            qint64 maxDuration,
                            QWidget* parent = nullptr);
    
    EditableState getState() const;

private slots:
    void onStartTimeChanged(const QTime& time);
    void onEndTimeChanged(const QTime& time);
    void onHasEndChanged(int state);
    void onSpeedChanged(double value);

private:
    void setupUI();
    void updateEndTimeEnabled();
    
    int m_stateIndex;
    qint64 m_maxDuration;
    
    QTimeEdit* m_startTimeEdit;
    QTimeEdit* m_endTimeEdit;
    QDoubleSpinBox* m_speedSpinBox;
    QCheckBox* m_hasEndCheckBox;
    QLabel* m_warningLabel;
    
    EditableState m_state;
};

#endif // STATESEDITORDIALOG_H
