#ifndef BASEMODEL_H_H
#define BASEMODEL_H_H
#include <QtCore/qglobal.h>
#include <QHash>
#include <QJsonObject>
#include <QVector>
#include <optional>

#include "../includes/types.h"

// std::optional is messy but after many attempts and ideas it's needed
// Many database entries "sometimes" include certain keys and std::optional
// is simply the best way to represent that "sometimes"
using std::optional;

// Base class to all models
template<typename T>
class BaseModel
{
public:
  // Use numerical indexing for speed
  enum keys: var8 {
    key_name, // Model Name
    key_index, // Model Index
    keystore_size // Ending to be picked up by child classes
  };

  // Gets a value from the model, type is needed to correctly interpret
  // value
  template<typename K>
  optional<K*> val(var8 key)
  {
    optional<K*> ret;
    if(this->_modelData.contains(key))
      ret = reinterpret_cast<K*>(this->_modelData.value(key));
    return ret;
  }

  // Tells if model has value
  // Many values are "sometimes" there
  bool hasVal(var8 key);

  // Public properties pulled from model data
  // this makes accessing model data easier
  optional<QString*> name();
  optional<var8*> index();
  QHash<var8, void*>* modelData();

  // Applies to all model entries
  static T* dbLookup(QString name); // Lookup by name
  static QVector<T*>* store(); // Get entire numerical indexed store
  static QHash<QString, T*>* db(); // Get entire name indexed store

  // Fills a store with items from an array
  static void initStore(QString filename);

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
  QHash<var8, void*> _modelData = QHash<var8, void*>();

  // Set a value into the store with given key
  void setVal(var8 key, void* val);

  // Array of models
  static QVector<T*> _store;

  // DB Index of models (Array is indexed)
  static QHash<QString, T*> _db;
};

#endif // BASEMODEL_H_H
