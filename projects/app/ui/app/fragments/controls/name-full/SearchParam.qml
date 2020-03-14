import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

CheckBox {
  id: top

  signal reSearch();

  tristate: true
  onCheckStateChanged: top.reSearch();

  nextCheckState: function() {
    if (checkState === Qt.Unchecked)
      return Qt.Checked
    else if (checkState === Qt.Checked)
      return Qt.PartiallyChecked
    else
      return Qt.Unchecked
  }
}
