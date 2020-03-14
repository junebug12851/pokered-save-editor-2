#ifndef RAWSAVEDATA_H
#define RAWSAVEDATA_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include <pse-common/types.h>
class SaveFileExpanded;
class SaveFileIterator;
class SaveFileToolset;

constexpr var16 SAV_DATA_SIZE{0x8000};

class SaveFile : public QObject
{
  Q_OBJECT

  Q_PROPERTY(SaveFileExpanded* dataExpanded MEMBER dataExpanded NOTIFY dataExpandedChanged)

public:
  // Create a blank save file and a blank expanded save file
  SaveFile(QObject *parent = nullptr);
  virtual ~SaveFile();

  // Returns a unique iterator that's setup to iterate over the raw sav file
  // data. Heavily used by the file expansion to iterate and expand or flatten
  // data.
  // The iterator has to be deleted by the receiver or it will cause a memory
  // leak
  SaveFileIterator* iterator();

  // Change-out the save file
  // Re-Expand the new save file, overwriting prior expansion, unless marked
  // silent
  void setData(var8* data, bool silent = false);

signals:
  // SAV file has changed and it's expansion replaced with new SAV data
  void dataChanged(var8* data);

  // SAV file has changed but the old expansion has not been replaced with
  // exxpansion of new data
  void dataExpandedChanged(SaveFileExpanded* expanded);

public slots:
  // Empty this save file to zero's
  // Re-Expand the empty save file, overwriting prior expansion, unless marked
  // silent
  void resetData(bool silent = false);

  // Flatten expansion back to the save file, overwriting it's current contents
  // with only data that's strictly nesesary. A critical rule.
  void flattenData();

  // Replace expansion with new expansion of current sav file
  void expandData();

  // Erase expansion data, this makes expansion data act like a new file
  // but save file contents are preserved
  void eraseExpansion();

  // Fully randomizes the expansion data, doesn't change save file data
  // This tries to give fun and playable randomization. Due to the complexity of
  // Gen 1 Games there are limits on randomization. The idea is to randomize
  // everything we can and still allow you to jump in and play right away with
  // Your Psychic Traded Pikachu named Bob that uses Ice Beam and Splash
  void randomizeExpansion();

public:
  // Actual SAV Data, a raw internal binary copy of the file
  var8* data = nullptr;

  // Expanded SAV data to be readable and more usable
  SaveFileExpanded* dataExpanded = nullptr;

  // Tools to operate directly on the raw sav file data
  SaveFileToolset* toolset = nullptr;
};

#endif // RAWSAVEDATA_H
