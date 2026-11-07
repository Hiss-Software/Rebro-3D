#include "ViewportWidget.h"
#include "ViewportHelpers.h"
#include "MeshRay.h"

#include <glm/gtc/matrix_transform.hpp>

#include <vector>

namespace {

constexpr float kWeldEpsilon = 1e-4f; // tolerance for welding duplicate vertices

// Recalculate normals per triangle: accumulate sums per vertex and normalize.
// For primitives, a face is 2 triangles over its 4 vertices, so smoothing
// does not spill over a single face and the flat look of the cube is preserved.
void recalcNormals(MeshData& mesh) {
    for (auto& v : mesh.vertices)
        v.normal = glm::vec3(0.0f);

    for (size_t f = 0; f + 2 < mesh.indices.size(); f += 3) {
        unsigned a = mesh.indices[f];
        unsigned b = mesh.indices[f + 1];
        unsigned c = mesh.indices[f + 2];
        if (a >= mesh.vertices.size() ||
            b >= mesh.vertices.size() ||
            c >= mesh.vertices.size())
            continue; // skip corrupt indices

        const glm::vec3& p0 = mesh.vertices[a].pos;
        const glm::vec3& p1 = mesh.vertices[b].pos;
        const glm::vec3& p2 = mesh.vertices[c].pos;
        glm::vec3 n = glm::cross(p1 - p0, p2 - p0);
        mesh.vertices[a].normal += n;
        mesh.vertices[b].normal += n;
        mesh.vertices[c].normal += n;
    }

    for (auto& v : mesh.vertices) {
        float len = glm::length(v.normal);
        v.normal = (len > 1e-8f) ? v.normal / len : glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

} // namespace

// --- Ray picking of objects ---

bool ViewportWidget::pickPrecise(const QPoint& pos, int& outIndex, RayHit* hit) const {
    outIndex = -1;
    if (m_objects.isEmpty()) return false;

    glm::mat4 proj = buildProjectionMatrix();
    glm::mat4 view = buildViewMatrix();
    glm::vec3 nearPt = glm::unProject(
        glm::vec3(float(pos.x()), float(height() - pos.y()), 0.0f),
        view, proj, glm::vec4(0, 0, float(width()), float(height())));
    glm::vec3 farPt = glm::unProject(
        glm::vec3(float(pos.x()), float(height() - pos.y()), 1.0f),
        view, proj, glm::vec4(0, 0, float(width()), float(height())));
    glm::vec3 dir = glm::normalize(farPt - nearPt);

    int best = -1;
    MeshRay::Hit bestHit;
    for (int i = 0; i < m_objects.size(); ++i) {
        const SceneObject& obj = m_objects[i];
        if (!obj.mesh) continue;

        glm::mat4 model = buildModelMatrix(obj);
        glm::mat4 inv = glm::inverse(model);
        glm::vec3 oL = glm::vec3(inv * glm::vec4(nearPt, 1.0f));
        glm::vec3 dL = glm::normalize(glm::vec3(inv * glm::vec4(dir, 0.0f)));

        float tBox = 0.0f;
        if (!rayAABB(oL, dL, obj.mesh->boundsMin, obj.mesh->boundsMax, tBox))
            continue;

        MeshRay::Hit h;
        if (!MeshRay::rayMesh(*obj.mesh, oL, dL, h)) continue;
        if (best < 0 || h.t < bestHit.t) {
            best = i;
            bestHit = h;
        }
    }

    if (best < 0) return false;
    outIndex = best;
    if (hit) {
        hit->t = bestHit.t;
        hit->tri[0] = bestHit.tri[0];
        hit->tri[1] = bestHit.tri[1];
        hit->tri[2] = bestHit.tri[2];
        hit->point = glm::vec3(buildModelMatrix(m_objects[best])
                               * glm::vec4(bestHit.point, 1.0f));
    }
    return true;
}

bool ViewportWidget::pickObject(const QPoint& pos, int& outIndex) const {
    return pickPrecise(pos, outIndex, nullptr);
}

bool ViewportWidget::pickGizmo(const QPoint& pos, int& outAxis) const {
    if (m_selected < 0) return false;
    if (m_mode == TransformMode::Vertices) return false;
    const SceneObject& obj = m_objects[m_selected];
    float len = gizmoLength();
    QPointF mpos(pos.x(), pos.y());

    if (m_mode == TransformMode::Rotate) {
        float r = len * 0.9f;
        int best = -1;
        float bestD = 12.0f;
        for (int a = 0; a < 3; ++a) {
            glm::vec3 e1, e2;
            switch (a) {
            case 0:  e1 = {0,1,0}; e2 = {0,0,1}; break;
            case 1:  e1 = {1,0,0}; e2 = {0,0,1}; break;
            default: e1 = {1,0,0}; e2 = {0,1,0};
            }
            for (int i = 0; i < 32; ++i) {
                float ang = float(i) / 32.0f * glm::pi<float>() * 2.0f;
                glm::vec3 p = obj.position
                            + (e1 * std::cos(ang) + e2 * std::sin(ang)) * r;
                QPointF sp = projectToScreen(p);
                float d = std::hypot(sp.x() - mpos.x(), sp.y() - mpos.y());
                if (d < bestD) { bestD = d; best = a; }
            }
        }
        if (best < 0) return false;
        outAxis = best;
        return true;
    }

    int best = -1;
    float bestD = 12.0f;
    QPointF p0 = projectToScreen(obj.position);
    for (int a = 0; a < 3; ++a) {
        glm::vec3 axis(0.0f); axis[a] = 1.0f;
        QPointF p1 = projectToScreen(obj.position + axis * len);
        float d = distPointSegment(mpos, p0, p1);
        if (d < bestD) { bestD = d; best = a; }
    }
    if (best < 0) return false;
    outAxis = best;
    return true;
}

// --- Hover highlight of the face under the cursor ---

void ViewportWidget::updateHover(const QPoint& pos) {
    HoverInfo prev = m_hover;
    m_hover = HoverInfo{};

    int idx = -1;
    RayHit hit;
    if (pickPrecise(pos, idx, &hit)) {
        m_hover.index = idx;
        m_hover.tri[0] = hit.tri[0];
        m_hover.tri[1] = hit.tri[1];
        m_hover.tri[2] = hit.tri[2];
        m_hover.point = hit.point;
    }

    if (m_hover.index != prev.index || m_hover.point != prev.point)
        update();
}

void ViewportWidget::drawHoverTriangle(const glm::mat4& vp) {
    if (m_gizmoDragging) return;
    if (m_hover.index < 0 || m_hover.index >= m_objects.size()) return;
    const SceneObject& obj = m_objects[m_hover.index];
    if (!obj.mesh) return;
    if (m_hover.tri[2] >= obj.mesh->vertices.size()) return;

    glm::mat4 model = buildModelMatrix(obj);
    glm::vec3 tri[3];
    for (int k = 0; k < 3; ++k) {
        const glm::vec3& p = obj.mesh->vertices[m_hover.tri[k]].pos;
        tri[k] = glm::vec3(model * glm::vec4(p, 1.0f));
    }
    glm::vec3 edges[6] = { tri[0], tri[1], tri[1], tri[2], tri[2], tri[0] };

    glUniform1i(m_locFlat, 1);
    glUniform1i(m_locUseTex, 0);
    glUniform3f(m_locColor, 1.0f, 0.8f, 0.2f);
    glUniformMatrix4fv(m_locMVP, 1, GL_FALSE, glm::value_ptr(vp));

    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0f);
    glBindVertexArray(m_hoverVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_hoverVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(edges), edges);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}

// --- Vertex mode ---

int ViewportWidget::pickVertex(const QPoint& pos, float maxDistPx) const {
    if (m_selected < 0 || m_selected >= m_objects.size()) return -1;
    const SceneObject& obj = m_objects[m_selected];
    if (!obj.mesh) return -1;

    glm::mat4 model = buildModelMatrix(obj);
    glm::mat4 proj = buildProjectionMatrix();
    glm::mat4 view = buildViewMatrix();
    glm::vec4 viewport(0, 0, float(width()), float(height()));

    int best = -1;
    float bestD = maxDistPx * maxDistPx;
    for (int i = 0; i < int(obj.mesh->vertices.size()); ++i) {
        glm::vec3 wp = glm::vec3(model * glm::vec4(obj.mesh->vertices[i].pos, 1.0f));
        glm::vec3 sp = glm::project(wp, view, proj, viewport);
        if (sp.z < 0.0f || sp.z > 1.0f) continue;
        float dx = sp.x - float(pos.x());
        float dy = (float(height()) - sp.y) - float(pos.y());
        float d = dx * dx + dy * dy;
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

void ViewportWidget::beginVertexDrag(const QPoint& pos) {
    Q_UNUSED(pos);
    const SceneObject& obj = m_objects[m_selected];
    if (!obj.mesh) return;
    m_dragVertex = m_hoverVertex;
    if (m_dragVertex < 0) return;

    glm::mat4 model = buildModelMatrix(obj);
    m_dragBeforeLocal = obj.mesh->vertices[m_dragVertex].pos;
    m_dragStartWorld = glm::vec3(model * glm::vec4(m_dragBeforeLocal, 1.0f));

    // Collect ALL duplicates of the dragged vertex — otherwise the mesh tears at the seams
    m_dragGroup.clear();
    m_dragStartLocal.clear();
    for (int i = 0; i < int(obj.mesh->vertices.size()); ++i) {
        if (glm::length(obj.mesh->vertices[i].pos - m_dragBeforeLocal) < kWeldEpsilon)
            m_dragGroup.push_back(i);
    }
    m_dragStartLocal.reserve(m_dragGroup.size());
    for (int vi : m_dragGroup)
        m_dragStartLocal.push_back(obj.mesh->vertices[vi].pos);

    glm::mat4 view = buildViewMatrix();
    m_dragPlaneNormal = glm::normalize(
        glm::vec3(glm::inverse(view) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

    m_gizmoDragging = true;
    setCursor(Qt::ClosedHandCursor);
}

void ViewportWidget::updateVertexDrag(const QPoint& pos) {
    if (m_dragVertex < 0 || m_selected < 0 || m_selected >= m_objects.size()) return;
    const SceneObject& obj = m_objects[m_selected];
    if (!obj.mesh || m_dragGroup.empty()) return;

    glm::mat4 proj = buildProjectionMatrix();
    glm::mat4 view = buildViewMatrix();
    glm::vec3 nearPt = glm::unProject(
        glm::vec3(float(pos.x()), float(height() - pos.y()), 0.0f),
        view, proj, glm::vec4(0, 0, float(width()), float(height())));
    glm::vec3 farPt = glm::unProject(
        glm::vec3(float(pos.x()), float(height() - pos.y()), 1.0f),
        view, proj, glm::vec4(0, 0, float(width()), float(height())));
    glm::vec3 dir = glm::normalize(farPt - nearPt);

    float denom = glm::dot(m_dragPlaneNormal, dir);
    if (std::abs(denom) < 1e-6f) return;
    float t = glm::dot(m_dragPlaneNormal, m_dragStartWorld - nearPt) / denom;
    if (t < 0.0f) return;
    glm::vec3 hit = nearPt + dir * t;

    glm::mat4 inv = glm::inverse(buildModelMatrix(obj));
    glm::vec3 newLocal = glm::vec3(inv * glm::vec4(hit, 1.0f));

    // Same delta for all duplicates in the group
    glm::vec3 delta = newLocal - m_dragBeforeLocal;
    for (size_t k = 0; k < m_dragGroup.size(); ++k) {
        int vi = m_dragGroup[k];
        if (vi < 0 || vi >= int(obj.mesh->vertices.size())) continue;
        obj.mesh->vertices[vi].pos = m_dragStartLocal[k] + delta;
    }

    recalcNormals(*obj.mesh);

    // Bounds and GPU upload
    obj.mesh->boundsMin = obj.mesh->boundsMax = m_dragStartLocal.empty()
        ? glm::vec3(0.0f) : m_dragStartLocal[0];
    for (const auto& v : obj.mesh->vertices) {
        obj.mesh->boundsMin = glm::min(obj.mesh->boundsMin, v.pos);
        obj.mesh->boundsMax = glm::max(obj.mesh->boundsMax, v.pos);
    }
    m_dirtyMeshes.insert(obj.mesh.get());

    update();
}

void ViewportWidget::endVertexDrag() {
    if (m_dragVertex < 0) return;
    int objIdx = m_selected;
    int vi = m_dragVertex;
    glm::vec3 before = m_dragBeforeLocal;
    QVector<int> group = m_dragGroup;
    std::vector<glm::vec3> groupBefore = m_dragStartLocal;

    m_dragVertex = -1;
    m_gizmoDragging = false;
    m_hoverVertex = -1;
    m_dragGroup.clear();
    m_dragStartLocal.clear();
    setCursor(Qt::ArrowCursor);

    if (objIdx < 0 || objIdx >= m_objects.size()) return;
    if (!m_objects[objIdx].mesh) return;
    glm::vec3 after = m_objects[objIdx].mesh->vertices[vi].pos;
    if (after == before) return;

    // Undo for the whole group: if VertexEditCmd can store an array of vertices —
    // make one command per group (the correct approach). For now — one command
    // per vertex: works correctly, but undo takes several steps.
    for (size_t k = 0; k < group.size(); ++k) {
        int gvi = group[k];
        if (gvi < 0 || gvi >= int(m_objects[objIdx].mesh->vertices.size())) continue;
        if (m_objects[objIdx].mesh->vertices[gvi].pos == groupBefore[k]) continue;
        pushCommand(std::make_shared<VertexEditCmd>(this, objIdx, gvi,
                                                    groupBefore[k],
                                                    m_objects[objIdx].mesh->vertices[gvi].pos),
                    k == 0, false);
    }
}

void ViewportWidget::drawVertexHandles(const glm::mat4& vp) {
    const SceneObject& obj = m_objects[m_selected];
    if (!obj.mesh) return;
    using PFN_glPointSize = void(APIENTRY*)(GLfloat size);
    static PFN_glPointSize pglPointSize = nullptr;
    if (!pglPointSize)
        pglPointSize = reinterpret_cast<PFN_glPointSize>(
            QOpenGLContext::currentContext()->getProcAddress("glPointSize"));

    glm::mat4 model = buildModelMatrix(obj);
    const auto& vs = obj.mesh->vertices;
    std::vector<glm::vec3> pts(vs.size());
    for (size_t i = 0; i < vs.size(); ++i)
        pts[i] = glm::vec3(model * glm::vec4(vs[i].pos, 1.0f));

    glBindVertexArray(m_handleVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_handleVbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(pts.size() * sizeof(glm::vec3)),
                 pts.data(), GL_DYNAMIC_DRAW);

    glUniform1i(m_locFlat, 1);
    glUniform1i(m_locUseTex, 0);
    glUniformMatrix4fv(m_locMVP, 1, GL_FALSE, glm::value_ptr(vp));

    glDisable(GL_DEPTH_TEST);
    pglPointSize(7.0f);
    glUniform3f(m_locColor, 0.3f, 0.8f, 1.0f);
    glDrawArrays(GL_POINTS, 0, GLsizei(pts.size()));

    int active = m_dragVertex >= 0 ? m_dragVertex : m_hoverVertex;
    if (active >= 0 && active < int(pts.size())) {
        glUniform3f(m_locColor, 1.0f, 0.85f, 0.2f);
        pglPointSize(11.0f);
        glDrawArrays(GL_POINTS, active, 1);
    }

    pglPointSize(1.0f);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

// --- Gizmo dragging (move, rotate, scale) ---

void ViewportWidget::applyGizmoDrag(const QPointF& mousePos) {
    if (m_mode == TransformMode::Vertices) return;
    float len = gizmoLength();
    glm::vec3 axis(0.0f);
    axis[m_gizmoAxis] = 1.0f;

    if (m_mode == TransformMode::Move || m_mode == TransformMode::Scale) {
        glm::vec3 end = m_startState.position + axis * len;
        QPointF p0 = projectToScreen(m_startState.position);
        QPointF p1 = projectToScreen(end);
        QPointF sd = p1 - p0;
        float slen = std::sqrt(float(QPointF::dotProduct(sd, sd)));
        if (slen < 1e-3f) return;
        float wpp = len / slen;
        QPointF d = mousePos - m_dragStartMouse;
        float t = float(QPointF::dotProduct(d, sd)) / slen;

        if (m_mode == TransformMode::Move) {
            m_objects[m_selected].position = m_startState.position + axis * (t * wpp);
        } else {
            glm::vec3 s = m_startState.scale;
            s[m_gizmoAxis] = std::max(0.01f, s[m_gizmoAxis] + t * wpp);
            m_objects[m_selected].scale = s;
        }
    } else {
        glm::vec3 center = m_startState.position;
        QPointF c = projectToScreen(center);
        float ang = std::atan2(float(mousePos.y() - c.y()),
                               float(mousePos.x() - c.x()));
        float dAng = ang - m_lastDragAngle;
        while (dAng >  glm::pi<float>()) dAng -= glm::pi<float>() * 2.0f;
        while (dAng < -glm::pi<float>()) dAng += glm::pi<float>() * 2.0f;
        m_lastDragAngle = ang;

        glm::vec3 eyePos(glm::inverse(buildViewMatrix()) * glm::vec4(0, 0, 0, 1));
        bool toward = glm::dot(axis, eyePos - center) > 0.0f;
        float deg = glm::degrees(toward ? -dAng : dAng);
        m_objects[m_selected].rotation[m_gizmoAxis] += deg;
    }

    emit sceneChanged();
}