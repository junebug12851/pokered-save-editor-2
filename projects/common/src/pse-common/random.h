/*
  * Copyright 2020 Fairy Fox
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
#include <QRandomGenerator>

#include "./common_autoport.h"

class QRandomGenerator;
class QQmlEngine;
class QQmlContext;

/**
 * @brief Project-wide source of randomness, usable from both C++ and QML.
 *
 * A thin, intention-revealing wrapper around Qt's global QRandomGenerator. It
 * exists so the rest of the app (especially the map- and full-randomization
 * features) can express randomness in domain terms - "give me a range", "flip a
 * coin", "did this 30% chance succeed?" - instead of repeating bounds arithmetic
 * everywhere.
 *
 * @par Singleton
 * Constructed once via inst(); the constructor is private. It is a QObject so it
 * can be handed to QML, where it is registered as an @e uncreatable type and
 * reached through Utility (`pseCommon.random`) rather than instantiated.
 *
 * @par int vs float variants
 * Each chance/range idea comes in an integer (0-100) and a floating (0.00-1.00)
 * flavour. The integer forms read naturally but can bias on small ranges; the
 * float forms are offered for when they distribute better. Pick whichever yields
 * nicer results for a given use.
 *
 * @note Does not own the underlying generator - it borrows QRandomGenerator::global().
 * @see Utility, which exposes this object to QML and protects it from GC.
 */
class COMMON_AUTOPORT Random : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool flipCoin READ flipCoin STORED false)   ///< Convenience 50% coin flip (integer path), readable from QML.
  Q_PROPERTY(bool flipCoinF READ flipCoinF STORED false) ///< Convenience 50% coin flip (float path), readable from QML.

public:
  /// Returns the process-wide Random singleton, constructing it on first call.
  static Random* inst();

  // Often the int versions can yield bad randomization, the float versions are
  // provided in case they yield better results.

  /**
   * @brief Random float in the half-open interval [0.00, @p end).
   * @param end Exclusive upper bound.
   * @return A value `>= 0.0` and `< end`.
   */
  Q_INVOKABLE float range(const float end) const;

  /**
   * @brief Random integer in the closed interval [@p start, @p end].
   * @param start Inclusive lower bound.
   * @param end   Inclusive upper bound.
   * @return A value in `[start, end]`. If `start >= end`, returns @p start unchanged
   *         (degenerate ranges are treated as a fixed value rather than an error).
   */
  Q_INVOKABLE int rangeInclusive(const int start, const int end) const;

  /**
   * @brief Random integer in the half-open interval [@p start, @p end).
   * @param start Inclusive lower bound.
   * @param end   Exclusive upper bound.
   * @return A value in `[start, end)`. If `start >= end`, returns @p start unchanged.
   */
  Q_INVOKABLE int rangeExclusive(const int start, const int end) const;

  /**
   * @brief Did a @p percent chance @e fail? (integer scale, 0-100).
   * @param percent Success likelihood as a whole percentage.
   * @return @c true when the roll lands outside the success band.
   */
  Q_INVOKABLE bool chanceFailure(const int percent) const;
  /**
   * @brief Did a @p percent chance @e fail? (float scale, 0.00-1.00).
   * @param percent Success likelihood as a fraction.
   * @return @c true when the roll lands outside the success band.
   */
  Q_INVOKABLE bool chanceFailure(const float percent) const;

  /**
   * @brief Did a @p percent chance @e succeed? (integer scale, 0-100).
   * @param percent Success likelihood as a whole percentage.
   * @return @c true when the roll lands inside the success band.
   */
  Q_INVOKABLE bool chanceSuccess(const int percent) const;
  /**
   * @brief Did a @p percent chance @e succeed? (float scale, 0.00-1.00).
   * @param percent Success likelihood as a fraction.
   * @return @c true when the roll lands inside the success band.
   */
  Q_INVOKABLE bool chanceSuccess(const float percent) const;

  /// 50/50 coin flip via the integer path (`chanceSuccess(50)`).
  bool flipCoin() const;
  /// 50/50 coin flip via the float path (`chanceSuccess(0.5f)`).
  bool flipCoinF() const;

public slots:
  /**
   * @brief Pin this object to C++ ownership so QML's GC never deletes it.
   * @param engine The QML engine that would otherwise assume ownership.
   * @see Utility::qmlProtectUtil
   */
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  /// Registers Random with the QML type system as an uncreatable type (idempotent).
  void qmlRegister() const;

private:
  /// Private - use inst(). Registers the QML type on first construction.
  Random();

  QRandomGenerator* rnd = QRandomGenerator::global(); ///< Borrowed global RNG; not owned by this class.
};
