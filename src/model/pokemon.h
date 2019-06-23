#ifndef POKEMON_H
#define POKEMON_H

#include <QVector>
#include <QString>

#include "basemodel.h"

// To make things simpler, lets keep this to have non-constant members
// To much fighting with Qt for too long otherwise
struct LevelName {
    quint8 level;
    QString name;
};

Q_DECLARE_METATYPE(LevelName)

struct Pokemon : public BaseModel
{
    // Pokemon Pokedex Index
    quint8 pokedex;

    // Pokemon Growth Rate
    // How big or small is the exp range to reach max level
    // The number here is the internal growth rate index, in other words the
    // amount of the number (bigger or smaller) has no correlationto the growth
    // rate
    quint8 growthRate;

    // Base Stats
    quint8 baseHP;
    quint8 baseAttack;
    quint8 baseDefense;
    quint8 baseSpeed;
    quint8 baseSpecial;
    quint8 baseExpYield;

    // Evolution
    LevelName evolution;

    // Learnset
    QVector<LevelName> learnedMoves;
    QVector<QString> initialMoves;
    QVector<int> tmHm;

    QString type1;
    QString type2;
    QString catchRate;

    // Glitch Pokemon?
    bool glitch;
};

Q_DECLARE_METATYPE(Pokemon)

#endif // POKEMON_H
