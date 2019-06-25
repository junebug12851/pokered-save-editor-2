#include "pokemon.h"

#include <QJsonArray>

PokemonEvolution::PokemonEvolution()
{}

PokemonEvolution::PokemonEvolution(const QJsonObject &obj)
{
    if(obj.contains("item"))
        this->item = obj["item"].toString();

    if(obj.contains("level"))
        this->level = static_cast<vars>(obj["level"].toInt());

    this->toName = obj["toName"].toString();
}

Pokemon::Pokemon()
{}

Pokemon::Pokemon(const QJsonObject &obj) :
    BaseModel (obj)
{
    if(obj.contains("pokedex"))
        this->pokedex = static_cast<vars>(obj["pokedex"].toInt());

    if(obj.contains("growthRate"))
        this->growthRate = static_cast<vars>(obj["growthRate"].toInt());

    if(obj.contains("baseHP"))
        this->baseHP = static_cast<vars>(obj["baseHP"].toInt());

    if(obj.contains("baseAttack"))
        this->baseAttack = static_cast<vars>(obj["baseAttack"].toInt());

    if(obj.contains("baseDefense"))
        this->baseDefense = static_cast<vars>(obj["baseDefense"].toInt());

    if(obj.contains("baseSpeed"))
        this->baseSpeed = static_cast<vars>(obj["baseSpeed"].toInt());

    if(obj.contains("baseSpecial"))
        this->baseSpecial = static_cast<vars>(obj["baseSpecial"].toInt());

    if(obj.contains("baseExpYield"))
        this->baseExpYield = static_cast<vars>(obj["baseExpYield"].toInt());

    // Evolution is a bit tricky no thianks to Eevee
    // There can be one or more evolutions (Any more than 1 is Eevee)
    // If there's only 1 (99.9% of the time) it'll be an object
    // Otherwise it'll be an array of objects
    // PokemonEvolution handles the details of extracting object data
    if(obj.contains("evolution")) {
        // Assign empty evolution array
        this->evolution = QVector<PokemonEvolution>();

        // get evolution object or array
        QJsonValue evolution = obj["evolution"];

        // Determine if object or array and process accordingly
        if(evolution.isObject()) {
            QJsonObject evolutionObj = evolution.toObject();
            PokemonEvolution pokeEvo(evolutionObj);
            this->evolution->push_back(pokeEvo);
        }
        else if(evolution.isArray()) {
            QJsonArray evolutionArr = evolution.toArray();
            for(var i{0}; i < evolutionArr.size(); ++i) {
                QJsonObject evolutionArrItem = evolutionArr[i].toObject();
                PokemonEvolution pokeEvo(evolutionArrItem);
                this->evolution->push_back(pokeEvo);
            }
        }

        // Often there will only be 1 or 3 objects, theres no use in having
        // excess in memory
        this->evolution->squeeze();
    }

     if(obj.contains("moves")) {
         // Assign moves array
         this->learnedMoves = QVector<pair<vars, QString>>();

         QJsonArray movesArr = obj["moves"].toArray();
         for(var i{0}; i < movesArr.size(); ++i) {
             QJsonObject movesArrItem = movesArr[i].toObject();
             vars level = static_cast<vars>(movesArrItem["level"].toInt());
             QString move = movesArrItem["move"].toString();
             this->learnedMoves->push_back(pair<vars, QString>(level, move));
         }

         // Squeeze excess
         this->learnedMoves->squeeze();
     }

     if(obj.contains("initial")) {
         // Assign initial moves array
         this->initialMoves = QVector<QString>();

         QJsonArray initialMovesArr = obj["initial"].toArray();
         for(var i{0}; i < initialMovesArr.size(); ++i) {
             QString initialMovesArrItem = initialMovesArr[i].toString();
             this->initialMoves->push_back(initialMovesArrItem);
         }

         // Squeeze excess
         this->initialMoves->squeeze();
     }

     if(obj.contains("tmHm")) {
         // Assign initial moves array
         this->tmHm = QVector<vars>();

         QJsonArray tmHmArr = obj["tmHm"].toArray();
         for(var i{0}; i < tmHmArr.size(); ++i) {
             vars tmHmArrItem = static_cast<vars>(tmHmArr[i].toInt());
             this->tmHm->push_back(tmHmArrItem);
         }

         // Squeeze excess
         this->tmHm->squeeze();
     }

     if(obj.contains("type1"))
         this->type1 = obj["type1"].toString();

     if(obj.contains("type2"))
         this->type2 = obj["type2"].toString();

     if(obj.contains("catchRate"))
         this->catchRate = static_cast<vars>(obj["catchRate"].toInt());

     if(obj.contains("glitch"))
         this->glitch = obj["glitch"].toBool();
}
