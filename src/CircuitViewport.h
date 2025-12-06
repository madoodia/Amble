#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QColor>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPointF>
#include <QVector>
#include <QString>

// Wire connection structure
struct Wire
{
    int fromComponentId;
    int toComponentId;
    QVector<QPointF> points; // For curved/bent wires
    QColor color;

    Wire(int from = -1, int to = -1, const QColor& c = QColor(255, 255, 0))
        : fromComponentId(from), toComponentId(to), color(c) {}

    bool operator==(const Wire& other) const
    {
        return fromComponentId == other.fromComponentId &&
               toComponentId == other.toComponentId &&
               points == other.points &&
               color == other.color;
    }
};

// Component structure
struct Component
{
    int id;
    QString type;
    QPointF position;
    QColor color;
    float width;
    float height;
    bool selected;
    float rotation; // For future use

    // Connection terminals (input/output points)
    QVector<QPointF> inputTerminals;
    QVector<QPointF> outputTerminals;

    Component(int componentId, const QString& t, const QPointF& pos, const QColor& c = QColor(255, 255, 255),
              float w = 40.0f, float h = 20.0f)
        : id(componentId), type(t), position(pos), color(c), width(w), height(h), selected(false), rotation(0.0f)
    {
        setupTerminals();
    }

    void setupTerminals()
    {
        inputTerminals.clear();
        outputTerminals.clear();

        if (type == "Resistor" || type == "Inductor")
        {
            // Two-terminal components
            inputTerminals.append(QPointF(position.x(), position.y() + height / 2));
            outputTerminals.append(QPointF(position.x() + width, position.y() + height / 2));
        }
        else if (type == "Capacitor")
        {
            // Two-terminal components
            inputTerminals.append(QPointF(position.x(), position.y() + height / 2));
            outputTerminals.append(QPointF(position.x() + width, position.y() + height / 2));
        }
        else if (type == "Voltage Source")
        {
            // Source has positive and negative terminals
            inputTerminals.append(QPointF(position.x(), position.y() + height / 2));          // Negative
            outputTerminals.append(QPointF(position.x() + width, position.y() + height / 2)); // Positive
        }
    }

    // Equality operator for Qt container comparison
    bool operator==(const Component& other) const
    {
        return id == other.id &&
               type == other.type &&
               position == other.position &&
               color == other.color &&
               qFuzzyCompare(width, other.width) &&
               qFuzzyCompare(height, other.height) &&
               selected == other.selected;
    }

    bool operator!=(const Component& other) const
    {
        return !(*this == other);
    }

    // Helper methods
    bool containsPoint(const QPointF& point) const
    {
        return (point.x() >= position.x() && point.x() <= position.x() + width &&
                point.y() >= position.y() && point.y() <= position.y() + height);
    }

    QPointF getTerminal(bool isOutput, int index = 0) const
    {
        if (isOutput && index < outputTerminals.size())
        {
            return outputTerminals[index];
        }
        else if (!isOutput && index < inputTerminals.size())
        {
            return inputTerminals[index];
        }
        return QPointF(position.x() + width / 2, position.y() + height / 2); // Fallback to center
    }
};

