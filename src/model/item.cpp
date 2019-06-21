#include "item.h"

Item::Item(const QString name,
           const quint8 index,
           const bool normal,
           const bool typical,
           QObject *parent)
    : BaseModel(name, index, parent),
      normal(normal),
      typical(typical)
{}
