#ifndef BASEMODEL_H
#define BASEMODEL_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

/**
  * Base model to all model classes
  * Every model contains these values
 */

// Cannot use Q_PROPERTY
//     I spent over an hour having to make everything non-constant
//     because it kept complaining - having to make some things pointers because
//     it kept complaining - having to add operators in because it kept
//     complaining. In the end it wouldn't stop complaining so I literally
//     just flat gave up, reverted all my changes, and deemed it impossible to
//     make any of this accessible to QML. I am very upset at being forced to
//     make this design choice when I explicitly wanted these classes available
//     to QML and for wasting an hour of my time only to have to undo everything
//
//     I will just have to find other ways to make these accessible from QML
//     because my hand is being forced in this matter and
//     which is not the design descision I wanted.

class BaseModel : public QObject
{
    Q_OBJECT
public:
    explicit BaseModel(
            const QString name,
            const int index,
            QObject *parent = nullptr
        );

    // Name of data entry
    const QString name;

    // Internal game index of data entry
    const int index;
};

#endif // BASEMODEL_H
