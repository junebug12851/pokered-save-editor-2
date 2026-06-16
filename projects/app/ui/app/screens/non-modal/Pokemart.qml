// Pokemart.qml -- the buy/sell shop screen (Pokemart and Game Corner).
//
// Two panes side by side:
//   * LEFT  -- the shopping list over brg.marketModel: a clean list of item rows
//              (name | owned | unit price | an inline -/qty/+ stepper as the row's
//              action), with section headers, hover highlight and zebra striping.
//              This is where you build the cart.
//   * RIGHT -- a store-style RECEIPT over brg.marketCartModel (a cart-only filter of
//              the same model): money on hand, one itemized buy/sell line per carted
//              item, then the total, the resulting balance, and any warning. The
//              receipt does the totalling, so the list rows carry no running total.
//
// The list pane is wider than the receipt (stretch 5:3). Four modes (whichMode 0-3)
// combine buy/sell with money/coins; the header text and currency symbols adapt per
// mode. The JS helpers up top (maxMoney/curSym/signing/moneyStr/...) handle currency
// formatting, signing and out-of-range clamping. Footer: Buy/Sell, Checkout, Currency.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: topz

  // List-row sizing knobs.
  property int rowH: 46     // item row height
  property int headH: 32    // section-header row height
  property int qtyW: 34     // stepper quantity field width

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

  // The highest-priority transaction warning, or "" when the cart is fine.
  function warningText() {
    if(brg.marketModel.moneyLeftover < 0)
      return qsTr("There's not enough money for all items on the cart.");
    if(moneyOutOfRange(brg.marketModel.moneyLeftover))
      return qsTr("You don't have enough space for the money your getting back.");
    if(brg.marketModel.anyNotEnoughSpace)
      return qsTr("There's not enough space for one or more items on the shopping cart.");
    return "";
  }

  // ---- Two panes: shopping list (left, wider) | receipt (right) ---------------
  RowLayout {
    id: paneRow
    anchors.fill: parent
    spacing: 0

    // ====================== LEFT: the shopping list ==========================
    Rectangle {
      id: listPane
      Layout.fillWidth: true                // takes the remaining ~67%
      Layout.fillHeight: true
      color: "white"

      // Mode title on the accent bar.
      Rectangle {
        id: listHeader
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

      ListView {
        id: marketView
        model: brg.marketModel

        anchors.top: listHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        clip: true
        ScrollBar.vertical: ScrollBar {}

        // Breathing room under the last row.
        footer: Item { width: 1; height: 16 }

        delegate: Item {
          id: row
          width: marketView.width
          height: dataWhichType === "msg" ? topz.headH : topz.rowH

          // Whether this row can be added to the cart (always when buying; only
          // sellable items when selling).
          property bool canStep: dataCanSell || brg.marketModel.isBuyMode

          // ---------------- Section header row ("msg") ----------------
          Rectangle {
            visible: dataWhichType === "msg"
            anchors.fill: parent
            color: Qt.rgba(0, 0, 0, 0.045)

            Text {
              anchors.verticalCenter: parent.verticalCenter
              anchors.left: parent.left
              anchors.leftMargin: 14
              text: dataName
              font.pixelSize: 12
              font.bold: true
              font.capitalization: Font.AllUppercase
              font.letterSpacing: 1
              color: brg.settings.textColorMid
            }

            Rectangle {
              anchors.left: parent.left
              anchors.right: parent.right
              anchors.bottom: parent.bottom
              height: 1
              color: brg.settings.dividerColor
            }
          }

          // ---------------- Item row ----------------
          Item {
            visible: dataWhichType !== "msg"
            anchors.fill: parent

            // Whole-row hover (stays true over the child controls too).
            HoverHandler { id: rowHover }

            // Row background: accent-tinted on hover, faint zebra otherwise.
            Rectangle {
              anchors.fill: parent
              color: rowHover.hovered
                     ? Qt.rgba(brg.settings.accentColor.r,
                               brg.settings.accentColor.g,
                               brg.settings.accentColor.b, 0.12)
                     : (index % 2 === 1 ? Qt.rgba(0, 0, 0, 0.025) : "transparent")
            }

            RowLayout {
              anchors.fill: parent
              anchors.leftMargin: 14
              anchors.rightMargin: 16          // reserve the scrollbar lane
              spacing: 10

              // Item name.
              Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: dataName
                elide: Text.ElideRight
                font.pixelSize: 14
                color: row.canStep ? brg.settings.textColorDark
                                   : brg.settings.textColorMid
                verticalAlignment: Text.AlignVCenter
              }

              // Owned count (sell mode only).
              Text {
                visible: !brg.marketModel.isBuyMode
                Layout.alignment: Qt.AlignVCenter
                text: "x" + dataInStockCount.toLocaleString()
                font.pixelSize: 12
                color: brg.settings.textColorMid
                horizontalAlignment: Text.AlignRight
              }

              // Unit price.
              Text {
                Layout.alignment: Qt.AlignVCenter
                Layout.minimumWidth: 54
                text: topz.curSym(dataWhichType) + dataItemWorth.toLocaleString()
                font.pixelSize: 13
                color: row.canStep ? brg.settings.textColorDark
                                   : brg.settings.textColorMid
                horizontalAlignment: Text.AlignRight
              }

              // -- Action: an inline -/qty/+ stepper pill. --
              Rectangle {
                id: stepPill
                visible: row.canStep
                Layout.alignment: Qt.AlignVCenter
                implicitHeight: 30
                implicitWidth: stepRow.width + 8
                radius: 6
                color: rowHover.hovered ? "white" : Qt.rgba(0, 0, 0, 0.03)
                border.width: 1
                border.color: Qt.rgba(0, 0, 0, 0.10)

                Row {
                  id: stepRow
                  anchors.centerIn: parent

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
                    width: topz.qtyW
                    horizontalAlignment: TextInput.AlignHCenter
                    labelEl.visible: false
                    background: Item {}        // borderless inside the pill
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
              }

              // Can't-sell note (replaces the stepper for unsellable items).
              Text {
                visible: !row.canStep
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Can't sell")
                font.pixelSize: 12
                font.italic: true
                color: brg.settings.textColorMid
              }
            }

            // Faint row separator.
            Rectangle {
              anchors.left: parent.left
              anchors.right: parent.right
              anchors.bottom: parent.bottom
              height: 1
              color: Qt.rgba(0, 0, 0, 0.06)
            }
          }
        }
      }
    }

    // Divider between the two panes.
    Rectangle {
      Layout.fillHeight: true
      Layout.preferredWidth: 1
      color: brg.settings.dividerColor
    }

    // ====================== RIGHT: the receipt ===============================
    Rectangle {
      id: receiptPane
      Layout.fillHeight: true
      Layout.preferredWidth: Math.round(paneRow.width * 0.33)   // ~1/3 of the screen
      color: "white"

      Rectangle {
        id: receiptHeader
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
          text: qsTr("Cart")
          font.pixelSize: 18
          color: brg.settings.textColorLight

          // Live item count beside the title (hidden when empty).
          Text {
            anchors.left: parent.right
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            visible: brg.marketModel.totalCartCount > 0
            text: "x" + brg.marketModel.totalCartCount.toLocaleString()
            font.pixelSize: 14
            color: brg.settings.textColorLight
          }
        }
      }

      // Receipt body: money on hand, the itemized lines, then totals + warning.
      ColumnLayout {
        anchors.top: receiptHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        spacing: 8

        // -- Money on hand --
        RowLayout {
          Layout.fillWidth: true
          Text {
            Layout.fillWidth: true
            text: qsTr("Money on hand")
            font.pixelSize: 14
            color: brg.settings.textColorMid
          }
          Text {
            text: topz.curSym() + brg.marketModel.moneyStart.toLocaleString()
            font.pixelSize: 14
            color: brg.settings.textColorDark
          }
        }

        // Dashed-look divider.
        Rectangle {
          Layout.fillWidth: true
          height: 1
          color: brg.settings.dividerColor
        }

        // -- Itemized lines (one per carted item) --
        ListView {
          id: receiptList
          Layout.fillWidth: true
          Layout.fillHeight: true
          model: brg.marketCartModel
          clip: true
          ScrollBar.vertical: ScrollBar {}

          // Empty state.
          Text {
            anchors.centerIn: parent
            visible: receiptList.count === 0
            text: qsTr("Your cart is empty.")
            font.pixelSize: 14
            color: brg.settings.textColorMid
          }

          delegate: Item {
            width: receiptList.width
            height: 46

            ColumnLayout {
              anchors.left: parent.left
              anchors.right: parent.right
              anchors.rightMargin: 16        // reserve the scrollbar lane
              anchors.verticalCenter: parent.verticalCenter
              spacing: 2

              // Name + signed line total.
              RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                  Layout.fillWidth: true
                  text: dataName
                  font.pixelSize: 14
                  color: brg.settings.textColorDark
                  elide: Text.ElideRight
                }
                Text {
                  text: topz.moneyStr(dataCartWorth, false, true, dataWhichType)
                  font.pixelSize: 14
                  color: topz.moneyColor(dataCartWorth, false,
                                         topz.curSym(dataWhichType))
                  horizontalAlignment: Text.AlignRight
                }
              }

              // qty x unit price (the line's makeup).
              Text {
                text: "x" + dataCartCount.toLocaleString() + "  @ "
                      + topz.curSym(dataWhichType) + dataItemWorth.toLocaleString()
                font.pixelSize: 12
                color: brg.settings.textColorMid
              }
            }

            // Faint row separator.
            Rectangle {
              anchors.left: parent.left
              anchors.right: parent.right
              anchors.rightMargin: 16
              anchors.bottom: parent.bottom
              height: 1
              color: Qt.rgba(0, 0, 0, 0.06)
            }
          }
        }

        // Solid divider above the totals.
        Rectangle {
          Layout.fillWidth: true
          height: 1
          color: brg.settings.dividerColor
        }

        // -- Total --
        RowLayout {
          Layout.fillWidth: true
          Text {
            Layout.fillWidth: true
            text: qsTr("Total")
            font.pixelSize: 16
            font.bold: true
            color: brg.settings.textColorDark
          }
          Text {
            text: topz.moneyStr(brg.marketModel.totalCartWorth, true, true)
            font.pixelSize: 16
            font.bold: true
            color: topz.moneyColor(brg.marketModel.totalCartWorth)
            horizontalAlignment: Text.AlignRight
          }
        }

        // -- Resulting balance --
        RowLayout {
          Layout.fillWidth: true
          Text {
            Layout.fillWidth: true
            text: qsTr("Balance after")
            font.pixelSize: 14
            color: brg.settings.textColorMid
          }
          Text {
            text: (brg.marketModel.moneyLeftover < 0 ? "-" : "")
                  + topz.moneyStr(brg.marketModel.moneyLeftover, true)
            font.pixelSize: 14
            color: topz.moneyColor(brg.marketModel.moneyLeftover, true)
            horizontalAlignment: Text.AlignRight
          }
        }

        // -- Warning (not enough money / no space) --
        Text {
          Layout.fillWidth: true
          visible: topz.warningText() !== ""
          text: topz.warningText()
          font.pixelSize: 13
          color: brg.settings.errorColor
          wrapMode: Text.WordWrap
          horizontalAlignment: Text.AlignHCenter
        }
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
