#pragma once

#include <QMainWindow>
#include <QString>
#include <QColor>
#include <QList>

#include <glm/glm.hpp>

class ViewportWidget;
class QAction;
class QMenu;
class QDockWidget;
class QGroupBox;
class QTreeWidget;
class QLabel;
class QLineEdit;
class QCheckBox;
class QDoubleSpinBox;
class QComboBox;
class QPushButton;
class QTimer;
class QSettings;
class QEvent;
class QPalette;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onNewScene();
    void onOpenScene();
    void onSaveScene();
    void onSaveSceneAs();
    void onExportScene();
    void onSettings();
    void onUndo();
    void onRedo();
    void onHelp();
    void onAbout();
    void onBackupTick();
    void onBackupToggled(bool on);
    void onStatusMessage(const QString& text);
    void onThemeActivated(int index);
    void onBackgroundChanged(int mode, const QColor& color);
    void onLanguageChanged(const QString& code);

private:
    void setupMenus();
    void setupToolbar();
    void setupDocks();
    void retranslateUi();
    void updateTitle();
    void updateUndoActions();
    void updateProps();
    void syncModeActions(const QString& statusText);

    void applyTheme(int index);
    void applyBackground(int index, const QColor& custom);
    void animateToPalette(const QPalette& target);
    void saveSettings() const;
    QSettings* makeSettings() const;

    void addPrimitive(const QString& shape);
    void onImportModel();
    void onFastModeToggled(bool on);
    void onNameEdited(const QString& text);
    void onColorClicked();
    void onColorDialog();
    void onTextureClicked();
    void onSelectionChanged(int index);
    void onSceneChanged();
    void onOutlinerSelectionChanged();
    void onPropChanged();

    ViewportWidget* m_viewport = nullptr;
    QLabel* m_statusLabel = nullptr;
    QTreeWidget* m_outliner = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QCheckBox* m_fastCheck = nullptr;
    QDoubleSpinBox* m_speedSpin = nullptr;
    QCheckBox* m_backupCheck = nullptr;
    QComboBox* m_colorEdit = nullptr;
    QPushButton* m_texButton = nullptr;
    QDoubleSpinBox* m_propPos[3] = {};
    QDoubleSpinBox* m_propRot[3] = {};
    QDoubleSpinBox* m_propScale[3] = {};
    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;
    QAction* m_actMove = nullptr;
    QAction* m_actRotate = nullptr;
    QAction* m_actScale = nullptr;
    QAction* m_actVertices = nullptr;
    QTimer* m_backupTimer = nullptr;

    // For live language switching: store everything that needs retranslation
    QMenu* m_menuFile = nullptr;
    QMenu* m_menuEdit = nullptr;
    QMenu* m_menuAdd = nullptr;
    QMenu* m_menuHelp = nullptr;
    QAction* m_actNew = nullptr;
    QAction* m_actOpen = nullptr;
    QAction* m_actSave = nullptr;
    QAction* m_actSaveAs = nullptr;
    QAction* m_actExport = nullptr;
    QAction* m_actSettings = nullptr;
    QAction* m_actExit = nullptr;
    QAction* m_actDelete = nullptr;
    QList<QAction*> m_primActions;
    QAction* m_actImportModelMenu = nullptr;
    QAction* m_actHelp = nullptr;
    QAction* m_actAbout = nullptr;
    QAction* m_actAddCube = nullptr;
    QAction* m_actImportToolbar = nullptr;
    QAction* m_actExportToolbar = nullptr;
    QDockWidget* m_outlinerDock = nullptr;
    QDockWidget* m_propsDock = nullptr;
    QGroupBox* m_grpName = nullptr;
    QGroupBox* m_grpPos = nullptr;
    QGroupBox* m_grpRot = nullptr;
    QGroupBox* m_grpScale = nullptr;
    QGroupBox* m_grpMat = nullptr;

    int m_backupSlot = 0;
    QString m_currentFilePath;
    int m_themeIndex = 0;
    int m_bgMode = 0;
    QColor m_bgCustom;
    bool m_updatingProps = false;
    bool m_updatingOutliner = false;
};