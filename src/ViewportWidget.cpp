#include "ViewportWidget.h"
#include "ViewportShaders.h"
#include "ViewportHelpers.h"
#include "SceneDocument.h"
#include "MeshImporter.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QImage>
#include <QDebug>
#include <QLineEdit>
#include <QAbstractSpinBox>
#include <QTextEdit>
#include <QPlainTextEdit>

#include <array>
#include <cmath>
#include <algorithm>

ViewportWidget::ViewportWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    m_clock.start();

    connect(&m_frameTimer, &QTimer::timeout, this,
            QOverload<>::of(&ViewportWidget::update));
    m_frameTimer.start(16);

    QCoreApplication::instance()->installEventFilter(this);
}

ViewportWidget::~ViewportWidget() {
    QCoreApplication::instance()->removeEventFilter(this);
}

// --- Keyboard ---

bool ViewportWidget::mapNavKey(int qtKey, int& outNavKey) {
    switch (qtKey) {
    case Qt::Key_W: case 0x0426: outNavKey = Qt::Key_W; return true; // Cyrillic Ts
    case Qt::Key_A: case 0x0424: outNavKey = Qt::Key_A; return true; // Cyrillic Ef
    case Qt::Key_S: case 0x042B: outNavKey = Qt::Key_S; return true; // Cyrillic Yeru
    case Qt::Key_D: case 0x0412: outNavKey = Qt::Key_D; return true; // Cyrillic Ve
    case Qt::Key_Q: case 0x0419: outNavKey = Qt::Key_Q; return true; // Cyrillic Short I
    case Qt::Key_E: case 0x0423: outNavKey = Qt::Key_E; return true; // Cyrillic U
    default: return false;
    }
}

bool ViewportWidget::mapCommandKey(int qtKey, int& outCommand) {
    switch (qtKey) {
    case Qt::Key_G:    case 0x041F: outCommand = CmdG;      return true; // Cyrillic Pe
    case Qt::Key_R:    case 0x041A: outCommand = CmdR;      return true; // Cyrillic Ka
    case Qt::Key_V:    case 0x041C: outCommand = CmdV;      return true; // Cyrillic Em
    case Qt::Key_Delete:            outCommand = CmdDelete; return true;
    case Qt::Key_Escape:            outCommand = CmdEscape; return true;
    default: return false;
    }
}

bool ViewportWidget::isTextEntry(QObject* obj) {
    for (QObject* o = obj; o; o = o->parent()) {
        if (qobject_cast<QLineEdit*>(o) || qobject_cast<QAbstractSpinBox*>(o)
            || qobject_cast<QTextEdit*>(o) || qobject_cast<QPlainTextEdit*>(o))
            return true;
    }
    return false;
}

bool ViewportWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (!ke->isAutoRepeat() && !isTextEntry(watched)) {
            if (event->type() == QEvent::KeyPress &&
                (ke->modifiers() & Qt::ShiftModifier) &&
                ke->key() == Qt::Key_S) {
                m_pressedKeys.remove(Qt::Key_S); // prevent camera from getting stuck
                setTransformMode(TransformMode::Scale);
            } else {
                int nav = 0;
                if (mapNavKey(ke->key(), nav)) {
                    if (event->type() == QEvent::KeyPress)
                        m_pressedKeys.insert(nav);
                    else
                        m_pressedKeys.remove(nav);
                } else if (event->type() == QEvent::KeyPress) {
                    int cmd = CmdNone;
                    if (mapCommandKey(ke->key(), cmd)) {
                        switch (cmd) {
                        case CmdG:      setTransformMode(TransformMode::Move);     break;
                        case CmdR:      setTransformMode(TransformMode::Rotate);   break;
                        case CmdV:      setTransformMode(TransformMode::Vertices); break;
                        case CmdDelete: deleteSelected();                          break;
                        case CmdEscape: setSelected(-1);                           break;
                        default: break;
                        }
                    }
                }
            }
        }
    }
    return QOpenGLWidget::eventFilter(watched, event);
}

// --- Scene ---

