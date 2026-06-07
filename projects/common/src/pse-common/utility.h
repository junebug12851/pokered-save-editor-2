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
#include <QObject>
#include <QString>

class QQmlContext;
class QQmlEngine;

#include "./common_autoport.h"

class Random;

/**
 * @brief Grab-bag of shared helpers, and the QML entry point for the common layer.
 *
 * Utility is a QObject singleton exposed to QML under the context name
 * `pseCommon`. Through it, QML reaches the shared Random instance
 * (`pseCommon.random`) and a couple of string helpers used by the name editors.
 *
 * It also hosts qmlProtectUtil() - the one-liner every database and save object
 * in the project calls to keep its C++ QObjects from being garbage-collected by
 * the QML engine. Centralising that here means there is exactly one place that
 * knows the GC-ownership incantation.
 *
 * @par Singleton
 * Private constructor; obtain via inst(). Registered with QML as an uncreatable
 * type. Constructing it also brings up Random.
 *
 * @see Random, qmlProtectUtil()
 */
class COMMON_AUTOPORT Utility : public QObject
{
  Q_OBJECT
  Q_PROPERTY(Random* random READ random CONSTANT) ///< The shared Random instance, reachable from QML as `pseCommon.random`.

public:
  /// Returns the process-wide Utility singleton, constructing it on first call.
  static Utility* inst();

  /// Accessor for the shared Random singleton (also backs the @c random property).
  Random* random();

  /**
   * @brief Encode a string into space-separated hex of each character's code point.
   *
   * Used to pass arbitrary name text safely through a URL/route fragment.
   * @param beforeStr The text to encode.
   * @return e.g. `"41 42"` for `"AB"`.
   * @see decodeAfterUrl(), the exact inverse.
   */
  Q_INVOKABLE const QString encodeBeforeUrl(const QString beforeStr) const;
  /**
   * @brief Decode the space-separated hex produced by encodeBeforeUrl() back to text.
   * @param beforeStr The encoded hex string (spaces are stripped before decoding).
   * @return The original text.
   */
  Q_INVOKABLE const QString decodeAfterUrl(QString beforeStr) const;

  // Generic utility for any of the databases to use
  /**
   * @brief Pin @p obj to C++ ownership so the QML engine never garbage-collects it.
   *
   * The shared building block behind every `qmlProtect()` in the project. Static
   * so any class can call it without a Utility instance.
   * @param obj    The C++ QObject to protect (kept alive on the C++ side).
   * @param engine The QML engine that would otherwise claim ownership.
   * @note Internally casts away constness because Qt's ownership setter is non-const.
   */
  static void qmlProtectUtil(const QObject* const obj, const QQmlEngine* const engine);

public slots:
  /**
   * @brief Protect this Utility (and the Random it owns) from QML GC.
   * @param engine The active QML engine.
   */
  void qmlProtect(const QQmlEngine* const engine) const;
  /**
   * @brief Install this object into a QML context as the `pseCommon` property.
   * @param context The QML context to inject into.
   */
  void qmlHook(QQmlContext* const context) const;

private slots:
  /// Registers Utility with the QML type system as an uncreatable type (idempotent).
  void qmlRegister() const;

private:
  /// Private - use inst(). Registers the QML type and brings up Random.
  Utility();
};
