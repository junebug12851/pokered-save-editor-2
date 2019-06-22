#include "pokemon.h"

MoveLearnset::MoveLearnset(const quint8 level,
                           const QString name,
                           QObject *parent) :
    QObject(parent),
    level(level),
    name(name)
{}

MoveLearnset::MoveLearnset(const MoveLearnset& obj,
                           QObject *parent) :
    QObject(parent),
    level(obj.level),
    name(obj.name)
{}

Evolution::Evolution(const quint8 level,
                           const QString name,
                           QObject *parent) :
    QObject(parent),
    level(level),
    name(name)
{}

Evolution::Evolution(const Evolution& obj,
                     QObject *parent) :
    QObject(parent),
    level(obj.level),
    name(obj.name)
{}

Pokemon::Pokemon(
        const QString name,
        const quint8 index,
        const quint8 pokedex,
        const quint8 growthRate,
        const quint8 baseHP,
        const quint8 baseAttack,
        const quint8 baseDefense,
        const quint8 baseSpeed,
        const quint8 baseSpecial,
        const quint8 baseExpYield,
        const Evolution evolution,
        const QVector<MoveLearnset> learnedMoves,
        const QVector<QString> initialMoves,
        const QVector<int> tmHm,
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
