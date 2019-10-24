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

optional<var8*> Pokemon::pokedex()
{
    return this->val<var8>(key_pokedex);
}

optional<var8*> Pokemon::growthRate()
{
    return this->val<var8>(key_growth_rate);
}

optional<var8*> Pokemon::baseHp()
{
    return this->val<var8>(key_base_hp);
}

optional<var8*> Pokemon::baseAttack()
{
    return this->val<var8>(key_base_attack);
}

optional<var8*> Pokemon::baseDefense()
{
    return this->val<var8>(key_base_defense);
}

optional<var8*> Pokemon::baseSpeed()
{
    return this->val<var8>(key_base_speed);
}

optional<var8*> Pokemon::baseSpecial()
{
    return this->val<var8>(key_base_special);
}

optional<var8*> Pokemon::baseExpYield()
{
    return this->val<var8>(key_base_exp_yield);
}

optional<QVector<PokemonEvolution*>*> Pokemon::evolution()
{
    return this->val<QVector<PokemonEvolution*>>(key_evolution);
}

optional<QVector<QPair<var8,QString*>*>*> Pokemon::learnedMoves()
{
    return this->val<QVector<QPair<var8,QString*>*>>(key_learned_moves);
}

optional<QVector<QString*>*> Pokemon::initialMoves()
{
    return this->val<QVector<QString*>>(key_initial_moves);
}

optional<QVector<var8*>*> Pokemon::tmHm()
{
    return this->val<QVector<var8*>>(key_tm_hm);
}

optional<QString*> Pokemon::type1()
{
    return this->val<QString>(key_type1);
}

optional<QString*> Pokemon::type2()
{
    return this->val<QString>(key_type2);
}

optional<var8*> Pokemon::catchRate()
{
    return this->val<var8>(key_catch_rate);
}

optional<bool*> Pokemon::glitch()
{
    return this->val<bool>(key_glitch);
}

optional<QVector<QPair<var8, QString*>*>*> Pokemon::toLearnedMoves()
{
    return this->val<QVector<QPair<var8, QString*>*>>(key_to_learned_moves);
}

optional<QVector<Move*>*> Pokemon::toInitialMoves()
{
    return this->val<QVector<Move*>>(key_to_initial_moves);
}

optional<QVector<Move*>*> Pokemon::toTmHmMoves()
{
    return this->val<QVector<Move*>>(key_to_tm_hm_moves);
}

optional<QVector<Item*>*> Pokemon::toTmHmItems()
{
    return this->val<QVector<Item*>>(key_to_tm_hm_items);
}

optional<Type*> Pokemon::toType1()
{
    return this->val<Type>(key_to_type1);
}

optional<Type*> Pokemon::toType2()
{
    return this->val<Type>(key_to_type2);
}

optional<Pokemon*> Pokemon::deEvolution()
{
    return this->val<Pokemon>(key_de_evolution);
}

