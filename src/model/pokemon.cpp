#include "pokemon.h"

LevelName::LevelName(const quint8 level,
                           const QString name,
                           QObject *parent) :
    QObject(parent),
    level(level),
    name(name)
{}

LevelName::LevelName(const LevelName& obj,
                           QObject *parent) :
    QObject(parent),
    level(obj.level),
    name(obj.name)
{}

Pokemon::Pokemon(const QString name,
        const quint8 index,
        const quint8 pokedex,
        const quint8 growthRate,
        const quint8 baseHP,
        const quint8 baseAttack,
        const quint8 baseDefense,
        const quint8 baseSpeed,
        const quint8 baseSpecial,
        const quint8 baseExpYield,
        const LevelName evolution,
        const vector<LevelName> learnedMoves,
        const vector<QString> initialMoves,
        const vector<int> tmHm,
        const QString type1,
        const QString type2,
        const QString catchRate,
        const bool glitch,
        QObject *parent
        ) :
    BaseModel(name, index, parent),
    pokedex(pokedex),
    growthRate(growthRate),
    baseHP(baseHP),
    baseAttack(baseAttack),
    baseDefense(baseDefense),
    baseSpeed(baseSpeed),
    baseSpecial(baseSpecial),
    baseExpYield(baseExpYield),
    evolution(evolution),
    learnedMoves(learnedMoves),
    initialMoves(initialMoves),
    tmHm(tmHm),
    type1(type1),
    type2(type2),
    catchRate(catchRate),
    glitch(glitch)
{}