int ViewportWidget::addObject(const QString& name,
                              std::shared_ptr<MeshData> mesh,
                              const QString& meshPath,
                              const QString& shapeId)
{
    SceneObject obj;
    obj.name = name;
    obj.meshPath = meshPath;
    obj.shapeId = shapeId;
    obj.mesh = mesh;

    if (!obj.mesh && meshPath.isEmpty())
        obj.mesh = makePrimitiveMesh(shapeId);
    if (!obj.mesh)
        obj.mesh = m_cubeMesh;

    m_objects.push_back(obj);
    int index = int(m_objects.size()) - 1;
    pushCommand(std::make_shared<AddObjectCmd>(this, index, m_objects[index]),
                true, false);

    if (meshPath.isEmpty() && shapeId != "cube" && mesh)
        frameToBounds(*mesh);
    else if (!meshPath.isEmpty() && mesh)
        frameToBounds(*mesh);

    emit sceneChanged();
    setSelected(index);
    return index;
}

void ViewportWidget::clearScene() {
    m_objects.clear();
    m_selected = -1;
    m_gizmoDragging = false;
    m_hoverAxis = -1;
    m_hover.index = -1;
    m_hoverVertex = -1;
    m_stack.clear();
    emit undoRedoChanged();
    emit sceneChanged();
    emit selectionChanged(-1);
    update();
}

void ViewportWidget::setObjects(const QVector<SceneObject>& objs) {
    m_objects = objs;
    m_selected = -1;
    m_gizmoDragging = false;
    m_hoverAxis = -1;
    m_hover.index = -1;
    m_hoverVertex = -1;
    m_stack.clear();

    QStringList failed;
    for (SceneObject& o : m_objects) {
        if (o.mesh) continue;
        if (!o.meshPath.isEmpty()) {
            QString error;
            if (!MeshImporter::loadFromFile(o.meshPath, o.mesh, &error))
                failed << o.name;
        } else {
            o.mesh = makePrimitiveMesh(o.shapeId);
        }
    }
    for (SceneObject& o : m_objects)
        if (!o.mesh) o.mesh = m_cubeMesh;

    emit undoRedoChanged();
    emit sceneChanged();
    emit selectionChanged(-1);
    update();

    if (!failed.isEmpty())
        emit statusMessage(QString("Failed to import: %1 (placeholder cubes used)")
                               .arg(failed.join(", ")));
}

void ViewportWidget::deleteSelected() {
    if (m_selected < 0) return;
    int index = m_selected;
    SceneObject removed = m_objects[index];
    m_objects.remove(index);
    m_selected = -1;
    m_gizmoDragging = false;
    m_hoverAxis = -1;
    m_hover.index = -1;
    m_hoverVertex = -1;
    pushCommand(std::make_shared<DeleteObjectCmd>(this, index, removed), true, false);
    emit sceneChanged();
    emit selectionChanged(-1);
    emit statusMessage("Object deleted");
    update();
}

void ViewportWidget::renameSelected(const QString& name) {
    if (m_selected < 0) return;
    QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) return;
    QString before = m_objects[m_selected].name;
    if (before == trimmed) return;
    m_objects[m_selected].name = trimmed;
    pushCommand(std::make_shared<RenameCmd>(this, m_selected, before, trimmed),
                true, false);
    emit sceneChanged();
}

void ViewportWidget::setSelected(int index) {
    if (index == m_selected) return;
    m_selected = index;
    m_gizmoDragging = false;
    m_hoverAxis = -1;
    m_hover.index = -1;
    m_hoverVertex = -1;
    emit selectionChanged(m_selected);
    update();
}

void ViewportWidget::setObjectTransform(int index, const glm::vec3& pos,
                                        const glm::vec3& rot, const glm::vec3& scale)
{
    applyTransformDirect(index, pos, rot, scale);
    update();
}

void ViewportWidget::setObjectTransformTracked(int index, const glm::vec3& pos,
                                               const glm::vec3& rot, const glm::vec3& scale)
{
    if (index < 0 || index >= m_objects.size()) return;
    SceneObject& o = m_objects[index];
    if (o.position == pos && o.rotation == rot && o.scale == scale) return;

    glm::vec3 before[3] = { o.position, o.rotation, o.scale };
    glm::vec3 after[3]  = { pos, rot, scale };
    o.position = pos;
    o.rotation = rot;
    o.scale = scale;
    pushCommand(std::make_shared<TransformCmd>(this, index, before, after,
                                               m_clock.elapsed()), true, true);
    emit sceneChanged();
    update();
}

