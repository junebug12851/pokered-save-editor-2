#ifndef POKEMON_H
#define POKEMON_H

#include <QVector>

#include "basemodel.h"

// To make things simpler, lets keep this to have non-constant members
// To much fighting with Qt for too long otherwise
struct LevelName {
    vars level;
    QString name;
};

Q_DECLARE_METATYPE(LevelName)

struct Pokemon : public BaseModel
{
    // Pokemon Pokedex Index
    vars pokedex;

    // Pokemon Growth Rate
    // How big or small is the exp range to reach max level
    // The number here is the internal growth rate index, in other words the
    // amount of the number (bigger or smaller) has no correlationto the growth
    // rate
    vars growthRate;

    // Base Stats
    vars baseHP;
    vars baseAttack;
    vars baseDefense;
    vars baseSpeed;
    vars baseSpecial;
    vars baseExpYield;

    // Evolution
    LevelName evolution;

    // Learnset
    QVector<LevelName> learnedMoves;
    QVector<QString> initialMoves;
    QVector<vars> tmHm;

    QString type1;
    QString type2;
    QString catchRate;

    // Glitch Pokemon?
    bool glitch;
};

Q_DECLARE_METATYPE(Pokemon)

#endif // POKEMON_H
