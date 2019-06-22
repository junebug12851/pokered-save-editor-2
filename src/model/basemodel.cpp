#include "basemodel.h"

BaseModel::BaseModel(
        const QString name,
        const int index,
        QObject *parent
    ) :
    QObject(parent),
    name(name),
    index(index)
{}
