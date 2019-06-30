#include "pokemon.h"
#include "pokemonevolution.h"
#include "move.h"
#include "type.h"
#include "item.h"

Pokemon::Pokemon()
{}

Pokemon::Pokemon(QJsonObject &obj) :
    BaseModel<Pokemon> (obj)
{
    this->init(obj);
}

const optional<var> Pokemon::pokedex()
{
    return this->val<var>(key_pokedex);
}

const optional<var> Pokemon::growthRate()
{
    return this->val<var>(key_growth_rate);
}

const optional<var> Pokemon::baseHp()
{
    return this->val<var>(key_base_hp);
}

const optional<var> Pokemon::baseAttack()
{
    return this->val<var>(key_base_attack);
}

const optional<var> Pokemon::baseDefense()
{
    return this->val<var>(key_base_defense);
}

const optional<var> Pokemon::baseSpeed()
{
    return this->val<var>(key_base_speed);
}

const optional<var> Pokemon::baseSpecial()
{
    return this->val<var>(key_base_special);
}

const optional<var> Pokemon::baseExpYield()
{
    return this->val<var>(key_base_exp_yield);
}

const optional<QVector<PokemonEvolution>*> Pokemon::evolution()
{
    return this->valVoidPtr<QVector<PokemonEvolution>*>(key_evolution);
}

const optional<QVector<QPair<var,QString>>*> Pokemon::learnedMoves()
{
    return this->valVoidPtr<QVector<QPair<var,QString>>*>(key_learned_moves);
}

const optional<QVector<QString>*> Pokemon::initialMoves()
{
    return this->valVoidPtr<QVector<QString>*>(key_initial_moves);
}

const optional<QVector<var>*> Pokemon::tmHm()
{
    return this->valVoidPtr<QVector<var>*>(key_tm_hm);
}

const optional<QString> Pokemon::type1()
{
    return this->val<QString>(key_type1);
}

const optional<QString> Pokemon::type2()
{
    return this->val<QString>(key_type2);
}

const optional<var> Pokemon::catchRate()
{
    return this->val<var>(key_catch_rate);
}

const optional<bool> Pokemon::glitch()
{
    return this->val<bool>(key_glitch);
}

const optional<QVector<QPair<var, QString>>*> Pokemon::toLearnedMoves()
{
    return this->valVoidPtr<QVector<QPair<var, QString>>*>(key_to_learned_moves);
}

const optional<QVector<Move*>*> Pokemon::toInitialMoves()
{
    return this->valVoidPtr<QVector<Move*>*>(key_to_initial_moves);
}

const optional<QVector<Move*>*> Pokemon::toTmHmMoves()
{
    return this->valVoidPtr<QVector<Move*>*>(key_to_tm_hm_moves);
}

const optional<QVector<Item*>*> Pokemon::toTmHmItems()
{
    return this->valVoidPtr<QVector<Item*>*>(key_to_tm_hm_items);
}

const optional<Type*> Pokemon::toType1()
{
    return this->valVoidPtr<Type*>(key_to_type1);
}

const optional<Type*> Pokemon::toType2()
{
    return this->valVoidPtr<Type*>(key_to_type2);
}

const optional<Pokemon*> Pokemon::deEvolution()
{
    return this->valVoidPtr<Pokemon*>(key_de_evolution);
}

