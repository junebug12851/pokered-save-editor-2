#ifndef BASEMODEL_H
#define BASEMODEL_H

#include <QtCore/QObject>
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QVariant>

#include <optional>
#include <vector>
#include <unordered_map>
#include <functional>

using std::optional;
using std::vector;
using std::unordered_map;

namespace std {
  template<> struct hash<QString> {
    std::size_t operator()(const QString& s) const {
      return qHash(s);
    }
  };
}

#include "../includes/vars.h"

/**
  * Base model to all models
  * Most Every model contains these values
 */
class BaseModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(const optional<QString>& name READ name CONSTANT FINAL)
    Q_PROPERTY(const optional<vars>& index READ index CONSTANT FINAL)

public:
    BaseModel();
    BaseModel(const QJsonObject& obj);

    void init(const QJsonObject& obj);

    // Read-Only Public Getters
    const optional<QString>& name();
    const optional<vars>& index();

protected:
    // Fills a store with items from an array, the class itself does the
    // hard work, this does the grunt work of getting the needed data to the
    // class in each instance and inserting resulting data into the store
    template<class T>
    static void initStore(const QString& filename, vector<T*>& store);

    // This partly does some of the work in indexing the store
    template<class T>
    static void initIndex(unordered_map<QString, T*>& db,
                       vector<T*>& store);

private:
    /**
     * Stage 1 Variables: Extracted from JSON Data
     */

    // Name of data entry
    optional<QString> _name;

    // Internal game index of data entry
    optional<vars> _index;
};

template<typename T>
void BaseModel::initStore(const QString &filename, vector<T*>& store)
{
    store.clear();

    // Prepare to read in file
    QByteArray val;
    QFile file;

    // Read in file
    file.setFileName(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    val = file.readAll();
    file.close();

    // Extract Json Array
    QJsonDocument doc = QJsonDocument::fromJson(val);
    QJsonArray arr = doc.array();

    // Insert JSON Array items
    for(auto arrItem : arr)
    {
        store.push_back(new T(arrItem.toObject()));
    }

    store.shrink_to_fit();
}

// Index the store seperately
template<typename T>
void BaseModel::initIndex(unordered_map<QString, T*>& db,
                          vector<T*>& store)
{
    // Clear out db
    db.clear();

    // Loop through all store items
    for(auto el : store)
    {
        // Index index if present
        if(el->_index)
        {
            QString indStr = QString::number(*el->_index);
            db[indStr] = el;
        }

        // Index name if present in 4 different ways
        // Duplicates will just be overwritten
        // Normal (Master Ball)
        // Lowercase (master ball)
        // Lower runon (masterball)
        // snake_case (master_ball)
        if(el->_name)
        {
            db[*el->_name] = el;

            QString tmp = *el->_name;
            tmp = tmp.toLower();
            db[tmp] = el;

            tmp = *el->_name;
            tmp = tmp.toLower();
            tmp = tmp.replace(" ", "");
            db[tmp] = el;

            tmp = *el->_name;
            tmp = tmp.toLower();
            tmp = tmp.replace(' ', '_');
            db[tmp] = el;
        }
    }
}

#endif // BASEMODEL_H