class CircuitViewport : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(float gridSize READ gridSize WRITE setGridSize NOTIFY gridSizeChanged)
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY gridColorChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(float zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(QPointF panOffset READ panOffset WRITE setPanOffset NOTIFY panOffsetChanged)

public:
    explicit CircuitViewport(QQuickItem* parent = nullptr);
    Renderer* createRenderer() const override;

    float gridSize() const { return m_gridSize; }
    void setGridSize(float size);

    QColor gridColor() const { return m_gridColor; }
    void setGridColor(const QColor& color);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    // Zoom and pan
    float zoom() const { return m_zoom; }
    void setZoom(float z);
    Q_INVOKABLE void zoomAtPosition(float zoomFactor, const QPointF& position);
    QPointF panOffset() const { return m_panOffset; }
    void setPanOffset(const QPointF& offset);

    // Component management
    Q_INVOKABLE void addComponent(const QString& type, float x, float y);
    Q_INVOKABLE void clearComponents();
    Q_INVOKABLE void selectComponent(float x, float y);
    Q_INVOKABLE void deselectAll();
    Q_INVOKABLE void moveSelectedComponents(float deltaX, float deltaY);
    Q_INVOKABLE void snapSelectedToGrid();
    const QVector<Component>& components() const { return m_components; }

    // Wire management
    Q_INVOKABLE void startWire(int componentId);
    Q_INVOKABLE void finishWire(int componentId);
    Q_INVOKABLE void cancelWire();
    Q_INVOKABLE void handleWireConnection(int componentId);
    Q_INVOKABLE int getComponentAtPosition(float x, float y);
    const QVector<Wire>& wires() const { return m_wires; }

    // Coordinate transformation
    QPointF screenToWorld(const QPointF& screenPos) const;
    QPointF worldToScreen(const QPointF& worldPos) const;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

signals:
    void gridSizeChanged();
    void gridColorChanged();
    void backgroundColorChanged();
    void rightClicked(float x, float y);
    void componentAdded();
    void zoomChanged();
    void panOffsetChanged();
    void componentSelected(int componentId);
    void wireStarted(int componentId);
    void wireFinished(int fromId, int toId);

private:
    float m_gridSize = 20.0f;
    QColor m_gridColor = QColor(200, 100, 100, 255);
    QColor m_backgroundColor = QColor(30, 30, 30, 255);
    QVector<Component> m_components;
    QVector<Wire> m_wires;
    QPointF m_lastRightClickPos;

    // Zoom and pan
    float m_zoom = 1.0f;
    QPointF m_panOffset = QPointF(0, 0);

    // Selection and interaction
    int m_nextComponentId = 1;
    int m_selectedComponentId = -1;
    bool m_dragging = false;
    QPointF m_lastMousePos;
    bool m_panning = false;

    // Wire creation
    bool m_creatingWire = false;
    int m_wireStartComponentId = -1;

    // Helper methods
    int getComponentAt(const QPointF& pos) const;
    QPointF snapToGrid(const QPointF& pos) const;
};

// Inherit from QOpenGLFunctions instead of specific version
class CircuitRenderer : public QQuickFramebufferObject::Renderer,
                        protected QOpenGLFunctions
{
public:
    CircuitRenderer();
    ~CircuitRenderer();

    void render() override;
    void synchronize(QQuickFramebufferObject* item) override;
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;

private:
    void initializeGL();
    void updateGridGeometry();
    void updateComponentGeometry();
    void updateWireGeometry();
    void updateDotGeometry();
    void renderGrid();
    void renderDots();
    void renderComponents();
    void renderTerminals();
    void renderWires();
    void renderResistor(const Component& comp);
    void renderCapacitor(const Component& comp);
    void renderInductor(const Component& comp);
    void renderVoltageSource(const Component& comp);

    QOpenGLShaderProgram* m_gridProgram = nullptr;
    QOpenGLShaderProgram* m_componentProgram = nullptr;
    QOpenGLShaderProgram* m_wireProgram = nullptr;
    QOpenGLShaderProgram* m_dotProgram = nullptr;
    QOpenGLBuffer m_gridVBO;
    QOpenGLBuffer m_componentVBO;
    QOpenGLBuffer m_wireVBO;
    QOpenGLBuffer m_dotVBO;
    QOpenGLVertexArrayObject m_gridVAO;
    QOpenGLVertexArrayObject m_componentVAO;
    QOpenGLVertexArrayObject m_wireVAO;
    QOpenGLVertexArrayObject m_dotVAO;

    // Data copied from UI
    float m_gridSize = 20.0f;
    QColor m_gridColor;
    QColor m_backgroundColor;
    QSize m_viewportSize;
    QVector<Component> m_components;
    QVector<Wire> m_wires;
    float m_zoom = 1.0f;
    QPointF m_panOffset;

    bool m_initialized = false;
    bool m_gridDirty = true;
    bool m_componentsDirty = true;
    bool m_wiresDirty = true;
    bool m_dotsDirty = true;

    // Vertex counts for rendering
    int m_gridVertexCount = 0;
    int m_componentVertexCount = 0;
};
