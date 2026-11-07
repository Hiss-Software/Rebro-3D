#include "MainWindow.h"
#include "MainWindowHelpers.h"
#include "ViewportWidget.h"
#include "MeshImporter.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QColorDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDoubleSpinBox>

using namespace MainWindowUtil;

// --- Scene and panels ---

void MainWindow::onFastModeToggled(bool on) {
    m_speedSpin->setEnabled(on);
    m_viewport->setFastMode(on);
    if (on)
        m_statusLabel->setText(QString("Fast camera: %1 u/s")
                                   .arg(m_speedSpin->value(), 0, 'f', 1));
    else
        m_statusLabel->setText("Normal camera");
}

void MainWindow::addPrimitive(const QString& shape) {
    int index = m_viewport->addObject(
        QString("%1.%2").arg(shape).arg(m_viewport->objects().size() + 1),
        nullptr, QString(), shape);
    m_statusLabel->setText(QString("Added %1")
                               .arg(m_viewport->objects()[index].name));
    updateUndoActions();
}

void MainWindow::onImportModel() {
    QString path = QFileDialog::getOpenFileName(
        this, "Import Model", QString(), tr(kModelFilter));
    if (path.isEmpty()) return;

    auto mesh = std::make_shared<MeshData>();
    QString error;
    if (!MeshImporter::loadFromFile(path, mesh, &error)) {
        QMessageBox::warning(this, "Rebro", error);
        return;
    }

    QString name = QFileInfo(path).completeBaseName();
    m_viewport->addObject(name, mesh, path, QString());
    float size = glm::length(mesh->boundsMax - mesh->boundsMin);
    m_statusLabel->setText(
        QString("Imported %1 (%2 vertices, size %3 units)")
            .arg(name).arg(mesh->vertices.size()).arg(size, 0, 'f', 1));
    updateUndoActions();
}

void MainWindow::onNameEdited(const QString& text) {
    m_viewport->renameSelected(text);
}

void MainWindow::onColorClicked() {
    if (m_updatingProps) return;
    int sel = m_viewport->selectedIndex();
    if (sel < 0) return;
    glm::vec3 c;
    if (!parseHexColor(m_colorEdit->currentText(), c)) return;
    m_viewport->setMaterialColorTracked(sel, c);
}

void MainWindow::onColorDialog() {
    int sel = m_viewport->selectedIndex();
    if (sel < 0) return;
    const auto& c = m_viewport->objects()[sel].material.diffuse;
    QColor initial = QColor::fromRgbF(c.r, c.g, c.b);
    QColor picked = QColorDialog::getColor(initial, this, "Material Color");
    if (!picked.isValid()) return;
    m_viewport->setMaterialColorTracked(sel, glm::vec3(
        float(picked.redF()), float(picked.greenF()), float(picked.blueF())));
    m_updatingProps = true;
    m_colorEdit->setCurrentText(colorToHex(
        m_viewport->objects()[sel].material.diffuse));
    m_updatingProps = false;
}

void MainWindow::onTextureClicked() {
    int sel = m_viewport->selectedIndex();
    if (sel < 0) return;
    if (!m_viewport->objects()[sel].material.texturePath.isEmpty()) {
        m_viewport->setMaterialTextureTracked(sel, QString());
        m_statusLabel->setText("Texture removed");
        updateProps();
        return;
    }
    QString path = QFileDialog::getOpenFileName(
        this, "Texture", QString(), tr(kTextureFilter));
    if (path.isEmpty()) return;
    m_viewport->setMaterialTextureTracked(sel, path);
    m_statusLabel->setText(QString("Texture: %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::onSelectionChanged(int index) {
    m_updatingOutliner = true;
    if (index >= 0)
        m_outliner->setCurrentItem(m_outliner->topLevelItem(index));
    else
        m_outliner->clearSelection();
    m_updatingOutliner = false;
    updateProps();
}

void MainWindow::onSceneChanged() {
    m_updatingOutliner = true;
    m_outliner->clear();
    const auto& objs = m_viewport->objects();
    for (int i = 0; i < objs.size(); ++i)
        m_outliner->addTopLevelItem(new QTreeWidgetItem({objs[i].name}));
    int sel = m_viewport->selectedIndex();
    if (sel >= 0 && sel < m_outliner->topLevelItemCount())
        m_outliner->setCurrentItem(m_outliner->topLevelItem(sel));
    m_updatingOutliner = false;
    updateProps();
}

void MainWindow::onOutlinerSelectionChanged() {
    if (m_updatingOutliner) return;
    QList<QTreeWidgetItem*> sel = m_outliner->selectedItems();
    m_viewport->setSelected(sel.isEmpty() ? -1
                          : m_outliner->indexOfTopLevelItem(sel.first()));
}

void MainWindow::updateProps() {
    m_updatingProps = true;
    int sel = m_viewport->selectedIndex();
    bool has = sel >= 0;
    m_nameEdit->setEnabled(has);
    m_nameEdit->setText(has ? m_viewport->objects()[sel].name : QString());
    m_colorEdit->setEnabled(has);
    m_texButton->setEnabled(has);
    for (int i = 0; i < 3; ++i) {
        m_propPos[i]->setEnabled(has);
        m_propRot[i]->setEnabled(has);
        m_propScale[i]->setEnabled(has);
    }
    if (has) {
        const SceneObject& obj = m_viewport->objects()[sel];
        m_propPos[0]->setValue(obj.position.x);
        m_propPos[1]->setValue(obj.position.y);
        m_propPos[2]->setValue(obj.position.z);
        m_propRot[0]->setValue(obj.rotation.x);
        m_propRot[1]->setValue(obj.rotation.y);
        m_propRot[2]->setValue(obj.rotation.z);
        m_propScale[0]->setValue(obj.scale.x);
        m_propScale[1]->setValue(obj.scale.y);
        m_propScale[2]->setValue(obj.scale.z);
        m_colorEdit->setCurrentText(colorToHex(obj.material.diffuse));
        QString tex = obj.material.texturePath;
        m_texButton->setText(tex.isEmpty() ? "Texture: none"
                                           : "Texture: " + QFileInfo(tex).fileName());
    }
    m_updatingProps = false;
}

void MainWindow::onPropChanged() {
    if (m_updatingProps) return;
    int sel = m_viewport->selectedIndex();
    if (sel < 0) return;
    m_viewport->setObjectTransformTracked(sel,
        glm::vec3(float(m_propPos[0]->value()), float(m_propPos[1]->value()), float(m_propPos[2]->value())),
        glm::vec3(float(m_propRot[0]->value()), float(m_propRot[1]->value()), float(m_propRot[2]->value())),
        glm::max(glm::vec3(0.01f), glm::vec3(float(m_propScale[0]->value()),
                                             float(m_propScale[1]->value()),
                                             float(m_propScale[2]->value()))));
    updateUndoActions();
}