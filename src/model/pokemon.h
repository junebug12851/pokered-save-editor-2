#ifndef POKEMON_H
#define POKEMON_H

#include <QVector>

#include "basemodel.h"

// To make things simpler, lets keep this to have non-constant members
// To much fighting with Qt for too long otherwise
struct LevelName {
    std::uint_least8_t level;
    QString name;
};

Q_DECLARE_METATYPE(LevelName)

struct Pokemon : public BaseModel
{
    // Pokemon Pokedex Index
    std::uint_least8_t pokedex;

    // Pokemon Growth Rate
    // How big or small is the exp range to reach max level
    // The number here is the internal growth rate index, in other words the
    // amount of the number (bigger or smaller) has no correlationto the growth
    // rate
    std::uint_least8_t growthRate;

    // Base Stats
    std::uint_least8_t baseHP;
    std::uint_least8_t baseAttack;
    std::uint_least8_t baseDefense;
    std::uint_least8_t baseSpeed;
    std::uint_least8_t baseSpecial;
    std::uint_least8_t baseExpYield;

    // Evolution
    LevelName evolution;

    // Learnset
    QVector<LevelName> learnedMoves;
    QVector<QString> initialMoves;
    QVector<std::uint_least8_t> tmHm;

    QString type1;
    QString type2;
    QString catchRate;

    // Glitch Pokemon?
    bool glitch;
};

Q_DECLARE_METATYPE(Pokemon)

#endif // POKEMON_H
