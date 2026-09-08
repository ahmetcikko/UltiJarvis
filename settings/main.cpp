#include "backend.h"
#include <string>
#ifdef __linux__
#include <boost/dll/runtime_symbol_info.hpp>
#include <cstdlib>
#endif
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#ifdef __linux__
static void jarvis_confine_plugin_paths() {
    std::string dir = boost::dll::program_location().parent_path().string();
    setenv("OPENSSL_MODULES", (dir + "/ossl-modules").c_str(), 0);
    setenv("OPENSSL_ENGINES", (dir + "/engines-3").c_str(), 0);
    setenv("GIO_MODULE_DIR", (dir + "/gio-modules").c_str(), 0);
    setenv("SASL_PATH", (dir + "/sasl2").c_str(), 0);
}
#endif

int main(int argc, char *argv[]) {
#ifdef __linux__
    jarvis_confine_plugin_paths();
#endif
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
