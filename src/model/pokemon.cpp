#include "pokemon.h"

#include <QJsonArray>

PokemonEvolution::PokemonEvolution()
{}

PokemonEvolution::PokemonEvolution(const QJsonObject &obj)
{
    this->init(obj);
}

void PokemonEvolution::init(const QJsonObject &obj)
{
    this->_toItem.reset();
    this->_toPokemon.reset();

    if(obj.contains("item"))
        this->_item = obj["item"].toString();
    else
        this->_item.reset();

    if(obj.contains("level"))
        this->_level = static_cast<vars>(obj["level"].toInt());
    else
        this->_level.reset();

    this->_toName = obj["toName"].toString();
}

const optional<vars>& PokemonEvolution::level()
{
    return this->_level;
}

const optional<QString>& PokemonEvolution::item()
{
    return this->_item;
}

const QString& PokemonEvolution::toName()
{
    return this->_toName;
}

const optional<Item*>& PokemonEvolution::toItem()
{
    return this->_toItem;
}

const optional<Pokemon*>& PokemonEvolution::toPokemon()
{
    return this->_toPokemon;
}

Pokemon::Pokemon()
{}

Pokemon::Pokemon(const QJsonObject &obj) :
    BaseModel (obj)
{
    this->init(obj);
}

void Pokemon::init(const QJsonObject &obj)
{
    BaseModel::init(obj);

    this->_toType1.reset();
    this->_toType2.reset();
    this->_toTmHmMoves.reset();
    this->_toTmHmItems.reset();
    this->_toInitialMoves.reset();
    this->_toLearnedMoves.reset();

    if(obj.contains("pokedex"))
        this->_pokedex = static_cast<vars>(obj["pokedex"].toInt());
    else
        this->_pokedex.reset();

    if(obj.contains("growthRate"))
        this->_growthRate = static_cast<vars>(obj["growthRate"].toInt());
    else
        this->_growthRate.reset();

    if(obj.contains("baseHP"))
        this->_baseHP = static_cast<vars>(obj["baseHP"].toInt());
    else
        this->_baseHP.reset();

    if(obj.contains("baseAttack"))
        this->_baseAttack = static_cast<vars>(obj["baseAttack"].toInt());
    else
        this->_baseAttack.reset();

    if(obj.contains("baseDefense"))
        this->_baseDefense = static_cast<vars>(obj["baseDefense"].toInt());
    else
        this->_baseDefense.reset();

    if(obj.contains("baseSpeed"))
        this->_baseSpeed = static_cast<vars>(obj["baseSpeed"].toInt());
    else
        this->_baseSpeed.reset();

    if(obj.contains("baseSpecial"))
        this->_baseSpecial = static_cast<vars>(obj["baseSpecial"].toInt());
    else
        this->_baseSpecial.reset();

    if(obj.contains("baseExpYield"))
        this->_baseExpYield = static_cast<vars>(obj["baseExpYield"].toInt());
    else
        this->_baseExpYield.reset();

    // Evolution is a bit tricky no thianks to Eevee
    // There can be one or more evolutions (Any more than 1 is Eevee)
    // If there's only 1 (99.9% of the time) it'll be an object
    // Otherwise it'll be an array of objects
    // PokemonEvolution handles the details of extracting object data
    if(obj.contains("evolution")) {
        // Assign empty evolution array
        this->_evolution = vector<PokemonEvolution>();

        // get evolution object or array
        QJsonValue evolution = obj["evolution"];

        // Determine if object or array and process accordingly
        if(evolution.isObject()) {
            QJsonObject evolutionObj = evolution.toObject();
            PokemonEvolution pokeEvo(evolutionObj);
            this->_evolution->push_back(pokeEvo);
        }
        else if(evolution.isArray()) {
            QJsonArray evolutionArr = evolution.toArray();
            for(var i{0}; i < evolutionArr.size(); ++i) {
                QJsonObject evolutionArrItem = evolutionArr[i].toObject();
                PokemonEvolution pokeEvo(evolutionArrItem);
                this->_evolution->push_back(pokeEvo);
            }
        }

        // Often there will only be 1 or 3 objects, theres no use in having
        // excess in memory
        this->_evolution->shrink_to_fit();
    }
    else
        this->_evolution.reset();

     if(obj.contains("moves")) {
         // Assign moves array
         this->_learnedMoves = vector<pair<vars, QString>>();

         QJsonArray movesArr = obj["moves"].toArray();
         for(var i{0}; i < movesArr.size(); ++i) {
             QJsonObject movesArrItem = movesArr[i].toObject();
             vars level = static_cast<vars>(movesArrItem["level"].toInt());
             QString move = movesArrItem["move"].toString();
             this->_learnedMoves->push_back(pair<vars, QString>(level, move));
         }

         // Squeeze excess
         this->_learnedMoves->shrink_to_fit();
     }
     else
         this->_learnedMoves.reset();

     if(obj.contains("initial")) {
         // Assign initial moves array
         this->_initialMoves = vector<QString>();

         QJsonArray initialMovesArr = obj["initial"].toArray();
         for(var i{0}; i < initialMovesArr.size(); ++i) {
             QString initialMovesArrItem = initialMovesArr[i].toString();
             this->_initialMoves->push_back(initialMovesArrItem);
         }

         // Squeeze excess
         this->_initialMoves->shrink_to_fit();
     }
     else
         this->_initialMoves.reset();

     if(obj.contains("tmHm")) {
         // Assign initial moves array
         this->_tmHm = vector<vars>();

         QJsonArray tmHmArr = obj["tmHm"].toArray();
         for(var i{0}; i < tmHmArr.size(); ++i) {
             vars tmHmArrItem = static_cast<vars>(tmHmArr[i].toInt());
             this->_tmHm->push_back(tmHmArrItem);
         }

         // Squeeze excess
         this->_tmHm->shrink_to_fit();
     }
     else
         this->_tmHm.reset();

     if(obj.contains("type1"))
         this->_type1 = obj["type1"].toString();
     else
         this->_type1.reset();

     if(obj.contains("type2"))
         this->_type2 = obj["type2"].toString();
     else
         this->_type2.reset();

     if(obj.contains("catchRate"))
         this->_catchRate = static_cast<vars>(obj["catchRate"].toInt());
     else
         this->_catchRate.reset();

     if(obj.contains("glitch"))
         this->_glitch = obj["glitch"].toBool();
     else
         this->_glitch.reset();
}

