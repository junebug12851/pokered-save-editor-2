#include <QtCore/qglobal.h>
#include <QApplication>
#include <QtQuickControls2>

#include "./view/mainwindow.h"
#include "./store/pokemondatabase.h"

// Testing
#include "./model/item.h"

void test()
{
  PokemonDatabase tmp = PokemonDatabase();
  Item* itm = tmp.lookupItem("Potion");

  bool glitch = false;
  if(itm->glitch())
    glitch = true;

  QString name;
  if(itm->name())
    name = **itm->name();
}

int main(int argc, char *argv[])
{
    // Set Attributes
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);
    QApplication::setAttribute(Qt::AA_CompressTabletEvents);
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

    // Set Application Info
    QApplication::setApplicationName("Pokered Save Editor");
    QApplication::setOrganizationName("June Hanabi");
    QApplication::setApplicationVersion("v1.0.0");
    QApplication::setOrganizationDomain("pokeredsaveeditor.junehanabi.gmail.com");

    // Create the app
    QApplication app(argc, argv);

    // Initialize databases into memory
    // This will take a long time
    PokemonDatabase::initStores();

    // Register models and stores into qml
    // PokemonDatabase::qmlRegisterModels();

    // Pull the icon from resources and set as window icon
    // It's also set to properly be built-in during compile
    QIcon icon("qrc:/icon");
    app.setWindowIcon(icon);

    test();
    return 0;

    //MainWindow win;
    //win.show();

    // Run the app
    //return app.exec();
}
