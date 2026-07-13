/*
  * Copyright 2026 Twilight
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

/// One read-only label/value line in the Details panel's "nothing selected" view.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
  id: row

  required property string label
  required property string value

  Layout.fillWidth: true
  spacing: 6

  Label {
    text: row.label
    font.pixelSize: 11
    opacity: 0.6
    Layout.preferredWidth: 78
  }

  Label {
    text: row.value
    font.pixelSize: 11
    Layout.fillWidth: true
    elide: Text.ElideRight
  }
}
