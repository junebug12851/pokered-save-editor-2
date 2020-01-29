import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../common/Style.js" as Style

/*
 * This is a fairly complicated tile that needs to be placed into it's own
 * file due to it's size and complexity.
 *
 * Shows the tile and when clicked replaces the tile with a
 * menu showing the recent files and the option to clear them. It stays that
 * way until the menu is interacted with or closed. It keeps the same styling
 * and formatting between the 2
*/

TileButton {
  id: recentFilesBtn

  onClicked: {
    // Clear menu entries
    recentFilesMenu.clearEntries()

    // Add a top menu entru to clear all, disable it if there are no files
    // at all
    var enabled = file.recentFilesCount() > 0
    recentFilesMenu.addEntry({text: 'Clear Recent Files', enabled});

    // Loop through all recent files
    for(var i = 0; i < file.recentFilesCount(); i++) {
      // We need to split the path up into an array by it's seperator, here
      var recentFileSplit;

      // This is the seperator we'll use later for display, default to unix/linux
      var seperator = "/"

      // Perform the split by checking for unix/linux or windows and splitting
      // accordingly
      if(file.getRecentFile(i).includes("\\")) {
        recentFileSplit = file.getRecentFile(i).split("\\")
        seperator = "\\"
      }
      else
        recentFileSplit = file.getRecentFile(i).split("/")

      // For display to inform the user the position of the recent file
      var recentFileDisplay = "[" + i + "] "

      // Here is where we perform the split
      //     C:\Users\Somebody\Documents\Folder1\Folder2\Folder3\Red.sav
      // That's far too long to display or try to display
      // Instead let's turn that into:
      //     C:\...Folder3\Red.sav <- Much better
      // We check path length and stitch together accordingly
      if(recentFileSplit.length >= 3)
        recentFileDisplay += recentFileSplit[0] + seperator + "..." + recentFileSplit[recentFileSplit.length - 2] + seperator + recentFileSplit[recentFileSplit.length - 1]
      else if(recentFileSplit.length === 2)
          recentFileDisplay += recentFileSplit[recentFileSplit.length - 2] + seperator + recentFileSplit[recentFileSplit.length - 1]
      else
          recentFileDisplay += recentFileSplit[recentFileSplit.length - 1]

      // Add display string to menu list
      recentFilesMenu.addEntry({text: recentFileDisplay, ind: i});
    }

    recentFilesBtn.opacity = 0;

    // Open Menu
    recentFilesMenu.open();
  }

  Menu {
    id: recentFilesMenu
    x: 0
    y: 0
    width: parent.width
    height: parent.height

    background: Rectangle {
      color: "transparent"
      border.color: Style.primaryColorDark
      border.width: 1
    }

    function clearEntries() {
      while(recentFilesMenu.itemAt(0) !== null)
          recentFilesMenu.removeItem(0);
    }

    function addEntry(props) {
      recentFilesMenu.addItem(
            recentFilesMenuItem.createObject(
              recentFilesMenu, props));
    }

    Component {
      id: recentFilesMenuItem

      MenuItem {
        background: Rectangle {
          color: manualMouse.curColor
        }

        contentItem: Text {
          color: (parent.enabled)
                 ? Qt.darker(Style.textColorPrimary, 2)
                 : Style.textColorPrimary

          opacity: recentFilesBtn.titleOpacity
          topPadding: 7
          bottomPadding: 5
          anchors.centerIn: parent
          font.pixelSize: recentFilesBtn.hotKeySize * 0.85

          text: parent.text
          horizontalAlignment: Text.AlignLeft
          verticalAlignment: Text.AlignVCenter
        }

        ManualButton {
          id: manualMouse
          refColor: "#7fefefef"
          refDefColor: "transparent"
          anchors.fill: parent
          onClicked: {
            var letter = text[1]; // <- Get embedded index number

            // If it's C[l]ear Recent Files, then clear files, otherwise
            // Open recent file based on embedded index number
            if(letter === "l")
              file.clearRecentFiles()
            else {
              file.openFileRecent(text[1])
              root.changeScreen("home")
            }

            recentFilesBtn.opacity = 1;
            recentFilesMenu.close();
          }
        }
      }
    }
  }
}
