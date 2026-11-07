#include "MainWindow.h"
#include "MainWindowHelpers.h"
#include "ViewportWidget.h"
#include "SceneDocument.h"
#include "SettingsDialog.h"
#include "Localization.h"

#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QAction>
#include <QActionGroup>
#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QEvent>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QSettings>
#include <QColor>
#include <memory>
#include <algorithm>

using namespace MainWindowUtil;
using namespace Localization;

namespace {

QPalette makeLightPalette() {
    // Explicit light palette: Fusion in Qt 6.5+ follows the system Windows theme,
    // so the default palette in dark system mode produces a dark UI.
    QPalette pal;
    pal.setColor(QPalette::Window,          QColor(240, 240, 240));
    pal.setColor(QPalette::WindowText,      QColor(25, 25, 25));
    pal.setColor(QPalette::Base,            QColor(255, 255, 255));
    pal.setColor(QPalette::AlternateBase,   QColor(245, 245, 245));
    pal.setColor(QPalette::Text,            QColor(20, 20, 20));
    pal.setColor(QPalette::Button,          QColor(240, 240, 240));
    pal.setColor(QPalette::ButtonText,      QColor(20, 20, 20));
    pal.setColor(QPalette::Light,           QColor(255, 255, 255));
    pal.setColor(QPalette::Midlight,        QColor(247, 247, 247));
    pal.setColor(QPalette::Mid,             QColor(190, 190, 190));
    pal.setColor(QPalette::Dark,            QColor(130, 130, 130));
    pal.setColor(QPalette::Shadow,          QColor(90, 90, 90));
    pal.setColor(QPalette::Highlight,       QColor(70, 130, 200));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    pal.setColor(QPalette::ToolTipBase,     QColor(255, 255, 220));
    pal.setColor(QPalette::ToolTipText,     QColor(20, 20, 20));
    pal.setColor(QPalette::PlaceholderText, QColor(120, 120, 120));

    pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(140, 140, 140));
    pal.setColor(QPalette::Disabled, QPalette::Text,       QColor(140, 140, 140));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(140, 140, 140));
    return pal;
}

QPalette makeDarkPalette() {
    QPalette pal(QColor(45, 45, 48));
    pal.setColor(QPalette::Window,          QColor(45, 45, 48));
    pal.setColor(QPalette::WindowText,      QColor(220, 220, 220));
    pal.setColor(QPalette::Base,            QColor(30, 30, 32));
    pal.setColor(QPalette::AlternateBase,   QColor(38, 38, 41));
    pal.setColor(QPalette::Text,            QColor(220, 220, 220));
    pal.setColor(QPalette::Button,          QColor(55, 55, 58));
    pal.setColor(QPalette::ButtonText,      QColor(220, 220, 220));
    pal.setColor(QPalette::Light,           QColor(70, 70, 74));
    pal.setColor(QPalette::Midlight,        QColor(60, 60, 63));
    pal.setColor(QPalette::Mid,             QColor(50, 50, 53));
    pal.setColor(QPalette::Dark,            QColor(30, 30, 32));
    pal.setColor(QPalette::Shadow,          QColor(15, 15, 16));
    pal.setColor(QPalette::Highlight,       QColor(70, 130, 200));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    pal.setColor(QPalette::ToolTipBase,     QColor(50, 50, 52));
    pal.setColor(QPalette::ToolTipText,     QColor(220, 220, 220));
    pal.setColor(QPalette::PlaceholderText, QColor(140, 140, 140));

    pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(110, 110, 110));
    pal.setColor(QPalette::Disabled, QPalette::Text,       QColor(110, 110, 110));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(110, 110, 110));
    return pal;
}

QColor lerpColor(const QColor& a, const QColor& b, qreal t) {
    return QColor(int(a.red()   + (b.red()   - a.red())   * t),
                  int(a.green() + (b.green() - a.green()) * t),
                  int(a.blue()  + (b.blue()  - a.blue())  * t));
}

