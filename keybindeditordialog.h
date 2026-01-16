#ifndef KEYBINDEDITORDIALOG_H
#define KEYBINDEDITORDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QKeyEvent>
#include "keybindmanager.h"

/**
 * @class KeybindEditorDialog
 * @brief Dialog for editing keybindings
 * 
 * Allows users to view and modify keybindings for various actions.
 * Each action can have up to 2 different keybinds.
 */
class KeybindEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeybindEditorDialog(KeybindManager* keybindManager, QWidget *parent = nullptr);
    ~KeybindEditorDialog();

private slots:
    void onCellClicked(int row, int column);
    void onCellRightClicked(const QPoint& pos);
    void onResetToDefaultsClicked();
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupUI();
    void populateTable();
    void updateKeybindDisplay(int row);
    void startEditingKeybind(int row, int keybindIndex);
    void clearKeybind(int row, int keybindIndex);
    void resetInstructionLabel();
    QString getKeybindDisplayText(const QKeySequence& keySeq) const;
    
    // Keybind manager reference
    KeybindManager* m_keybindManager;
    
    // UI components
    QTableWidget* m_tableWidget;
    QPushButton* m_resetButton;
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
    QLabel* m_instructionLabel;
    
    // Temporary storage for edited keybinds (not saved until user clicks Save)
    QMap<KeybindManager::Action, QList<QKeySequence>> m_tempKeybinds;
    
    // Tracking which cell is being edited
    int m_editingRow;
    int m_editingColumn;
    bool m_isEditingKeybind;
    
    // Custom widget for capturing key presses
    class KeyCaptureWidget;
};

/**
 * @class KeyCaptureWidget
 * @brief Widget that captures key presses for keybind assignment
 */
class KeybindEditorDialog::KeyCaptureWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KeyCaptureWidget(QWidget *parent = nullptr);
    
    QKeySequence capturedKey() const { return m_capturedKey; }
    void reset();

signals:
    void keyCaptured(const QKeySequence& keySequence);
    void captureCancelled();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QKeySequence m_capturedKey;
    Qt::KeyboardModifiers m_modifiers;
};

#endif // KEYBINDEDITORDIALOG_H
