// BadgesModel.qml -- the eight Kanto gym badges, in order.
//
// A ListModel of {iconOffSrc, iconOnSrc, tooltipText} for the badge grid
// (ListBadges), ordered Boulder -> Cascade -> Thunder -> Rainbow -> Soul -> Marsh
// -> Volcano -> Earth (matching the badge bitfield's bit order). iconOnSrc (earned)
// is the badge icon; iconOffSrc (unearned) is the gym leader's shadow.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ListModel {
  ListElement {
    iconOffSrc: "qrc:/assets/images/badges/brock-shadow.png"
    iconOnSrc: "qrc:/assets/images/badges/boulder-badge.png"
    tooltipText: "Brock / Boulder Badge"
  }
  ListElement {
    iconOffSrc: "qrc:/assets/images/badges/misty-shadow.png"
    iconOnSrc: "qrc:/assets/images/badges/cascade-badge.png"
    tooltipText: "Misty / Cascade Badge"
  }
  ListElement {
    iconOffSrc: "qrc:/assets/images/badges/ltsurge-shadow.png"
    iconOnSrc: "qrc:/assets/images/badges/thunder-badge.png"
    tooltipText: "Lt. Surge / Thunder Badge"
  }
  ListElement {
    iconOffSrc: "qrc:/assets/images/badges/erika-shadow.png"
    iconOnSrc: "qrc:/assets/images/badges/rainbow-badge.png"
    tooltipText: "Erika / Rainbow Badge"
  }
  ListElement {
    iconOffSrc: "qrc:/assets/images/badges/koga-shadow.png"
    iconOnSrc: "qrc:/assets/images/badges/soul-badge.png"
    tooltipText: "Koga / Soul Badge"
  }
  ListElement {
    iconOffSrc: "qrc:/assets/images/badges/sabrina-shadow.png"
    iconOnSrc: "qrc:/assets/images/badges/marsh-badge.png"
    tooltipText: "Sabrina / Marsh Badge"
  }
  ListElement {
    iconOffSrc: "qrc:/assets/images/badges/blaine-shadow.png"
    iconOnSrc: "qrc:/assets/images/badges/volcano-badge.png"
    tooltipText: "Blaine / Volcano Badge"
  }
  ListElement {
    iconOffSrc: "qrc:/assets/images/badges/giovanni-shadow.png"
    iconOnSrc: "qrc:/assets/images/badges/earth-badge.png"
    tooltipText: "Giovanni / Earth Badge"
  }
}
