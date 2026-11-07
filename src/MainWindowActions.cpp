#include "MainWindow.h"
#include "MainWindowHelpers.h"
#include "ViewportWidget.h"
#include "SceneDocument.h"
#include "MeshExporter.h"

#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QLabel>
#include <QCheckBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QTextBrowser>
#include <QDialogButtonBox>

using namespace MainWindowUtil;

// --- File ---

void MainWindow::onNewScene() {
    m_viewport->clearScene();
    m_currentFilePath.clear();
    updateTitle();
    m_statusLabel->setText("Scene cleared");
}

void MainWindow::onOpenScene() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Project", QString(), tr(kFileFilter));
    if (path.isEmpty()) return;

    QString error;
    if (!m_viewport->loadScene(path, &error)) {
        QMessageBox::warning(this, "Rebro", error);
        return;
    }
    m_currentFilePath = path;
    updateTitle();
}

void MainWindow::onSaveScene() {
    if (m_currentFilePath.isEmpty()) {
        onSaveSceneAs();
        return;
    }
    QString error;
    if (!m_viewport->saveScene(m_currentFilePath, &error))
        QMessageBox::warning(this, "Rebro", error);
}

void MainWindow::onSaveSceneAs() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Project", "scene.r3dp", tr(kFileFilter));
    if (path.isEmpty()) return;
    path = ensureExtension(path, "r3dp");

    QString error;
    if (!m_viewport->saveScene(path, &error)) {
        QMessageBox::warning(this, "Rebro", error);
        return;
    }
    m_currentFilePath = path;
    updateTitle();
}

void MainWindow::onExportScene() {
    QString selectedFilter;
    QString path = QFileDialog::getSaveFileName(
        this, "Export Scene", QString(), tr(kExportFilter), &selectedFilter);
    if (path.isEmpty()) return;

    QString formatId;
    if (selectedFilter.contains("GLB")) {
        formatId = "glb2";
        path = ensureExtension(path, "glb");
    } else if (selectedFilter.contains("glTF")) {
        formatId = "gltf2";
        path = ensureExtension(path, "gltf");
    } else {
        formatId = "obj";
        path = ensureExtension(path, "obj");
    }

    QString error;
    if (!MeshExporter::exportToFile(m_viewport->objects(), path, formatId, &error)) {
        QMessageBox::warning(this, "Rebro", error);
        return;
    }
    m_statusLabel->setText(QString("Exported: %1").arg(path));
}

void MainWindow::onUndo() {
    m_viewport->undo();
    updateUndoActions();
}

void MainWindow::onRedo() {
    m_viewport->redo();
    updateUndoActions();
}

void MainWindow::updateUndoActions() {
    m_undoAction->setEnabled(m_viewport->canUndo());
    m_redoAction->setEnabled(m_viewport->canRedo());
}

// --- Autobackup and Title ---

void MainWindow::onBackupTick() {
    if (!m_backupCheck->isChecked()) return;
    if (m_viewport->objects().isEmpty()) return;
    QDir dir(QCoreApplication::applicationDirPath());
    dir.mkpath("backup");

    m_backupSlot = (m_backupSlot >= 10) ? 1 : m_backupSlot + 1;
    QString path = dir.filePath(
        QString("backup/model_backup%1.r3dp").arg(m_backupSlot));

    QString error;
    if (!SceneDocument::save(m_viewport->objects(), path, &error)) {
        m_statusLabel->setText("Backup failed: " + error);
        return;
    }
}

void MainWindow::onBackupToggled(bool on) {
    if (on)
        m_backupTimer->start(60 * 1000);
    else
        m_backupTimer->stop();
    m_statusLabel->setText(on ? "Autobackup enabled" : "Autobackup disabled");
}

void MainWindow::updateTitle() {
    if (m_currentFilePath.isEmpty())
        setWindowTitle("Rebro");
    else
        setWindowTitle(QString("Rebro — %1").arg(m_currentFilePath));
}

// --- Help ---

void MainWindow::onHelp() {
    QDialog dlg(this);
    dlg.setWindowTitle("Help — Rebro");
    dlg.resize(560, 660);

    auto* layout = new QVBoxLayout(&dlg);
    auto* browser = new QTextBrowser(&dlg);
    browser->setOpenExternalLinks(false);

    browser->setHtml(QStringLiteral(
        "<h2>Rebro — Quick Help</h2>"

        "<h3>Modes</h3>"
        "<table width='100%'>"
        "<tr><td><b>G</b></td><td>Move object</td></tr>"
        "<tr><td><b>R</b></td><td>Rotate object</td></tr>"
        "<tr><td><b>Shift+S</b></td><td>Scale object</td></tr>"
        "<tr><td><b>V</b></td><td>Edit vertices</td></tr>"
        "</table>"
        "<p>In G/R/S modes, drag the colored gizmo axis. "
        "In V mode, vertices are shown as cyan dots: hover over a dot "
        "and drag it with the mouse. The face under the cursor is highlighted in orange.</p>"

        "<h3>Camera</h3>"
        "<p>Camera flies with <b>WASD</b>, keys <b>Q</b>/<b>E</b> — down "
        "and up. Orbit — hold middle mouse button, zoom — scroll wheel. "
        "Speed is set by the 'u/s' field on the toolbar — "
        "active when 'Fast camera' is checked.</p>"

        "<h3>Files</h3>"
        "<table width='100%'>"
        "<tr><td><b>Ctrl+N</b></td><td>New scene</td></tr>"
        "<tr><td><b>Ctrl+O</b></td><td>Open project (.r3dp)</td></tr>"
        "<tr><td><b>Ctrl+S</b></td><td>Save</td></tr>"
        "<tr><td><b>Ctrl+Shift+S</b></td><td>Save As</td></tr>"
        "<tr><td><b>Ctrl+E</b></td><td>Export (OBJ, glTF, GLB)</td></tr>"
        "<tr><td><b>Ctrl+2</b></td><td>Import model from file</td></tr>"
        "</table>"
        "<p>The project is saved in .r3dp format: primitives are stored together "
        "with all vertex edits, imported models — by reference "
        "to the source file (keep the model next to the project).</p>"

        "<h3>Editing</h3>"
        "<table width='100%'>"
        "<tr><td><b>Ctrl+Z</b></td><td>Undo</td></tr>"
        "<tr><td><b>Ctrl+Y</b></td><td>Redo</td></tr>"
        "<tr><td><b>Delete</b></td><td>Delete selected object</td></tr>"
        "</table>"

        "<h3>Interface</h3>"
        "<p>On the left — outliner with a list of objects, on the right — properties: name, "
        "position, rotation, scale, color, and texture. Select an object by clicking "
        "in the viewport or in the outliner.</p>"

        "<h3>Autobackup</h3>"
        "<p>Once a minute the scene is saved to the <b>backup</b> folder next to "
        "the program (files model_backup1…10.r3dp, oldest are overwritten). "
        "Can be disabled with the checkbox on the toolbar.</p>"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    layout->addWidget(browser);
    layout->addWidget(buttons);
    dlg.exec();
}

void MainWindow::onAbout() {
    QMessageBox::about(
        this, "About Rebro",
        "<b>Rebro</b> — 3D editor. MVP.<br><br>"
        "OpenGL rendering, precise triangle picking, gizmos "
        "(move / rotate / scale), vertex editing "
        "with normal recalculation, full undo/redo, custom "
        ".r3dp project format with embedded geometry, OBJ/FBX/glTF import, "
        "export to OBJ/glTF/GLB, autobackup.<br><br>"
        "Built with Qt 6 and assimp.");
}