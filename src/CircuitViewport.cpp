#include "CircuitViewport.h"

// --- ADD THIS INCLUDE ---
#include <QOpenGLFramebufferObject>
// ------------------------

#include <QQuickWindow>
#include <QOpenGLContext>
#include <QDebug> // Good to have for logging
#include <QtMath> // For M_PI and trig functions
#include <QtMath> // For M_PI and math functions

// --- CircuitViewport Implementation ---

CircuitViewport::CircuitViewport(QQuickItem* parent)
    : QQuickFramebufferObject(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
    setFlag(QQuickItem::ItemAcceptsInputMethod, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setMirrorVertically(true);
}

QQuickFramebufferObject::Renderer* CircuitViewport::createRenderer() const
{
    return new CircuitRenderer();
}

void CircuitViewport::setGridSize(float size)
{
    if (qFuzzyCompare(m_gridSize, size))
        return;
    m_gridSize = size;
    emit gridSizeChanged();
    update();
}

void CircuitViewport::setGridColor(const QColor& color)
{
    if (m_gridColor == color)
        return;
    m_gridColor = color;
    emit gridColorChanged();
    update();
}

void CircuitViewport::setBackgroundColor(const QColor& color)
{
    if (m_backgroundColor == color)
        return;
    m_backgroundColor = color;
    emit backgroundColorChanged();
    update();
}

void CircuitViewport::setZoom(float z)
{
    z = qMax(0.1f, qMin(10.0f, z)); // Clamp zoom between 0.1x and 10x
    if (qFuzzyCompare(m_zoom, z))
        return;
    m_zoom = z;
    emit zoomChanged();
    update();
}

void CircuitViewport::setPanOffset(const QPointF& offset)
{
    if (m_panOffset == offset)
        return;
    m_panOffset = offset;
    emit panOffsetChanged();
    update();
}

void CircuitViewport::addComponent(const QString& type, float x, float y)
{
    // Convert screen coordinates to world coordinates
    QPointF worldPos = screenToWorld(QPointF(x, y));
    QPointF snappedPos = snapToGrid(worldPos);

    // Choose color based on component type
    QColor componentColor = QColor(100, 150, 255); // Default blue
    if (type == "Resistor")
        componentColor = QColor(255, 100, 100); // Red
    else if (type == "Capacitor")
        componentColor = QColor(100, 255, 100); // Green
    else if (type == "Inductor")
        componentColor = QColor(255, 255, 100); // Yellow
    else if (type == "Voltage Source")
        componentColor = QColor(255, 150, 100); // Orange

    Component newComponent(m_nextComponentId++, type, snappedPos, componentColor);
    newComponent.setupTerminals();
    m_components.append(newComponent);
    emit componentAdded();
    update();
}

void CircuitViewport::clearComponents()
{
    m_components.clear();
    m_wires.clear();
    m_nextComponentId = 1;
    m_selectedComponentId = -1;
    update();
}

void CircuitViewport::selectComponent(float x, float y)
{
    QPointF worldPos = screenToWorld(QPointF(x, y));
    int componentId = getComponentAt(worldPos);

    qDebug() << "Selecting at screen" << QPointF(x, y) << "world" << worldPos << "found component" << componentId;

    // Deselect all first
    for (Component& comp : m_components)
    {
        comp.selected = false;
    }

    if (componentId >= 0)
    {
        for (Component& comp : m_components)
        {
            if (comp.id == componentId)
            {
                comp.selected = true;
                m_selectedComponentId = componentId;
                qDebug() << "Selected component" << componentId << "at position" << comp.position;
                emit componentSelected(componentId);
                break;
            }
        }
    }
    else
    {
        m_selectedComponentId = -1;
    }

    update();
}

void CircuitViewport::deselectAll()
{
    for (Component& comp : m_components)
    {
        comp.selected = false;
    }
    m_selectedComponentId = -1;
    update();
}

void CircuitViewport::moveSelectedComponents(float deltaX, float deltaY)
{
    // Convert screen delta to world delta
    QPointF worldDelta = QPointF(deltaX / m_zoom, deltaY / m_zoom);

    bool moved = false;
    for (Component& comp : m_components)
    {
        if (comp.selected)
        {
            comp.position += worldDelta;
            moved = true;
            qDebug() << "Moving component" << comp.id << "to" << comp.position;
        }
    }

    if (moved)
    {
        update();
    }
}

void CircuitViewport::snapSelectedToGrid()
{
    for (Component& comp : m_components)
    {
        if (comp.selected)
        {
            comp.position = snapToGrid(comp.position);
        }
    }
    update();
}

void CircuitViewport::mousePressEvent(QMouseEvent* event)
{
    event->accept();
    m_lastMousePos = event->position();

    if (event->button() == Qt::RightButton)
    {
        m_lastRightClickPos = event->position();
        qDebug() << "Right mouse press at:" << event->position();
    }
    else if (event->button() == Qt::LeftButton)
    {
        QPointF worldPos = screenToWorld(event->position());
        int componentId = getComponentAt(worldPos);

        if (componentId >= 0)
        {
            selectComponent(event->position().x(), event->position().y());
            m_dragging = true;
        }
        else
        {
            deselectAll();
        }
    }
    else if (event->button() == Qt::MiddleButton)
    {
        m_panning = true;
    }

    QQuickFramebufferObject::mousePressEvent(event);
}

void CircuitViewport::mouseReleaseEvent(QMouseEvent* event)
{
    event->accept();
    if (event->button() == Qt::RightButton)
    {
        // Emit signal for context menu with coordinates
        qDebug() << "Right mouse release at:" << event->position();
        emit rightClicked(event->position().x(), event->position().y());
    }
    else if (event->button() == Qt::LeftButton)
    {
        if (m_dragging)
        {
            // Snap to grid when drag ends
            snapSelectedToGrid();
        }
        m_dragging = false;
        m_panning = false;
    }
    QQuickFramebufferObject::mouseReleaseEvent(event);
}

void CircuitViewport::mouseMoveEvent(QMouseEvent* event)
{
    event->accept();
    QPointF currentPos = event->position();

    if (m_panning && event->buttons() & Qt::MiddleButton)
    {
        QPointF delta = currentPos - m_lastMousePos;
        setPanOffset(m_panOffset + delta);
    }
    else if (m_dragging && event->buttons() & Qt::LeftButton && m_selectedComponentId >= 0)
    {
        QPointF delta = currentPos - m_lastMousePos;
        moveSelectedComponents(delta.x(), delta.y());
    }

    m_lastMousePos = currentPos;
    QQuickFramebufferObject::mouseMoveEvent(event);
}

void CircuitViewport::wheelEvent(QWheelEvent* event)
{
    event->accept();
    float zoomFactor = event->angleDelta().y() > 0 ? 1.1f : 0.9f;
    setZoom(m_zoom * zoomFactor);
}

void CircuitViewport::startWire(int componentId)
{
    m_creatingWire = true;
    m_wireStartComponentId = componentId;
    emit wireStarted(componentId);
}

void CircuitViewport::finishWire(int componentId)
{
    if (m_creatingWire && m_wireStartComponentId >= 0 && componentId >= 0 && componentId != m_wireStartComponentId)
    {
        // Find the components
        Component* startComp = nullptr;
        Component* endComp = nullptr;

        for (Component& comp : m_components)
        {
            if (comp.id == m_wireStartComponentId)
                startComp = &comp;
            if (comp.id == componentId)
                endComp = &comp;
        }

        if (startComp && endComp)
        {
            // Create wire between components
            Wire newWire(m_wireStartComponentId, componentId);

            // Add terminal positions as wire route points
            QPointF startPos = startComp->getTerminal(true, 0); // Output terminal
            QPointF endPos = endComp->getTerminal(false, 0);    // Input terminal
            newWire.points.append(startPos);
            newWire.points.append(endPos);

            m_wires.append(newWire);
            qDebug() << "Created wire from component" << m_wireStartComponentId << "to" << componentId;
            emit wireFinished(m_wireStartComponentId, componentId);
        }
    }
    cancelWire();
}

void CircuitViewport::cancelWire()
{
    m_creatingWire = false;
    m_wireStartComponentId = -1;
    update();
}

void CircuitViewport::handleWireConnection(int componentId)
{
    if (!m_creatingWire)
    {
        // Start a new wire
        startWire(componentId);
        qDebug() << "Starting wire from component" << componentId;
    }
    else
    {
        // Finish the wire
        finishWire(componentId);
    }
}

int CircuitViewport::getComponentAtPosition(float x, float y)
{
    QPointF worldPos = screenToWorld(QPointF(x, y));
    return getComponentAt(worldPos);
}

QPointF CircuitViewport::screenToWorld(const QPointF& screenPos) const
{
    return (screenPos - m_panOffset) / m_zoom;
}

QPointF CircuitViewport::worldToScreen(const QPointF& worldPos) const
{
    return worldPos * m_zoom + m_panOffset;
}

int CircuitViewport::getComponentAt(const QPointF& pos) const
{
    for (const Component& comp : m_components)
    {
        if (comp.containsPoint(pos))
        {
            return comp.id;
        }
    }
    return -1;
}

QPointF CircuitViewport::snapToGrid(const QPointF& pos) const
{
    float snappedX = qRound(pos.x() / m_gridSize) * m_gridSize;
    float snappedY = qRound(pos.y() / m_gridSize) * m_gridSize;
    return QPointF(snappedX, snappedY);
}

// --- CircuitRenderer Implementation ---

CircuitRenderer::CircuitRenderer()
{
}

CircuitRenderer::~CircuitRenderer()
{
    delete m_gridProgram;
    delete m_componentProgram;
    delete m_wireProgram;
    delete m_dotProgram;
}

void CircuitRenderer::synchronize(QQuickFramebufferObject* item)
{
    auto* vp = static_cast<CircuitViewport*>(item);

    QSize newSize = vp->size().toSize();
    QColor newGridColor = vp->gridColor();
    QVector<Component> newComponents = vp->components();
    QVector<Wire> newWires = vp->wires();
    float newZoom = vp->zoom();
    QPointF newPanOffset = vp->panOffset();

    if (newSize != m_viewportSize || !qFuzzyCompare(m_gridSize, vp->gridSize()) || newGridColor != m_gridColor ||
        !qFuzzyCompare(m_zoom, newZoom) || m_panOffset != newPanOffset)
    {
        m_gridDirty = true;
        m_dotsDirty = true;
    }

    if (newComponents != m_components)
    {
        m_componentsDirty = true;
    }

    if (newWires != m_wires)
    {
        m_wiresDirty = true;
    }

    m_viewportSize = newSize;
    m_gridSize = vp->gridSize();
    m_gridColor = newGridColor;
    m_backgroundColor = vp->backgroundColor();
    m_components = newComponents;
    m_wires = newWires;
    m_zoom = newZoom;
    m_panOffset = newPanOffset;

    qDebug() << "Synchronized - Size:" << m_viewportSize << "Grid Size:" << m_gridSize << "Components:" << m_components.size() << "Wires:" << m_wires.size() << "Zoom:" << m_zoom;
}

void CircuitRenderer::render()
{
    if (!m_initialized)
    {
        initializeOpenGLFunctions();
        initializeGL();
        m_initialized = true;
    }

    if (m_gridDirty)
    {
        updateGridGeometry();
        m_gridDirty = false;
    }

    // Reset QML State
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Clear Background
    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // --- FIX: Use Physical FBO Size for Viewport ---
    if (framebufferObject())
    {
        QSize physicalSize = framebufferObject()->size();
        glViewport(0, 0, physicalSize.width(), physicalSize.height());

        // Update geometry if needed
        if (m_componentsDirty)
        {
            updateComponentGeometry();
            m_componentsDirty = false;
        }

        if (m_wiresDirty)
        {
            updateWireGeometry();
            m_wiresDirty = false;
        }

        if (m_dotsDirty)
        {
            updateDotGeometry();
            m_dotsDirty = false;
        }

        // Render in order: grid, dots, components, wires
        renderGrid();
        renderDots();
        renderComponents();
        renderWires();
    }

    // Cleanup is handled in individual render methods
}

QOpenGLFramebufferObject* CircuitRenderer::createFramebufferObject(const QSize& size)
{
    // This requires <QOpenGLFramebufferObject> to be included
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

void CircuitRenderer::initializeGL()
{
    m_gridProgram = new QOpenGLShaderProgram();

    bool isES = QOpenGLContext::currentContext()->isOpenGLES();
    QString version = isES ? "#version 300 es\n" : "#version 330 core\n";

    QString vertexShader = version + R"(
        layout (location = 0) in vec2 position;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(position, 0.0, 1.0);
        }
    )";

    QString fragmentShader;
    if (isES)
    {
        fragmentShader = version + R"(
            precision mediump float;
            uniform vec4 gridColor;
            out vec4 FragColor;
            void main() {
                FragColor = gridColor;
            }
        )";
    }
    else
    {
        fragmentShader = version + R"(
            uniform vec4 gridColor;
            out vec4 FragColor;
            void main() {
                FragColor = gridColor;
            }
        )";
    }

    if (!m_gridProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader))
        qWarning() << "Vertex Shader Error:" << m_gridProgram->log();

    if (!m_gridProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader))
        qWarning() << "Fragment Shader Error:" << m_gridProgram->log();

    if (!m_gridProgram->link())
        qWarning() << "Link Error:" << m_gridProgram->log();

    // Create component shader program
    m_componentProgram = new QOpenGLShaderProgram();

    QString componentVertexShader = version + R"(
        layout (location = 0) in vec2 position;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(position, 0.0, 1.0);
        }
    )";

    QString componentFragmentShader;
    if (isES)
    {
        componentFragmentShader = version + R"(
            precision mediump float;
            uniform vec4 componentColor;
            out vec4 FragColor;
            void main() {
                FragColor = componentColor;
            }
        )";
    }
    else
    {
        componentFragmentShader = version + R"(
            uniform vec4 componentColor;
            out vec4 FragColor;
            void main() {
                FragColor = componentColor;
            }
        )";
    }

    if (!m_componentProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, componentVertexShader))
        qWarning() << "Component Vertex Shader Error:" << m_componentProgram->log();

    if (!m_componentProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, componentFragmentShader))
        qWarning() << "Component Fragment Shader Error:" << m_componentProgram->log();

    if (!m_componentProgram->link())
        qWarning() << "Component Link Error:" << m_componentProgram->log();

    // Create wire shader program (same as grid/component)
    m_wireProgram = new QOpenGLShaderProgram();
    if (!m_wireProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, componentVertexShader))
        qWarning() << "Wire Vertex Shader Error:" << m_wireProgram->log();
    if (!m_wireProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, componentFragmentShader))
        qWarning() << "Wire Fragment Shader Error:" << m_wireProgram->log();
    if (!m_wireProgram->link())
        qWarning() << "Wire Link Error:" << m_wireProgram->log();

    // Create dot shader program with circle rendering
    m_dotProgram = new QOpenGLShaderProgram();

    QString dotVertexShader = version + R"(
        layout (location = 0) in vec2 position;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(position, 0.0, 1.0);
        }
    )";

    QString dotFragmentShader;
    if (isES)
    {
        dotFragmentShader = version + R"(
            precision mediump float;
            uniform vec4 dotColor;
            out vec4 FragColor;
            void main() {
                FragColor = dotColor;
            }
        )";
    }
    else
    {
        dotFragmentShader = version + R"(
            uniform vec4 dotColor;
            out vec4 FragColor;
            void main() {
                FragColor = dotColor;
            }
        )";
    }

    if (!m_dotProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, dotVertexShader))
        qWarning() << "Dot Vertex Shader Error:" << m_dotProgram->log();
    if (!m_dotProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, dotFragmentShader))
        qWarning() << "Dot Fragment Shader Error:" << m_dotProgram->log();
    if (!m_dotProgram->link())
        qWarning() << "Dot Link Error:" << m_dotProgram->log();

    // Create all VAOs and VBOs
    m_gridVAO.create();
    m_gridVBO.create();
    m_componentVAO.create();
    m_componentVBO.create();
    m_wireVAO.create();
    m_wireVBO.create();
    m_dotVAO.create();
    m_dotVBO.create();
}

