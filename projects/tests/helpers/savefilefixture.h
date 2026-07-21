/*
  * Copyright 2026 Fairy Fox
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/
#pragma once

/**
 * @file savefilefixture.h
 * @brief Header-only helpers shared by the savefile tests: locate and read the
 *        real .sav fixtures, load them into a SaveFile, snapshot the raw 32 KB,
 *        and diff two snapshots byte-by-byte.
 *
 * Fixtures are the genuine saves in <repo>/assets, found via the PSE_ASSETS_DIR
 * compile definition (set in tests/CMakeLists.txt). Helpers read into memory only;
 * the on-disk originals are never modified.
 */

#include <QByteArray>
#include <QFile>
#include <QString>
#include <QVector>

#include <pse-common/types.h>
#include <pse-savefile/savefile.h>

namespace pse_test {

/// Size of a Gen 1 save (mirrors SAV_DATA_SIZE; redeclared so helpers are self-contained).
constexpr int kSaveSize = 0x8000;

/// Absolute path to a file in the assets dir, e.g. assetPath("BaseSAV.sav").
inline QString assetPath(const QString& name)
{
  return QString::fromUtf8(PSE_ASSETS_DIR) + QStringLiteral("/") + name;
}

/// Read the first kSaveSize bytes of an assets fixture.
/// @return exactly kSaveSize bytes on success, or a short/empty array on failure
///         (callers assert `.size() == kSaveSize`, which localises the failure).
inline QByteArray readSaveBytes(const QString& name)
{
  QFile f(assetPath(name));
  if(!f.open(QIODevice::ReadOnly))
    return QByteArray();

  QByteArray bytes = f.read(kSaveSize);
  f.close();
  return bytes;
}

/// Adopt @p bytes into @p sf (copies via SaveFile::setData, which then expands).
/// @pre bytes.size() == kSaveSize.
inline void loadInto(SaveFile& sf, const QByteArray& bytes)
{
  // setData copies kSaveSize bytes out of the buffer (it does not take ownership),
  // so a const_cast of the QByteArray's data is safe here.
  sf.setData(reinterpret_cast<var8*>(const_cast<char*>(bytes.constData())));
}

/// Copy the current raw 32 KB out of @p sf for comparison.
inline QByteArray snapshot(const SaveFile& sf)
{
  return QByteArray(reinterpret_cast<const char*>(sf.data), kSaveSize);
}

/// Offsets at which two equal-length snapshots differ (ascending).
inline QVector<int> diffOffsets(const QByteArray& a, const QByteArray& b)
{
  QVector<int> diffs;
  const int n = qMin(a.size(), b.size());
  for(int i = 0; i < n; ++i)
    if(a[i] != b[i])
      diffs.append(i);
  return diffs;
}

} // namespace pse_test
