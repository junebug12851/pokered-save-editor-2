import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "./sections"
import "./common/Pages.js" as Pages
import "./common/Style.js" as Style

Rectangle {
  id: root
  color: Style.primaryColorLight

  // Toggle global tooltips and help
  property bool infoBtnPressed: false

  // When app is booting, this disables animations
  property bool startingUp: true

  // Title of current screen
  property string title: ""

  // Name of current screen
  property string curScrn: ""

  // Tileset to use for font preview
  property string previewTileset: "Overworld"
  property bool previewOutdoor: true

  // Main Sections of the app showing normal screens
  ColumnLayout {
    id: sectionLayout
    anchors.fill: parent
    spacing: 0

    // Navbar Section
    Loader {
      id: navBar
      Layout.fillWidth: true
    }

    // Body Section showing non-modal screens
    Loader {
      id: body
      Layout.fillHeight: true
      Layout.fillWidth: true
    }

    // Footer Section
    Loader {
      id: footer
      Layout.fillWidth: true
    }
  }

  // A Pseudo Modal screen, it's a modal screen that's not managed by the modal
  // screen system. Real modals can be placed on top as well. Pseudo Modal's
  // also typically animate from the bottom up rather then from left to right
  // It's essentially a full-screen normal non-modal screen
  Loader {
    id: pseudoModal
    anchors.fill: parent
  }

  // A real modal that's entirely managed by the screen change system, animates
  // left to right and is registered into the list of screens so it's opened
  // by name not by url
  Loader {
    id: modal
    anchors.fill: parent
  }

  // On start, load new file screen then setup screen beneath it
  // Afterwards mark everything has booted so that animations can play
  Component.onCompleted: {
    changeScreen("newFile");
    setupScreen(Pages.screens["home"]);
    startingUp = false;
  }

  // the modal is responsible for closing animation
  function closeModal() {
    modal.source = "";
  }

  // Open and Close Pseudo Modal Screens
  // Since Pseudo Modals are not part of the screen system they must manually
  // be opened and closed from code
  function openPseudoModal(path) {
    pseudoModal.source = path;
  }

  function closePseudoModal() {
    pseudoModal.source = "";
  }

  // Given a name, changes the screen
  function changeScreen(name)
  {
    // Grab screen data
    curScrn = name;
    var screen = Pages.screens[name];

    // If modal, load that instead, it overlays the main page
    if(screen.modal) {
      modal.source = screen.body;
      return;
    }

    // Otherwise keep the modal open while we change the contents below it
    // When all is done, destroy modal
    setupScreen(screen)

    // destroy modal
    modal.source = "";
  }

  // Loads a screen without touching modal, mainly useful for working the
  // screens behind the modal while it's still open
  function setupScreen(screen) {

    // Set Title
    title = screen.title

    // load the navbar and screen body
    navBar.source = "./sections/major/NavBar.qml";
    body.source = screen.body;

    // Load the footer. Footer is a 2-part loading process because of it's
    // visual design. We load the upper-half and then tell the upper-half
    // which lower half to laod.
    // We refer to the upper half as "Number of Buttons" because it's designed
    // for how many buttons the lower half actually has
    if(screen.footerBtns === 1)
      footer.setSource("./sections/major/Footer1.qml", {subFooterSrc: screen.footer});
  }
}