void CircuitRenderer::updateGridGeometry()
{
    if (m_viewportSize.isEmpty())
        return;

    QVector<float> vertices;

    // Calculate world bounds considering zoom and pan
    float worldLeft = -m_panOffset.x() / m_zoom;
    float worldTop = -m_panOffset.y() / m_zoom;
    float worldRight = (m_viewportSize.width() - m_panOffset.x()) / m_zoom;
    float worldBottom = (m_viewportSize.height() - m_panOffset.y()) / m_zoom;

    // Extend bounds much further for truly unlimited grid
    float extension = m_gridSize * 50; // Much larger extension
    worldLeft -= extension;
    worldTop -= extension;
    worldRight += extension;
    worldBottom += extension;

    // Generate vertical lines
    float startX = floor(worldLeft / m_gridSize) * m_gridSize;
    for (float x = startX; x <= worldRight; x += m_gridSize)
    {
        vertices << x << worldTop << x << worldBottom;
    }

    // Generate horizontal lines
    float startY = floor(worldTop / m_gridSize) * m_gridSize;
    for (float y = startY; y <= worldBottom; y += m_gridSize)
    {
        vertices << worldLeft << y << worldRight << y;
    }

    // Store vertex count for rendering
    m_gridVertexCount = vertices.size() / 2; // Each vertex has 2 coordinates (x, y)

    if (!vertices.isEmpty())
    {
        m_gridVAO.bind();
        m_gridVBO.bind();
        m_gridVBO.allocate(vertices.data(), vertices.size() * sizeof(float));

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        m_gridVBO.release();
        m_gridVAO.release();
    }
}

