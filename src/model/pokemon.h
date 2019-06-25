#ifndef POKEMON_H
#define POKEMON_H

#include <QVector>
#include <utility>

using std::pair;

#include "basemodel.h"

// Pokemon can evolve from level or item
struct PokemonEvolution {
    PokemonEvolution();
    PokemonEvolution(const QJsonObject& obj);

    optional<vars> level;
    optional<QString> item;
    QString toName;
};

Q_DECLARE_METATYPE(PokemonEvolution)

struct Pokemon : public BaseModel
{
    Pokemon();
    Pokemon(const QJsonObject& obj);

    // Pokemon Pokedex Index
    optional<vars> pokedex;

    // Pokemon Growth Rate
    // How big or small is the exp range to reach max level
    // The number here is the internal growth rate index, in other words the
    // amount of the number (bigger or smaller) has no correlationto the growth
    // rate
    optional<vars> growthRate;

    // Base Stats
    optional<vars> baseHP;
    optional<vars> baseAttack;
    optional<vars> baseDefense;
    optional<vars> baseSpeed;
    optional<vars> baseSpecial;
    optional<vars> baseExpYield;

    // Evolution
    // Can be an array of one or more evolutions
    // The only time it'll be more than 1 is with eevee
    optional<QVector<PokemonEvolution>> evolution;

    // Learnset
    optional<QVector<pair<vars, QString>>> learnedMoves;
    optional<QVector<QString>> initialMoves;
    optional<QVector<vars>> tmHm;

    optional<QString> type1;
    optional<QString> type2;
    optional<vars> catchRate;

    // Glitch Pokemon?
    optional<bool> glitch;
};

Q_DECLARE_METATYPE(Pokemon)

#endif // POKEMON_H
