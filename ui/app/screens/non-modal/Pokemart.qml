import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: topz

  function curSym(dataWhichType) {
    if(brg.marketModel.isMoneyCurrency)
      return "₽"
    else if(!brg.marketModel.isMoneyCurrency && dataWhichType !== "money")
      return "⭘"
    else if(!brg.marketModel.isMoneyCurrency && dataWhichType === "money" && brg.marketModel.isBuyMode)
      return "⭘"
    else if(!brg.marketModel.isMoneyCurrency && dataWhichType === "money" && !brg.marketModel.isBuyMode)
      return "₽"

    return "?"
  }

  function signing(dataWhichType) {
    if(brg.marketModel.isBuyMode && dataWhichType !== "money")
      return "-"
    else if(!brg.marketModel.isBuyMode && dataWhichType !== "money")
      return "+"
    else if(dataWhichType === "money")
      return "+"

    return "?"
  }

  function curName() {
    if(brg.marketModel.isMoneyCurrency)
      return "Money"
    else
      return "Coins"
  }

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

        function getTo() {
          if(dataWhichType === "playerItem" || dataWhichType === "money")
            return dataInStockCount;

          return 999999;
        }

        function getRightMargin() {
          //
        }

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
            anchors.top: eachPrice.top
            anchors.right: eachPrice.left
            anchors.rightMargin: font.pixelSize / 1
            anchors.bottom: eachPrice.bottom

            text: dataName
            font.pixelSize: 14
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter

            //topPadding: 15
          }

          Text {
            id: eachPrice

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

            color: (cartAmount.enabled)
                   ? brg.settings.textColorDark
                   : brg.settings.textColorMid
            width: (dataWhichType === "money")
                   ? implicitWidth
                   : font.pixelSize * 2.75

            text: topz.curSym(dataWhichType) + dataItemWorth.toLocaleString()
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
            anchors.rightMargin: font.pixelSize / 3
            anchors.bottom: cartAmount.bottom

            color: (cartAmount.enabled)
                   ? brg.settings.textColorDark
                   : brg.settings.textColorMid

            // Only when selling
            visible: !brg.marketModel.isBuyMode
            text: "x" + dataInStockCount.toString().padStart(2, " ")
            font.pixelSize: 14
            width: (dataWhichType === "money")
                   ? implicitWidth //font.pixelSize * 3.5
                   : font.pixelSize * 1.75
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
            to: getTo()
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
                  : topz.signing(dataWhichType) + " " + topz.curSym(dataWhichType) + dataCartWorth.toLocaleString()
            font.pixelSize: 14
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter

            //topPadding: 15
          }

          Text {
            visible: !cartAmount.enabled

            anchors.top: cartAmount.top
            anchors.left: cartAmount.right
            anchors.leftMargin: font.pixelSize / 4
            anchors.bottom: cartAmount.bottom

            color: brg.settings.textColorMid
            text: "Item cannot be sold"
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
      id: summaryScreen

      x: parent.width - width - 5
      y: 0 - height - 5

      width: 250
      height: 100

      color: "white"
      border.color: brg.settings.accentColor
      border.width: 3
      radius: 15

      state: (brg.marketModel.totalCartCount > 0)
             ? "displayed"
             : ""

      ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        Text {
          Layout.fillWidth: true
          leftPadding: font.pixelSize + 4

          text: topz.curSym() + brg.marketModel.moneyStart.toLocaleString()
          font.pixelSize: 14
        }

        Row {
          Layout.fillWidth: true

          Text {
            text: topz.signing()
            font.pixelSize: 14
            width: font.pixelSize + 4
          }

          Text {
            Layout.fillWidth: true

            text: topz.curSym() + brg.marketModel.totalCartWorth.toLocaleString()
            font.pixelSize: 14
          }
        }

        Row {
          Layout.fillWidth: true

          Text {
            text: (brg.marketModel.moneyLeftover < 0)
                  ? "-"
                  : ""
            color: (brg.marketModel.moneyLeftover < 0)
                   ? "red"
                   : brg.settings.textColorDark
            font.pixelSize: 14
            width: font.pixelSize + 4
          }

          Text {
            Layout.fillWidth: true

            text: (brg.marketModel.moneyLeftover < 0)
                  ? topz.curSym() + (brg.marketModel.moneyLeftover * (-1)).toLocaleString()
                  : gtTxt()

            color: (brg.marketModel.moneyLeftover < 0 || brg.marketModel.moneyLeftover > moneyMax())
                   ? "red"
                   : brg.settings.textColorDark

            font.pixelSize: 14

            function moneyMax() {
              return (brg.marketModel.isMoneyCurrency)
                  ? 999999
                  : 9999;
            }

            function gtTxt() {
              if(brg.marketModel.isMoneyCurrency && brg.marketModel.moneyLeftover > 999999)
                return "> " + topz.curSym() + "999,999"
              else if(!brg.marketModel.isMoneyCurrency && brg.marketModel.moneyLeftover > 9999)
                return "> " + topz.curSym() + "9,999"

              return topz.curSym() + brg.marketModel.moneyLeftover.toLocaleString();
            }
          }
        }
      }

      Behavior on y {
        NumberAnimation {
          easing {
            type: Easing.InOutQuad
          }
        }
      }

      states: [
        State {
          name: "displayed"
          PropertyChanges { target: summaryScreen; y: 5 }
        }
      ]
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
        anchors.centerIn: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: (brg.marketModel.totalCartCount <= 0)
              ? ""
              : "On Cart: x" + brg.marketModel.totalCartCount.toLocaleString()
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