const optional<vars>& Pokemon::pokedex()
{
    return this->_pokedex;
}

const optional<vars>& Pokemon::growthRate()
{
    return this->_growthRate;
}

const optional<vars>& Pokemon::baseHP()
{
    return this->_baseHP;
}

const optional<vars>& Pokemon::baseAttack()
{
    return this->_baseAttack;
}

const optional<vars>& Pokemon::baseDefense()
{
    return this->_baseDefense;
}

const optional<vars>& Pokemon::baseSpeed()
{
    return this->_baseSpeed;
}

const optional<vars>& Pokemon::baseSpecial()
{
    return this->_baseSpecial;
}

const optional<vars>& Pokemon::baseExpYield()
{
    return this->_baseExpYield;
}

const optional<vector<PokemonEvolution>>& Pokemon::evolution()
{
    return this->_evolution;
}

const optional<vector<pair<vars, QString>>>& Pokemon::learnedMoves()
{
    return this->_learnedMoves;
}

const optional<vector<QString>>& Pokemon::initialMoves()
{
    return this->_initialMoves;
}

const optional<vector<vars>>& Pokemon::tmHm()
{
    return this->_tmHm;
}

const optional<QString>& Pokemon::type1()
{
    return this->_type1;
}

const optional<QString>& Pokemon::type2()
{
    return this->_type2;
}

const optional<vars>& Pokemon::catchRate()
{
    return this->_catchRate;
}

const optional<bool>& Pokemon::glitch()
{
    return this->_glitch;
}

const optional<vector<pair<vars, Move*>>>& Pokemon::toLearnedMoves()
{
    return this->_toLearnedMoves;
}

const optional<vector<Move*>>& Pokemon::toInitialMoves()
{
    return this->_toInitialMoves;
}

const optional<vector<Move*>>& Pokemon::toTmHmMoves()
{
    return this->_toTmHmMoves;
}

const optional<vector<Move *> > &Pokemon::toTmHmItems()
{
    return this->_toTmHmItems;
}

const optional<Type*>& Pokemon::toType1()
{
    return this->_toType1;
}

const optional<Type*>& Pokemon::toType2()
{
    return this->_toType2;
}