void Pokemon::init(QJsonObject &obj)
{
    BaseModel<Pokemon>::init(obj);

    if(obj.contains("pokedex"))
        this->setVal(key_pokedex, new var8(static_cast<var8>(obj["pokedex"].toInt())));
    if(obj.contains("growthRate"))
        this->setVal(key_growth_rate, new var8(static_cast<var8>(obj["growthRate"].toInt())));
    if(obj.contains("baseHp"))
        this->setVal(key_base_hp, new var8(static_cast<var8>(obj["baseHp"].toInt())));
    if(obj.contains("baseAttack"))
        this->setVal(key_base_attack, new var8(static_cast<var8>(obj["baseAttack"].toInt())));
    if(obj.contains("baseDefense"))
        this->setVal(key_base_defense, new var8(static_cast<var8>(obj["baseDefense"].toInt())));
    if(obj.contains("baseSpeed"))
        this->setVal(key_base_speed, new var8(static_cast<var8>(obj["baseSpeed"].toInt())));
    if(obj.contains("baseSpecial"))
        this->setVal(key_base_special, new var8(static_cast<var8>(obj["baseSpecial"].toInt())));
    if(obj.contains("baseExpYield"))
        this->setVal(key_base_exp_yield, new var8(static_cast<var8>(obj["baseExpYield"].toInt())));
    if(obj.contains("type1"))
        this->setVal(key_type1, new QString(obj["type1"].toString()));
    if(obj.contains("type2"))
        this->setVal(key_type2, new QString(obj["type2"].toString()));
    if(obj.contains("catchRate"))
        this->setVal(key_catch_rate, new var8(static_cast<var8>(obj["catchRate"].toInt())));
    if(obj.contains("glitch"))
        this->setVal(key_glitch, new bool(obj["glitch"].toBool()));

    // Evolution is a bit tricky no thianks to Eevee
    // There can be one or more evolutions (Any more than 1 is Eevee)
    // If there's only 1 (99.9% of the time) it'll be an object
    // Otherwise it'll be an array of objects
    // PokemonEvolution handles the details of extracting object data
    if(obj.contains("evolution")) {
        // Assign empty evolution array
        auto localEvolutionArr = new QVector<PokemonEvolution*>();

        // get evolution object or array
        auto evolution = obj["evolution"];

        // Determine if object or array and process accordingly
        // It will only ever be an array for eevee because eevee has to be
        // difficult lol
        if(evolution.isObject()) {
            auto evolutionObj = evolution.toObject();
            auto pokeEvo = new PokemonEvolution(evolutionObj, this);
            localEvolutionArr->push_back(pokeEvo);
        }
        else if(evolution.isArray()) {
            auto evolutionArr = evolution.toArray();
            for(var8 i{0}; i < evolutionArr.size(); ++i) {
                auto evolutionArrItem = evolutionArr[i].toObject();
                auto pokeEvo = new PokemonEvolution(evolutionArrItem, this);
                localEvolutionArr->push_back(pokeEvo);
            }
        }

        // Often there will only be 1 or 3 objects, theres no use in having
        // excess in memory
        localEvolutionArr->shrink_to_fit();

        // Save locally
        this->setVal(key_evolution, localEvolutionArr);
    }

    // Pokemon moves learned over time
    if(obj.contains("moves")) {
        // Assign moves array
        auto localMovesArr = new QVector<QPair<var8, QString*>*>();

        auto movesArr = obj["moves"].toArray();
        for(var8 i{0}; i < movesArr.size(); ++i) {
            auto movesArrItem = movesArr[i].toObject();
            auto level = static_cast<var8>(movesArrItem["level"].toInt());
            auto move = new QString(movesArrItem["move"].toString());
            localMovesArr->push_back(new QPair<var8, QString*>(level, move));
        }

        // Squeeze excess
        localMovesArr->shrink_to_fit();
        this->setVal(key_learned_moves, localMovesArr);
    }

    // Pokemon moves start off knowing
    if(obj.contains("initial")) {
        // Assign initial moves array
        auto localInitialMovesArray = new QVector<QString*>();

        auto initialMovesArr = obj["initial"].toArray();
        for(var8 i{0}; i < initialMovesArr.size(); ++i) {
            auto initialMovesArrItem = new QString(initialMovesArr[i].toString());
            localInitialMovesArray->push_back(initialMovesArrItem);
        }

        // Squeeze excess
        localInitialMovesArray->shrink_to_fit();
        this->setVal(key_initial_moves, localInitialMovesArray);
    }

    // Pokemon Moves can learn via TM/HM
    if(obj.contains("tmHm")) {
        // Assign initial moves array
        auto localTmHmArray = new QVector<var8*>();

        auto tmHmArr = obj["tmHm"].toArray();
        for(var8 i{0}; i < tmHmArr.size(); ++i) {
            auto tmHmArrItem = new var8(static_cast<var8>(tmHmArr[i].toInt()));
            localTmHmArray->push_back(tmHmArrItem);
        }

        // Squeeze excess
        localTmHmArray->shrink_to_fit();
        this->setVal(key_tm_hm, localTmHmArray);
    }
}

void Pokemon::initDb()
{
    BaseModel<Pokemon>::initDb();

    QString tmp;
    for(auto el : _store)
    {
        if(el->pokedex())
        {
            auto val = **el->pokedex();

            tmp = "dex";
            tmp = tmp.append(QString::number(val));
            _db.insert(tmp, el);
        }
    }
}

void Pokemon::initDeepLink()
{
    // Loop through pokemon store
    for(auto el : _store)
    {
        // If there is a list of learned moves
        if(el->learnedMoves())
        {
            // Grab that list
            auto learnedMoves = **el->learnedMoves();

            // Initialize deep-linked array
            auto localArray = new QVector<QPair<var8, Move*>*>();

            // Begin deep-linking all moves in list
            for(auto learnedMove : learnedMoves)
                // Insert deep linked, crash otherwise which is needed
                localArray->push_back(
                            new QPair<var8, Move*>(
                                learnedMove->first,
                                Move::db()->value(*learnedMove->second)
                            ));

            localArray->shrink_to_fit();
            el->setVal(key_to_learned_moves, localArray);
        }

        // Check for initial moves
        if(el->initialMoves())
        {
            // Grab the list
            auto initialMoves = **el->initialMoves();

            // Initialize deep-linked array
            auto localArray = new QVector<Move*>();

            // Deep link all initial moves
            // Crashes if not found which is needed
            for(auto initialMove : initialMoves)
                localArray->push_back(Move::db()->value(*initialMove));

            localArray->shrink_to_fit();
            el->setVal(key_to_initial_moves, localArray);
        }

        // If list of learnable TMs/HMs
        // We actually need to link this to moves and items
        if(el->tmHm())
        {
            // Grab list
            auto tmHmMoves = **el->tmHm();

            // Init both deep linked arrays
            auto localArrayMoves = new QVector<Move*>();
            auto localArrayItems = new QVector<Item*>();

            // Deep link
            for(auto tmHm : tmHmMoves)
            {
                auto tmHmStr = "tm" + QString::number(*tmHm);
                localArrayMoves->push_back(Move::db()->value(tmHmStr));
                localArrayItems->push_back(Item::db()->value(tmHmStr));
            }

            localArrayMoves->shrink_to_fit();
            localArrayItems->shrink_to_fit();
            el->setVal(key_to_tm_hm_moves, localArrayMoves);
            el->setVal(key_to_tm_hm_items, localArrayItems);
        }

        // If type 1/2, deep link it to types
        if(el->type1())
            el->setVal(key_to_type1, Type::db()->value(**el->type1()));

        if(el->type2())
            el->setVal(key_to_type2, Type::db()->value(**el->type2()));

        // Loop through 1 or more evolutions and initiate deep links
        if(el->evolution())
            for(auto evolEntry : **el->evolution())
            {
                evolEntry->initDeepLink();
            }
    }
}
