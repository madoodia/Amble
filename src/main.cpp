#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow> // <--- ADD THIS
#include "CircuitViewport.h"

int main(int argc, char* argv[])
{
    // 1. FORCE OPENGL BACKEND (Must be before QGuiApplication)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);

    // 2. Optional: Set Surface Format for better Context handling
    QSurfaceFormat format;
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4); // Anti-aliasing
    QSurfaceFormat::setDefaultFormat(format);

    qDebug() << "Starting Amble application";

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []()
                     { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Amble", "Main");

    return app.exec();
}
