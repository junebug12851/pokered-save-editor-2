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

/**
 * A Game Boy image, drawn at ANY zoom -- including the fractional ones -- without ruining it.
 *
 * Drop-in for `Image { smooth: false }`, which is what the map used to use. That was fine while the
 * zoom was whole numbers and wrong the moment it wasn't: at 2.37x, nearest-neighbour gives some
 * source pixels two screen pixels and others three, so the art ripples and crawls as you zoom.
 * Bilinear instead just makes the whole map soft, which is worse.
 *
 * This runs `shaders/pixelart.frag` -- anti-aliased point sampling. Flat inside a pixel, one screen
 * pixel of blend across the seam. Crisp like nearest, smooth like bilinear, and **identical to
 * nearest at whole-number zooms**, so nothing is lost where it used to be exact.
 *
 * @see notes/reference/ui-patterns.md -> "Smooth zoom on pixel art"
 */
import QtQuick

Item {
  id: root

  /// What to draw. Same as Image.source.
  property alias source: src.source

  /// Same as Image.status -- lets a caller wait for it.
  readonly property alias status: src.status

  /**
   * ⚠️ **The shader needs a GPU, and not every run has one.**
   *
   * Every headless run in this project -- the screenshooter, the GUI suites, `tst_visual_regression`,
   * `tst_qml_screens` -- uses the `offscreen` platform with Qt Quick's **software** renderer, and the
   * software renderer **cannot run a ShaderEffect at all**: it silently draws nothing, which is how
   * the whole map came out BLACK in the first headless screenshot of it.
   *
   * So: on a real GPU we run the shader; on the software backend we fall back to plain
   * nearest-neighbour, which is exactly what the map used to do and is perfectly correct at the whole
   * zooms a test uses. Nothing is lost and nothing is faked -- the fallback is not an approximation
   * of the shader, it is the honest behaviour of the only sampler that backend has.
   */
  readonly property bool canShade: GraphicsInfo.api !== GraphicsInfo.Software
                                && GraphicsInfo.api !== GraphicsInfo.Unknown

  Image {
    id: src

    anchors.fill: parent

    // On the GPU path this is only a TEXTURE SOURCE -- the ShaderEffect above does the drawing, and
    // this must not paint over it. On the software path it IS the drawing.
    visible: !root.canShade

    smooth: false           // pixel art -- never interpolate it
    mipmap: false
    fillMode: Image.Stretch
    cache: true
  }

  ShaderEffect {
    anchors.fill: parent

    // Nothing to sample yet -- draw nothing rather than a black rectangle.
    visible: root.canShade && src.status === Image.Ready && src.sourceSize.width > 0

    property variant source: src
    property size sourceSize: Qt.size(Math.max(1, src.sourceSize.width),
                                      Math.max(1, src.sourceSize.height))

    fragmentShader: "qrc:/shaders/pixelart.frag.qsb"
  }
}
