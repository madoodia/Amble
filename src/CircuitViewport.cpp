#include "CircuitViewport.h"

// --- ADD THIS INCLUDE ---
#include <QOpenGLFramebufferObject>
// ------------------------

#include <QQuickWindow>
#include <QOpenGLContext>
#include <QDebug> // Good to have for logging

// --- CircuitViewport Implementation ---

CircuitViewport::CircuitViewport(QQuickItem* parent)
    : QQuickFramebufferObject(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
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

// --- CircuitRenderer Implementation ---

CircuitRenderer::CircuitRenderer()
{
}

CircuitRenderer::~CircuitRenderer()
{
    delete m_gridProgram;
}

void CircuitRenderer::synchronize(QQuickFramebufferObject* item)
{
    auto* vp = static_cast<CircuitViewport*>(item);

    QSize newSize = vp->size().toSize();
    QColor newGridColor = vp->gridColor();

    if (newSize != m_viewportSize || !qFuzzyCompare(m_gridSize, vp->gridSize()) || newGridColor != m_gridColor)
    {
        m_gridDirty = true;
    }

    m_viewportSize = newSize;
    m_gridSize = vp->gridSize();
    m_gridColor = newGridColor;
    m_backgroundColor = vp->backgroundColor();

    qDebug() << "Synchronized - Size:" << m_viewportSize << "Grid Size:" << m_gridSize << "Grid Color:" << m_gridColor;
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

        // Render
        renderGrid();
    }

    m_gridVAO.release();
    if (m_gridProgram)
        m_gridProgram->release();
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

    m_gridVAO.create();
    m_gridVBO.create();
}

void CircuitRenderer::updateGridGeometry()
{
    if (m_viewportSize.isEmpty())
        return;

    QVector<float> vertices;
    float w = m_viewportSize.width();
    float h = m_viewportSize.height();

    for (float x = 0; x <= w; x += m_gridSize)
    {
        vertices << x << 0.0f << x << h;
    }
    for (float y = 0; y <= h; y += m_gridSize)
    {
        vertices << 0.0f << y << w << y;
    }

    m_gridVAO.bind();
    m_gridVBO.bind();
    m_gridVBO.allocate(vertices.data(), vertices.size() * sizeof(float));

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    m_gridVBO.release();
    m_gridVAO.release();
}

void CircuitRenderer::renderGrid()
{
    if (!m_gridProgram || m_viewportSize.isEmpty())
        return;

    m_gridProgram->bind();

    QMatrix4x4 projection;
    projection.ortho(0, m_viewportSize.width(), m_viewportSize.height(), 0, -1, 1);

    m_gridProgram->setUniformValue("projection", projection);

    // Convert QColor to QVector4D for proper uniform setting
    QVector4D colorVec(m_gridColor.redF(), m_gridColor.greenF(),
                       m_gridColor.blueF(), m_gridColor.alphaF());
    m_gridProgram->setUniformValue("gridColor", colorVec);

    m_gridVAO.bind();

    // Calculate vertex count based on grid lines
    int verticalLines = (m_viewportSize.width() / m_gridSize) + 1;
    int horizontalLines = (m_viewportSize.height() / m_gridSize) + 1;
    int vertexCount = (verticalLines + horizontalLines) * 2;

    // Set line width to make grid more visible
    glLineWidth(1.0f);

    qDebug() << "Rendering grid with" << vertexCount << "vertices, color:" << m_gridColor;
    glDrawArrays(GL_LINES, 0, vertexCount);
}
