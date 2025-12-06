#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QColor>

class CircuitViewport : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(float gridSize READ gridSize WRITE setGridSize NOTIFY gridSizeChanged)
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY gridColorChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)

public:
    explicit CircuitViewport(QQuickItem* parent = nullptr);
    Renderer* createRenderer() const override;

    float gridSize() const { return m_gridSize; }
    void setGridSize(float size);

    QColor gridColor() const { return m_gridColor; }
    void setGridColor(const QColor& color);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

signals:
    void gridSizeChanged();
    void gridColorChanged();
    void backgroundColorChanged();

private:
    float m_gridSize = 20.0f;
    QColor m_gridColor = QColor(200, 100, 100, 255);
    QColor m_backgroundColor = QColor(30, 30, 30, 255);
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
    void updateGridGeometry(); // Renamed from createGridGeometry
    void renderGrid();

    QOpenGLShaderProgram* m_gridProgram = nullptr;
    QOpenGLBuffer m_gridVBO;
    QOpenGLVertexArrayObject m_gridVAO;

    // Data copied from UI
    float m_gridSize = 20.0f;
    QColor m_gridColor;
    QColor m_backgroundColor;
    QSize m_viewportSize;

    bool m_initialized = false;
    bool m_gridDirty = true; // Flag to trigger VBO update
};
