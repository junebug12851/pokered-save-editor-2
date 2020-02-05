import QtQuick 2.14
import QtQuick.Controls 2.14

import "../../common/Style.js" as Style

RoundButton {
  display: AbstractButton.IconOnly
  icon.width: 15
  icon.height: 15
  icon.color: Style.textColorPrimary
  flat: true
}
