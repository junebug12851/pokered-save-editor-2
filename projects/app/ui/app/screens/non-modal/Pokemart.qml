// Pokemart.qml -- the buy/sell/exchange shop screen (Pokemart and Game Corner).
//
// Two panes side by side:
//   * LEFT  -- the shopping list over brg.marketModel: a clean list of item rows
//              (name | owned | unit price | an inline -/qty/+ stepper action), with
//              section headers, hover highlight and zebra striping. In Exchange mode
//              it instead lists the two money<->coins swaps.
//   * RIGHT -- a store-style RECEIPT over brg.marketCartModel: in buy/sell, an
//              itemized money receipt (money on hand, lines, total, balance); in
//              Exchange, a dual-currency before->after summary for money AND coins.
//
// The mode is chosen by two segmented control strips in the left header: the action
// (Buy / Sell / Exchange) and the venue (Pokemart / Game Corner; disabled in
// Exchange). The footer is a single Checkout button. The list pane is wider than the
// receipt (~67/33). The JS helpers up top handle currency formatting/clamping.
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

  // ---- A connected, single-select segmented control (styled for the accent bar) --
  component SegStrip: Rectangle {
    id: seg
    property var options: []          // array of label strings
    property int currentIndex: 0
    property bool stripEnabled: true
    signal picked(int index)

    implicitHeight: 28
    implicitWidth: segRow.implicitWidth
    radius: 6
    color: "transparent"
    border.width: 1
    border.color: Qt.rgba(1, 1, 1, 0.55)   // light outline on the accent bar
    clip: true
    opacity: stripEnabled ? 1 : 0.45

    Row {
      id: segRow
      anchors.fill: parent

      Repeater {
        model: seg.options
        delegate: Item {
          id: segItem
          required property int index
          required property string modelData
          height: seg.height
          width: Math.max(segLabel.implicitWidth + 24, 46)

          // Selected segment fills light; others are transparent.
          Rectangle {
            anchors.fill: parent
            color: segItem.index === seg.currentIndex
                   ? brg.settings.textColorLight : "transparent"
          }

          // Divider before every segment but the first.
          Rectangle {
            visible: segItem.index > 0
            anchors.left: parent.left
            width: 1
            height: parent.height
            color: Qt.rgba(1, 1, 1, 0.45)
          }

          Text {
            id: segLabel
            anchors.centerIn: parent
            text: segItem.modelData
            font.pixelSize: 13
            font.bold: segItem.index === seg.currentIndex
            color: segItem.index === seg.currentIndex
                   ? brg.settings.accentColor : brg.settings.textColorLight
          }

          MouseArea {
            anchors.fill: parent
            enabled: seg.stripEnabled
            cursorShape: Qt.PointingHandCursor
            onClicked: seg.picked(segItem.index)
          }
        }
      }
    }
  }

  // ---- Currency formatting helpers -------------------------------------------

  // Maximum money for the active currency (money 999,999; coins 9,999).
  function maxMoney(sym) {
    if(sym === "₽" || brg.marketModel.isMoneyCurrency)
      return 999999;
    return 9999;
  }

  function maxMoneyNeg(sym) { return -maxMoney(sym); }

  function moneyOutOfRange(val, sym) { return Math.abs(val) > maxMoney(sym); }

  // Error red when out of range (or, with zeroRed, when negative); else normal.
  function moneyColor(val, zeroRed, sym) {
    if(moneyOutOfRange(val, sym))
      return brg.settings.errorColor;
    if(zeroRed === true && val < 0)
      return brg.settings.errorColor;
    return brg.settings.textColorDark;
  }

  function curSym(dataWhichType) {
    if(brg.marketModel.isMoneyCurrency)
      return "₽";
    if(dataWhichType !== "money")
      return "⭘";
    if(brg.marketModel.isBuyMode)
      return "⭘";
    return "₽";
  }

  function signing(dataWhichType) {
    if(dataWhichType === "money")
      return "+";
    return brg.marketModel.isBuyMode ? "-" : "+";
  }

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

  // The highest-priority buy/sell warning, or "" when the cart is fine.
  function warningText() {
    if(brg.marketModel.moneyLeftover < 0)
      return qsTr("There's not enough money for all items on the cart.");
    if(moneyOutOfRange(brg.marketModel.moneyLeftover))
      return qsTr("You don't have enough space for the money your getting back.");
    if(brg.marketModel.anyNotEnoughSpace)
      return qsTr("There's not enough space for one or more items on the shopping cart.");
    return "";
  }

  // Exchange-mode warning, or "" when the swap is valid.
  function exchangeWarningText() {
    if(brg.marketModel.exchangeMoneyAfter < 0)
      return qsTr("You don't have enough money for this exchange.");
    if(brg.marketModel.exchangeCoinsAfter < 0)
      return qsTr("You don't have enough coins for this exchange.");
    if(brg.marketModel.exchangeMoneyAfter > 999999)
      return qsTr("That's more money than you can hold.");
    if(brg.marketModel.exchangeCoinsAfter > 9999)
      return qsTr("That's more coins than you can hold.");
    return "";
  }

  // "(+₽150)" / "(-⭘30)" style delta string.
  function deltaStr(after, start, sym) {
    var d = after - start;
    return "(" + (d >= 0 ? "+" : "-") + sym + Math.abs(d).toLocaleString() + ")";
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

      // Header: the two segmented mode strips, side by side, on the accent bar.
      Rectangle {
        id: listHeader
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 45
        color: brg.settings.accentColor
        Material.foreground: brg.settings.textColorLight
        Material.background: brg.settings.accentColor

        Row {
          anchors.centerIn: parent
          spacing: 14

          // Action: Buy / Sell / Exchange.
          SegStrip {
            anchors.verticalCenter: parent.verticalCenter
            options: ["Buy", "Sell", "Exchange"]
            currentIndex: brg.marketModel.isExchangeMode
                          ? 2 : (brg.marketModel.isBuyMode ? 0 : 1)
            onPicked: (i) => {
              if(i === 2) {
                brg.marketModel.isExchangeMode = true;
              } else {
                brg.marketModel.isExchangeMode = false;
                brg.marketModel.isBuyMode = (i === 0);
              }
            }
          }

          // Venue: Pokemart (money) / Game Corner (coins). Irrelevant in Exchange.
          SegStrip {
            anchors.verticalCenter: parent.verticalCenter
            options: ["Pokemart", "Game Corner"]
            stripEnabled: !brg.marketModel.isExchangeMode
            currentIndex: brg.marketModel.isMoneyCurrency ? 0 : 1
            onPicked: (i) => brg.marketModel.isMoneyCurrency = (i === 0)
          }
        }
      }

      ListView {
        id: marketView
        model: brg.marketViewModel        // Buy or Sell slice of the unified list

        anchors.top: listHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        clip: true
        ScrollBar.vertical: ScrollBar {}

        footer: Item { width: 1; height: 16 }

        delegate: Item {
          id: row
          width: marketView.width
          height: dataWhichType === "msg" ? topz.headH : topz.rowH

          // Store rows and money rows report canSell == true, so this covers buys,
          // sellable items, and the exchange; only unsellable owned items fall out.
          property bool canStep: dataCanSell

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

            HoverHandler { id: rowHover }

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

              // Owned count (sell mode only; not in exchange).
              Text {
                visible: !brg.marketModel.isBuyMode && !brg.marketModel.isExchangeMode
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
                    background: Item {}
                    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhDigitsOnly

                    color: (!dataCanCheckout && dataCartCount > 0)
                           ? brg.settings.errorColor
                           : brg.settings.textColorDark

                    validator: IntValidator { bottom: 0; top: 2147483647 }

                    // Don't write back to the model during the initial programmatic
                    // fill -- only once the field is live -- so creating a delegate
                    // never mutates the model mid-incubation (re-entrancy through the
                    // view proxy crashed delegate creation on a fresh save).
                    property bool live: false
                    function getValInt() { return parseInt(text, 10); }
                    function setValInt(val) { text = val.toString(); }

                    onTextChanged: if(live && acceptableInput) dataCartCount = getValInt();
                    Component.onCompleted: { setValInt(dataCartCount); live = true; }
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
          text: brg.marketModel.isExchangeMode ? qsTr("Exchange") : qsTr("Cart")
          font.pixelSize: 18
          color: brg.settings.textColorLight

          Text {
            anchors.left: parent.right
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            visible: !brg.marketModel.isExchangeMode
                     && brg.marketModel.totalCartCount > 0
            text: "x" + brg.marketModel.totalCartCount.toLocaleString()
            font.pixelSize: 14
            color: brg.settings.textColorLight
          }
        }
      }

      // ----- BUY/SELL receipt: money on hand, itemized lines, total, balance -----
      ColumnLayout {
        visible: !brg.marketModel.isExchangeMode
        anchors.top: receiptHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        spacing: 8

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

        Rectangle { Layout.fillWidth: true; height: 1; color: brg.settings.dividerColor }

        ListView {
          id: receiptList
          Layout.fillWidth: true
          Layout.fillHeight: true
          model: brg.marketCartModel
          clip: true
          ScrollBar.vertical: ScrollBar {}

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
              anchors.rightMargin: 16
              anchors.verticalCenter: parent.verticalCenter
              spacing: 2

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
                  // Per-row sign: + for a sell (money in), - for a buy (money out).
                  text: (dataCartSign > 0 ? "+ " : "- ")
                        + topz.moneyStr(dataCartWorth, true, false, dataWhichType)
                  font.pixelSize: 14
                  color: topz.moneyColor(dataCartWorth, false,
                                         topz.curSym(dataWhichType))
                  horizontalAlignment: Text.AlignRight
                }
              }

              Text {
                text: "x" + dataCartCount.toLocaleString() + "  @ "
                      + topz.curSym(dataWhichType) + dataItemWorth.toLocaleString()
                font.pixelSize: 12
                color: brg.settings.textColorMid
              }
            }

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

        Rectangle { Layout.fillWidth: true; height: 1; color: brg.settings.dividerColor }

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
            // Signed net of the whole cart (sells +, buys -).
            text: (brg.marketModel.totalCartWorth >= 0 ? "+ " : "- ")
                  + topz.moneyStr(brg.marketModel.totalCartWorth, true, false)
            font.pixelSize: 16
            font.bold: true
            color: topz.moneyColor(brg.marketModel.totalCartWorth)
            horizontalAlignment: Text.AlignRight
          }
        }

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

      // ----- EXCHANGE receipt: dual-currency before -> after for money AND coins ---
      ColumnLayout {
        visible: brg.marketModel.isExchangeMode
        anchors.top: receiptHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        spacing: 14

        // Money balance row.
        ColumnLayout {
          Layout.fillWidth: true
          spacing: 2
          Text {
            text: qsTr("Money")
            font.pixelSize: 13
            color: brg.settings.textColorMid
          }
          RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
              text: "₽" + brg.marketModel.exchangeMoneyStart.toLocaleString()
              font.pixelSize: 15
              color: brg.settings.textColorMid
            }
            Text { text: "→"; font.pixelSize: 15; color: brg.settings.textColorMid }
            Text {
              text: "₽" + brg.marketModel.exchangeMoneyAfter.toLocaleString()
              font.pixelSize: 15
              font.bold: true
              color: (brg.marketModel.exchangeMoneyAfter < 0
                      || brg.marketModel.exchangeMoneyAfter > 999999)
                     ? brg.settings.errorColor : brg.settings.textColorDark
            }
            Item { Layout.fillWidth: true }
            Text {
              visible: brg.marketModel.exchangeMoneyAfter !== brg.marketModel.exchangeMoneyStart
              text: topz.deltaStr(brg.marketModel.exchangeMoneyAfter,
                                  brg.marketModel.exchangeMoneyStart, "₽")
              font.pixelSize: 13
              color: brg.settings.textColorMid
            }
          }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: brg.settings.dividerColor }

        // Coins balance row.
        ColumnLayout {
          Layout.fillWidth: true
          spacing: 2
          Text {
            text: qsTr("Coins")
            font.pixelSize: 13
            color: brg.settings.textColorMid
          }
          RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
              text: "⭘" + brg.marketModel.exchangeCoinsStart.toLocaleString()
              font.pixelSize: 15
              color: brg.settings.textColorMid
            }
            Text { text: "→"; font.pixelSize: 15; color: brg.settings.textColorMid }
            Text {
              text: "⭘" + brg.marketModel.exchangeCoinsAfter.toLocaleString()
              font.pixelSize: 15
              font.bold: true
              color: (brg.marketModel.exchangeCoinsAfter < 0
                      || brg.marketModel.exchangeCoinsAfter > 9999)
                     ? brg.settings.errorColor : brg.settings.textColorDark
            }
            Item { Layout.fillWidth: true }
            Text {
              visible: brg.marketModel.exchangeCoinsAfter !== brg.marketModel.exchangeCoinsStart
              text: topz.deltaStr(brg.marketModel.exchangeCoinsAfter,
                                  brg.marketModel.exchangeCoinsStart, "⭘")
              font.pixelSize: 13
              color: brg.settings.textColorMid
            }
          }
        }

        // Push the warning to the bottom.
        Item { Layout.fillWidth: true; Layout.fillHeight: true }

        Text {
          Layout.fillWidth: true
          visible: topz.exchangeWarningText() !== ""
          text: topz.exchangeWarningText()
          font.pixelSize: 13
          color: brg.settings.errorColor
          wrapMode: Text.WordWrap
          horizontalAlignment: Text.AlignHCenter
        }
      }
    }
  }

  // ---- Footer: a single Checkout button. ----
  footer: AppFooterBtn1 {
    btn1.enabled: brg.marketModel.canAnyCheckout
    icon1.source: "qrc:/assets/icons/fontawesome/shopping-cart.svg"
    text1: "Checkout"
    onBtn1Clicked: brg.marketModel.checkout()
  }
}
