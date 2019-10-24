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
    enum keys: var8 {
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

    optional<var8*> pokedex();
    optional<var8*> growthRate();
    optional<var8*> baseHp();
    optional<var8*> baseAttack();
    optional<var8*> baseDefense();
    optional<var8*> baseSpeed();
    optional<var8*> baseSpecial();
    optional<var8*> baseExpYield();
    optional<QVector<PokemonEvolution*>*> evolution();
    optional<QVector<QPair<var8, QString*>*>*> learnedMoves();
    optional<QVector<QString*>*> initialMoves();
    optional<QVector<var8*>*> tmHm();
    optional<QString*> type1();
    optional<QString*> type2();
    optional<var8*> catchRate();
    optional<bool*> glitch();

    optional<QVector<QPair<var8, QString*>*>*> toLearnedMoves();
    optional<QVector<Move*>*> toInitialMoves();
    optional<QVector<Move*>*> toTmHmMoves();
    optional<QVector<Item*>*> toTmHmItems();
    optional<Type*> toType1();
    optional<Type*> toType2();
    optional<Pokemon*> deEvolution();

    // Indexes a store to a db for speedy lookup
    static void initDb();

    // Deep link items from this store with items in other stores
    static void initDeepLink();
private:
    // Init Model
    void init(QJsonObject& obj);
};

#endif // POKEMON_H
