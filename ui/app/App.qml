import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "./sections"
import "./contents"
import "./common/Pages.js" as Pages
import "./common/Style.js" as Style

Rectangle {
  id: root
  color: Style.primaryColorLight

  property bool infoBtnPressed: false
  property string title: ""

  Loader {
    id: modal
    source: "./contents/NewFile.qml"
    anchors.fill: parent
  }

  ColumnLayout {
    id: sectionLayout
    anchors.fill: parent
    spacing: 0

    Loader {
      id: navBar
      Layout.fillWidth: true
    }

    Loader {
      id: body
      Layout.fillHeight: true
      Layout.fillWidth: true
    }

    Loader {
      id: footer
      Layout.fillWidth: true
    }
  }

  // Given a name, changes the screen
  function changeScreen(name)
  {
    // Grab screen data
    var screen = Pages.screens[name];

    // Only load body if modal, destroying navBar and footer
    if(screen.modal) {
      navBar.source = "";
      footer.source = "";
      body.source = "";
      modal.source = screen.body;
      return;
    }

    // Otherwise eliminate the modal body
    modal.source = "";

    // Set Title
    title = screen.title

    // load the navbar and screen body
    navBar.source = "./sections/NavBar.qml";
    body.source = screen.body;

    // Load the footer. Footer is a 2-part loading process because of it's
    // visual design. We load the upper-half and then tell the upper-half
    // which lower half to laod.
    // We refer to the upper half as "Number of Buttons" because it's designed
    // for how many buttons the lower half actually has
    if(screen.footerBtns === 1)
      footer.setSource("./sections/Footer1.qml", {subFooterSrc: screen.footer});
  }
}
