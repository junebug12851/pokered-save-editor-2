#ifndef POKEMON_H
#define POKEMON_H

#include <utility>

using std::pair;

#include "basemodel.h"

// Forward Declare for PokemonEvolution class
class Pokemon;

class Move;
class Type;
class Item;

using _PokemonArr = vector<Pokemon*>;
using _PokemonDb = unordered_map<QString, Pokemon*>;

using PokemonArr = const _PokemonArr*;
using PokemonDb = const _PokemonDb*;

using LearnedMoves = pair<vars, QString>;
using ToLearnedMoves = pair<vars, Move*>;

// Pokemon can evolve from level or item
class PokemonEvolution : public QObject {
    Q_OBJECT
    Q_PROPERTY(const optional<vars>& level READ level CONSTANT FINAL)
    Q_PROPERTY(const optional<QString>& item READ item CONSTANT FINAL)
    Q_PROPERTY(const QString& toName READ toName CONSTANT FINAL)
    Q_PROPERTY(const optional<Item*>& toItem READ toItem CONSTANT FINAL)
    Q_PROPERTY(const Pokemon* toPokemon READ toPokemon CONSTANT FINAL)

public:
    friend class Pokemon;

    PokemonEvolution();
    PokemonEvolution(const QJsonObject& obj);

    void init(const QJsonObject& obj);

    const optional<vars>& level();
    const optional<QString>& item();
    const QString& toName();

    const optional<Item*>& toItem();
    const Pokemon* toPokemon();

private:
    /**
     * Stage 1 Variables: Extracted from JSON Data
     */
    optional<vars> _level;
    optional<QString> _item;
    QString _toName;

    /**
     * Stage 2 Variables: Inter-Linking amongst data
     */
    optional<Item*> _toItem;
    Pokemon* _toPokemon;
};

class Pokemon : public BaseModel
{
    Q_OBJECT
    Q_PROPERTY(const optional<vars>& pokedex READ pokedex CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& growthRate READ growthRate CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& baseHP READ baseHP CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& baseAttack READ baseAttack CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& baseDefense READ baseDefense CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& baseSpeed READ baseSpeed CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& baseSpecial READ baseSpecial CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& baseExpYield READ baseExpYield CONSTANT FINAL)
    Q_PROPERTY(const optional<vector<PokemonEvolution*>>& evolution READ evolution CONSTANT FINAL)
    Q_PROPERTY(const optional<vector<LearnedMoves>>& learnedMoves READ learnedMoves CONSTANT FINAL)
    Q_PROPERTY(const optional<vector<QString>>& initialMoves READ initialMoves CONSTANT FINAL)
    Q_PROPERTY(const optional<vector<vars>>& tmHm READ tmHm CONSTANT FINAL)
    Q_PROPERTY(const optional<QString>& type1 READ type1 CONSTANT FINAL)
    Q_PROPERTY(const optional<QString>& type2 READ type2 CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& catchRate READ catchRate CONSTANT FINAL)
    Q_PROPERTY(const optional<bool>& glitch READ glitch CONSTANT FINAL)
    Q_PROPERTY(const optional<vector<ToLearnedMoves>>& toLearnedMoves READ toLearnedMoves CONSTANT FINAL)
    Q_PROPERTY(const optional<vector<Move*>>& toInitialMoves READ toInitialMoves CONSTANT FINAL)
    Q_PROPERTY(const optional<vector<Move*>>& toTmHmMoves READ toTmHmMoves CONSTANT FINAL)
    Q_PROPERTY(const optional<vector<Item*>>& toTmHmItems READ toTmHmItems CONSTANT FINAL)
    Q_PROPERTY(const optional<Type*>& toType1 READ toType1 CONSTANT FINAL)
    Q_PROPERTY(const optional<Type*>& toType2 READ toType2 CONSTANT FINAL)
    Q_PROPERTY(const optional<Pokemon*>& devolve READ devolve CONSTANT FINAL)
    Q_PROPERTY(PokemonArr store READ store CONSTANT FINAL)
    Q_PROPERTY(PokemonDb db READ db CONSTANT FINAL)

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
    const optional<vector<PokemonEvolution*>>& evolution();
    const optional<vector<LearnedMoves>>& learnedMoves();
    const optional<vector<QString>>& initialMoves();
    const optional<vector<vars>>& tmHm();
    const optional<QString>& type1();
    const optional<QString>& type2();
    const optional<vars>& catchRate();
    const optional<bool>& glitch();

    const optional<vector<ToLearnedMoves>>& toLearnedMoves();
    const optional<vector<Move*>>& toInitialMoves();
    const optional<vector<Move*>>& toTmHmMoves();
    const optional<vector<Item*>>& toTmHmItems();
    const optional<Type*>& toType1();
    const optional<Type*>& toType2();
    const optional<Pokemon*>& devolve();

    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static PokemonArr store();
    static PokemonDb db();
    static void initStore(const QString& filename);
    static void initDb();
    static void initDeepLink();

private:
    /**
     * Stage 1 Variables: Extracted from JSON Data
     */
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
    optional<vector<PokemonEvolution*>> _evolution;

    // Learnset
    optional<vector<pair<vars, QString>>> _learnedMoves;
    optional<vector<QString>> _initialMoves;
    optional<vector<vars>> _tmHm;

    optional<QString> _type1;
    optional<QString> _type2;
    optional<vars> _catchRate;

    // Glitch Pokemon?
    optional<bool> _glitch;

    /**
     * Stage 2 Variables: Inter-Linking amongst data
     */
    optional<vector<pair<vars, Move*>>> _toLearnedMoves;
    optional<vector<Move*>> _toInitialMoves;
    optional<vector<Move*>> _toTmHmMoves;
    optional<vector<Item*>> _toTmHmItems;
    optional<Type*> _toType1;
    optional<Type*> _toType2;
    optional<Pokemon*> _devolve; // Pokemons de-evolution

    /**
     * Data Store and other static properties and methods related to building
     * and maintaining the store
     */
    static _PokemonArr _store;

    // Index
    // BaseModel does some of the initial indexing of it's own
    // dex + #    (dex1)
    // dex + ###  (dex001)
    static _PokemonDb _db; // Indexed for lookup
};

#endif // POKEMON_H
