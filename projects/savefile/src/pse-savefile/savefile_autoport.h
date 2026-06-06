/*
  * Copyright 2020 Twilight
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
#include <QtCore/qglobal.h>
#include <QMetaType>

#if defined(SAVEFILE_LIBRARY)
#  define SAVEFILE_AUTOPORT Q_DECL_EXPORT
#else
#  define SAVEFILE_AUTOPORT Q_DECL_IMPORT
#endif


// -- Opaque pointer declarations ----------------------------------------------
// CAUTION: Q_DECLARE_OPAQUE_POINTER(T*) forces
// QtPrivate::IsPointerToTypeDerivedFromQObject<T*> = false. For a *real* QObject
// type that makes Qt store the pointer as a plain opaque value, so any QML read
// of `obj.thatProperty.subProperty` returns `undefined` even though the C++
// object is valid. This is why the whole brg.file.data.dataExpanded.* chain used
// to read as undefined.
//
// The fix for QObject types is to fully #include their headers wherever their
// pointers appear in a Q_PROPERTY / signal / slot / Q_INVOKABLE (see the
// expanded/*.h headers). With the full definition visible, Qt correctly detects
// them as QObject pointers and QML can traverse the chain -- no opaque decl
// needed. See notes/reference/qt6-patterns.md.
//
// The types below are kept opaque ON PURPOSE: nothing in QML traverses them
// (verified by grepping the .qml for dataExpanded.* chains). Keeping them opaque
// + forward-declared means the widely-included headers (savefileexpanded.h /
// area.h / world.h) do NOT have to pull these sub-trees into every translation
// unit -- that fan-out was making the build very slow. If you later traverse one
// of these in QML, remove it here and #include its full header at the
// declaration site instead. See notes/reference/qt6-patterns.md.

class Daycare;
class HallOfFame;
class Rival;
class WorldCompleted;
class WorldEvents;
class WorldGeneral;
class WorldHidden;
class WorldMissables;
class WorldScripts;
class WorldTowns;
class WorldTrades;
class WorldLocal;

Q_DECLARE_OPAQUE_POINTER(Daycare*)
Q_DECLARE_OPAQUE_POINTER(HallOfFame*)
Q_DECLARE_OPAQUE_POINTER(Rival*)
Q_DECLARE_OPAQUE_POINTER(WorldCompleted*)
Q_DECLARE_OPAQUE_POINTER(WorldEvents*)
Q_DECLARE_OPAQUE_POINTER(WorldGeneral*)
Q_DECLARE_OPAQUE_POINTER(WorldHidden*)
Q_DECLARE_OPAQUE_POINTER(WorldMissables*)
Q_DECLARE_OPAQUE_POINTER(WorldScripts*)
Q_DECLARE_OPAQUE_POINTER(WorldTowns*)
Q_DECLARE_OPAQUE_POINTER(WorldTrades*)
Q_DECLARE_OPAQUE_POINTER(WorldLocal*)