void CircuitRenderer::renderGrid()
{
    if (!m_gridProgram || m_viewportSize.isEmpty())
        return;

    m_gridProgram->bind();

    // Use world-to-screen transformation
    QMatrix4x4 projection;
    projection.setToIdentity();
    projection.ortho(-m_panOffset.x() / m_zoom,
                     (m_viewportSize.width() - m_panOffset.x()) / m_zoom,
                     (m_viewportSize.height() - m_panOffset.y()) / m_zoom,
                     -m_panOffset.y() / m_zoom, -1, 1);
    projection.scale(m_zoom, m_zoom, 1);

    m_gridProgram->setUniformValue("projection", projection);

    // Convert QColor to QVector4D for proper uniform setting
    QVector4D colorVec(m_gridColor.redF(), m_gridColor.greenF(),
                       m_gridColor.blueF(), m_gridColor.alphaF());
    m_gridProgram->setUniformValue("gridColor", colorVec);

    m_gridVAO.bind();

    // Set line width to make grid more visible
    glLineWidth(1.0f);

    if (m_gridVertexCount > 0)
    {
        glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    }

    m_gridVAO.release();
    m_gridProgram->release();
}

void CircuitRenderer::updateComponentGeometry()
{
    if (m_components.isEmpty())
        return;

    QVector<float> vertices;

    // Create filled shapes for each component
    for (const Component& comp : m_components)
    {
        float x = comp.position.x();
        float y = comp.position.y();
        float w = comp.width;
        float h = comp.height;

        if (comp.type == "Resistor")
        {
            // Resistor: Rectangle with zigzag pattern
            // Main body (2 triangles = 6 vertices)
            vertices << x << y << x + w << y << x << y + h;
            vertices << x + w << y << x + w << y + h << x << y + h;
        }
        else if (comp.type == "Capacitor")
        {
            // Capacitor: Two parallel lines
            float mid = x + w / 2;
            float gap = w * 0.1f;
            // Left plate
            vertices << mid - gap << y << mid - gap / 2 << y << mid - gap << y + h;
            vertices << mid - gap / 2 << y << mid - gap / 2 << y + h << mid - gap << y + h;
            // Right plate
            vertices << mid + gap / 2 << y << mid + gap << y << mid + gap / 2 << y + h;
            vertices << mid + gap << y << mid + gap << y + h << mid + gap / 2 << y + h;
        }
        else if (comp.type == "Inductor")
        {
            // Inductor: Coil shape (simplified as rectangle for now)
            vertices << x << y << x + w << y << x << y + h;
            vertices << x + w << y << x + w << y + h << x << y + h;
        }
        else if (comp.type == "Voltage Source")
        {
            // Voltage Source: Circle (simplified as diamond)
            float cx = x + w / 2, cy = y + h / 2;
            vertices << cx << y << x + w << cy << cx << y + h;
            vertices << x << cy << x + w << cy << cx << y + h;
        }
        else
        {
            // Default: filled rectangle
            vertices << x << y << x + w << y << x << y + h;
            vertices << x + w << y << x + w << y + h << x << y + h;
        }
    }

    // Store vertex count for rendering
    m_componentVertexCount = vertices.size() / 2;

    if (!vertices.isEmpty())
    {
        m_componentVAO.bind();
        m_componentVBO.bind();
        m_componentVBO.allocate(vertices.data(), vertices.size() * sizeof(float));

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        m_componentVBO.release();
        m_componentVAO.release();
    }
}

