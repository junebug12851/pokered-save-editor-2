import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

// SearchParam.qml --
// Single-select filter (radio). Mutual exclusivity is enforced by the shared
// ButtonGroup set up in SearchCriteria, so one category is always active and no
// Clear button is needed. Compacted: Material controls floor at ~40px (touch
// target) which spread the filters far apart — trim padding and let it shrink.
RadioButton {
  id: top

  signal reSearch();

  topPadding: 2
  bottomPadding: 2
  Layout.minimumHeight: 0

  onCheckedChanged: top.reSearch();
}
