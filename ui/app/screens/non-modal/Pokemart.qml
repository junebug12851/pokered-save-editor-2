import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"

Page {

  Rectangle {
    anchors.fill: parent

    Rectangle {
      id: header
      anchors.left: parent.left
      anchors.top: parent.top
      anchors.right: parent.right
      height: 45

      Material.foreground: brg.settings.textColorLight
      Material.background: brg.settings.accentColor
      color: brg.settings.accentColor

      Text {
        // Buy with Money
        visible: brg.marketModel.whichMode == 0
        anchors.centerIn: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: "Shop at the Pokemart"
        font.pixelSize: 18
        color: brg.settings.textColorLight
      }

      Text {
        // Buy with Coins
        visible: brg.marketModel.whichMode == 1
        anchors.centerIn: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: "Shop at the Game Corner"
        font.pixelSize: 18
        color: brg.settings.textColorLight
      }

      Text {
        // Sell with Money
        visible: brg.marketModel.whichMode == 2
        anchors.centerIn: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: "Sell to the Pokemart"
        font.pixelSize: 18
        color: brg.settings.textColorLight
      }

      Text {
        // Sell with Coins
        visible: brg.marketModel.whichMode == 3
        anchors.centerIn: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: "Sell to the Game Corner"
        font.pixelSize: 18
        color: brg.settings.textColorLight
      }
    }

    ListView {
      id: marketView

      model: brg.marketModel

      anchors.top: header.bottom
      anchors.left: parent.left
      anchors.leftMargin: 15
      anchors.right: parent.right
      anchors.rightMargin: 15
      anchors.bottom: footer.top

      clip: true
      ScrollBar.vertical: ScrollBar {}

      delegate: ColumnLayout {
        id: delegate
        property bool isLast: index+1 < marketView.count ? false : true
        width: parent.width

        Text {
          visible: dataWhichType === "msg"
          Layout.alignment: Qt.AlignHCenter
          text: dataName
          font.pixelSize: 16
          font.bold: true

          topPadding: font.pixelSize
          bottomPadding: font.pixelSize * 1.5
        }

        Rectangle {
          id: rowContainer
          visible: dataWhichType !== "msg"
          Layout.alignment: Qt.AlignHCenter
          width: 1
          height: 50

          Text {
            anchors.top: (inventoryAmount.visible)
                         ? inventoryAmount.top
                         : cartAmount.top
            anchors.right: (inventoryAmount.visible)
                           ? inventoryAmount.left
                           : cartAmount.left
            anchors.rightMargin: (inventoryAmount.visible)
                                 ? font.pixelSize / 1
                                 : font.pixelSize / 4
            anchors.bottom: (inventoryAmount.visible)
                            ? inventoryAmount.bottom
                            : cartAmount.bottom

            text: dataName
            font.pixelSize: 14
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter

            //topPadding: 15
          }

          Text {
            id: inventoryAmount

            anchors.top: cartAmount.top
            anchors.right: cartAmount.left
            anchors.rightMargin: font.pixelSize / 4
            anchors.bottom: cartAmount.bottom

            // Only when selling
            visible: !brg.marketModel.isBuyMode
            text: dataInStockCount.toString().padStart(2, " ")
            font.pixelSize: 14
            width: (dataWhichType === "money")
                   ? font.pixelSize * 3.5
                   : font.pixelSize * 1
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter

            //topPadding: 15
          }

          SpinBox {
            id: cartAmount
            anchors.centerIn: parent

            enabled: dataCanSell || brg.marketModel.isBuyMode
            onValueChanged: dataCartCount = value;
            Component.onCompleted: value = dataCartCount;
            editable: true
            from: 0
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhDigitsOnly
            //width: font.pixelSize * 5
            implicitWidth: font.pixelSize * 10
          }

          Text {
            anchors.top: cartAmount.top
            anchors.left: cartAmount.right
            anchors.leftMargin: font.pixelSize / 4
            anchors.bottom: cartAmount.bottom

            text: (dataCartWorth <= 0)
                  ? " "
                  : dataCartWorth
            font.pixelSize: 14
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter

            //topPadding: 15
          }
        }

        Text {
          visible: isLast
          bottomPadding: 25
        }
      }
    }

    Rectangle {
      id: footer
      color: brg.settings.accentColor
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      height: 45

      Material.foreground: brg.settings.textColorLight
      Material.background: brg.settings.accentColor

      Text {
        // Sell with Money
        visible: brg.marketModel.whichMode == 3
        anchors.centerIn: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: "On Cart: " + brg.marketModel.totalCartCount + " (" + brg.marketModel.totalCartWorth + ")";
        font.pixelSize: 14
        color: brg.settings.textColorLight
      }
    }
  }

  footer: AppFooterBtn3 {
    // Converts between Buying and Selling
    icon1.source: "qrc:/assets/icons/fontawesome/exchange-alt.svg"
    text1: "Buy/Sell"
    onBtn1Clicked: brg.marketModel.isBuyMode = !brg.marketModel.isBuyMode;

    // Checks out shopping cart
    icon2.source: "qrc:/assets/icons/fontawesome/shopping-cart.svg"
    text2: "Checkout"
    onBtn2Clicked: brg.marketModel.checkout();

    // Converts between Money and Coins
    icon3.source: "qrc:/assets/icons/fontawesome/coins.svg"
    text3: "Currency"
    onBtn3Clicked: brg.marketModel.isMoneyCurrency = !brg.marketModel.isMoneyCurrency;
  }
}