void CircuitRenderer::renderComponents()
{
    if (!m_componentProgram || m_components.isEmpty())
        return;

    m_componentProgram->bind();

    // Use world-to-screen transformation like the grid
    QMatrix4x4 projection;
    projection.setToIdentity();
    projection.ortho(-m_panOffset.x() / m_zoom,
                     (m_viewportSize.width() - m_panOffset.x()) / m_zoom,
                     (m_viewportSize.height() - m_panOffset.y()) / m_zoom,
                     -m_panOffset.y() / m_zoom, -1, 1);
    projection.scale(m_zoom, m_zoom, 1);
    m_componentProgram->setUniformValue("projection", projection);

    m_componentVAO.bind();

    // Render each component with its own color
    int vertexOffset = 0;
    for (const Component& comp : m_components)
    {
        QVector4D colorVec(comp.color.redF(), comp.color.greenF(),
                           comp.color.blueF(), comp.color.alphaF());
        if (comp.selected)
        {
            // Highlight selected components
            colorVec = QVector4D(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
        }
        m_componentProgram->setUniformValue("componentColor", colorVec);

        int triangleCount = 2; // Default for most components
        if (comp.type == "Capacitor")
            triangleCount = 4; // Two plates
        if (comp.type == "Voltage Source")
            triangleCount = 2; // Diamond shape

        // Draw filled triangles
        glDrawArrays(GL_TRIANGLES, vertexOffset, triangleCount * 3);
        vertexOffset += triangleCount * 3;
    }

    m_componentVAO.release();
    m_componentProgram->release();

    // Render connection terminals as small circles
    renderTerminals();
}

void CircuitRenderer::renderTerminals()
{
    if (!m_componentProgram || m_components.isEmpty())
        return;

    // Reuse component program for terminals
    m_componentProgram->bind();

    // Use the same projection as components
    QMatrix4x4 projection;
    projection.setToIdentity();
    projection.ortho(-m_panOffset.x() / m_zoom,
                     (m_viewportSize.width() - m_panOffset.x()) / m_zoom,
                     (m_viewportSize.height() - m_panOffset.y()) / m_zoom,
                     -m_panOffset.y() / m_zoom, -1, 1);
    projection.scale(m_zoom, m_zoom, 1);
    m_componentProgram->setUniformValue("projection", projection);

    // Set terminal color (white for visibility)
    QVector4D terminalColor(1.0f, 1.0f, 1.0f, 1.0f);
    m_componentProgram->setUniformValue("componentColor", terminalColor);

    // Create temporary vertex data for terminals
    QVector<float> terminalVertices;
    float terminalSize = 3.0f; // Small circles

    for (const Component& comp : m_components)
    {
        // Input terminals
        for (const QPointF& terminal : comp.inputTerminals)
        {
            float x = terminal.x();
            float y = terminal.y();
            float r = terminalSize;

            // Simple square for terminal (2 triangles)
            terminalVertices << x - r << y - r << x + r << y - r << x - r << y + r;
            terminalVertices << x + r << y - r << x + r << y + r << x - r << y + r;
        }

        // Output terminals
        for (const QPointF& terminal : comp.outputTerminals)
        {
            float x = terminal.x();
            float y = terminal.y();
            float r = terminalSize;

            // Simple square for terminal (2 triangles)
            terminalVertices << x - r << y - r << x + r << y - r << x - r << y + r;
            terminalVertices << x + r << y - r << x + r << y + r << x - r << y + r;
        }
    }

    if (!terminalVertices.isEmpty())
    {
        // Create temporary VBO for terminals
        QOpenGLBuffer terminalVBO;
        terminalVBO.create();
        terminalVBO.bind();
        terminalVBO.allocate(terminalVertices.data(), terminalVertices.size() * sizeof(float));

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_TRIANGLES, 0, terminalVertices.size() / 2);

        terminalVBO.release();
    }

    m_componentProgram->release();
}

