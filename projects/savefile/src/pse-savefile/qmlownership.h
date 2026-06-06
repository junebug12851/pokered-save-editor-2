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
#include <QQmlEngine>

// Hand QML CppOwnership of a C++-owned QObject returned from a Q_INVOKABLE.
//
// Q_INVOKABLE (and slot) returns of a *parentless* QObject default to
// JavaScriptOwnership, so QML's garbage collector frees them once the QML-side
// reference is dropped (e.g. a details editor closes). That leaves a dangling
// pointer in the owning C++ container -> use-after-free crash on the next access.
// These savefile objects are owned by their container in C++ for its lifetime,
// so QML must never delete them. (Q_PROPERTY returns do NOT need this — QML
// already assumes CppOwnership for those.)
//
// Wrap any "…At()"-style Q_INVOKABLE return:  return qmlCppOwned(vec.at(ind));
// See notes/reference/qt6-patterns.md ("Q_PROPERTY returns are safe; Q_INVOKABLE
// returns are NOT").
template<typename T>
static inline T* qmlCppOwned(T* obj)
{
  if(obj != nullptr)
    QQmlEngine::setObjectOwnership(obj, QQmlEngine::CppOwnership);
  return obj;
}
