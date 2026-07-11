// Pokemart.qml -- the Market screen: buy / sell / exchange (Pokemart & Game Corner).
//
// A full-width header holds two segmented strips -- action (Buy / Sell) and venue
// (Pokemart / Game Corner / Exchange). Below it, the body switches:
//   * SHOP (Buy/Sell) -- two panes: the item list (left, wider) over
//     brg.marketViewModel and a store-style receipt (right) over brg.marketCartModel.
//     One single-currency cart holds buys AND sells; the receipt nets them (+ sell,
//     - buy) to one total.
//   * EXCHANGE -- a focused money<->coins CONVERTER card: both balances with the two
//     swap lanes (spend -> get) and a live resulting balance for each currency.
// The footer is a single Checkout button (commits whichever cart is active). The JS
// helpers up top handle currency formatting/clamping.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: topz

  // Non-visual automation hook for the debug control channel (same idea as the trainer
  // card's `trainerMoneyField`): the harness can drive straight to an Exchange sub-tab
  // without simulating clicks --  `set marketPage debugExchangeSub <0|1|2>`
  // (0 Currency, 1 Healing, 2 Custom). -1 leaves the screen untouched.
  objectName: "marketPage"
  property int debugExchangeSub: -1
  onDebugExchangeSubChanged: {
    if(debugExchangeSub < 0)
      return;
    brg.marketModel.isExchangeMode = true;
    exchangeArea.subTab = debugExchangeSub;
    if(debugExchangeSub >= 1)
      itemConv.enterTab(debugExchangeSub === 1);
  }

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

    // Colors default to the accent-bar look (light outline/text, light selected
    // fill + accent selected text). A strip on a LIGHT surface (the Exchange
    // sub-tab bar) overrides these for a dark outline/text + accent selected fill.
    property color stripBorder: Qt.rgba(1, 1, 1, 0.55)
    property color dividerCol: Qt.rgba(1, 1, 1, 0.45)
    property color selFill: brg.settings.textColorLight
    property color selText: brg.settings.accentColor
    property color unselText: brg.settings.textColorLight

    implicitHeight: 28
    implicitWidth: segRow.implicitWidth
    radius: 6
    color: "transparent"
    border.width: 1
    border.color: stripBorder
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

          // Selected segment fills light; round the OUTER corners on the end
          // segments so the fill follows the strip's rounded border (`clip` is
          // rectangular and won't round child corners).
          Rectangle {
            anchors.fill: parent
            color: segItem.index === seg.currentIndex ? seg.selFill : "transparent"
            topLeftRadius: segItem.index === 0 ? seg.radius : 0
            bottomLeftRadius: segItem.index === 0 ? seg.radius : 0
            topRightRadius: segItem.index === (seg.options.length - 1) ? seg.radius : 0
            bottomRightRadius: segItem.index === (seg.options.length - 1) ? seg.radius : 0
          }

          Rectangle {
            visible: segItem.index > 0
            anchors.left: parent.left
            width: 1
            height: parent.height
            color: seg.dividerCol
          }

          Text {
            id: segLabel
            anchors.centerIn: parent
            text: segItem.modelData
            font.pixelSize: 13
            font.bold: segItem.index === seg.currentIndex
            color: segItem.index === seg.currentIndex ? seg.selText : seg.unselText
          }

          MouseArea {
            // objectName lets the debug control channel click a segment by label.
            objectName: "segmouse_" + segItem.modelData
            anchors.fill: parent
            enabled: seg.stripEnabled
            cursorShape: Qt.PointingHandCursor
            onClicked: seg.picked(segItem.index)
          }
        }
      }
    }
  }

  // ---- An inline -/qty/+ stepper pill bound to a model role (shop rows + lanes) ---
  component StepPill: Rectangle {
    id: pill
    property int value: 0          // current amount (the model's cart count)
    property int onLeft: 0         // how many MORE can be added (dataOnCartLeft)
    property bool errored: false
    signal commit(int v)           // user changed the amount

    implicitHeight: 30
    implicitWidth: pillRow.width + 8
    radius: 6
    color: Qt.rgba(0, 0, 0, 0.03)
    border.width: 1
    border.color: Qt.rgba(0, 0, 0, 0.10)

    Row {
      id: pillRow
      anchors.centerIn: parent

      IconButtonSquare {
        icon.source: "qrc:/assets/icons/fontawesome/minus.svg"
        enabled: !(pill.onLeft === 0 && field.getValInt() === 0)
        onClicked: {
          if(field.getValInt() === 0 && pill.onLeft > 0)
            field.setValInt(pill.onLeft);
          else if(field.getValInt() > 0)
            field.setValInt(field.getValInt() - 1);
        }
      }

      DefTextEdit {
        id: field
        anchors.verticalCenter: parent.verticalCenter
        height: 30
        width: topz.qtyW
        horizontalAlignment: TextInput.AlignHCenter
        labelEl.visible: false
        background: Item {}
        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhDigitsOnly
        color: pill.errored ? brg.settings.errorColor : brg.settings.textColorDark
        validator: IntValidator { bottom: 0; top: 2147483647 }

        // Don't write back during the initial fill (re-entrancy through a proxy can
        // crash delegate creation) -- only once live.
        property bool live: false
        function getValInt() { return parseInt(text, 10); }
        function setValInt(val) { text = val.toString(); }
        onTextChanged: if(live && acceptableInput) pill.commit(getValInt());
        Component.onCompleted: { setValInt(pill.value); live = true; }
        // Re-seat if the model value changes out from under us (e.g. a reset).
        Connections {
          target: pill
          function onValueChanged() {
            if(field.live && field.getValInt() !== pill.value)
              field.setValInt(pill.value);
          }
        }
      }

      IconButtonSquare {
        icon.source: "qrc:/assets/icons/fontawesome/plus.svg"
        enabled: !(pill.onLeft === 0 && field.getValInt() === 0)
        onClicked: {
          if(field.getValInt() > 0 && pill.onLeft === 0)
            field.setValInt(0);
          else if(pill.onLeft > 0)
            field.setValInt(field.getValInt() + 1);
        }
      }
    }
  }

  // ---- Currency formatting helpers -------------------------------------------

  function maxMoney(sym) {
    if(sym === "₽" || brg.marketModel.isMoneyCurrency)
      return 999999;
    return 9999;
  }

  function maxMoneyNeg(sym) { return -maxMoney(sym); }

  function moneyOutOfRange(val, sym) { return Math.abs(val) > maxMoney(sym); }

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

    var sign = useSigning === true ? (brg.marketModel.isBuyMode ? "-" : "+") : "";

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

  // ============================================================================
  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    // ---- Full-width header: the two segmented mode strips. ----
    Rectangle {
      id: headerBar
      Layout.fillWidth: true
      Layout.preferredHeight: 45
      color: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight
      Material.background: brg.settings.accentColor

      Row {
        anchors.centerIn: parent
        spacing: 14

        // Action: Buy / Sell. Disabled while Exchange is selected.
        SegStrip {
          anchors.verticalCenter: parent.verticalCenter
          options: ["Buy", "Sell"]
          stripEnabled: !brg.marketModel.isExchangeMode
          currentIndex: brg.marketModel.isBuyMode ? 0 : 1
          onPicked: (i) => brg.marketModel.isBuyMode = (i === 0)
        }

        // Venue: Pokemart (money) / Game Corner (coins) / Exchange (money<->coins).
        SegStrip {
          anchors.verticalCenter: parent.verticalCenter
          options: ["Poké Mart", "Game Corner", "Exchange"]
          currentIndex: brg.marketModel.isExchangeMode
                        ? 2 : (brg.marketModel.isMoneyCurrency ? 0 : 1)
          onPicked: (i) => {
            if(i === 2) {
              brg.marketModel.isExchangeMode = true;
            } else {
              brg.marketModel.isExchangeMode = false;
              brg.marketModel.isMoneyCurrency = (i === 0);
            }
          }
        }
      }
    }

    // ---- Body: SHOP (two panes) or, in Exchange, the CONVERTER. ----
    Item {
      id: bodyArea
      Layout.fillWidth: true
      Layout.fillHeight: true

      // =================== SHOP (Buy / Sell) ===================
      RowLayout {
        id: paneRow
        anchors.fill: parent
        spacing: 0
        visible: !brg.marketModel.isExchangeMode

        // ---- LEFT: the item list ----
        Rectangle {
          id: listPane
          Layout.fillWidth: true
          Layout.fillHeight: true
          color: "white"

          ListView {
            id: marketView
            // Bound only in shop mode (avoid building shop delegates for the
            // exchange rows while hidden).
            model: brg.marketModel.isExchangeMode ? null : brg.marketViewModel
            anchors.fill: parent
            clip: true
            ScrollBar.vertical: ScrollBar {}
            footer: Item { width: 1; height: 16 }

            delegate: Item {
              id: row
              width: marketView.width
              height: dataWhichType === "msg" ? topz.headH : topz.rowH
              property bool canStep: dataCanSell

              // Section header ("msg").
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
                  anchors.left: parent.left; anchors.right: parent.right
                  anchors.bottom: parent.bottom
                  height: 1; color: brg.settings.dividerColor
                }
              }

              // Item row.
              Item {
                visible: dataWhichType !== "msg"
                anchors.fill: parent

                HoverHandler { id: rowHover }

                // Detailed (help-mode) tooltip: what the item is / how it's obtained.
                // Shows only when the header "?" toggle is on and the item has info.
                DetailToolTip {
                  title: dataName
                  text: dataInfo
                  hovered: rowHover.hovered
                }

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
                  anchors.rightMargin: 16
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

                  Text {
                    visible: !brg.marketModel.isBuyMode
                    Layout.alignment: Qt.AlignVCenter
                    text: "x" + dataInStockCount.toLocaleString()
                    font.pixelSize: 12
                    color: brg.settings.textColorMid
                    horizontalAlignment: Text.AlignRight
                  }

                  Text {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.minimumWidth: 54
                    text: topz.curSym(dataWhichType) + dataItemWorth.toLocaleString()
                    font.pixelSize: 13
                    color: row.canStep ? brg.settings.textColorDark
                                       : brg.settings.textColorMid
                    horizontalAlignment: Text.AlignRight
                  }

                  StepPill {
                    visible: row.canStep
                    Layout.alignment: Qt.AlignVCenter
                    value: dataCartCount
                    onLeft: dataOnCartLeft
                    errored: !dataCanCheckout && dataCartCount > 0
                    onCommit: (v) => dataCartCount = v
                  }

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
                  anchors.left: parent.left; anchors.right: parent.right
                  anchors.bottom: parent.bottom
                  height: 1; color: Qt.rgba(0, 0, 0, 0.06)
                }
              }
            }
          }
        }

        Rectangle {
          Layout.fillHeight: true
          Layout.preferredWidth: 1
          color: brg.settings.dividerColor
        }

        // ---- RIGHT: the receipt ----
        Rectangle {
          id: receiptPane
          Layout.fillHeight: true
          Layout.preferredWidth: Math.round(paneRow.width * 0.37)
          color: "white"

          // A tint section header matching the left list's section headers (same
          // height/style) so the two panes line up under the one accent header above.
          Rectangle {
            id: receiptHeader
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            height: topz.headH
            color: Qt.rgba(0, 0, 0, 0.045)
            Text {
              anchors.verticalCenter: parent.verticalCenter
              anchors.left: parent.left; anchors.leftMargin: 14
              text: brg.marketModel.totalCartCount > 0
                    ? qsTr("CART") + "  ×" + brg.marketModel.totalCartCount.toLocaleString()
                    : qsTr("CART")
              font.pixelSize: 12; font.bold: true
              font.capitalization: Font.AllUppercase; font.letterSpacing: 1
              color: brg.settings.textColorMid
            }
            Rectangle {
              anchors.left: parent.left; anchors.right: parent.right
              anchors.bottom: parent.bottom
              height: 1; color: brg.settings.dividerColor
            }
          }

          ColumnLayout {
            anchors.top: receiptHeader.bottom
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
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
                  anchors.left: parent.left; anchors.right: parent.right
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
                      text: (dataCartSign > 0 ? "+ " : "- ")
                            + topz.moneyStr(dataCartWorth, true, false, dataWhichType)
                      font.pixelSize: 14
                      color: topz.moneyColor(dataCartWorth, false, topz.curSym(dataWhichType))
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
                  anchors.left: parent.left; anchors.right: parent.right
                  anchors.rightMargin: 16; anchors.bottom: parent.bottom
                  height: 1; color: Qt.rgba(0, 0, 0, 0.06)
                }
              }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: brg.settings.dividerColor }

            RowLayout {
              Layout.fillWidth: true
              Text {
                Layout.fillWidth: true
                text: qsTr("Total")
                font.pixelSize: 16; font.bold: true
                color: brg.settings.textColorDark
              }
              Text {
                text: (brg.marketModel.totalCartWorth >= 0 ? "+ " : "- ")
                      + topz.moneyStr(brg.marketModel.totalCartWorth, true, false)
                font.pixelSize: 16; font.bold: true
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
        }
      }

      // =================== EXCHANGE converters ===================
      Item {
        id: exchangeArea
        anchors.fill: parent
        visible: brg.marketModel.isExchangeMode

        // Which exchange: 0 Currency (money<->coins), 1 Healing (items), 2 Custom (items).
        property int subTab: 0

        // Subtle backdrop so the card reads as a focused panel.
        Rectangle { anchors.fill: parent; color: Qt.rgba(0, 0, 0, 0.03) }

        // ---- Sub-tab bar (Currency / Healing / Custom), on the light backdrop. ----
        SegStrip {
          id: exSubStrip
          anchors.horizontalCenter: parent.horizontalCenter
          anchors.verticalCenter: parent.verticalCenter
          anchors.verticalCenterOffset: -(convCard.height / 2) - 26
          options: ["Currency", "Healing", "Custom"]
          currentIndex: exchangeArea.subTab
          // Light-surface styling (dark outline/text, accent selected fill).
          stripBorder: Qt.rgba(0, 0, 0, 0.18)
          dividerCol: Qt.rgba(0, 0, 0, 0.15)
          selFill: brg.settings.accentColor
          selText: brg.settings.textColorLight
          unselText: brg.settings.textColorMid
          onPicked: (i) => {
            exchangeArea.subTab = i;
            if(i >= 1)
              itemConv.enterTab(i === 1);   // Healing = i===1 (healing-only dropdowns)
          }
        }

        // The money<->coins converter card (Currency sub-tab).
        Rectangle {
          id: convCard
          visible: exchangeArea.subTab === 0
          anchors.horizontalCenter: parent.horizontalCenter
          anchors.verticalCenter: parent.verticalCenter
          anchors.verticalCenterOffset: 18
          width: Math.min(parent.width - 48, 560)
          height: convCol.implicitHeight + 40
          radius: 14
          color: "white"
          border.width: 1
          border.color: Qt.rgba(0, 0, 0, 0.10)

          ColumnLayout {
            id: convCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 22
            anchors.rightMargin: 22
            spacing: 16

            // ---- MONEY (left)  ⇄  COINS (right): each balance with a "+" button
            //      that pulls from the OTHER side at the per-coin rate. ----
            RowLayout {
              Layout.fillWidth: true
              spacing: 10

              // ===== MONEY (left): the "+ Money" button SELLS coins for money. =====
              ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1          // equal halves (50/50 with coins)
                Layout.alignment: Qt.AlignTop
                spacing: 4

                Text {
                  Layout.alignment: Qt.AlignHCenter
                  text: qsTr("MONEY"); font.pixelSize: 11; font.letterSpacing: 1
                  color: brg.settings.textColorMid
                }
                // Big balance with the +/- change as a small superscript at top-right.
                Item {
                  Layout.alignment: Qt.AlignHCenter
                  implicitWidth: mNum.implicitWidth
                  implicitHeight: mNum.implicitHeight
                  Text {
                    id: mNum
                    anchors.centerIn: parent
                    text: "₽" + brg.marketModel.exchangeMoneyAfter.toLocaleString()
                    font.pixelSize: 24; font.bold: true
                    color: (brg.marketModel.exchangeMoneyAfter < 0
                            || brg.marketModel.exchangeMoneyAfter > 999999)
                           ? brg.settings.errorColor : brg.settings.textColorDark
                  }
                  // Superscript delta -- OVERLAID at the number's top-right (not in
                  // the layout) so it never shifts the number as it appears / changes.
                  Text {
                    anchors.left: mNum.right; anchors.leftMargin: 2
                    anchors.top: mNum.top; anchors.topMargin: -1
                    visible: brg.marketModel.exchangeMoneyAfter !== brg.marketModel.exchangeMoneyStart
                    text: topz.deltaStr(brg.marketModel.exchangeMoneyAfter,
                                        brg.marketModel.exchangeMoneyStart, "₽")
                    font.pixelSize: 11; font.bold: true
                    color: brg.settings.textColorMid
                  }
                }
                Button {
                  Layout.fillWidth: true
                  Layout.topMargin: 4
                  text: qsTr("+ Money")
                  autoRepeat: true; autoRepeatDelay: 350; autoRepeatInterval: 70
                  Material.background: brg.settings.accentColor
                  Material.foreground: brg.settings.textColorLight
                  enabled: brg.marketModel.exchangeCoinsAfter >= 1
                           && (brg.marketModel.exchangeMoneyAfter
                               + brg.marketModel.exchangeSellRate) <= 999999
                  onClicked: brg.marketModel.exchangeAdjust(-1)   // sell 1 coin
                }
                Text {
                  Layout.alignment: Qt.AlignHCenter
                  text: "₽" + brg.marketModel.exchangeSellRate + qsTr(" / coin")
                  font.pixelSize: 11
                  color: brg.settings.textColorMid
                }
              }

              Text {
                Layout.alignment: Qt.AlignVCenter
                text: "⇄"; font.pixelSize: 26
                color: brg.settings.accentColor
              }

              // ===== COINS (right): the "+ Coins" button BUYS coins with money. =====
              ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1          // equal halves (50/50 with money)
                Layout.alignment: Qt.AlignTop
                spacing: 4

                Text {
                  Layout.alignment: Qt.AlignHCenter
                  text: qsTr("COINS"); font.pixelSize: 11; font.letterSpacing: 1
                  color: brg.settings.textColorMid
                }
                Item {
                  Layout.alignment: Qt.AlignHCenter
                  implicitWidth: cNum.implicitWidth
                  implicitHeight: cNum.implicitHeight
                  Text {
                    id: cNum
                    anchors.centerIn: parent
                    text: "⭘" + brg.marketModel.exchangeCoinsAfter.toLocaleString()
                    font.pixelSize: 24; font.bold: true
                    color: (brg.marketModel.exchangeCoinsAfter < 0
                            || brg.marketModel.exchangeCoinsAfter > 9999)
                           ? brg.settings.errorColor : brg.settings.textColorDark
                  }
                  Text {
                    anchors.left: cNum.right; anchors.leftMargin: 2
                    anchors.top: cNum.top; anchors.topMargin: -1
                    visible: brg.marketModel.exchangeCoinsAfter !== brg.marketModel.exchangeCoinsStart
                    text: topz.deltaStr(brg.marketModel.exchangeCoinsAfter,
                                        brg.marketModel.exchangeCoinsStart, "⭘")
                    font.pixelSize: 11; font.bold: true
                    color: brg.settings.textColorMid
                  }
                }
                Button {
                  Layout.fillWidth: true
                  Layout.topMargin: 4
                  text: qsTr("+ Coins")
                  autoRepeat: true; autoRepeatDelay: 350; autoRepeatInterval: 70
                  Material.background: brg.settings.accentColor
                  Material.foreground: brg.settings.textColorLight
                  enabled: brg.marketModel.exchangeMoneyAfter >= brg.marketModel.exchangeBuyRate
                           && brg.marketModel.exchangeCoinsAfter < 9999
                  onClicked: brg.marketModel.exchangeAdjust(1)    // buy 1 coin
                }
                Text {
                  Layout.alignment: Qt.AlignHCenter
                  text: "₽" + brg.marketModel.exchangeBuyRate + qsTr(" / coin")
                  font.pixelSize: 11
                  color: brg.settings.textColorMid
                }
              }
            }

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

        // The item<->item converter card (Healing / Custom sub-tabs), driven by
        // brg.itemExchangeModel. The LEFT side is what you give (a dropdown of the items
        // you own); the RIGHT side is what you get (every item -- the ones your stock
        // can't cover are greyed, which keeps at least one "+" always live).
        QtObject {
          id: itemConv
          // Sensible starting pair for the entered tab (Healing opens on Potion <=> Fresh Water).
          function enterTab(healingOnly) {
            brg.itemExchangeModel.pickDefaults(healingOnly);
          }
        }

        Rectangle {
          id: itemCard
          visible: exchangeArea.subTab >= 1
          anchors.horizontalCenter: parent.horizontalCenter
          anchors.verticalCenter: parent.verticalCenter
          anchors.verticalCenterOffset: 18
          width: Math.min(parent.width - 48, 560)
          height: itemCol.implicitHeight + 40
          radius: 14
          color: "white"
          border.width: 1
          border.color: Qt.rgba(0, 0, 0, 0.10)

          // A borderless item dropdown. `targetSide` picks which list it shows: the give
          // side lists owned items, the get side lists every item (unaffordable ones
          // greyed + unselectable). Mutual exclusion via excludeInd.
          component ItemCombo: ComboBox {
            id: ic
            property bool healingOnly: false
            property bool targetSide: false    // false = give (owned), true = get (all)
            property int excludeInd: -1
            property int boundInd: -1          // the model value this side owns
            signal chose(int ind)

            // Automation hook (debug channel): a ComboBox has no clicked() to drive, so
            // the harness flips this to inspect the open list.
            property bool debugPopup: false
            onDebugPopupChanged: debugPopup ? ic.popup.open() : ic.popup.close()

            flat: true
            font.capitalization: Font.Capitalize
            font.pixelSize: 15
            textRole: "name"
            valueRole: "ind"
            implicitWidth: font.pixelSize * 11

            // sourceItems/targetItems are invokables, so the binding has nothing to
            // re-run on by itself -- reading `revision` gives it a dependency, so the
            // list (and the get-side `affordable` flags) rebuilds whenever state moves.
            model: {
              var exm = brg.itemExchangeModel;
              var rev = exm.revision;   // dependency -- do not remove
              return ic.targetSide ? exm.targetItems(ic.healingOnly, ic.excludeInd)
                                   : exm.sourceItems(ic.healingOnly, ic.excludeInd);
            }

            // indexOfValue() doesn't resolve against a plain QVariantList model, so
            // find the row for our bound item id ourselves (else currentIndex stays
            // -1 and the combo renders with no text).
            currentIndex: {
              var m = ic.model;
              if(!m)
                return -1;
              for(var i = 0; i < m.length; i++) {
                if(m[i].ind === ic.boundInd)
                  return i;
              }
              return -1;
            }
            onActivated: (i) => ic.chose(ic.model[i].ind)

            // Greys (and blocks) the items the player's stock can't cover -- only the
            // get side carries `affordable`, so the give side's rows are always enabled.
            delegate: ItemDelegate {
              id: icDel
              required property var modelData
              required property int index
              width: ic.width
              enabled: modelData.affordable === undefined || modelData.affordable === true
              highlighted: ic.highlightedIndex === icDel.index
              contentItem: Text {
                text: icDel.modelData.name
                font: ic.font
                color: brg.settings.textColorDark
                opacity: icDel.enabled ? 1.0 : 0.38
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
              }
            }

            background: Rectangle {
              color: "transparent"
              Rectangle {
                anchors.bottom: parent.bottom; width: parent.width; height: 2
                visible: ic.hovered; color: brg.settings.accentColor
              }
            }
          }

          ColumnLayout {
            id: itemCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 22; anchors.rightMargin: 22
            spacing: 14

            property var exm: brg.itemExchangeModel
            property bool healingOnly: exchangeArea.subTab === 1

            // ---- Item A  ⇄  Item B ----
            RowLayout {
              Layout.fillWidth: true
              spacing: 10

              // ===== Item A -- the GIVE side (your items). "+A" gains A, spends B. =====
              ColumnLayout {
                Layout.fillWidth: true; Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                spacing: 4

                ItemCombo {
                  Layout.alignment: Qt.AlignHCenter
                  Layout.maximumWidth: parent.width
                  healingOnly: itemCol.healingOnly
                  excludeInd: itemCol.exm.itemBInd
                  boundInd: itemCol.exm.itemAInd
                  onChose: (ind) => itemCol.exm.itemAInd = ind
                }
                Item {
                  Layout.alignment: Qt.AlignHCenter
                  implicitWidth: aNum.implicitWidth; implicitHeight: aNum.implicitHeight
                  Text {
                    id: aNum
                    anchors.centerIn: parent
                    text: "x" + itemCol.exm.aAfter.toLocaleString()
                    font.pixelSize: 24; font.bold: true
                    color: brg.settings.textColorDark
                  }
                  Text {
                    anchors.left: aNum.right; anchors.leftMargin: 2
                    anchors.top: aNum.top; anchors.topMargin: -1
                    visible: itemCol.exm.aAfter !== itemCol.exm.aStart
                    text: topz.deltaStr(itemCol.exm.aAfter, itemCol.exm.aStart, "")
                    font.pixelSize: 11; font.bold: true
                    color: brg.settings.textColorMid
                  }
                }
                Button {
                  objectName: "exAddA"   // automation hook for the debug channel
                  Layout.fillWidth: true; Layout.topMargin: 4
                  text: qsTr("+ ") + (itemCol.exm.aName !== "" ? itemCol.exm.aName : qsTr("Item"))
                  autoRepeat: true; autoRepeatDelay: 350; autoRepeatInterval: 70
                  Material.background: brg.settings.accentColor
                  Material.foreground: brg.settings.textColorLight
                  enabled: itemCol.exm.canAddA
                  onClicked: itemCol.exm.adjust(1)
                }
                Text {
                  Layout.alignment: Qt.AlignHCenter
                  visible: itemCol.exm.valid
                  text: "-" + itemCol.exm.perAGive + " " + itemCol.exm.bName
                        + "  +₽" + itemCol.exm.perARefund
                  font.pixelSize: 11
                  color: brg.settings.textColorMid
                  horizontalAlignment: Text.AlignHCenter
                }
              }

              Text {
                Layout.alignment: Qt.AlignVCenter
                text: "⇄"; font.pixelSize: 26; color: brg.settings.accentColor
              }

              // ===== Item B -- the GET side (every item). "+B" gains B, spends A. =====
              ColumnLayout {
                Layout.fillWidth: true; Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                spacing: 4

                ItemCombo {
                  objectName: "exGetCombo"       // automation hook for the debug channel
                  Layout.alignment: Qt.AlignHCenter
                  Layout.maximumWidth: parent.width
                  healingOnly: itemCol.healingOnly
                  targetSide: true               // "get" side -- every item, unaffordable greyed
                  excludeInd: itemCol.exm.itemAInd
                  boundInd: itemCol.exm.itemBInd
                  onChose: (ind) => itemCol.exm.itemBInd = ind
                }
                Item {
                  Layout.alignment: Qt.AlignHCenter
                  implicitWidth: bNum.implicitWidth; implicitHeight: bNum.implicitHeight
                  Text {
                    id: bNum
                    anchors.centerIn: parent
                    text: "x" + itemCol.exm.bAfter.toLocaleString()
                    font.pixelSize: 24; font.bold: true
                    color: brg.settings.textColorDark
                  }
                  Text {
                    anchors.left: bNum.right; anchors.leftMargin: 2
                    anchors.top: bNum.top; anchors.topMargin: -1
                    visible: itemCol.exm.bAfter !== itemCol.exm.bStart
                    text: topz.deltaStr(itemCol.exm.bAfter, itemCol.exm.bStart, "")
                    font.pixelSize: 11; font.bold: true
                    color: brg.settings.textColorMid
                  }
                }
                Button {
                  objectName: "exAddB"   // automation hook for the debug channel
                  Layout.fillWidth: true; Layout.topMargin: 4
                  text: qsTr("+ ") + (itemCol.exm.bName !== "" ? itemCol.exm.bName : qsTr("Item"))
                  autoRepeat: true; autoRepeatDelay: 350; autoRepeatInterval: 70
                  Material.background: brg.settings.accentColor
                  Material.foreground: brg.settings.textColorLight
                  enabled: itemCol.exm.canAddB
                  onClicked: itemCol.exm.adjust(-1)
                }
                Text {
                  Layout.alignment: Qt.AlignHCenter
                  visible: itemCol.exm.valid
                  text: "-" + itemCol.exm.perBGive + " " + itemCol.exm.aName
                        + "  +₽" + itemCol.exm.perBRefund
                  font.pixelSize: 11
                  color: brg.settings.textColorMid
                  horizontalAlignment: Text.AlignHCenter
                }
              }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: brg.settings.dividerColor }

            // ---- Money refunded ----
            RowLayout {
              Layout.fillWidth: true
              Text {
                Layout.fillWidth: true
                text: qsTr("Money")
                font.pixelSize: 14; color: brg.settings.textColorMid
              }
              Text {
                text: "₽" + itemCol.exm.moneyAfter.toLocaleString()
                      + (itemCol.exm.moneyAfter !== itemCol.exm.moneyStart
                         ? "  " + topz.deltaStr(itemCol.exm.moneyAfter, itemCol.exm.moneyStart, "₽")
                         : "")
                font.pixelSize: 14
                color: brg.settings.textColorDark
                horizontalAlignment: Text.AlignRight
              }
            }

            Text {
              Layout.fillWidth: true
              visible: !itemCol.exm.valid
              text: qsTr("Pick two different items you own to exchange.")
              font.pixelSize: 12; font.italic: true
              color: brg.settings.textColorMid
              wrapMode: Text.WordWrap
              horizontalAlignment: Text.AlignHCenter
            }
          }
        }
      }
    }
  }

  // ---- Footer: a single Checkout button (commits whichever cart is active). ----
  // On the Healing/Custom item-exchange sub-tabs it commits the item exchange;
  // otherwise it commits the buy/sell/currency cart.
  footer: AppFooterBtn1 {
    readonly property bool itemExchange: brg.marketModel.isExchangeMode && exchangeArea.subTab >= 1
    btn1.enabled: itemExchange ? (brg.itemExchangeModel.net !== 0)
                               : brg.marketModel.canAnyCheckout
    icon1.source: "qrc:/assets/icons/fontawesome/shopping-cart.svg"
    text1: "Checkout"
    onBtn1Clicked: itemExchange ? brg.itemExchangeModel.checkout()
                                : brg.marketModel.checkout()
  }
}
