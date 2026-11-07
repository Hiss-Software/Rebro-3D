#pragma once

#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QElapsedTimer>
#include <QTimer>
#include <QSet>
#include <QVector>
#include <QHash>
#include <QPointF>

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SceneObject.h"
#include "UndoStack.h"

class ViewportWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions {
    Q_OBJECT
public:
    enum class TransformMode { Move, Rotate, Scale, Vertices };

    explicit ViewportWidget(QWidget* parent = nullptr);
    ~ViewportWidget() override;

    int addObject(const QString& name,
                  std::shared_ptr<MeshData> mesh,
                  const QString& meshPath = QString(),
                  const QString& shapeId = "cube");
    void clearScene();
    void deleteSelected();
    void setSelected(int index);
    int selectedIndex() const { return m_selected; }
    const QVector<SceneObject>& objects() const { return m_objects; }

    void setObjects(const QVector<SceneObject>& objs);

    bool saveScene(const QString& path, QString* error = nullptr);
    bool loadScene(const QString& path, QString* error = nullptr);

    void renameSelected(const QString& name);

    void setObjectTransform(int index, const glm::vec3& pos,
                            const glm::vec3& rot, const glm::vec3& scale);
    void setObjectTransformTracked(int index, const glm::vec3& pos,
                                   const glm::vec3& rot, const glm::vec3& scale);
    void setMaterialColorTracked(int index, const glm::vec3& color);
    void setMaterialTextureTracked(int index, const QString& path);

    void setTransformMode(TransformMode mode);
    TransformMode transformMode() const { return m_mode; }

    void setCameraSpeed(float unitsPerSecond);
    void setClearColor(const glm::vec3& c) { m_clearColor = c; update(); }
    void setFastMode(bool on);

    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;

signals:
    void selectionChanged(int index);
    void sceneChanged();
    void statusMessage(const QString& text);
    void undoRedoChanged();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // --- Undo/Redo Commands ---
    class CmdBase : public ICommand {
    public:
        explicit CmdBase(ViewportWidget* vp) : vp(vp) {}
        ViewportWidget* vp;
    };

    class TransformCmd : public CmdBase {
    public:
        TransformCmd(ViewportWidget* vp, int idx,
                     const glm::vec3* before, const glm::vec3* after, qint64 ts);
        void undo() override;
        void redo() override;
        bool tryMerge(ICommand* incoming) override;
    private:
        int m_idx;
        glm::vec3 m_before[3], m_after[3]; // pos, rot, scale
        qint64 m_ts;
    };

    class AddObjectCmd : public CmdBase {
    public:
        AddObjectCmd(ViewportWidget* vp, int idx, SceneObject obj);
        void undo() override;
        void redo() override;
    private:
        int m_idx;
        SceneObject m_obj;
    };

    class DeleteObjectCmd : public CmdBase {
    public:
        DeleteObjectCmd(ViewportWidget* vp, int idx, SceneObject obj);
        void undo() override;
        void redo() override;
    private:
        int m_idx;
        SceneObject m_obj;
    };

    class RenameCmd : public CmdBase {
    public:
        RenameCmd(ViewportWidget* vp, int idx, QString before, QString after);
        void undo() override;
        void redo() override;
    private:
        int m_idx;
        QString m_before, m_after;
    };

    class MaterialCmd : public CmdBase {
    public:
        MaterialCmd(ViewportWidget* vp, int idx,
                    const Material& before, const Material& after, qint64 ts);
        void undo() override;
        void redo() override;
        bool tryMerge(ICommand* incoming) override;
    private:
        int m_idx;
        Material m_before, m_after;
        qint64 m_ts;
    };

    class VertexEditCmd : public CmdBase {
    public:
        VertexEditCmd(ViewportWidget* v, int objIdx, int vi,
                      const glm::vec3& before, const glm::vec3& after)
            : CmdBase(v), m_objIdx(objIdx), m_vi(vi),
              m_before(before), m_after(after) {}
        void undo() override { vp->setVertexDirect(m_objIdx, m_vi, m_before); }
        void redo() override { vp->setVertexDirect(m_objIdx, m_vi, m_after); }
    private:
        int m_objIdx;
        int m_vi;
        glm::vec3 m_before;
        glm::vec3 m_after;
    };

    // --- Internal operations for commands ---
    void applyTransformDirect(int index, const glm::vec3& p,
                              const glm::vec3& r, const glm::vec3& s);
    void insertObjectAt(int index, const SceneObject& obj);
    void removeObjectAt(int index);
    void setNameDirect(int index, const QString& name);
    void applyMaterialDirect(int index, const Material& m);
    void pushCommand(std::shared_ptr<ICommand> cmd, bool alreadyApplied, bool allowMerge);

