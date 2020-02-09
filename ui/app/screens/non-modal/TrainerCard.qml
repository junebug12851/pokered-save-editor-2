import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: top

  Rectangle {
    id: cardFront
    anchors.centerIn: parent
    border.color: brg.settings.textColorMid
    color: "transparent"

    width: 500
    height: width / 2

    NameDisplay {
      id: playerNameEdit

      anchors.left: parent.left
      anchors.top: parent.top
      anchors.leftMargin: 15
      anchors.topMargin: 33

      isPersonName: true
      isPlayerName: true

      str: brg.file.data.dataExpanded.player.basics.playerName
      onStrChanged: brg.file.data.dataExpanded.player.basics.playerName = str;

      Connections {
        target: brg.file.data.dataExpanded.player.basics
        onPlayerNameChanged: playerNameEdit.str = brg.file.data.dataExpanded.player.basics.playerName;
      }
    }

    DefTextEdit {
      id: playerIdEdit

      anchors.top: playerNameEdit.top
      anchors.topMargin: -5
      anchors.left: playerNameEdit.right
      anchors.leftMargin: 150
      labelEl.text: "ID"

      maximumLength: 4
      placeholderText: "0000"
      width: starterEdit.width

      text: brg.file.data.dataExpanded.player.basics.playerID.toString(16).toUpperCase()
      onTextChanged: {
        if(text === "")
          return;

        var idDec = parseInt(text, 16);
        if(idDec === NaN)
          return;

        if(idDec < 0 || idDec > 0xFFFF)
          return;

        brg.file.data.dataExpanded.player.basics.playerID = idDec;
      }

      Connections {
        target: brg.file.data.dataExpanded.player.basics
        onPlayerIDChanged: playerIdEdit.text = brg.file.data.dataExpanded.player.basics.playerID.toString(16).toUpperCase();
      }

      IconButtonSquare {
        id: idMenuBtn

        anchors.top: parent.top
        anchors.topMargin: -12
        anchors.left: parent.right

        icon.width: 7
        icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"

        onClicked: idMenu.open();

        Menu {
          id: idMenu

          MenuItem {
            text: "Randomize";
            onTriggered: brg.file.data.dataExpanded.player.basics.randomizeID();
          }

          MenuItem {
            text: "Close"
            onTriggered: idMenu.close();
          }
        }
      }
    }

    Spacer {
      id: spacer

      anchors.top: playerNameEdit.bottom
      anchors.topMargin: 25
      anchors.left: parent.left

      width: parent.width
      height: 1
      border.color: brg.settings.dividerColor
    }

    DefTextEdit {
      id: moneyEdit

      anchors.top: spacer.top
      anchors.topMargin: 25

      anchors.left: playerIdEdit.left
      labelEl.text: "Money"

      maximumLength: 6
      placeholderText: "0"
      width: starterEdit.width

      text: brg.file.data.dataExpanded.player.basics.money
      onTextChanged: {
        if(text === "")
          return;

        var txtDec = parseInt(text, 10);
        if(txtDec === NaN)
          return;

        if(txtDec < 0 || txtDec > 999999)
          return;

        brg.file.data.dataExpanded.player.basics.money = txtDec;
      }

      Connections {
        target: brg.file.data.dataExpanded.player.basics
        onMoneyChanged: moneyEdit.text = brg.file.data.dataExpanded.player.basics.money
      }

      IconButtonSquare {
        id: moneyMenuBtn

        anchors.top: parent.top
        anchors.topMargin: -12
        anchors.left: parent.right

        icon.width: 7
        icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"

        onClicked: moneyMenu.open();

        Menu {
          id: moneyMenu

          MenuItem {
            text: "Randomize";
            onTriggered: brg.file.data.dataExpanded.player.basics.randomizeMoney();
          }

          MenuItem {
            text: "Close"
            onTriggered: moneyMenu.close();
          }
        }
      }
    }

    DefTextEdit {
      id: coinsEdit

      anchors.top: moneyEdit.top
      anchors.topMargin: 40

      anchors.left: moneyEdit.left
      labelEl.text: "Coins"

      maximumLength: 4
      placeholderText: "0"
      width: starterEdit.width

      text: brg.file.data.dataExpanded.player.basics.coins
      onTextChanged: {
        if(text === "")
          return;

        var txtDec = parseInt(text, 10);
        if(txtDec === NaN)
          return;

        if(txtDec < 0 || txtDec > 9999)
          return;

        brg.file.data.dataExpanded.player.basics.coins = txtDec;
      }

      Connections {
        target: brg.file.data.dataExpanded.player.basics
        onCoinsChanged: coinsEdit.text = brg.file.data.dataExpanded.player.basics.coins
      }

      IconButtonSquare {
        id: coinsMenuBtn

        anchors.top: parent.top
        anchors.topMargin: -12
        anchors.left: parent.right

        icon.width: 7
        icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"

        onClicked: coinsMenu.open();

        Menu {
          id: coinsMenu

          MenuItem {
            text: "Randomize";
            onTriggered: brg.file.data.dataExpanded.player.basics.randomizeCoins();
          }

          MenuItem {
            text: "Close"
            onTriggered: coinsMenu.close();
          }
        }
      }
    }

    ComboBox {
      id: starterEdit

      anchors.top: coinsEdit.top
      anchors.topMargin: 25

      anchors.left: coinsEdit.left

      textRole: "monName"
      valueRole: "monInd"

      font.capitalization: Font.Capitalize
      font.pixelSize: 14
      flat: true
      width: font.pixelSize * 10
      model: brg.starterModel

      onActivated: brg.file.data.dataExpanded.player.basics.playerStarter = currentValue;
      Component.onCompleted: currentIndex = brg.starterModel.valToIndex(brg.file.data.dataExpanded.player.basics.playerStarter);

      Connections {
        target: brg.file.data.dataExpanded.player.basics
        onPlayerStarterChanged: starterEdit.currentIndex = brg.starterModel.valToIndex(brg.file.data.dataExpanded.player.basics.playerStarter);
      }

      Label {
        anchors.right: parent.left
        anchors.rightMargin: 7
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        font.pixelSize: 14

        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter

        text: "Starter"
      }

      IconButtonSquare {
        id: starterMenuBtn

        anchors.top: parent.top
        anchors.left: parent.right
        anchors.bottom: parent.bottom

        icon.width: 7
        icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"

        onClicked: starterMenu.open();

        Menu {
          id: starterMenu

          MenuItem {
            text: "Randomize";
            onTriggered: brg.file.data.dataExpanded.player.basics.randomizeStarter();
          }

          MenuItem {
            text: "Close"
            onTriggered: starterMenu.close();
          }
        }
      }
    }

    Row {
      id: playtimeEdit

      anchors.bottom: cardFront.bottom
      anchors.bottomMargin: 2

      anchors.right: cardFront.right
      anchors.rightMargin: 82
      spacing: 5

      DefTextEdit {
        id: daysEdit

        opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
                 ? 0.50
                 : 1.00
        maximumLength: 2
        placeholderText: "0"
        width: 2 * font.pixelSize

        horizontalAlignment: Text.AlignRight

        //text: brg.file.data.dataExpanded.world.other.playtime.days
        onTextChanged: {
          if(text === "")
            return;

          var txtDec = parseInt(text, 10);
          if(txtDec === NaN)
            return;

          if(txtDec < 0 || txtDec > 10)
            return;

          brg.file.data.dataExpanded.world.other.playtime.days = txtDec;
        }

        Connections {
          target: brg.file.data.dataExpanded.world.other.playtime
          onHoursChanged: daysEdit.text = brg.file.data.dataExpanded.world.other.playtime.days
        }

        Component.onCompleted: daysEdit.text = brg.file.data.dataExpanded.world.other.playtime.days;
      }

      Text {
        text: ":"
        font.pixelSize: 14
      }

      DefTextEdit {
        id: hoursEdit

        opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
                 ? 0.50
                 : 1.00
        maximumLength: 2
        placeholderText: "0"
        width: 2 * font.pixelSize

        horizontalAlignment: Text.AlignRight

        onTextChanged: {
          if(text === "")
            return;

          var txtDec = parseInt(text, 10);
          if(txtDec === NaN)
            return;

          if(txtDec < 0 || txtDec > 15)
            return;

          brg.file.data.dataExpanded.world.other.playtime.hoursAdjusted = txtDec;
        }

        Connections {
          target: brg.file.data.dataExpanded.world.other.playtime
          onHoursChanged: hoursEdit.text = brg.file.data.dataExpanded.world.other.playtime.hoursAdjusted
        }

        Component.onCompleted: hoursEdit.text = brg.file.data.dataExpanded.world.other.playtime.hoursAdjusted;
      }

      Text {
        text: ":"
        font.pixelSize: 14
      }

      DefTextEdit {
        id: minutesEdit

        opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
                 ? 0.50
                 : 1.00
        maximumLength: 2
        placeholderText: "0"
        width: 2 * font.pixelSize

        horizontalAlignment: Text.AlignRight

        onTextChanged: {
          if(text === "")
            return;

          var txtDec = parseInt(text, 10);
          if(txtDec === NaN)
            return;

          if(txtDec < 0 || txtDec > 59)
            return;

          brg.file.data.dataExpanded.world.other.playtime.minutes = txtDec;
        }

        Connections {
          target: brg.file.data.dataExpanded.world.other.playtime
          onMinutesChanged: minutesEdit.text = brg.file.data.dataExpanded.world.other.playtime.minutes
        }

        Component.onCompleted: minutesEdit.text = brg.file.data.dataExpanded.world.other.playtime.minutes;
      }

      Text {
        text: ":"
        font.pixelSize: 14
      }

      DefTextEdit {
        id: secondsEdit

        opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
                 ? 0.50
                 : 1.00
        maximumLength: 2
        placeholderText: "0"
        width: 2 * font.pixelSize

        horizontalAlignment: Text.AlignRight

        onTextChanged: {
          if(text === "")
            return;

          var txtDec = parseInt(text, 10);
          if(txtDec === NaN)
            return;

          if(txtDec < 0 || txtDec > 59)
            return;

          brg.file.data.dataExpanded.world.other.playtime.seconds = txtDec;
        }

        Connections {
          target: brg.file.data.dataExpanded.world.other.playtime
          onSecondsChanged: secondsEdit.text = brg.file.data.dataExpanded.world.other.playtime.seconds
        }

        Component.onCompleted: secondsEdit.text = brg.file.data.dataExpanded.world.other.playtime.seconds;
      }

      Text {
        text: ":"
        font.pixelSize: 14
      }

      DefTextEdit {
        id: framesEdit

        opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
                 ? 0.50
                 : 1.00
        maximumLength: 2
        placeholderText: "0"
        width: 2 * font.pixelSize

        horizontalAlignment: Text.AlignRight

        onTextChanged: {
          if(text === "")
            return;

          var txtDec = parseInt(text, 10);
          if(txtDec === NaN)
            return;

          if(txtDec < 0 || txtDec > 59)
            return;

          brg.file.data.dataExpanded.world.other.playtime.frames = txtDec;
        }

        Connections {
          target: brg.file.data.dataExpanded.world.other.playtime
          onFramesChanged: framesEdit.text = brg.file.data.dataExpanded.world.other.playtime.frames
        }

        Component.onCompleted: framesEdit.text = brg.file.data.dataExpanded.world.other.playtime.frames;

        IconButtonSquare {
          id: playtimeMenuBtn

          anchors.topMargin: -12

          anchors.top: parent.top
          anchors.left: parent.right

          icon.width: 7
          icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"

          onClicked: playtimeMenu.open();

          Menu {
            id: playtimeMenu

            MenuItem {
              text: "Enabled";
              checkable: true
              checked: !brg.file.data.dataExpanded.world.other.playtime.clockMaxed
              onTriggered: brg.file.data.dataExpanded.world.other.playtime.clockMaxed = !brg.file.data.dataExpanded.world.other.playtime.clockMaxed;
            }

            MenuItem {
              text: "Paused";
              checkable: true
              checked: !brg.file.data.dataExpanded.area.general.countPlaytime
              onTriggered: brg.file.data.dataExpanded.area.general.countPlaytime = !brg.file.data.dataExpanded.area.general.countPlaytime;
            }

            MenuItem {
              text: "Randomize";
              onTriggered: brg.file.data.dataExpanded.world.other.randomizePlaytime();
            }

            MenuItem {
              text: "Clear";
              onTriggered: brg.file.data.dataExpanded.world.other.clearPlaytime();
            }

            MenuItem {
              text: "Close"
              onTriggered: playtimeMenu.close();
            }
          }
        }
      }
    }

    Image {
      source: "qrc:/assets/images/red-larger.png"

      anchors.left: parent.left
      anchors.leftMargin: 35

      anchors.top: spacer.bottom
      anchors.topMargin: 10

      anchors.bottom: parent.bottom
      anchors.bottomMargin: 10

      width: parent.width / 3

      fillMode: Image.PreserveAspectFit
    }
  }

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: {
      // Most of the data on the screen
      brg.file.data.dataExpanded.player.basics.randomize();

      // Playtime
      brg.file.data.dataExpanded.world.other.randomizePlaytime();
    }
  }
}