void CircuitRenderer::updateDotGeometry()
{
    if (m_viewportSize.isEmpty())
        return;

    QVector<float> vertices;

    // Calculate world bounds
    float worldLeft = -m_panOffset.x() / m_zoom;
    float worldTop = -m_panOffset.y() / m_zoom;
    float worldRight = (m_viewportSize.width() - m_panOffset.x()) / m_zoom;
    float worldBottom = (m_viewportSize.height() - m_panOffset.y()) / m_zoom;

    // Extend bounds for unlimited dots
    float extension = m_gridSize * 50;
    worldLeft -= extension;
    worldTop -= extension;
    worldRight += extension;
    worldBottom += extension;

    // Create dots every 8x8 grid intersection
    float dotSpacing = m_gridSize * 8;
    float startX = floor(worldLeft / dotSpacing) * dotSpacing;
    float startY = floor(worldTop / dotSpacing) * dotSpacing;

    // Scale dot size with zoom for better visibility
    float dotSize = (m_gridSize * 0.4f) / m_zoom;
    dotSize = qMax(2.0f, qMin(dotSize, 10.0f)); // Clamp between 2 and 10 pixels

    for (float x = startX; x <= worldRight; x += dotSpacing)
    {
        for (float y = startY; y <= worldBottom; y += dotSpacing)
        {
            // Create a small square for each dot (2 triangles = 6 vertices)
            float half = dotSize / 2.0f;

            // Triangle 1
            vertices << x - half << y - half;
            vertices << x + half << y - half;
            vertices << x - half << y + half;

            // Triangle 2
            vertices << x + half << y - half;
            vertices << x + half << y + half;
            vertices << x - half << y + half;
        }
    }

    m_dotVAO.bind();
    m_dotVBO.bind();
    m_dotVBO.allocate(vertices.data(), vertices.size() * sizeof(float));

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    m_dotVBO.release();
    m_dotVAO.release();
}

