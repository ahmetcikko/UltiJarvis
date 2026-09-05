#include "backend.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QUrl>

int main(int argc, char *argv[]) {
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QGuiApplication app(argc, argv);
    Backend backend;
    QQmlApplicationEngine engine;
    (*engine.rootContext()).setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/app/main.qml"));
    return app.exec();
}
