#ifndef RAWSAVEDATA_H
#define RAWSAVEDATA_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include "../../common/types.h"
class SaveFileExpanded;

constexpr var16 SAV_DATA_SIZE{0x8000};

class SaveFile : public QObject
{
  Q_OBJECT
  Q_PROPERTY(var8* data MEMBER data NOTIFY wholeDataChanged)

public:
  // Create a blank save file and a blank expanded save file
  explicit SaveFile(QObject *parent = nullptr);

  // Load a sav file and expand it's data
  explicit SaveFile(var8 data[SAV_DATA_SIZE], QObject *parent = nullptr);

  // Empty this save file to zero's
  // Re-Expand the empty save file, overwriting prior expansion, unless marked
  // silent
  void resetData(bool silent = false);

  // Change-out the save file
  // Re-Expand the new save file, overwriting prior expansion, unless marked
  // silent
  void setData(var8* data, bool silent = false);

  // Flatten expansion back to the save file, overwriting it's current contents
  // with only data that's strictly nesesary. A critical rule.
  void flattenData();

  // Replace expansion with new expansion of current sav file
  void expandData();

  // Actual SAV Data, a raw internal binary copy of the file
  var8* data = new var8[SAV_DATA_SIZE];

  // Expanded SAV data to be readable and more usable
  SaveFileExpanded* dataExpanded = nullptr;

signals:
  // SAV file has changed and it's expansion replaced with new SAV data
  void wholeDataChanged(var8* data);

  // SAV file has changed but the old expansion has not been replaced with
  // exxpansion of new data
  void silentWholeDataChanged(var8* data);
};

#endif // RAWSAVEDATA_H
