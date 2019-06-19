#include <QtCore/qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#else
#endif

#include <QtQuickControls2>

int main(int argc, char *argv[])
{
    // Set Attributes
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);
    QGuiApplication::setAttribute(Qt::AA_CompressTabletEvents);
    QGuiApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

    // Set Application Info
    QGuiApplication::setApplicationDisplayName("Pokered Save Editor");
    QGuiApplication::setApplicationName("Pokered Save Editor");
    QGuiApplication::setOrganizationName("June Hanabi");
    QGuiApplication::setApplicationVersion("v1.0.0");
    QGuiApplication::setOrganizationDomain("pokeredsaveeditor.junehanabi.gmail.com");

    // Create the app
    QGuiApplication app(argc, argv);

    // Pull the icon from resources and set as window icon
    // It's also set to properly be built-in during compile
    const QIcon icon("qrc:/icon");
    app.setWindowIcon(icon);

    // Load up QML
    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/ux/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    // Run the app
    return app.exec();
}
