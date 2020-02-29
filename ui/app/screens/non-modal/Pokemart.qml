import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: topz

  // Maximum money depending on the currency
  function maxMoney() {
    if(brg.marketModel.isMoneyCurrency)
      return 999999;
    else
      return 9999;
  }

  // Negative version of max money
  function maxMoneyNeg() {
    return -maxMoney();
  }

  // If the money is above or below the maximum range
  function moneyOutOfRange(val) {
    if(Math.abs(val) > maxMoney())
      return true;
    else
      return false;
  }

  // Returns red if outside of range and if below zero if enabled
  function moneyColor(val, zeroRed) {
    var ret = moneyOutOfRange(val)
          ? "red"
          : brg.settings.textColorDark;

    if(zeroRed === true && val < 0)
      ret = "red";

    return ret;
  }

  // Returns currency symbol including the exchange exception which follows
  // different rules
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

  // Buy/Sell signing including exchange exception which follows different rules
  function signing(dataWhichType) {
    if(brg.marketModel.isBuyMode && dataWhichType !== "money")
      return "-"
    else if(!brg.marketModel.isBuyMode && dataWhichType !== "money")
      return "+"
    else if(dataWhichType === "money")
      return "+"

    return "?"
  }

  // Name of currency
  function curName() {
    if(brg.marketModel.isMoneyCurrency)
      return "Money"
    else
      return "Coins"
  }

  // This converts money to a properly formatted string and adaptable to
  // different scenarios.
  function moneyStr(val, abs, useSigning, dataWhichType) {
    var _val = val;

    if(_val > maxMoney())
      _val = maxMoney();
    else if(_val < maxMoneyNeg())
      _val = -maxMoney();

    if(abs === true)
      _val = Math.abs(_val);

    _val = _val.toLocaleString();
    _val = curSym(dataWhichType) + _val;

    var curSigning = "";
    if(useSigning === true)
      curSigning = signing(dataWhichType);

    if(abs !== true && useSigning === true)
      _val = curSigning + " " + _val;

    if(useSigning === true && curSigning === "-" && moneyOutOfRange(val))
      _val = "< " + _val;
    else if(useSigning === true && curSigning === "+" && moneyOutOfRange(val))
      _val = "> " + _val;
    else if(useSigning === true && moneyOutOfRange(val))
      _val = "? " + _val;
    else if(useSigning !== true && val < maxMoneyNeg())
      _val = "< " + _val;
    else if(useSigning !== true && val > maxMoney() && useSigning !== true)
      _val = "> " + _val;

    return _val;
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

//        function getTo() {
//          if(dataWhichType === "playerItem" || dataWhichType === "money")
//            return dataInStockCount;

//          return 2147483647;
//        }

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
                   : font.pixelSize * 3

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

          Row {
            id: cartAmount
            anchors.centerIn: parent
            enabled: dataCanSell || brg.marketModel.isBuyMode

            IconButtonSquare {
              id: negBtn
              icon.source: "qrc:/assets/icons/fontawesome/minus.svg"
              enabled: !(dataOnCartLeft === 0 && amountEdit.getValInt() === 0)
              onClicked: {
                if(amountEdit.getValInt() === 0 && dataOnCartLeft > 0)
                  amountEdit.setValInt(dataOnCartLeft);
                else if(amountEdit.getValInt() > 0)
                  amountEdit.setValInt(amountEdit.getValInt() - 1);
              }
            }

            DefTextEdit {
              id: amountEdit
              anchors.top: negBtn.top
              anchors.bottom: negBtn.bottom
              horizontalAlignment: TextInput.AlignHCenter

              function getValInt() {
                return parseInt(text, 10);
              }

              function setValInt(val) {
                text = val.toString();
              }

              validator: IntValidator {
                bottom: 0;
                top: 2147483647;
              }

              onTextChanged: {
                if(acceptableInput)
                  dataCartCount = getValInt();
              }

              Component.onCompleted: setValInt(dataCartCount);

              width: 3 * font.pixelSize
              labelEl.visible: false

              inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhDigitsOnly
            }

            IconButtonSquare {
              id: posBtn
              icon.source: "qrc:/assets/icons/fontawesome/plus.svg"

              enabled: !(dataOnCartLeft === 0 && amountEdit.getValInt() === 0)
              onClicked: {
                if(amountEdit.getValInt() > 0 && dataOnCartLeft === 0)
                  amountEdit.setValInt(0);
                else if(dataOnCartLeft > 0)
                  amountEdit.setValInt(amountEdit.getValInt() + 1);
              }
            }
          }

          Text {
            anchors.top: cartAmount.top
            anchors.left: cartAmount.right
            anchors.leftMargin: font.pixelSize / 4
            anchors.bottom: cartAmount.bottom

            text: (dataCartWorth <= 0)
                  ? " "
                  : topz.moneyStr(dataCartWorth, false, true, dataWhichType)

            color: (dataCartWorth === "phony" || topz.moneyOutOfRange(dataCartWorth))
                   ? topz.moneyColor(dataCartWorth)
                   : topz.moneyColor(dataCartWorth)

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

      x: parent.width - width - 40
      y: 0 - height - 5

      width: 150
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

            color: topz.moneyOutOfRange(brg.marketModel.totalCartWorth)
                   ? "red"
                   : brg.settings.textColorDark

            font.pixelSize: 14
            width: font.pixelSize + 4
          }

          Text {
            Layout.fillWidth: true

            text: (brg.marketModel.totalCartWorth === "phony")
                  ? topz.moneyStr(brg.marketModel.totalCartWorth, true, true)
                  : topz.moneyStr(brg.marketModel.totalCartWorth, true, true)

            color: (brg.marketModel.totalCartWorth === "phony" || topz.moneyOutOfRange(brg.marketModel.totalCartWorth))
                   ? topz.moneyColor(brg.marketModel.totalCartWorth)
                   : topz.moneyColor(brg.marketModel.totalCartWorth)

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

            text: (brg.marketModel.moneyLeftover === "phony")
                  ? topz.moneyStr(brg.marketModel.moneyLeftover, true)
                  : topz.moneyStr(brg.marketModel.moneyLeftover, true)

            color: (brg.marketModel.moneyLeftover === "phony" || topz.moneyOutOfRange(brg.marketModel.moneyLeftover))
                   ? topz.moneyColor(brg.marketModel.moneyLeftover, true)
                   : topz.moneyColor(brg.marketModel.moneyLeftover, true)

            font.pixelSize: 14
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
        visible: !errSpaceLeft.visible && !errMoneyLeft.visible && !errMoneyTooMuch.visible
        anchors.centerIn: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: (brg.marketModel.totalCartCount <= 0)
              ? ""
              : "On Cart: x" + brg.marketModel.totalCartCount.toLocaleString()
        font.pixelSize: 14
        color: brg.settings.textColorLight
      }

      Text {
        id: errSpaceLeft
        visible: brg.marketModel.anyNotEnoughSpace && !errMoneyLeft.visible && !errMoneyTooMuch.visible
        anchors.centerIn: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: "There's not enough space for one or more items on the shopping cart."
        font.pixelSize: 14
        color: brg.settings.textColorLight
      }

      Text {
        id: errMoneyTooMuch
        visible: topz.moneyOutOfRange(brg.marketModel.moneyLeftover) &&  !errMoneyLeft.visible
        anchors.centerIn: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: "You don't have enough space for the money your getting back."
        font.pixelSize: 14
        color: brg.settings.textColorLight
      }

      Text {
        id: errMoneyLeft
        visible: brg.marketModel.moneyLeftover < 0
        anchors.centerIn: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: "There's not enough money for all items on the cart."
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
    btn2.enabled: brg.marketModel.canAnyCheckout
    icon2.source: "qrc:/assets/icons/fontawesome/shopping-cart.svg"
    text2: "Checkout"
    onBtn2Clicked: brg.marketModel.checkout();

    // Converts between Money and Coins
    icon3.source: "qrc:/assets/icons/fontawesome/coins.svg"
    text3: "Currency"
    onBtn3Clicked: brg.marketModel.isMoneyCurrency = !brg.marketModel.isMoneyCurrency;
  }
}