void CircuitRenderer::updateWireGeometry()
{
    if (m_wires.isEmpty())
        return;

    QVector<float> vertices;

    for (const Wire& wire : m_wires)
    {
        // Find component positions
        QPointF fromPos, toPos;
        bool foundFrom = false, foundTo = false;

        for (const Component& comp : m_components)
        {
            if (comp.id == wire.fromComponentId)
            {
                fromPos = QPointF(comp.position.x() + comp.width / 2, comp.position.y() + comp.height / 2);
                foundFrom = true;
            }
            if (comp.id == wire.toComponentId)
            {
                toPos = QPointF(comp.position.x() + comp.width / 2, comp.position.y() + comp.height / 2);
                foundTo = true;
            }
        }

        if (foundFrom && foundTo)
        {
            // Simple straight line for now
            vertices << fromPos.x() << fromPos.y();
            vertices << toPos.x() << toPos.y();
        }
    }

    m_wireVAO.bind();
    m_wireVBO.bind();
    m_wireVBO.allocate(vertices.data(), vertices.size() * sizeof(float));

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    m_wireVBO.release();
    m_wireVAO.release();
}

void CircuitRenderer::renderDots()
{
    if (!m_dotProgram || m_viewportSize.isEmpty())
        return;

    m_dotProgram->bind();

    // Use world-to-screen transformation
    QMatrix4x4 projection;
    projection.setToIdentity();
    projection.ortho(-m_panOffset.x() / m_zoom,
                     (m_viewportSize.width() - m_panOffset.x()) / m_zoom,
                     (m_viewportSize.height() - m_panOffset.y()) / m_zoom,
                     -m_panOffset.y() / m_zoom, -1, 1);
    projection.scale(m_zoom, m_zoom, 1);

    m_dotProgram->setUniformValue("projection", projection);

    // Set dot color (darker than grid)
    QVector4D dotColorVec(m_gridColor.redF() * 1.5f, m_gridColor.greenF() * 1.5f,
                          m_gridColor.blueF() * 1.5f, m_gridColor.alphaF());
    m_dotProgram->setUniformValue("dotColor", dotColorVec);

    m_dotVAO.bind();

    // Calculate number of dots based on actual vertex count
    float dotSpacing = m_gridSize * 8;
    float worldLeft = -m_panOffset.x() / m_zoom;
    float worldTop = -m_panOffset.y() / m_zoom;
    float worldRight = (m_viewportSize.width() - m_panOffset.x()) / m_zoom;
    float worldBottom = (m_viewportSize.height() - m_panOffset.y()) / m_zoom;

    // Extend bounds for calculation
    float extension = m_gridSize * 50;
    worldLeft -= extension;
    worldTop -= extension;
    worldRight += extension;
    worldBottom += extension;

    int dotsX = ceil((worldRight - worldLeft) / dotSpacing) + 1;
    int dotsY = ceil((worldBottom - worldTop) / dotSpacing) + 1;
    int totalDots = dotsX * dotsY;

    glDrawArrays(GL_TRIANGLES, 0, totalDots * 6); // 6 vertices per dot (2 triangles)

    m_dotVAO.release();
    m_dotProgram->release();
}