void Pokemon::init(QJsonObject &obj)
{
    BaseModel<Pokemon>::init(obj);

    if(obj.contains("pokedex"))
        this->setVal(key_pokedex, obj["pokedex"].toInt());
    if(obj.contains("growthRate"))
        this->setVal(key_growth_rate, obj["growthRate"].toInt());
    if(obj.contains("baseHp"))
        this->setVal(key_base_hp, obj["baseHp"].toInt());
    if(obj.contains("baseAttack"))
        this->setVal(key_base_attack, obj["baseAttack"].toInt());
    if(obj.contains("baseDefense"))
        this->setVal(key_base_defense, obj["baseDefense"].toInt());
    if(obj.contains("baseSpeed"))
        this->setVal(key_base_speed, obj["baseSpeed"].toInt());
    if(obj.contains("baseSpecial"))
        this->setVal(key_base_special, obj["baseSpecial"].toInt());
    if(obj.contains("baseExpYield"))
        this->setVal(key_base_exp_yield, obj["baseExpYield"].toInt());
    if(obj.contains("type1"))
        this->setVal(key_type1, obj["type1"].toString());
    if(obj.contains("type2"))
        this->setVal(key_type2, obj["type2"].toString());
    if(obj.contains("catchRate"))
        this->setVal(key_catch_rate, obj["catchRate"].toInt());
    if(obj.contains("glitch"))
        this->setVal(key_glitch, obj["glitch"].toBool());

    // Evolution is a bit tricky no thianks to Eevee
    // There can be one or more evolutions (Any more than 1 is Eevee)
    // If there's only 1 (99.9% of the time) it'll be an object
    // Otherwise it'll be an array of objects
    // PokemonEvolution handles the details of extracting object data
    if(obj.contains("evolution")) {
        // Assign empty evolution array
        auto localEvolutionArr = new QVector<PokemonEvolution>();

        // get evolution object or array
        auto evolution = obj["evolution"];

        // Determine if object or array and process accordingly
        if(evolution.isObject()) {
            auto evolutionObj = evolution.toObject();
            auto pokeEvo = PokemonEvolution(evolutionObj, this);
            localEvolutionArr->push_back(pokeEvo);
        }
        else if(evolution.isArray()) {
            auto evolutionArr = evolution.toArray();
            for(var i{0}; i < evolutionArr.size(); ++i) {
                auto evolutionArrItem = evolutionArr[i].toObject();
                auto pokeEvo = PokemonEvolution(evolutionArrItem, this);
                localEvolutionArr->push_back(pokeEvo);
            }
        }

        // Often there will only be 1 or 3 objects, theres no use in having
        // excess in memory
        localEvolutionArr->shrink_to_fit();

        // Save locally
        this->setValVoidPtr<QVector<PokemonEvolution>*>(key_evolution, localEvolutionArr);
    }

    // Pokemon moves learned over time
    if(obj.contains("moves")) {
        // Assign moves array
        auto localMovesArr = new QVector<QPair<var, QString>>();

        auto movesArr = obj["moves"].toArray();
        for(var i{0}; i < movesArr.size(); ++i) {
            auto movesArrItem = movesArr[i].toObject();
            auto level = static_cast<vars>(movesArrItem["level"].toInt());
            auto move = movesArrItem["move"].toString();
            localMovesArr->push_back(QPair<vars, QString>(level, move));
        }

        // Squeeze excess
        localMovesArr->shrink_to_fit();
        this->setValVoidPtr<QVector<QPair<var, QString>>*>(key_learned_moves, localMovesArr);
    }

    // Pokemon moves start off knowing
    if(obj.contains("initial")) {
        // Assign initial moves array
        auto localInitialMovesArray = new QVector<QString>();

        auto initialMovesArr = obj["initial"].toArray();
        for(var i{0}; i < initialMovesArr.size(); ++i) {
            auto initialMovesArrItem = initialMovesArr[i].toString();
            localInitialMovesArray->push_back(initialMovesArrItem);
        }

        // Squeeze excess
        localInitialMovesArray->shrink_to_fit();
        this->setValVoidPtr<QVector<QString>*>(key_initial_moves, localInitialMovesArray);
    }

    // Pokemon Moves can learn via TM/HM
    if(obj.contains("tmHm")) {
        // Assign initial moves array
        auto localTmHmArray = new QVector<var>();

        auto tmHmArr = obj["tmHm"].toArray();
        for(var i{0}; i < tmHmArr.size(); ++i) {
            auto tmHmArrItem = static_cast<vars>(tmHmArr[i].toInt());
            localTmHmArray->push_back(tmHmArrItem);
        }

        // Squeeze excess
        localTmHmArray->shrink_to_fit();
        this->setValVoidPtr<QVector<var>*>(key_tm_hm, localTmHmArray);
    }
}

void Pokemon::initDb()
{
    BaseModel<Pokemon>::initDb();

    QString tmp;
    for(auto el : _store)
    {
        if(el.pokedex())
        {
            auto val = *el.pokedex();

            tmp = "dex";
            tmp = tmp.append(QString::number(val));
            _db.insert(tmp, &el);
        }
    }
}

void Pokemon::initDeepLink()
{
    // Loop through pokemon store
    for(auto el : _store)
    {
        // If there is a list of learned moves
        if(el.learnedMoves())
        {
            // Grab that list
            auto learnedMoves = *el.learnedMoves();

            // Initialize deep-linked array
            auto localArray = new QVector<QPair<vars, Move*>>();

            // Begin deep-linking all moves in list
            for(auto learnedMove : *learnedMoves)
                // Insert deep linked, crash otherwise which is needed
                localArray->push_back(
                            QPair<vars, Move*>(
                                learnedMove.first,
                                Move::db()->value(learnedMove.second)
                            ));

            localArray->shrink_to_fit();
            el.setValVoidPtr<QVector<QPair<vars, Move*>>*>(key_to_learned_moves, localArray);
        }

        // Check for initial moves
        if(el.initialMoves())
        {
            // Grab the list
            auto initialMoves = *el.initialMoves();

            // Initialize deep-linked array
            auto localArray = new QVector<Move*>();

            // Deep link all initial moves
            // Crashes if not found which is needed
            for(auto initialMove : *initialMoves)
                localArray->push_back(Move::db()->value(initialMove));

            localArray->shrink_to_fit();
            el.setValVoidPtr<QVector<Move*>*>(key_to_initial_moves, localArray);
        }

        // If list of learnable TMs/HMs
        // We actually need to link this to moves and items
        if(el.tmHm())
        {
            // Grab list
            auto tmHmMoves = *el.tmHm();

            // Init both deep linked arrays
            auto localArrayMoves = new QVector<Move*>();
            auto localArrayItems = new QVector<Item*>();

            // Deep link
            for(auto tmHm : *tmHmMoves)
            {
                auto tmHmStr = "tm" + QString::number(tmHm);
                localArrayMoves->push_back(Move::db()->value(tmHmStr));
                localArrayItems->push_back(Item::db()->value(tmHmStr));
            }

            localArrayMoves->shrink_to_fit();
            localArrayItems->shrink_to_fit();
            el.setValVoidPtr<QVector<Move*>*>(key_to_tm_hm_moves, localArrayMoves);
            el.setValVoidPtr<QVector<Item*>*>(key_to_tm_hm_items, localArrayItems);
        }

        // If type 1/2, deep link it to types
        if(el.type1())
            el.setValVoidPtr<Type*>(key_to_type1, Type::db()->value(*el.type1()));

        if(el.type2())
            el.setValVoidPtr<Type*>(key_to_type2, Type::db()->value(*el.type2()));

        // Loop through 1 or more evolutions and initiate deep links
        if(el.evolution())
            for(auto evolEntry : **el.evolution())
            {
                evolEntry.initDeepLink();
            }
    }
}
