#include "pokemondatabase.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QByteArray>

PokemonDatabase::PokemonDatabase(QObject *parent) :
    QObject(parent)
{
    // Read In Json Data to array list
//    this->jsonItems = this->jsonToList("qrc:/data/items.json");
//    this->jsonMoves = this->jsonToList("qrc:/data/moves.json");
//    this->jsonPokemon = this->jsonToList("qrc:/data/pokemon.json");
//    this->jsonTypes = this->jsonToList("qrc:/data/types.json");
}

//QVariantList PokemonDatabase::jsonToList(QString filename)
//{
//    // Prepare to read in data
//    QByteArray val;
//    QFile file;

//    // Read in file
//    file.setFileName(filename);
//    file.open(QIODevice::ReadOnly | QIODevice::Text);
//    val = file.readAll();
//    file.close();

//    // Return QVariant List
//    QJsonDocument doc = QJsonDocument::fromJson(val);
//    return doc.array()[0].toObject().
//}