void ViewportWidget::setMaterialColorTracked(int index, const glm::vec3& color) {
    if (index < 0 || index >= m_objects.size()) return;
    SceneObject& o = m_objects[index];
    if (o.material.diffuse == color) return;
    Material before = o.material;
    Material after = o.material;
    after.diffuse = color;
    o.material = after;
    pushCommand(std::make_shared<MaterialCmd>(this, index, before, after,
                                              m_clock.elapsed()), true, true);
    emit sceneChanged();
    update();
}

void ViewportWidget::setMaterialTextureTracked(int index, const QString& path) {
    if (index < 0 || index >= m_objects.size()) return;
    SceneObject& o = m_objects[index];
    if (o.material.texturePath == path) return;
    Material before = o.material;
    Material after = o.material;
    after.texturePath = path;
    o.material = after;
    pushCommand(std::make_shared<MaterialCmd>(this, index, before, after,
                                              m_clock.elapsed()), true, false);
    emit sceneChanged();
    update();
}

bool ViewportWidget::saveScene(const QString& path, QString* error) {
    if (!SceneDocument::save(m_objects, path, error)) {
        emit statusMessage("Save error");
        return false;
    }
    emit statusMessage(QString("Saved: %1").arg(path));
    return true;
}

bool ViewportWidget::loadScene(const QString& path, QString* error) {
    QVector<SceneObject> objs;
    if (!SceneDocument::load(objs, path, error)) {
        emit statusMessage("Load error");
        return false;
    }
    setObjects(objs);
    emit statusMessage(QString("Opened: %1 (%2 objects)")
                           .arg(path).arg(objs.size()));
    return true;
}

// --- Modes and Camera ---

void ViewportWidget::setTransformMode(TransformMode mode) {
    m_mode = mode;
    m_hover.index = -1;
    m_hoverVertex = -1;
    const char* names[] = { "Mode: Move (drag arrows)",
                            "Mode: Rotate (drag rings)",
                            "Mode: Scale (drag boxes)",
                            "Mode: Vertices (drag points)" };
    emit statusMessage(names[int(mode)]);
    setFocus();
    update();
}

void ViewportWidget::setCameraSpeed(float unitsPerSecond) {
    m_speed = std::max(0.1f, unitsPerSecond);
}

void ViewportWidget::setFastMode(bool on) {
    m_fastMode = on;
}

// --- OpenGL ---

