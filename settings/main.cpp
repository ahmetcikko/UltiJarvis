#include "backend.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    Settings settings;
    QQmlApplicationEngine engine;
    (*engine.rootContext()).setContextProperty("settings", &settings);
    engine.load(QUrl("qrc:/settings.qml"));
    return app.exec();
}
