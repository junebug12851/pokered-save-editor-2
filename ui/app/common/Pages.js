.pragma library

var screens = {
  "": {
    title: "",
    body: "",
    footerBtns: 1,
    footer: "./sections/sub/Footer1Randomize.qml"
  },

  newFile: {
    modal: true,
    body: "./screens/modal/NewFile.qml"
  },

  fileTools: {
    modal: true,
    body: "./screens/modal/FileTools.qml"
  },

  about: {
    modal: true,
    body: "./screens/modal/AboutScreen.qml"
  },

  home: {
    title: "Home",
    body: "./screens/non-modal/HomeScreen.qml",
    footerBtns: 1,
    footer: "./sections/sub/Footer1Randomize.qml"
  },

  trainerCard: {
    title: "Trainer Card",
    body: "./screens/non-modal/TrainerCardScreen.qml",
    footerBtns: 1,
    footer: "./sections/sub/Footer1Randomize.qml"
  }
}
