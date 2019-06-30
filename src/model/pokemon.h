#ifndef POKEMON_H
#define POKEMON_H

#include <QPair>

#include "basemodel.h"

class Item;
class Move;
class Type;
class PokemonEvolution;

class Pokemon : public BaseModel<Pokemon>
{
public:
    friend class PokemonEvolution;
    enum keys: var {
        // Continue where the parent left off
        key_pokedex = BaseModel<Pokemon>::keystore_size,
        key_growth_rate,
        key_base_hp,
        key_base_attack,
        key_base_defense,
        key_base_speed,
        key_base_special,
        key_base_exp_yield,
        key_evolution,
        key_learned_moves,
        key_initial_moves,
        key_tm_hm,
        key_type1,
        key_type2,
        key_catch_rate,
        key_glitch,
        key_to_learned_moves,
        key_to_initial_moves,
        key_to_tm_hm_moves,
        key_to_tm_hm_items,
        key_to_type1,
        key_to_type2,
        key_de_evolution,
        keystore_size
    };

    Pokemon();
    Pokemon(QJsonObject& obj);

    const optional<var> pokedex();
    const optional<var> growthRate();
    const optional<var> baseHp();
    const optional<var> baseAttack();
    const optional<var> baseDefense();
    const optional<var> baseSpeed();
    const optional<var> baseSpecial();
    const optional<var> baseExpYield();
    const optional<QVector<PokemonEvolution>*> evolution();
    const optional<QVector<QPair<var, QString>>*> learnedMoves();
    const optional<QVector<QString>*> initialMoves();
    const optional<QVector<var>*> tmHm();
    const optional<QString> type1();
    const optional<QString> type2();
    const optional<var> catchRate();
    const optional<bool> glitch();

    const optional<QVector<QPair<var, QString>>*> toLearnedMoves();
    const optional<QVector<Move*>*> toInitialMoves();
    const optional<QVector<Move*>*> toTmHmMoves();
    const optional<QVector<Item*>*> toTmHmItems();
    const optional<Type*> toType1();
    const optional<Type*> toType2();
    const optional<Pokemon*> deEvolution();

    // Indexes a store to a db for speedy lookup
    static void initDb();

    // Deep link items from this store with items in other stores
    static void initDeepLink();
private:
    // Init Model
    void init(QJsonObject& obj);
};

Q_DECLARE_METATYPE(Pokemon)
Q_DECLARE_METATYPE(Pokemon*)

#endif // POKEMON_H
