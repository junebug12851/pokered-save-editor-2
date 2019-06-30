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

const optional<var> PokemonEvolution::level()
{
    return this->val<var>(key_level);
}

const optional<QString> PokemonEvolution::item()
{
    return this->val<QString>(key_item);
}

const optional<Item*> PokemonEvolution::toItem()
{
    return this->valVoidPtr<Item*>(key_to_item);
}

const Pokemon* PokemonEvolution::toPokemon()
{
    return *this->valVoidPtr<Pokemon*>(key_to_pokemon);
}

const Pokemon* PokemonEvolution::parent()
{
    return *this->valVoidPtr<Pokemon*>(key_parent);
}

void PokemonEvolution::init(QJsonObject &obj, Pokemon* parent)
{
    BaseModel<PokemonEvolution>::init(obj);
    this->setValVoidPtr<Pokemon*>(key_parent, parent);

    if(obj.contains("level"))
        this->setVal(key_level, obj["level"].toInt());
    if(obj.contains("item"))
        this->setVal(key_item, obj["item"].toString());
}

void PokemonEvolution::initDeepLink()
{
    QString tmp;
    for(auto& el : _store)
    {
        // Link To Pokemon
        el.setValVoidPtr<Pokemon*>(key_to_pokemon, Pokemon::db()->value(*el.name()));

        // Link To Item
        if(el.item())
            el.setValVoidPtr<Item*>(key_to_item, Item::db()->value(*el.item()));

        // Because of const this is somewhat complicated
        // Link to de-evolution pokemon (parent) but work around const-ness
        Pokemon* tmpPokemon = const_cast<Pokemon*>(el.toPokemon());
        tmpPokemon->setValVoidPtr<Pokemon*>(
                    Pokemon::key_de_evolution,
                    const_cast<Pokemon*>(el.parent())
                );
    }
}
