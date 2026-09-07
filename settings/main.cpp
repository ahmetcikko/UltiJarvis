#include "backend.h"
#include <string>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

int main(int argc, char *argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--repair") {
        jarvis_repair_install();
        return 0;
    }
    QGuiApplication app(argc, argv);
    Settings settings;
    QQmlApplicationEngine engine;
    (*engine.rootContext()).setContextProperty("settings", &settings);
    engine.load(QUrl("qrc:/settings.qml"));
    return app.exec();
}
