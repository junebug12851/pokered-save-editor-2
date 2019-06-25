#ifndef POKEMON_H
#define POKEMON_H

#include <vector>
#include <utility>

using std::vector;
using std::pair;

#include "basemodel.h"

// Pokemon can evolve from level or item
class PokemonEvolution {
public:
    PokemonEvolution();
    PokemonEvolution(const QJsonObject& obj);

    void init(const QJsonObject& obj);

    const optional<vars>& level();
    const optional<QString>& item();
    const QString& toName();

private:
    optional<vars> _level;
    optional<QString> _item;
    QString _toName;
};

Q_DECLARE_METATYPE(PokemonEvolution)

class Pokemon : public BaseModel
{
public:
    Pokemon();
    Pokemon(const QJsonObject& obj);

    void init(const QJsonObject& obj);

    const optional<vars>& pokedex();
    const optional<vars>& growthRate();
    const optional<vars>& baseHP();
    const optional<vars>& baseAttack();
    const optional<vars>& baseDefense();
    const optional<vars>& baseSpeed();
    const optional<vars>& baseSpecial();
    const optional<vars>& baseExpYield();
    const optional<vector<PokemonEvolution>>& evolution();
    const optional<vector<pair<vars, QString>>>& learnedMoves();
    const optional<vector<QString>>& initialMoves();
    const optional<vector<vars>>& tmHm();
    const optional<QString>& type1();
    const optional<QString>& type2();
    const optional<vars>& catchRate();
    const optional<bool>& glitch();

private:
    // Pokemon Pokedex Index
    optional<vars> _pokedex;

    // Pokemon Growth Rate
    // How big or small is the exp range to reach max level
    // The number here is the internal growth rate index, in other words the
    // amount of the number (bigger or smaller) has no correlationto the growth
    // rate
    optional<vars> _growthRate;

    // Base Stats
    optional<vars> _baseHP;
    optional<vars> _baseAttack;
    optional<vars> _baseDefense;
    optional<vars> _baseSpeed;
    optional<vars> _baseSpecial;
    optional<vars> _baseExpYield;

    // Evolution
    // Can be an array of one or more evolutions
    // The only time it'll be more than 1 is with eevee
    optional<vector<PokemonEvolution>> _evolution;

    // Learnset
    optional<vector<pair<vars, QString>>> _learnedMoves;
    optional<vector<QString>> _initialMoves;
    optional<vector<vars>> _tmHm;

    optional<QString> _type1;
    optional<QString> _type2;
    optional<vars> _catchRate;

    // Glitch Pokemon?
    optional<bool> _glitch;
};

Q_DECLARE_METATYPE(Pokemon)

#endif // POKEMON_H
