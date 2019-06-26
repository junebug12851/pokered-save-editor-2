#ifndef BASEMODEL_H
#define BASEMODEL_H

#include <QtCore/QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QFile>
#include <QJsonDocument>

#include <optional>
#include <vector>

using std::optional;
using std::vector;

#include "../includes/vars.h"

/**
  * Base model to all models
  * Most Every model contains these values
 */
class BaseModel
{
public:
    BaseModel();
    BaseModel(const QJsonObject& obj);

    void init(const QJsonObject& obj);

    // Read-Only Public Getters
    const optional<QString>& name();
    const optional<vars>& index();

protected:
    template<class T>
    static void initStore(const QString& filename, vector<T*>& store);

private:
    /**
     * Stage 1 Variables: Extracted from JSON Data
     */

    // Name of data entry
    optional<QString> _name;

    // Internal game index of data entry
    optional<vars> _index;
};

Q_DECLARE_METATYPE(BaseModel)

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

#endif // BASEMODEL_H
