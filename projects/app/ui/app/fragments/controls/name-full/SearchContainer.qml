// SearchContainer.qml -- the scrollable filter sidebar of the character picker.
//
// A Flickable wrapping SearchCriteria; reSearch() drives brg.fontSearch -- "All"
// calls startOver() (whole store, including uncategorized entries) while any other
// single category calls keepAnyOf() with the selected flag. Leave the inline notes
// on the single-select behavior.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Flickable {
  id: topz

  clip: true
  contentWidth: width
  contentHeight: criteria.height

  flickableDirection: Flickable.VerticalFlick
  ScrollBar.vertical: ScrollBar { id: vbar; width: 12 }

  // Filters are single-select radios now, so exactly one of these is true.
  // "All" shows the whole store; otherwise keepAnyOf narrows to the one selected
  // category. (keepAnyOf still does the general union, so multi-select could
  // return; "All" uses startOver so entries with no category flag still show.)
  function reSearch() {
    if(criteria.allChecked) {
      brg.fontSearch.startOver();
      return;
    }

    brg.fontSearch.keepAnyOf(
      criteria.normalChecked,
      criteria.controlChecked,
      criteria.pictureChecked,
      criteria.singleChecked,
      criteria.multiChecked,
      criteria.varChecked);
  }

  SearchCriteria {
    id: criteria
    // Reserve room for the scrollbar so the ⓘ help dots (right-aligned in each
    // row) sit clear of it instead of underneath.
    width: topz.width - 16
    onReSearch: topz.reSearch();
  }
}
