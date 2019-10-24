#ifndef BASEMODEL_C_H
#define BASEMODEL_C_H

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QScopedPointer>

#include "basemodel_h.h"

template<typename T>
optional<QString*> BaseModel<T>::name()
{
  return this->val<QString>(key_name);
}

template<typename T>
optional<var8*> BaseModel<T>::index()
{
  return this->val<var8>(key_name);
}

template<typename T>
QHash<var8, void*>* BaseModel<T>::modelData()
{
  return &this->_modelData;
}

template<typename T>
T* BaseModel<T>::dbLookup(QString name)
{
  return BaseModel<T>::_db.value(name, nullptr);
}

template<typename T>
QVector<T*>* BaseModel<T>::store()
{
  return BaseModel<T>::_store;
}

template<typename T>
QHash<QString, T*>* BaseModel<T>::db()
{
  return &BaseModel<T>::_db;
}

template<typename T>
BaseModel<T>::BaseModel()
{}

template<typename T>
BaseModel<T>::BaseModel(QJsonObject& obj)
{
  this->init(obj);
}

template<typename T>
void BaseModel<T>::init(QJsonObject& obj)
{
  if(obj.contains("name"))
    this->setVal(key_name, new QString(obj["name"].toString()));

  if(obj.contains("ind"))
    this->setVal(key_index, new var8(obj["ind"].toInt()));
}

template<typename T>
QVector<T*> BaseModel<T>::_store = QVector<T*>();

template<typename T>
QHash<QString, T*> BaseModel<T>::_db = QHash<QString, T*>();

template<typename T>
void BaseModel<T>::initStore(QString filename)
{
  _store.clear();

  // Prepare to read in file
  QByteArray val;
  QFile file;

  // Read in file
  file.setFileName(filename);
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  val = file.readAll();
  file.close();

  // Extract Json Array
  QJsonDocument doc{QJsonDocument::fromJson(val)};
  QJsonArray arr{doc.array()};

  // Insert JSON Array items
  for(auto arrItem : arr)
  {
    // Kind of weird it has to be seperate like this but combined together
    // doesn't work at all
    auto tmp = arrItem.toObject();
    _store.push_back(new T(tmp));
  }

  _store.squeeze();
}

// Index the store seperately
template<typename T>
void BaseModel<T>::initDb()
{
  // Clear out db
  _db.clear();

  // Loop through all store items
  for(auto el : _store)
  {
    // Index index if present
    if(el->index())
      _db.insert(QString::number(**el->index()), el);

    // Index name if present
    if(el->name())
      _db.insert(**el->name(), el);
  }
}

template<typename T>
void BaseModel<T>::initDeepLink()
{}

template<typename T>
bool BaseModel<T>::hasVal(var8 key)
{
  return this->_modelData.contains(key);
}

template<typename T>
void BaseModel<T>::setVal(var8 key, void* val)
{
  this->_modelData.insert(key, val);
}

#endif // BASEMODEL_C_H
