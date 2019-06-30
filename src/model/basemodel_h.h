#ifndef BASEMODEL_H_H
#define BASEMODEL_H_H
#include <QtCore/qglobal.h>
#include <QHash>
#include <QJsonObject>
#include <QVariant>
#include <QVector>
#include <optional>

#include "../includes/types.h"

using std::optional;

// Base class to all models
template<typename T>
class BaseModel
{
public:
    enum keys: var {
        key_name,
        key_index,
        keystore_size
    };

    // Apparently sub-templates cannot be declared outside of class
    template<typename K>
    const optional<K> val(var key)
    {
        optional<K> ret;
        if(this->_modelData.contains(key))
            ret = this->_modelData.value(key).template value<K>();
        return ret;
    }

    // Casts a stubborn pointer type from QVariant
    template<typename K>
    const optional<K> valVoidPtr(var key)
    {
        optional<K> ret;
        if(this->_modelData.contains(key))
        {
            void* _tmp = this->_modelData.value(key).template value<void*>();
            ret = static_cast<K>(_tmp);
        }
        return ret;
    }

    bool hasVal(var key);

    // Public properties pulled from model data
    // there is just no getting around std::optional meaning this won't be
    // Qt Quick acceptable. Oh Well.
    const optional<QString> name();
    const optional<var> index();
    const QHash<var, QVariant>* modelData();

    static const T* dbLookup(QString& name);
    static const QVector<T>* store();
    static const QHash<QString, T*>* db();

    // Fills a store with items from an array
    static void initStore(const QString& filename);

    // Indexes a store to a db for speedy lookup
    static void initDb();

    // Deep link items from this store with items in other stores
    static void initDeepLink();

protected:
    // Empty constructor and auto init-ing of data from JSON object
    BaseModel();
    BaseModel(
            QJsonObject& obj
        );
    void init(QJsonObject& obj);

    // Internal data store where all the model data is stored in
    QHash<var, QVariant> _modelData = QHash<var, QVariant>();

    void setVal(var key, QVariant val);

    // Casts a stubborn pointer type to QVariant
    template<typename K>
    void setValVoidPtr(var key, K val)
    {
        QVariant _val(QVariant::fromValue(static_cast<void*>(val)));
        this->setVal(key, _val);
    }

    // Array of models
    static QVector<T> _store;

    // DB Index of models (Array is indexed)
    static QHash<QString, T*> _db;
};

#endif // BASEMODEL_H_H
