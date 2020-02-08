import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

Flickable {
  id: topz

  contentWidth: criteria.width
  contentHeight: criteria.height

  flickableDirection: Flickable.VerticalFlick
  ScrollBar.vertical: ScrollBar {}

  function reSearch() {

    // Start over on the results
    brg.fontSearch.startOver();

    if(criteria.normalSearchState === Qt.PartiallyChecked)
      brg.fontSearch.notNormal();
    else if(criteria.normalSearchState === Qt.Checked)
      brg.fontSearch.andNormal();

    if(criteria.controlSearchState === Qt.PartiallyChecked)
      brg.fontSearch.notControl();
    else if(criteria.controlSearchState === Qt.Checked)
      brg.fontSearch.andControl();

    if(criteria.pictureSearchState === Qt.PartiallyChecked)
      brg.fontSearch.notPicture();
    else if(criteria.pictureSearchState === Qt.Checked)
      brg.fontSearch.andPicture();

    if(criteria.singleSearchStateState === Qt.PartiallyChecked)
      brg.fontSearch.notSingleChar();
    else if(criteria.singleSearchState === Qt.Checked)
      brg.fontSearch.andSingleChar();

    if(criteria.multiSearchState === Qt.PartiallyChecked)
      brg.fontSearch.notMultiChar();
    else if(criteria.multiSearchState === Qt.Checked)
      brg.fontSearch.andMultiChar();

    if(criteria.varSearchState === Qt.PartiallyChecked)
      brg.fontSearch.notVariable();
    else if(criteria.varSearchState === Qt.Checked)
      brg.fontSearch.andVariable();
  }

  SearchCriteria {
    id: criteria
    onReSearch: topz.reSearch();
  }
}
