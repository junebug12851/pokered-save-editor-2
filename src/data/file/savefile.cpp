#include "savefile.h"
#include "./expanded/savefileexpanded.h"
#include "./savefileiterator.h"
#include "./savefiletoolset.h"

SaveFile::SaveFile(QObject* parent)
  : QObject(parent)
{
  // One-Time Init Data in order of dependencies
  // Since the sav file is a primitive array we have to clear it first to zeroes
  data = new var8[SAV_DATA_SIZE];
  memset(data, 0, SAV_DATA_SIZE);

  // Create toolset 2nd and then dataExpanded
  // dataExpanded will perform an initial load from sav data which is why
  // we had to clear it first up there, dataExpanded also depends on toolset
  toolset = new SaveFileToolset(this);
  dataExpanded = new SaveFileExpanded(this);

  // Perform post-init stuff including notifying other code of data changes
  resetData();
}

SaveFile::~SaveFile()
{
  // Erase backwards from creation order
  delete dataExpanded;
  delete toolset;
  delete[] data;
}

SaveFileIterator* SaveFile::iterator()
{
  return new SaveFileIterator(this);
}

void SaveFile::resetData(bool silent)
{
  memset(data, 0, SAV_DATA_SIZE);
  dataChanged(data);

  if(!silent) {
    expandData();
    dataExpandedChanged(dataExpanded);
  }
}

void SaveFile::setData(var8* data, bool silent)
{
  memcpy(this->data, data, SAV_DATA_SIZE);
  dataChanged(data);

  if(!silent) {
    expandData();
    dataExpandedChanged(dataExpanded);
  }
}

int SaveFile::dataSize()
{
  return SAV_DATA_SIZE;
}

int SaveFile::dataAt(int ind)
{
  return data[ind];
}

void SaveFile::flattenData()
{
  dataExpanded->save(this);
}

void SaveFile::expandData()
{
  dataExpanded->load(this);
}

void SaveFile::eraseExpansion()
{
  dataExpanded->reset();
}

void SaveFile::randomizeExpansion()
{
  dataExpanded->randomize();
}