QPalette lerpPalette(const QPalette& from, const QPalette& to, qreal t) {
    QPalette out = from;
    static const QPalette::ColorRole roles[] = {
        QPalette::Window, QPalette::WindowText,
        QPalette::Base, QPalette::AlternateBase,
        QPalette::Text, QPalette::Button, QPalette::ButtonText,
        QPalette::Light, QPalette::Midlight, QPalette::Mid,
        QPalette::Dark, QPalette::Shadow,
        QPalette::Highlight, QPalette::HighlightedText,
        QPalette::ToolTipBase, QPalette::ToolTipText,
        QPalette::PlaceholderText, QPalette::Link, QPalette::LinkVisited
    };
    const QPalette::ColorGroup groups[] = {
        QPalette::Active, QPalette::Inactive, QPalette::Disabled
    };
    for (QPalette::ColorRole r : roles)
        for (QPalette::ColorGroup g : groups)
            out.setColor(g, r, lerpColor(from.color(g, r), to.color(g, r), t));
    return out;
}

const char* kPrimKeys[] = {
    "Cube", "Plane", "Sphere", "Cylinder", "Cone", "Torus"
};

} // namespace

QSettings* MainWindow::makeSettings() const {
    // Settings live in settings.ini next to the program — we do not touch the registry
    const QString path =
        QCoreApplication::applicationDirPath() + "/settings.ini";
    return new QSettings(path, QSettings::IniFormat,
                         const_cast<MainWindow*>(this));
}

void MainWindow::saveSettings() const {
    std::unique_ptr<QSettings> s(makeSettings());
    s->setValue("ui/lang", Localization::code());
    s->setValue("ui/theme", m_themeIndex);
    s->setValue("ui/bgMode", m_bgMode);
    s->setValue("ui/bgCustom", m_bgCustom.isValid()
                                   ? m_bgCustom.name(QColor::HexRgb) : QString());
}

