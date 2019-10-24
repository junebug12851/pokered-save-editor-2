#include "pokemonevolution.h"
#include "pokemon.h"
#include "item.h"

PokemonEvolution::PokemonEvolution()
{}

PokemonEvolution::PokemonEvolution(QJsonObject &obj, Pokemon* parent) :
    BaseModel<PokemonEvolution> (obj)
{
    this->init(obj, parent);
}

optional<var8*> PokemonEvolution::level()
{
    return this->val<var8>(key_level);
}

optional<QString*> PokemonEvolution::item()
{
    return this->val<QString>(key_item);
}

optional<Item*> PokemonEvolution::toItem()
{
    return this->val<Item>(key_to_item);
}

Pokemon* PokemonEvolution::toPokemon()
{
    return *this->val<Pokemon>(key_to_pokemon);
}

Pokemon* PokemonEvolution::parent()
{
    return *this->val<Pokemon>(key_parent);
}

void PokemonEvolution::init(QJsonObject &obj, Pokemon* parent)
{
    BaseModel<PokemonEvolution>::init(obj);
    this->setVal(key_parent, parent);

    if(obj.contains("level"))
        this->setVal(key_level, new var8(static_cast<var8>(obj["level"].toInt())));
    if(obj.contains("item"))
        this->setVal(key_item, new QString(obj["item"].toString()));
}

void PokemonEvolution::initDeepLink()
{
    QString tmp;
    for(auto el : _store)
    {
        // Link To Pokemon
        el->setVal(key_to_pokemon, Pokemon::db()->value(**el->name()));

        // Link To Item
        if(el->item())
            el->setVal(key_to_item, Item::db()->value(**el->item()));

        // Because of this is somewhat complicated
        // Link to de-evolution pokemon (parent) but work around const-ness
        Pokemon* tmpPokemon = const_cast<Pokemon*>(el->toPokemon());
        tmpPokemon->setVal(
                    Pokemon::key_de_evolution,
                    const_cast<Pokemon*>(el->parent())
                );
    }
}