void CircuitRenderer::renderWires()
{
    if (!m_wireProgram || m_wires.isEmpty())
        return;

    m_wireProgram->bind();

    // Use world-to-screen transformation
    QMatrix4x4 projection;
    projection.setToIdentity();
    projection.ortho(-m_panOffset.x() / m_zoom,
                     (m_viewportSize.width() - m_panOffset.x()) / m_zoom,
                     (m_viewportSize.height() - m_panOffset.y()) / m_zoom,
                     -m_panOffset.y() / m_zoom, -1, 1);
    projection.scale(m_zoom, m_zoom, 1);

    m_wireProgram->setUniformValue("projection", projection);

    // Set wire color (yellow)
    QVector4D wireColorVec(1.0f, 1.0f, 0.0f, 1.0f);
    m_wireProgram->setUniformValue("componentColor", wireColorVec);

    m_wireVAO.bind();

    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, m_wires.size() * 2);

    m_wireVAO.release();
    m_wireProgram->release();
}

void CircuitRenderer::renderResistor(const Component& comp)
{
    // Resistor is rendered using the zigzag pattern from updateComponentGeometry
    glDrawArrays(GL_LINES, 0, 12); // 6 line segments = 12 vertices
}

void CircuitRenderer::renderCapacitor(const Component& comp)
{
    // Capacitor is rendered using the parallel plates from updateComponentGeometry
    glDrawArrays(GL_LINES, 0, 8); // 4 line segments = 8 vertices
}

void CircuitRenderer::renderInductor(const Component& comp)
{
    // Inductor is rendered using the coil pattern from updateComponentGeometry
    glDrawArrays(GL_LINES, 0, 16); // 8 line segments = 16 vertices
}

void CircuitRenderer::renderVoltageSource(const Component& comp)
{
    // Voltage source is rendered using the circle from updateComponentGeometry
    glDrawArrays(GL_LINES, 0, 24); // 12 line segments = 24 vertices
}
