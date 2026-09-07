#include "backend.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QUrl>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static std::filesystem::path jarvis_log_path() {
    std::error_code ec;
    return std::filesystem::temp_directory_path(ec) / "ultijarvis.log";
}

static void jlog(const std::string &msg) {
    std::ofstream f(jarvis_log_path(), std::ios::app);
    if (!f.is_open())
        return;
    std::time_t t = std::time(nullptr);
    char stamp[32] = {0};
    std::tm tm {};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tm);
    f << stamp << "  app: " << msg << "\n";
}

static void on_terminate() {
    try {
        std::exception_ptr e = std::current_exception();
        if (e)
            std::rethrow_exception(e);
        jlog("FATAL: terminate with no active exception");
    } catch (const std::exception &ex) {
        jlog(std::string("FATAL: unhandled exception: ") + ex.what());
    } catch (...) {
        jlog("FATAL: unhandled exception of unknown type");
    }
    std::_Exit(1);
}

#ifdef _WIN32
static LONG WINAPI crash_filter(EXCEPTION_POINTERS *info) {
    char buf[160];
    std::snprintf(buf, sizeof(buf), "FATAL: exception 0x%08lX at %p",
                  static_cast<unsigned long>(
                      info->ExceptionRecord->ExceptionCode),
                  info->ExceptionRecord->ExceptionAddress);
    jlog(buf);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char *argv[]) {
    std::set_terminate(on_terminate);
#ifdef _WIN32
    SetUnhandledExceptionFilter(crash_filter);
#endif
    jlog("=== app start ===");
    QGuiApplication app(argc, argv);
    jlog("qt application created");
    bool opengl = false;
    {
        QOpenGLContext probe;
        QOffscreenSurface surface;
        surface.create();
        if (probe.create() && surface.isValid() && probe.makeCurrent(&surface)) {
            opengl = true;
            probe.doneCurrent();
        }
    }
    QQuickWindow::setGraphicsApi(opengl ? QSGRendererInterface::OpenGL
                                        : QSGRendererInterface::Software);
    jlog(opengl ? "graphics: OpenGL" : "graphics: software (no usable OpenGL)");
    Backend backend;
    jlog("backend created");
    QQmlApplicationEngine engine;
    (*engine.rootContext()).setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/app/main.qml"));
    if (engine.rootObjects().isEmpty()) {
        jlog("FATAL: qml failed to load");
        return 1;
    }
    jlog("qml loaded, entering event loop");
    int rc = app.exec();
    jlog("event loop finished with " + std::to_string(rc));
    return rc;
}