ViewportWidget::MeshGL& ViewportWidget::ensureGL(const std::shared_ptr<MeshData>& mesh) {
    auto it = m_meshGL.find(mesh.get());
    if (it != m_meshGL.end())
        return *it;

    MeshGL gl;
    gl.indexCount = int(mesh->indices.size());

    glGenVertexArrays(1, &gl.vao);
    glGenBuffers(1, &gl.vbo);
    glGenBuffers(1, &gl.ibo);

    glBindVertexArray(gl.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 GLsizeiptr(mesh->vertices.size() * sizeof(MeshData::Vertex)),
                 mesh->vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 GLsizeiptr(mesh->indices.size() * sizeof(unsigned)),
                 mesh->indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshData::Vertex),
                          reinterpret_cast<void*>(offsetof(MeshData::Vertex, pos)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshData::Vertex),
                          reinterpret_cast<void*>(offsetof(MeshData::Vertex, normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshData::Vertex),
                          reinterpret_cast<void*>(offsetof(MeshData::Vertex, uv)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    return m_meshGL.insert(mesh.get(), gl).value();
}

QOpenGLTexture* ViewportWidget::ensureTexture(const QString& path) {
    if (path.isEmpty()) return nullptr;
    auto it = m_textures.constFind(path);
    if (it != m_textures.constEnd())
        return *it;

    QImage img(path);
    QOpenGLTexture* tex = nullptr;
    if (!img.isNull()) {
        img = img.convertToFormat(QImage::Format_RGBA8888);
        tex = new QOpenGLTexture(img);
        tex->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        tex->setMagnificationFilter(QOpenGLTexture::Linear);
        tex->setWrapMode(QOpenGLTexture::Repeat);
    } else {
        emit statusMessage(QString("Failed to load texture: %1").arg(path));
    }
    m_textures.insert(path, tex);
    return tex;
}

void ViewportWidget::frameToBounds(const MeshData& mesh) {
    glm::vec3 center = (mesh.boundsMin + mesh.boundsMax) * 0.5f;
    float radius = glm::length(mesh.boundsMax - mesh.boundsMin) * 0.5f;
    if (radius < 0.001f) radius = 1.0f;
    m_target = center;
    m_distance = glm::clamp(radius * 2.5f, 1.0f, 50.0f);
}

void ViewportWidget::initializeGL() {
    initializeOpenGLFunctions();

    m_glPolygonMode = reinterpret_cast<PFN_glPolygonMode>(
        QOpenGLContext::currentContext()->getProcAddress("glPolygonMode"));

    qDebug() << "GL:" << reinterpret_cast<const char*>(glGetString(GL_VERSION));

    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, 1.0f);
    glEnable(GL_DEPTH_TEST);

    auto compile = [this](GLenum type, const char* src) {
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        GLint ok = 0;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
            qFatal("Shader compile error: %s", log);
        }
        return sh;
    };

    m_program = glCreateProgram();
    glAttachShader(m_program, compile(GL_VERTEX_SHADER, kVertexShader));
    glAttachShader(m_program, compile(GL_FRAGMENT_SHADER, kFragmentShader));
    glLinkProgram(m_program);
    GLint ok = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok) qFatal("Program link error");

    m_locMVP     = glGetUniformLocation(m_program, "uMVP");
    m_locModel   = glGetUniformLocation(m_program, "uModel");
    m_locFlat    = glGetUniformLocation(m_program, "uFlat");
    m_locColor   = glGetUniformLocation(m_program, "uColor");
    m_locUseTex  = glGetUniformLocation(m_program, "uUseTex");
    m_locSampler = glGetUniformLocation(m_program, "uTexture");
    m_locCamPos = glGetUniformLocation(m_program, "uCamPos");

    m_cubeMesh = makeCubeMesh();
    ensureGL(m_cubeMesh);

    const glm::vec3 seg[2] = { {0,0,0}, {1,0,0} };
    glGenVertexArrays(1, &m_segmentVao);
    glGenBuffers(1, &m_segmentVbo);
    glBindVertexArray(m_segmentVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_segmentVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(seg), seg, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    std::array<glm::vec3, 64> circle;
    for (int i = 0; i < 64; ++i) {
        float ang = float(i) / 64.0f * glm::pi<float>() * 2.0f;
        circle[i] = glm::vec3(std::cos(ang), std::sin(ang), 0.0f);
    }
    glGenVertexArrays(1, &m_circleVao);
    glGenBuffers(1, &m_circleVbo);
    glBindVertexArray(m_circleVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_circleVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(circle), circle.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    std::array<glm::vec3, 48> cone;
    const int segCount = 8;
    int idx = 0;
    const glm::vec3 apex(0, 1, 0), center(0, 0, 0);
    for (int i = 0; i < segCount; ++i) {
        float a0 = float(i) / segCount * glm::pi<float>() * 2.0f;
        float a1 = float(i + 1) / segCount * glm::pi<float>() * 2.0f;
        glm::vec3 p0(std::cos(a0), 0, std::sin(a0));
        glm::vec3 p1(std::cos(a1), 0, std::sin(a1));
        cone[idx++] = apex;   cone[idx++] = p0; cone[idx++] = p1;
        cone[idx++] = center; cone[idx++] = p1; cone[idx++] = p0;
    }
    glGenVertexArrays(1, &m_coneVao);
    glGenBuffers(1, &m_coneVbo);
    glBindVertexArray(m_coneVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_coneVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cone), cone.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &m_hoverVao);
    glGenBuffers(1, &m_hoverVbo);
    glBindVertexArray(m_hoverVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_hoverVbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &m_handleVao);
    glGenBuffers(1, &m_handleVbo);
    glBindVertexArray(m_handleVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_handleVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

glm::mat4 ViewportWidget::buildProjectionMatrix() const {
    return glm::perspective(glm::radians(45.0f),
                            float(width()) / std::max(height(), 1), 0.1f, 100.0f);
}

glm::mat4 ViewportWidget::buildModelMatrix(const SceneObject& obj) const {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), obj.position);
    m = glm::rotate(m, glm::radians(obj.rotation.x), glm::vec3(1,0,0));
    m = glm::rotate(m, glm::radians(obj.rotation.y), glm::vec3(0,1,0));
    m = glm::rotate(m, glm::radians(obj.rotation.z), glm::vec3(0,0,1));
    return glm::scale(m, obj.scale);
}