void MainWindow::animateToPalette(const QPalette& target) {
    // Smooth transition: interpolate all roles from current palette to target
    const QPalette from = QApplication::palette();

    auto* anim = new QVariantAnimation(this);
    anim->setDuration(300);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    connect(anim, &QVariantAnimation::valueChanged, this,
            [from, target](const QVariant& value) {
                QApplication::setPalette(
                    lerpPalette(from, target, value.toReal()));
            });
    connect(anim, &QVariantAnimation::finished, this, [target]() {
        QApplication::setPalette(target); // fix exact final value
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// Custom animations for popups: menus, tooltips, dropdowns fade in
// smoothly instead of appearing instantly
bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Show) {
        QWidget* w = qobject_cast<QWidget*>(watched);
        if (w && w->isWindow()) {
            const Qt::WindowType type = w->windowType();
            if (type == Qt::Popup || type == Qt::ToolTip) {
                auto* anim = new QPropertyAnimation(w, "windowOpacity", w);
                anim->setDuration(160);
                anim->setStartValue(0.0);
                anim->setEndValue(1.0);
                anim->setEasingCurve(QEasingCurve::OutCubic);
                anim->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Rebro");
    resize(1600, 900);

    // Fusion is required: the native Windows style partially ignores the palette
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    // Start with the light theme, regardless of Windows mode
    QApplication::setPalette(makeLightPalette());

    // Read language before building the UI so strings land in place immediately
    QString langCode;
    int theme = 0, bgMode = 0;
    QColor bgCustom;
    {
        std::unique_ptr<QSettings> s(makeSettings());
        langCode = s->value("ui/lang", "en").toString();
        theme = s->value("ui/theme", 0).toInt();
        bgMode = s->value("ui/bgMode", 0).toInt();
        bgCustom = QColor(s->value("ui/bgCustom").toString());
    }
    Localization::setLanguage(langCode);
    QApplication::setLayoutDirection(Localization::isRtl()
                                         ? Qt::RightToLeft : Qt::LeftToRight);

    m_viewport = new ViewportWidget;
    setCentralWidget(m_viewport);

    setupMenus();
    setupToolbar();
    setupDocks();

    m_statusLabel = new QLabel(Tr("Ready"), this);
    statusBar()->addWidget(m_statusLabel);

    connect(m_viewport, &ViewportWidget::selectionChanged,
            this, &MainWindow::onSelectionChanged);
    connect(m_viewport, &ViewportWidget::sceneChanged,
            this, &MainWindow::onSceneChanged);
    connect(m_viewport, &ViewportWidget::statusMessage,
            this, &MainWindow::onStatusMessage);

    m_backupTimer = new QTimer(this);
    connect(m_backupTimer, &QTimer::timeout, this, &MainWindow::onBackupTick);
    m_backupTimer->start(60 * 1000);

    updateUndoActions();

    // Custom animations for menus/tooltips — install filter on the whole app
    qApp->installEventFilter(this);

    // Restore theme and background from settings.ini
    m_themeIndex = std::clamp(theme, 0, 1);
    m_bgMode = std::clamp(bgMode, 0, 2);
    m_bgCustom = bgCustom;
    applyTheme(m_themeIndex);
    applyBackground(m_bgMode, m_bgCustom);
}

void MainWindow::applyTheme(int index) {
    animateToPalette(index == 1 ? makeDarkPalette() : makeLightPalette());
}

void MainWindow::applyBackground(int index, const QColor& custom) {
    if (index == 2) {
        if (custom.isValid()) {
            m_viewport->setClearColor(glm::vec3(custom.redF(),
                                                custom.greenF(),
                                                custom.blueF()));
        }
        return;
    }
    const bool dark = (index == 1);
    m_viewport->setClearColor(dark ? glm::vec3(0.13f, 0.13f, 0.15f)
                                   : glm::vec3(0.96f, 0.95f, 0.92f));
}

void MainWindow::setupMenus() {
    m_menuFile = menuBar()->addMenu("&" + Tr("File"));
    m_actNew     = m_menuFile->addAction(Tr("New Scene"), QKeySequence::New,
                                         this, &MainWindow::onNewScene);
    m_actOpen    = m_menuFile->addAction(Tr("Open..."), QKeySequence::Open,
                                         this, &MainWindow::onOpenScene);
    m_actSave    = m_menuFile->addAction(Tr("Save"), QKeySequence::Save,
                                         this, &MainWindow::onSaveScene);
    m_actSaveAs  = m_menuFile->addAction(Tr("Save As..."),
                                         QKeySequence::SaveAs,
                                         this, &MainWindow::onSaveSceneAs);
    m_actExport  = m_menuFile->addAction(Tr("Export..."), QKeySequence("Ctrl+E"),
                                         this, &MainWindow::onExportScene);
    m_menuFile->addSeparator();
    m_actSettings = m_menuFile->addAction(Tr("Settings..."),
                                          QKeySequence::Preferences,
                                          this, &MainWindow::onSettings);
    m_menuFile->addSeparator();
    m_actExit    = m_menuFile->addAction(Tr("Exit"), QKeySequence::Quit,
                                         &QApplication::quit);

    m_menuEdit = menuBar()->addMenu("&" + Tr("Edit"));
    m_undoAction = m_menuEdit->addAction(Tr("Undo"), QKeySequence::Undo,
                                         this, &MainWindow::onUndo);
    m_redoAction = m_menuEdit->addAction(Tr("Redo"), QKeySequence::Redo,
                                         this, &MainWindow::onRedo);
    m_menuEdit->addSeparator();
    m_actDelete  = m_menuEdit->addAction(Tr("Delete Object"),
                                         QKeySequence::Delete,
                                         m_viewport, &ViewportWidget::deleteSelected);

    m_menuAdd = menuBar()->addMenu("&" + Tr("Add"));
    struct PrimitiveEntry { const char* key; const char* shape; };
    const PrimitiveEntry prims[] = {
        {"Cube",      "cube"},     {"Plane",     "plane"},
        {"Sphere",    "sphere"},   {"Cylinder",  "cylinder"},
        {"Cone",      "cone"},     {"Torus",     "torus"}
    };
    for (const PrimitiveEntry& p : prims) {
        QString shape = p.shape;
        QAction* act = m_menuAdd->addAction(Tr(p.key), this,
                                            [this, shape] { addPrimitive(shape); });
        m_primActions.append(act);
    }
    m_menuAdd->addSeparator();
    m_actImportModelMenu =
        m_menuAdd->addAction(Tr("Model from File..."), QKeySequence("Ctrl+2"),
                             this, &MainWindow::onImportModel);

    m_menuHelp = menuBar()->addMenu("&" + Tr("Help"));
    m_actHelp    = m_menuHelp->addAction(Tr("Help..."), QKeySequence("F1"),
                                         this, &MainWindow::onHelp);
    m_actAbout   = m_menuHelp->addAction(Tr("About Rebro"), this,
                                         &MainWindow::onAbout);
}

void MainWindow::setupToolbar() {
    QToolBar* toolbar = addToolBar(Tr("Main"));
    toolbar->setMovable(false);

    m_actAddCube = toolbar->addAction(Tr("Add Cube"));
    connect(m_actAddCube, &QAction::triggered, this,
            [this] { addPrimitive("cube"); });
    m_actImportToolbar = toolbar->addAction(Tr("Import Model"));
    connect(m_actImportToolbar, &QAction::triggered, this,
            &MainWindow::onImportModel);
    m_actExportToolbar = toolbar->addAction(Tr("Export"));
    connect(m_actExportToolbar, &QAction::triggered, this,
            &MainWindow::onExportScene);

    toolbar->addSeparator();

    auto addModeAction = [&](const char* key, ViewportWidget::TransformMode mode,
                             QAction** slot) {
        QAction* act = toolbar->addAction(Tr(key));
        act->setCheckable(true);
        connect(act, &QAction::triggered, this, [this, act, mode] {
            m_viewport->setTransformMode(mode);
            act->setChecked(true);
        });
        *slot = act;
        return act;
    };

    QActionGroup* modeGroup = new QActionGroup(this);
    m_actMove     = addModeAction("Move (G)",        ViewportWidget::TransformMode::Move,     &m_actMove);
    m_actRotate   = addModeAction("Rotate (R)",      ViewportWidget::TransformMode::Rotate,   &m_actRotate);
    m_actScale    = addModeAction("Scale (Shift+S)", ViewportWidget::TransformMode::Scale,    &m_actScale);
    m_actVertices = addModeAction("Vertices (V)",    ViewportWidget::TransformMode::Vertices, &m_actVertices);
    modeGroup->addAction(m_actMove);
    modeGroup->addAction(m_actRotate);
    modeGroup->addAction(m_actScale);
    modeGroup->addAction(m_actVertices);
    m_actMove->setChecked(true);

    toolbar->addSeparator();

    m_fastCheck = new QCheckBox(Tr("Fast Camera"), toolbar);
    toolbar->addWidget(m_fastCheck);

    m_speedSpin = new QDoubleSpinBox(toolbar);
    m_speedSpin->setRange(0.1, 100.0);
    m_speedSpin->setValue(5.0);
    m_speedSpin->setSuffix(" " + Tr("u/s"));
    m_speedSpin->setToolTip(Tr("WASD camera speed (units per second)"));
    m_speedSpin->setEnabled(false);
    toolbar->addWidget(m_speedSpin);

    m_backupCheck = new QCheckBox(Tr("Autobackup"), toolbar);
    m_backupCheck->setChecked(true);
    m_backupCheck->setToolTip(Tr("Once a minute the scene is saved to the backup folder next to the program"));
    toolbar->addWidget(m_backupCheck);

    connect(m_speedSpin, &QDoubleSpinBox::valueChanged, m_viewport,
            [this](double v) { m_viewport->setCameraSpeed(float(v)); });
    connect(m_fastCheck, &QCheckBox::toggled, this, &MainWindow::onFastModeToggled);
    connect(m_backupCheck, &QCheckBox::toggled, this, &MainWindow::onBackupToggled);
}

void MainWindow::setupDocks() {
    m_outlinerDock = new QDockWidget(Tr("Outliner"), this);
    m_outlinerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_outliner = new QTreeWidget(m_outlinerDock);
    m_outliner->setHeaderLabel(Tr("Objects"));
    m_outlinerDock->setWidget(m_outliner);
    addDockWidget(Qt::LeftDockWidgetArea, m_outlinerDock);
    connect(m_outliner, &QTreeWidget::itemSelectionChanged,
            this, &MainWindow::onOutlinerSelectionChanged);

    m_propsDock = new QDockWidget(Tr("Properties"), this);
    auto* panel = new QWidget(m_propsDock);
    auto* layout = new QVBoxLayout(panel);

    m_grpName = new QGroupBox(Tr("Name"), panel);
    auto* nameLayout = new QVBoxLayout(m_grpName);
    m_nameEdit = new QLineEdit(m_grpName);
    nameLayout->addWidget(m_nameEdit);
    layout->addWidget(m_grpName);

    auto makeRow = [panel](const char* key, QDoubleSpinBox** slots3,
                           int decimals) {
        auto* group = new QGroupBox(Tr(key), panel);
        auto* row = new QHBoxLayout(group);
        for (int i = 0; i < 3; ++i) {
            slots3[i] = new QDoubleSpinBox(group);
            slots3[i]->setRange(-9999, 9999);
            slots3[i]->setDecimals(decimals);
            slots3[i]->setSingleStep(0.1);
            slots3[i]->setFrame(false);
            row->addWidget(slots3[i]);
        }
        return group;
    };

    m_grpPos   = makeRow("Position", m_propPos, 1);
    m_grpRot   = makeRow("Rotation", m_propRot, 0);
    m_grpScale = makeRow("Scale", m_propScale, 1);
    layout->addWidget(m_grpPos);
    layout->addWidget(m_grpRot);
    layout->addWidget(m_grpScale);

    m_grpMat = new QGroupBox(Tr("Material"), panel);
    auto* matLayout = new QVBoxLayout(m_grpMat);
    auto* colorRow = new QWidget(m_grpMat);
    auto* colorLay = new QHBoxLayout(colorRow);
    colorLay->setContentsMargins(0, 0, 0, 0);
    m_colorEdit = new QComboBox(colorRow);
    m_colorEdit->setEditable(true);
    m_colorEdit->addItem("#FFFFFF");
    m_colorEdit->addItem("#FF5A36");
    m_colorEdit->addItem("#7FCF6B");
    m_colorEdit->addItem("#4D9DE0");
    m_colorEdit->addItem("#F2C14E");
    colorLay->addWidget(m_colorEdit);
    auto* colorPick = new QPushButton("...", colorRow);
    colorPick->setFixedWidth(28);
    colorLay->addWidget(colorPick);
    matLayout->addWidget(colorRow);
    m_texButton = new QPushButton(Tr("Texture: none"), m_grpMat);
    matLayout->addWidget(m_texButton);
    layout->addWidget(m_grpMat);

    layout->addStretch();

    for (int i = 0; i < 3; ++i) {
        connect(m_propPos[i], &QDoubleSpinBox::valueChanged, this, &MainWindow::onPropChanged);
        connect(m_propRot[i], &QDoubleSpinBox::valueChanged, this, &MainWindow::onPropChanged);
        connect(m_propScale[i], &QDoubleSpinBox::valueChanged, this, &MainWindow::onPropChanged);
    }
    connect(m_nameEdit, &QLineEdit::textEdited, this, &MainWindow::onNameEdited);
    connect(m_colorEdit, &QComboBox::currentTextChanged,
            this, [this](const QString&) { onColorClicked(); });
    connect(colorPick, &QPushButton::clicked,
            this, [this]() { onColorDialog(); });
    connect(m_texButton, &QPushButton::clicked, this, &MainWindow::onTextureClicked);

    m_propsDock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, m_propsDock);
}

// --- Live language switching: retranslate the whole UI without a restart ---

void MainWindow::retranslateUi() {
    m_menuFile->setTitle("&" + Tr("File"));
    m_actNew->setText(Tr("New Scene"));
    m_actOpen->setText(Tr("Open..."));
    m_actSave->setText(Tr("Save"));
    m_actSaveAs->setText(Tr("Save As..."));
    m_actExport->setText(Tr("Export..."));
    m_actSettings->setText(Tr("Settings..."));
    m_actExit->setText(Tr("Exit"));

    m_menuEdit->setTitle("&" + Tr("Edit"));
    m_undoAction->setText(Tr("Undo"));
    m_redoAction->setText(Tr("Redo"));
    m_actDelete->setText(Tr("Delete Object"));

    m_menuAdd->setTitle("&" + Tr("Add"));
    for (int i = 0; i < m_primActions.size() &&
                    i < int(sizeof(kPrimKeys) / sizeof(kPrimKeys[0])); ++i)
        m_primActions[i]->setText(Tr(kPrimKeys[i]));
    m_actImportModelMenu->setText(Tr("Model from File..."));

    m_menuHelp->setTitle("&" + Tr("Help"));
    m_actHelp->setText(Tr("Help..."));
    m_actAbout->setText(Tr("About Rebro"));

    m_actAddCube->setText(Tr("Add Cube"));
    m_actImportToolbar->setText(Tr("Import Model"));
    m_actExportToolbar->setText(Tr("Export"));
    m_actMove->setText(Tr("Move (G)"));
    m_actRotate->setText(Tr("Rotate (R)"));
    m_actScale->setText(Tr("Scale (Shift+S)"));
    m_actVertices->setText(Tr("Vertices (V)"));
    m_fastCheck->setText(Tr("Fast Camera"));
    m_speedSpin->setSuffix(" " + Tr("u/s"));
    m_speedSpin->setToolTip(Tr("WASD camera speed (units per second)"));
    m_backupCheck->setText(Tr("Autobackup"));
    m_backupCheck->setToolTip(Tr("Once a minute the scene is saved to the backup folder next to the program"));

    m_outlinerDock->setWindowTitle(Tr("Outliner"));
    m_propsDock->setWindowTitle(Tr("Properties"));
    m_outliner->setHeaderLabel(Tr("Objects"));
    m_grpName->setTitle(Tr("Name"));
    m_grpPos->setTitle(Tr("Position"));
    m_grpRot->setTitle(Tr("Rotation"));
    m_grpScale->setTitle(Tr("Scale"));
    m_grpMat->setTitle(Tr("Material"));
    m_texButton->setText(Tr("Texture: none"));

    // Arabic — right-to-left layout, others — left-to-right
    QApplication::setLayoutDirection(Localization::isRtl()
                                         ? Qt::RightToLeft : Qt::LeftToRight);
}

// --- Settings ---

void MainWindow::onSettings() {
    SettingsDialog dlg(m_themeIndex, m_bgMode, m_bgCustom, this);
    connect(&dlg, &SettingsDialog::themeChanged,
            this, &MainWindow::onThemeActivated);
    connect(&dlg, &SettingsDialog::backgroundChanged,
            this, &MainWindow::onBackgroundChanged);
    connect(&dlg, &SettingsDialog::languageChanged,
            this, &MainWindow::onLanguageChanged);
    dlg.exec();
}

void MainWindow::onThemeActivated(int index) {
    m_themeIndex = std::clamp(index, 0, 1);
    applyTheme(m_themeIndex);
    m_statusLabel->setText(index == 1 ? Tr("Theme: Dark") : Tr("Theme: Light"));
    saveSettings();
}

void MainWindow::onBackgroundChanged(int mode, const QColor& color) {
    m_bgMode = std::clamp(mode, 0, 2);
    if (m_bgMode == 2 && color.isValid())
        m_bgCustom = color;
    applyBackground(m_bgMode, m_bgCustom);
    if (m_bgMode == 2) {
        m_statusLabel->setText(Tr("Scene Background") + ": " +
                               (m_bgCustom.isValid()
                                    ? m_bgCustom.name(QColor::HexRgb)
                                    : QString("(230,230,225)")));
    } else {
        m_statusLabel->setText(m_bgMode == 1 ? Tr("Scene Background: Dark")
                                             : Tr("Scene Background: Light"));
    }
    saveSettings();
}

void MainWindow::onLanguageChanged(const QString& code) {
    if (code == Localization::code()) return;
    Localization::setLanguage(code);
    retranslateUi();
    saveSettings();
    m_statusLabel->setText(code == "ru" ? Tr("Ready") : Tr("Ready"));
}

void MainWindow::onStatusMessage(const QString& text) {
    m_statusLabel->setText(text);
    syncModeActions(text);
}

// "Mode: ..." strings come from ViewportWidget and were already translated
void MainWindow::syncModeActions(const QString& statusText) {
    if (statusText.startsWith("Mode: Move")) {
        m_actMove->setChecked(true);
    } else if (statusText.startsWith("Mode: Rotate")) {
        m_actRotate->setChecked(true);
    } else if (statusText.startsWith("Mode: Scale")) {
        m_actScale->setChecked(true);
    } else if (statusText.startsWith("Mode: Vertices")) {
        m_actVertices->setChecked(true);
    }
}