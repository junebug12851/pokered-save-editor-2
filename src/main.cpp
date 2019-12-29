#include <QtCore/qglobal.h>
#include <QApplication>
#include <QtQuickControls2>

#include "./data/names.h"
//#include "./view/mainwindow.h"
//#include "./data/gamedata.h"

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

    // Pull the icon from resources and set as window icon
    // It's also set to properly be built-in during compile
    QIcon icon("qrc:/assets/icons/512x512.png");
    app.setWindowIcon(icon);

    // Seed random generator
    qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);

//    QJsonDocument* tmp = GameData::json("eventPokemon");
//    tmp = GameData::json("eventPokemon");
//    QJsonValue tmpObj = (*tmp)[2];
//    QString tmpName = tmpObj["title"].toString();

    Names::load();
    QString name1 = Names::randomName();
    QString name2 = Names::randomName();
    QString name3 = Names::randomName();

    //auto tmp2 = EventPokemon::eventPokemon->at(0);

/*    qmlRegisterSingletonType<GameData>("pse.gamedata",
                                       1, 0,
                                       "GameData",
                                       &GameData::GameData_Provider);

    MainWindow win;
    win.show();*/

    // Run the app
    return app.exec();
}