    struct MeshGL {
        GLuint vao = 0, vbo = 0, ibo = 0;
        int indexCount = 0;
    };

    GLuint m_program = 0;
    GLint m_locMVP = -1, m_locModel = -1, m_locFlat = -1, m_locColor = -1;
    GLint m_locUseTex = -1, m_locSampler = -1, m_locCamPos = -1;

    QHash<const MeshData*, MeshGL> m_meshGL;
    std::shared_ptr<MeshData> m_cubeMesh;

    QHash<QString, QOpenGLTexture*> m_textures;
    QOpenGLTexture* ensureTexture(const QString& path);

    GLuint m_segmentVao = 0, m_segmentVbo = 0;
    GLuint m_circleVao = 0, m_circleVbo = 0;
    GLuint m_coneVao = 0, m_coneVbo = 0;

    QVector<SceneObject> m_objects;
    int m_selected = -1;
    TransformMode m_mode = TransformMode::Move;
    CommandStack m_stack;

    QTimer m_frameTimer;

    float m_yaw = 45.0f, m_pitch = 30.0f, m_distance = 5.0f;
    glm::vec3 m_clearColor{0.96f, 0.95f, 0.92f};
    glm::vec3 m_target{0.0f, 0.0f, 0.0f};
    QPoint m_lastPos;
    bool m_orbiting = false;

    QSet<int> m_pressedKeys;
    QElapsedTimer m_clock;
    float m_speed = 5.0f;
    bool m_fastMode = false;

    bool m_gizmoDragging = false;
    int m_gizmoAxis = -1;
    int m_hoverAxis = -1;
    SceneObject m_startState;
    QPointF m_dragStartMouse;
    float m_lastDragAngle = 0.0f;

    // glPolygonMode is not in Qt's core profile: fetch it from the context
    using PFN_glPolygonMode = void(APIENTRY*)(GLenum mode, GLenum face);
    PFN_glPolygonMode m_glPolygonMode = nullptr;

    glm::mat4 buildViewMatrix() const;
    glm::mat4 buildProjectionMatrix() const;
    glm::mat4 buildModelMatrix(const SceneObject& obj) const;
    glm::mat4 cameraRotation() const;
    glm::vec3 forwardVec() const;
    glm::vec3 rightVec() const;
    glm::vec3 upVec() const;
    void applyKeyboard(float dt);

    static bool mapNavKey(int qtKey, int& outNavKey);
    static bool mapCommandKey(int qtKey, int& outCommand);
    static bool isTextEntry(QObject* obj);

    MeshGL& ensureGL(const std::shared_ptr<MeshData>& mesh);
    void frameToBounds(const MeshData& mesh);
    float gizmoLength() const;
    QPointF projectToScreen(const glm::vec3& world) const;
    bool pickObject(const QPoint& pos, int& outIndex) const;

    struct RayHit {
        float t = 0.0f;
        unsigned tri[3] = {0, 0, 0};
        glm::vec3 point{0.0f};
    };
    struct HoverInfo {
        int index = -1;
        unsigned tri[3] = {0, 0, 0};
        glm::vec3 point{0.0f};
    };

    bool pickPrecise(const QPoint& pos, int& outIndex, RayHit* hit = nullptr) const;
    void updateHover(const QPoint& pos);
    void drawHoverTriangle(const glm::mat4& vp);

    HoverInfo m_hover;
    GLuint m_hoverVao = 0, m_hoverVbo = 0;

    // Vertex mode
    int m_hoverVertex = -1;
    int m_dragVertex = -1;
    glm::vec3 m_dragBeforeLocal{0.0f};
    glm::vec3 m_dragStartWorld{0.0f};
    glm::vec3 m_dragPlaneNormal{0.0f};
    QVector<int> m_dragGroup;                 // indices of duplicate vertices being dragged
    std::vector<glm::vec3> m_dragStartLocal;  // their positions at drag start
    QSet<const MeshData*> m_dirtyMeshes;
    GLuint m_handleVao = 0, m_handleVbo = 0;

    int pickVertex(const QPoint& pos, float maxDistPx) const;
    void beginVertexDrag(const QPoint& pos);
    void updateVertexDrag(const QPoint& pos);
    void endVertexDrag();
    void drawVertexHandles(const glm::mat4& vp);
    void setVertexDirect(int objIndex, int vi, const glm::vec3& localPos);
    static void recomputeNormalsAround(const std::shared_ptr<MeshData>& mesh, int vi);

    bool pickGizmo(const QPoint& pos, int& outAxis) const;
    void applyGizmoDrag(const QPointF& mousePos);
    void renderObject(const SceneObject& obj, const glm::mat4& vp, bool selected);
    void renderGizmo(const glm::mat4& vp);
};