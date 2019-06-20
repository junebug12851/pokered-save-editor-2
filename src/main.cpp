#include <QtCore/qglobal.h>
#include <QApplication>
#include <QtQuickControls2>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // Set Attributes
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);
    QApplication::setAttribute(Qt::AA_CompressTabletEvents);
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

    // Set Application Info
    QApplication::setApplicationDisplayName("Pokered Save Editor");
    QApplication::setApplicationName("Pokered Save Editor");
    QApplication::setOrganizationName("June Hanabi");
    QApplication::setApplicationVersion("v1.0.0");
    QApplication::setOrganizationDomain("pokeredsaveeditor.junehanabi.gmail.com");

    // Create the app
    QApplication app(argc, argv);

    // Pull the icon from resources and set as window icon
    // It's also set to properly be built-in during compile
    const QIcon icon("qrc:/icon");
    app.setWindowIcon(icon);

    MainWindow win;
    win.show();

    // Run the app
    return app.exec();
}