#include "ViewportWidget.h"

// --- Undo/Redo Commands ---

ViewportWidget::TransformCmd::TransformCmd(ViewportWidget* vp, int idx,
                                           const glm::vec3* before,
                                           const glm::vec3* after, qint64 ts)
    : CmdBase(vp), m_idx(idx), m_ts(ts)
{
    for (int i = 0; i < 3; ++i) {
        m_before[i] = before[i];
        m_after[i] = after[i];
    }
}

void ViewportWidget::TransformCmd::undo() {
    vp->applyTransformDirect(m_idx, m_before[0], m_before[1], m_before[2]);
}

void ViewportWidget::TransformCmd::redo() {
    vp->applyTransformDirect(m_idx, m_after[0], m_after[1], m_after[2]);
}

bool ViewportWidget::TransformCmd::tryMerge(ICommand* incoming) {
    auto* o = dynamic_cast<TransformCmd*>(incoming);
    if (!o || o->m_idx != m_idx) return false;
    qint64 now = vp->m_clock.elapsed();
    if (now - m_ts > 1500) return false;
    for (int i = 0; i < 3; ++i) m_after[i] = o->m_after[i];
    m_ts = now;
    return true;
}

ViewportWidget::AddObjectCmd::AddObjectCmd(ViewportWidget* vp, int idx, SceneObject obj)
    : CmdBase(vp), m_idx(idx), m_obj(std::move(obj)) {}

void ViewportWidget::AddObjectCmd::undo()    { vp->removeObjectAt(m_idx); }
void ViewportWidget::AddObjectCmd::redo()    { vp->insertObjectAt(m_idx, m_obj); }

ViewportWidget::DeleteObjectCmd::DeleteObjectCmd(ViewportWidget* vp, int idx, SceneObject obj)
    : CmdBase(vp), m_idx(idx), m_obj(std::move(obj)) {}

void ViewportWidget::DeleteObjectCmd::undo() { vp->insertObjectAt(m_idx, m_obj); }
void ViewportWidget::DeleteObjectCmd::redo() { vp->removeObjectAt(m_idx); }

ViewportWidget::RenameCmd::RenameCmd(ViewportWidget* vp, int idx,
                                     QString before, QString after)
    : CmdBase(vp), m_idx(idx), m_before(std::move(before)), m_after(std::move(after)) {}

void ViewportWidget::RenameCmd::undo() { vp->setNameDirect(m_idx, m_before); }
void ViewportWidget::RenameCmd::redo() { vp->setNameDirect(m_idx, m_after); }

ViewportWidget::MaterialCmd::MaterialCmd(ViewportWidget* vp, int idx,
                                         const Material& before,
                                         const Material& after, qint64 ts)
    : CmdBase(vp), m_idx(idx), m_before(before), m_after(after), m_ts(ts) {}

void ViewportWidget::MaterialCmd::undo() { vp->applyMaterialDirect(m_idx, m_before); }
void ViewportWidget::MaterialCmd::redo() { vp->applyMaterialDirect(m_idx, m_after); }

bool ViewportWidget::MaterialCmd::tryMerge(ICommand* incoming) {
    auto* o = dynamic_cast<MaterialCmd*>(incoming);
    if (!o || o->m_idx != m_idx) return false;
    qint64 now = vp->m_clock.elapsed();
    if (now - m_ts > 1500) return false;
    m_after = o->m_after;
    m_ts = now;
    return true;
}

void ViewportWidget::pushCommand(std::shared_ptr<ICommand> cmd,
                                 bool alreadyApplied, bool allowMerge) {
    m_stack.push(std::move(cmd), alreadyApplied, allowMerge);
    emit undoRedoChanged();
}

void ViewportWidget::applyTransformDirect(int index, const glm::vec3& p,
                                          const glm::vec3& r, const glm::vec3& s)
{
    if (index < 0 || index >= m_objects.size()) return;
    m_objects[index].position = p;
    m_objects[index].rotation = r;
    m_objects[index].scale = s;
    emit sceneChanged();
    update();
}

void ViewportWidget::insertObjectAt(int index, const SceneObject& obj) {
    m_objects.insert(index, obj);
    emit sceneChanged();
    update();
}

void ViewportWidget::removeObjectAt(int index) {
    if (index < 0 || index >= m_objects.size()) return;
    m_objects.remove(index);
    if (m_selected >= int(m_objects.size())) m_selected = -1;
    emit sceneChanged();
    update();
}

void ViewportWidget::setNameDirect(int index, const QString& name) {
    if (index < 0 || index >= m_objects.size()) return;
    m_objects[index].name = name;
    emit sceneChanged();
}

void ViewportWidget::applyMaterialDirect(int index, const Material& m) {
    if (index < 0 || index >= m_objects.size()) return;
    m_objects[index].material = m;
    emit sceneChanged();
    update();
}

void ViewportWidget::undo() {
    if (!m_stack.canUndo()) return;
    m_stack.undo();
    if (m_selected >= int(m_objects.size())) m_selected = -1;
    emit sceneChanged();
    emit selectionChanged(m_selected);
    emit undoRedoChanged();
    emit statusMessage("Undone");
}

void ViewportWidget::redo() {
    if (!m_stack.canRedo()) return;
    m_stack.redo();
    if (m_selected >= int(m_objects.size())) m_selected = -1;
    emit sceneChanged();
    emit selectionChanged(m_selected);
    emit undoRedoChanged();
    emit statusMessage("Redone");
}

bool ViewportWidget::canUndo() const { return m_stack.canUndo(); }
bool ViewportWidget::canRedo() const { return m_stack.canRedo(); }

// --- Vertex Editing ---

void ViewportWidget::setVertexDirect(int objIndex, int vi, const glm::vec3& localPos) {
    if (objIndex < 0 || objIndex >= m_objects.size()) return;
    auto mesh = m_objects[objIndex].mesh;
    if (!mesh || vi < 0 || vi >= int(mesh->vertices.size())) return;
    mesh->vertices[vi].pos = localPos;
    recomputeNormalsAround(mesh, vi);
    if (!m_objects[objIndex].meshPath.isEmpty())
        m_objects[objIndex].meshEmbedded = true; // edited import is stored entirely in .r3dp
    m_dirtyMeshes.insert(mesh.get());
    emit sceneChanged();
    update();
}

void ViewportWidget::recomputeNormalsAround(const std::shared_ptr<MeshData>& mesh, int vi) {
    glm::vec3 n(0.0f);
    const auto& idx = mesh->indices;
    for (size_t f = 0; f + 2 < idx.size(); f += 3) {
        if (int(idx[f]) == vi || int(idx[f + 1]) == vi || int(idx[f + 2]) == vi) {
            glm::vec3 a = mesh->vertices[idx[f]].pos;
            glm::vec3 b = mesh->vertices[idx[f + 1]].pos;
            glm::vec3 c = mesh->vertices[idx[f + 2]].pos;
            n += glm::cross(b - a, c - a);
        }
    }
    float len = glm::length(n);
    mesh->vertices[vi].normal = len > 1e-8f ? n / len : glm::vec3(0.0f, 1.0f, 0.0f);
}