glm::mat4 ViewportWidget::cameraRotation() const {
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(m_pitch), glm::vec3(1,0,0));
    return glm::rotate(rot, glm::radians(m_yaw), glm::vec3(0,1,0));
}

glm::vec3 ViewportWidget::forwardVec() const {
    return glm::normalize(glm::vec3(glm::transpose(cameraRotation())
                                    * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
}

glm::vec3 ViewportWidget::rightVec() const {
    return glm::normalize(glm::vec3(glm::transpose(cameraRotation())
                                    * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
}

glm::vec3 ViewportWidget::upVec() const {
    return glm::normalize(glm::vec3(glm::transpose(cameraRotation())
                                    * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
}

glm::mat4 ViewportWidget::buildViewMatrix() const {
    glm::mat4 view = cameraRotation() * glm::translate(glm::mat4(1.0f), -m_target);
    return glm::translate(view, glm::vec3(0, 0, -m_distance));
}

void ViewportWidget::applyKeyboard(float dt) {
    if (m_pressedKeys.isEmpty()) return;
    float dist = m_speed * (m_fastMode ? 3.0f : 1.0f) * dt;
    glm::vec3 f = forwardVec(), r = rightVec(), u = upVec();
    glm::vec3 delta(0.0f);
    if (m_pressedKeys.contains(Qt::Key_W)) delta += f;
    if (m_pressedKeys.contains(Qt::Key_S)) delta -= f;
    if (m_pressedKeys.contains(Qt::Key_D)) delta += r;
    if (m_pressedKeys.contains(Qt::Key_A)) delta -= r;
    if (m_pressedKeys.contains(Qt::Key_E)) delta += u;
    if (m_pressedKeys.contains(Qt::Key_Q)) delta -= u;
    if (delta != glm::vec3(0.0f))
        m_target += delta * dist;
}

float ViewportWidget::gizmoLength() const {
    return m_distance * 0.22f;
}

QPointF ViewportWidget::projectToScreen(const glm::vec3& world) const {
    glm::vec4 clip = buildProjectionMatrix() * buildViewMatrix() * glm::vec4(world, 1.0f);
    if (clip.w <= 1e-6f) return QPointF(-1e6f, -1e6f);
    float ndcX = clip.x / clip.w;
    float ndcY = clip.y / clip.w;
    return QPointF((ndcX * 0.5f + 0.5f) * width(),
                   (1.0f - (ndcY * 0.5f + 0.5f)) * height());
}

void ViewportWidget::renderObject(const SceneObject& obj, const glm::mat4& vp, bool selected) {
    glm::mat4 model = buildModelMatrix(obj);
    glm::mat4 mvp = vp * model;

    const MeshGL& gl = ensureGL(obj.mesh);
    QOpenGLTexture* tex = ensureTexture(obj.material.texturePath);

    glUniformMatrix4fv(m_locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(m_locModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(m_locFlat, 0);
    glUniform1i(m_locUseTex, tex ? 1 : 0);
    if (tex)
        tex->bind(0);
    glUniform3f(m_locColor,
                obj.material.diffuse.r, obj.material.diffuse.g, obj.material.diffuse.b);
    glBindVertexArray(gl.vao);
    glDrawElements(GL_TRIANGLES, gl.indexCount, GL_UNSIGNED_INT, nullptr);

    if (selected) {
        glUniform1i(m_locUseTex, 0);
        glUniformMatrix4fv(m_locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform1i(m_locFlat, 1);
        glUniform3f(m_locColor, 1.0f, 0.35f, 0.21f);
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f, -1.0f);
        if (m_glPolygonMode) m_glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f);
        glDrawElements(GL_TRIANGLES, gl.indexCount, GL_UNSIGNED_INT, nullptr);
        if (m_glPolygonMode) m_glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_POLYGON_OFFSET_LINE);
    }
    glBindVertexArray(0);
}

void ViewportWidget::renderGizmo(const glm::mat4& vp) {
    if (m_mode == TransformMode::Vertices) return;

    const SceneObject& obj = m_objects[m_selected];
    float len = gizmoLength();
    glm::vec3 base = obj.position;

    static const glm::vec3 axisColors[3] = {
        {0.85f, 0.25f, 0.25f}, {0.30f, 0.80f, 0.30f}, {0.30f, 0.50f, 0.95f}
    };

    glDisable(GL_DEPTH_TEST);
    glUniform1i(m_locFlat, 1);
    glUniform1i(m_locUseTex, 0);
    glLineWidth(2.0f);

    const MeshGL& cubeGl = ensureGL(m_cubeMesh);

    for (int a = 0; a < 3; ++a) {
        bool hot = (a == m_hoverAxis) || (m_gizmoDragging && a == m_gizmoAxis);
        glm::vec3 color = hot ? glm::vec3(1.0f, 0.8f, 0.3f) : axisColors[a];
        glUniform3f(m_locColor, color.r, color.g, color.b);

        if (m_mode == TransformMode::Rotate) {
            glm::mat4 ring = glm::translate(glm::mat4(1.0f), base)
                           * mapZToAxis(a)
                           * glm::scale(glm::mat4(1.0f), glm::vec3(len * 0.9f));
            glUniformMatrix4fv(m_locMVP, 1, GL_FALSE, glm::value_ptr(vp * ring));
            glBindVertexArray(m_circleVao);
            glDrawArrays(GL_LINE_LOOP, 0, 64);
        } else {
            glm::vec3 axis(0.0f); axis[a] = 1.0f;
            glm::vec3 end = base + axis * len;

            glm::mat4 lineModel = glm::translate(glm::mat4(1.0f), base)
                                * mapXToAxis(a)
                                * glm::scale(glm::mat4(1.0f), glm::vec3(len, 1.0f, 1.0f));
            glUniformMatrix4fv(m_locMVP, 1, GL_FALSE, glm::value_ptr(vp * lineModel));
            glBindVertexArray(m_segmentVao);
            glDrawArrays(GL_LINES, 0, 2);

            if (m_mode == TransformMode::Scale) {
                float h = len * 0.12f;
                glm::mat4 handle = glm::translate(glm::mat4(1.0f), end)
                                 * glm::scale(glm::mat4(1.0f), glm::vec3(h));
                glUniformMatrix4fv(m_locMVP, 1, GL_FALSE, glm::value_ptr(vp * handle));
                if (m_glPolygonMode) m_glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glBindVertexArray(cubeGl.vao);
                glDrawElements(GL_TRIANGLES, cubeGl.indexCount, GL_UNSIGNED_INT, nullptr);
                if (m_glPolygonMode) m_glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            else {
                float coneH = len * 0.22f, coneR = len * 0.06f;
                glm::mat4 coneModel = glm::translate(glm::mat4(1.0f), end - axis * coneH)
                                    * mapYToAxis(a)
                                    * glm::scale(glm::mat4(1.0f), glm::vec3(coneR, coneH, coneR));
                glUniformMatrix4fv(m_locMVP, 1, GL_FALSE, glm::value_ptr(vp * coneModel));
                glBindVertexArray(m_coneVao);
                glDrawArrays(GL_TRIANGLES, 0, 48);
            }
        }
    }

    glLineWidth(1.0f);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void ViewportWidget::paintGL() {
    float dt = std::clamp(float(m_clock.restart()) / 1000.0f, 0.0f, 0.1f);
    applyKeyboard(dt);

    // Keep background color actual every frame — so changing the theme does not require GL recreation
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // Meshes modified by vertex dragging: re-upload data to GPU
    for (const MeshData* dm : m_dirtyMeshes) {
        auto it = m_meshGL.find(dm);
        if (it == m_meshGL.end()) continue;
        glBindBuffer(GL_ARRAY_BUFFER, it->vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        GLsizeiptr(dm->vertices.size() * sizeof(MeshData::Vertex)),
                        dm->vertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    m_dirtyMeshes.clear();

    glm::mat4 vp = buildProjectionMatrix() * buildViewMatrix();

    glUseProgram(m_program);
    glUniform1i(m_locSampler, 0);
    glm::vec3 eyePos(glm::inverse(buildViewMatrix()) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    glUniform3f(m_locCamPos, eyePos.x, eyePos.y, eyePos.z);
    for (int i = 0; i < m_objects.size(); ++i)
        renderObject(m_objects[i], vp, i == m_selected);

    if (m_selected >= 0)
        renderGizmo(vp);
    if (m_hover.index >= 0)
        drawHoverTriangle(vp);
    if (m_mode == TransformMode::Vertices && m_selected >= 0)
        drawVertexHandles(vp);
}

void ViewportWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

// --- Mouse ---

void ViewportWidget::mousePressEvent(QMouseEvent* e) {
    m_lastPos = e->position().toPoint();

    if (e->button() == Qt::MiddleButton) {
        m_orbiting = true;
        return;
    }
    if (e->button() != Qt::LeftButton) return;

    if (m_mode == TransformMode::Vertices && m_selected >= 0) {
        QPoint vPos = e->position().toPoint();
        int hv = pickVertex(vPos, 14.0f);
        if (hv >= 0) {
            m_hoverVertex = hv;
            beginVertexDrag(vPos);
            return;
        }
    }

    int axis = -1;
    if (m_selected >= 0 && pickGizmo(m_lastPos, axis)) {
        m_gizmoDragging = true;
        m_gizmoAxis = axis;
        m_startState = m_objects[m_selected];
        m_dragStartMouse = e->position();
        if (m_mode == TransformMode::Rotate) {
            QPointF c = projectToScreen(m_startState.position);
            m_lastDragAngle = std::atan2(float(e->position().y() - c.y()),
                                         float(e->position().x() - c.x()));
        }
        return;
    }

    int hit = -1;
    if (pickObject(m_lastPos, hit)) {
        setSelected(hit);
        emit statusMessage(QString("Selected: %1 (WASD — camera, G/R/V — mode)")
                               .arg(m_objects[hit].name));
    } else {
        setSelected(-1);
        emit statusMessage("Ready");
    }
    m_orbiting = true;
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* e) {
    QPointF posF = e->position();
    QPoint pos = posF.toPoint();
    QPoint delta = pos - m_lastPos;
    m_lastPos = pos;

    if (m_gizmoDragging && m_selected >= 0 && m_dragVertex < 0) {
        applyGizmoDrag(posF);
        update();
        return;
    }

    if (m_orbiting && m_dragVertex < 0) {
        m_yaw += delta.x() * 0.4f;
        m_pitch = glm::clamp(m_pitch + delta.y() * 0.4f, -89.0f, 89.0f);
        update();
        return;
    }

    if (m_selected >= 0 && m_dragVertex < 0) {
        int axis = -1;
        pickGizmo(pos, axis);
        if (axis != m_hoverAxis) {
            m_hoverAxis = axis;
            update();
        }
    }

    if (m_mode == TransformMode::Vertices) {
        if (m_dragVertex >= 0) {
            updateVertexDrag(pos);
        } else if (m_selected >= 0) {
            int hv = pickVertex(pos, 14.0f);
            if (hv != m_hoverVertex) { m_hoverVertex = hv; update(); }
        }
    } else {
        updateHover(pos);
    }
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (m_dragVertex >= 0) { endVertexDrag(); return; }

    if (e->button() == Qt::LeftButton) {
        m_orbiting = false;
        if (m_gizmoDragging) {
            m_gizmoDragging = false;
            m_gizmoAxis = -1;
            SceneObject& o = m_objects[m_selected];
            glm::vec3 after[3] = { o.position, o.rotation, o.scale };
            glm::vec3 before[3] = { m_startState.position, m_startState.rotation,
                                    m_startState.scale };
            pushCommand(std::make_shared<TransformCmd>(this, m_selected,
                                                       before, after,
                                                       m_clock.elapsed()),
                        true, false);
            emit statusMessage("Ready");
        }
    }
}

void ViewportWidget::wheelEvent(QWheelEvent* e) {
    m_distance = glm::clamp(m_distance * (e->angleDelta().y() > 0 ? 0.9f : 1.1f),
                            1.0f, 50.0f);
    update();
}