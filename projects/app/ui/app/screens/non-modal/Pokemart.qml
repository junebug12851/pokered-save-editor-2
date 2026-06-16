// Pokemart.qml -- the buy/sell shop screen (Pokemart and Game Corner).
//
// A shopping-cart UI over brg.marketModel. Four modes (whichMode 0-3) combine
// buy/sell with money/coins currency; the header text and currency symbols adapt
// per mode. The JS helpers up top (maxMoney / curSym / signing / moneyStr / ...)
// handle currency formatting, signing and out-of-range clamping for the two
// currencies (money caps at 999,999; coins at 9,999).
//
// Each item row is a centered -/amount/+ cart stepper with the item name, unit
// price and (when selling) owned-count fanned out to its left and the running
// cart cost to its right. A floating summary box shows money start / cart cost /
// leftover; an accent status bar shows the cart count or an error. Footer:
// Buy/Sell toggle, Checkout (commits the cart), Currency toggle.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: topz

  // ---- Currency formatting helpers -------------------------------------------

  // Maximum money for the active currency (money 999,999; coins 9,999).
  function maxMoney(sym) {
    if(sym === "₽" || brg.marketModel.isMoneyCurrency)
      return 999999;
    return 9999;
  }

  // Negative bound for the active currency.
  function maxMoneyNeg(sym) {
    return -maxMoney(sym);
  }

  // Is the value outside the currency's representable range?
  function moneyOutOfRange(val, sym) {
    return Math.abs(val) > maxMoney(sym);
  }

  // Error red when out of range (or, with zeroRed, when negative); else normal.
  function moneyColor(val, zeroRed, sym) {
    if(moneyOutOfRange(val, sym))
      return brg.settings.errorColor;
    if(zeroRed === true && val < 0)
      return brg.settings.errorColor;
    return brg.settings.textColorDark;
  }

  // Currency symbol, including the exchange exception (selling items for money
  // while in coins mode still pays out in coins, but the money row shows ₽).
  function curSym(dataWhichType) {
    if(brg.marketModel.isMoneyCurrency)
      return "₽";
    if(dataWhichType !== "money")
      return "⭘";
    if(brg.marketModel.isBuyMode)
      return "⭘";
    return "₽";
  }

  // Buy/Sell sign, with the money row always reading as a "+" (you receive it).
  function signing(dataWhichType) {
    if(dataWhichType === "money")
      return "+";
    return brg.marketModel.isBuyMode ? "-" : "+";
  }

  // Format a value as a currency string, clamped to range, optionally absolute
  // and/or signed, with a leading "<"/">" when the true value overflows range.
  function moneyStr(val, abs, useSigning, dataWhichType) {
    var sym = curSym(dataWhichType);

    var _val = val;
    if(_val > maxMoney(sym))
      _val = maxMoney(sym);
    else if(_val < maxMoneyNeg(sym))
      _val = -maxMoney(sym);

    if(abs === true)
      _val = Math.abs(_val);

    _val = sym + _val.toLocaleString();

    var sign = useSigning === true ? signing(dataWhichType) : "";

    if(abs !== true && useSigning === true)
      _val = sign + " " + _val;

    if(useSigning === true && moneyOutOfRange(val, sym))
      _val = (sign === "-" ? "< " : sign === "+" ? "> " : "? ") + _val;
    else if(useSigning !== true && val < maxMoneyNeg(sym))
      _val = "< " + _val;
    else if(useSigning !== true && val > maxMoney(sym))
      _val = "> " + _val;

    return _val;
  }

  // Header title for the current mode (buy/sell x mart/game-corner).
  function headerText() {
    switch(brg.marketModel.whichMode) {
      case 0: return qsTr("Shop at the Pokemart");
      case 1: return qsTr("Shop at the Game Corner");
      case 2: return qsTr("Sell to the Pokemart");
      case 3: return qsTr("Sell to the Game Corner");
    }
    return "";
  }

  // Status-bar text: the highest-priority error if any, else the cart count.
  function statusText() {
    if(brg.marketModel.moneyLeftover < 0)
      return qsTr("There's not enough money for all items on the cart.");
    if(moneyOutOfRange(brg.marketModel.moneyLeftover))
      return qsTr("You don't have enough space for the money your getting back.");
    if(brg.marketModel.anyNotEnoughSpace)
      return qsTr("There's not enough space for one or more items on the shopping cart.");
    if(brg.marketModel.totalCartCount > 0)
      return qsTr("On Cart: x") + brg.marketModel.totalCartCount.toLocaleString();
    return "";
  }

  // White page background.
  Rectangle {
    anchors.fill: parent

    // ---- Header bar: mode title, centered on the accent bar ----
    Rectangle {
      id: header
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: parent.top
      height: 45

      color: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight
      Material.background: brg.settings.accentColor

      Text {
        anchors.centerIn: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: topz.headerText()
        font.pixelSize: 18
        color: brg.settings.textColorLight
      }
    }

    // ---- Market list ----
    ListView {
      id: marketView
      model: brg.marketModel

      anchors.top: header.bottom
      anchors.left: parent.left
      anchors.leftMargin: 15
      anchors.right: parent.right
      anchors.rightMargin: 15
      anchors.bottom: statusBar.top

      clip: true
      ScrollBar.vertical: ScrollBar {}

      // Breathing room under the last row (was a per-delegate isLast hack).
      footer: Item { width: 1; height: 25 }

      delegate: Item {
        id: delegate
        width: marketView.width
        height: dataWhichType === "msg" ? msgText.implicitHeight : 50

        // Section / category message row.
        Text {
          id: msgText
          visible: dataWhichType === "msg"
          anchors.horizontalCenter: parent.horizontalCenter
          anchors.top: parent.top
          text: dataName
          font.pixelSize: 16
          font.bold: true
          topPadding: 16
          bottomPadding: 24
        }

        // Item row: a center-anchored cart stepper, with the item name / unit
        // price / owned-count right-aligned up against it, and the running cart
        // cost to its right. The stepper is centered on the screen so the rows
        // line up regardless of label width.
        Item {
          visible: dataWhichType !== "msg"
          anchors.fill: parent

          // -/amount/+ stepper, centered.
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
              width: 3 * font.pixelSize
              horizontalAlignment: TextInput.AlignHCenter
              labelEl.visible: false
              inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhDigitsOnly

              color: (!dataCanCheckout && dataCartCount > 0)
                     ? brg.settings.errorColor
                     : brg.settings.textColorDark

              validator: IntValidator { bottom: 0; top: 2147483647 }

              function getValInt() { return parseInt(text, 10); }
              function setValInt(val) { text = val.toString(); }

              onTextChanged: if(acceptableInput) dataCartCount = getValInt();
              Component.onCompleted: setValInt(dataCartCount);
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

          // -- Left group (right-aligned against the stepper): name, unit
          //    price, and -- when selling -- the owned-count. --

          // Owned-count ("x N"), only when selling.
          Text {
            id: inventoryAmount
            visible: !brg.marketModel.isBuyMode
            anchors.right: cartAmount.left
            anchors.rightMargin: font.pixelSize / 3
            anchors.verticalCenter: cartAmount.verticalCenter
            width: (dataWhichType === "money") ? implicitWidth : font.pixelSize * 1.75
            text: "x" + dataInStockCount.toString().padStart(2, " ")
            font.pixelSize: 14
            color: cartAmount.enabled ? brg.settings.textColorDark
                                      : brg.settings.textColorMid
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
          }

          // Unit price.
          Text {
            id: eachPrice
            anchors.right: inventoryAmount.visible ? inventoryAmount.left
                                                   : cartAmount.left
            anchors.rightMargin: inventoryAmount.visible ? font.pixelSize
                                                         : font.pixelSize / 4
            anchors.verticalCenter: cartAmount.verticalCenter
            width: (dataWhichType === "money") ? implicitWidth : font.pixelSize * 3
            text: topz.curSym(dataWhichType) + dataItemWorth.toLocaleString()
            font.pixelSize: 14
            color: cartAmount.enabled ? brg.settings.textColorDark
                                      : brg.settings.textColorMid
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
          }

          // Item name.
          Text {
            anchors.right: eachPrice.left
            anchors.rightMargin: font.pixelSize
            anchors.verticalCenter: cartAmount.verticalCenter
            text: dataName
            font.pixelSize: 14
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
          }

          // -- Right of the stepper: running cart cost, or a "can't sell" note. --
          Text {
            visible: cartAmount.enabled
            anchors.left: cartAmount.right
            anchors.leftMargin: font.pixelSize / 4
            anchors.verticalCenter: cartAmount.verticalCenter
            text: (dataCartWorth <= 0)
                  ? " "
                  : topz.moneyStr(dataCartWorth, false, true, dataWhichType)
            color: topz.moneyColor(dataCartWorth, false, topz.curSym(dataWhichType))
            font.pixelSize: 14
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
          }

          Text {
            visible: !cartAmount.enabled
            anchors.left: cartAmount.right
            anchors.leftMargin: font.pixelSize / 4
            anchors.verticalCenter: cartAmount.verticalCenter
            text: qsTr("Item cannot be sold")
            color: brg.settings.textColorMid
            font.pixelSize: 14
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
          }
        }
      }
    }

    // ---- Floating summary box: slides down when the cart is non-empty.
    //      Shows money before, signed cart cost, and money leftover. ----
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

      state: (brg.marketModel.totalCartCount > 0) ? "displayed" : ""

      ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        // Money before checkout.
        Text {
          Layout.fillWidth: true
          leftPadding: font.pixelSize + 4
          text: topz.curSym() + brg.marketModel.moneyStart.toLocaleString()
          font.pixelSize: 14
        }

        // Signed cart cost.
        Row {
          Layout.fillWidth: true

          Text {
            width: font.pixelSize + 4
            text: topz.signing()
            color: topz.moneyOutOfRange(brg.marketModel.totalCartWorth)
                   ? brg.settings.errorColor
                   : brg.settings.textColorDark
            font.pixelSize: 14
          }

          Text {
            Layout.fillWidth: true
            text: topz.moneyStr(brg.marketModel.totalCartWorth, true, true)
            color: topz.moneyColor(brg.marketModel.totalCartWorth)
            font.pixelSize: 14
          }
        }

        // Money leftover.
        Row {
          Layout.fillWidth: true

          Text {
            width: font.pixelSize + 4
            text: (brg.marketModel.moneyLeftover < 0) ? "-" : ""
            color: (brg.marketModel.moneyLeftover < 0)
                   ? brg.settings.errorColor
                   : brg.settings.textColorDark
            font.pixelSize: 14
          }

          Text {
            Layout.fillWidth: true
            text: topz.moneyStr(brg.marketModel.moneyLeftover, true)
            color: topz.moneyColor(brg.marketModel.moneyLeftover, true)
            font.pixelSize: 14
          }
        }
      }

      Behavior on y {
        NumberAnimation { easing.type: Easing.InOutQuad }
      }

      states: [
        State {
          name: "displayed"
          PropertyChanges { target: summaryScreen; y: 5 }
        }
      ]
    }

    // ---- Status bar: cart count, or the highest-priority error. ----
    Rectangle {
      id: statusBar
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      height: 45

      color: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight
      Material.background: brg.settings.accentColor

      Text {
        anchors.centerIn: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: topz.statusText()
        font.pixelSize: 14
        color: brg.settings.textColorLight
      }
    }
  }

  // ---- 3-button footer: Buy/Sell, Checkout, Currency. ----
  footer: AppFooterBtn3 {
    id: theFooter

    // Toggle between buying (store stock) and selling (your items).
    icon1.source: "qrc:/assets/icons/fontawesome/exchange-alt.svg"
    text1: "Buy/Sell"
    onBtn1Clicked: brg.marketModel.isBuyMode = !brg.marketModel.isBuyMode

    // Commit the cart. Disabled (and never stuck-highlighted -- see
    // FooterButton.qml) until the transaction can complete.
    btn2.enabled: brg.marketModel.canAnyCheckout
    icon2.source: "qrc:/assets/icons/fontawesome/shopping-cart.svg"
    text2: "Checkout"
    onBtn2Clicked: brg.marketModel.checkout()

    // Toggle between money and coins.
    icon3.source: "qrc:/assets/icons/fontawesome/coins.svg"
    text3: "Currency"
    onBtn3Clicked: brg.marketModel.isMoneyCurrency = !brg.marketModel.isMoneyCurrency
  }
}
