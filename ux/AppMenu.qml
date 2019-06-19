import QtQuick 2.0
import QtQuick.Controls 2.12

MenuBar {
    Menu {
        title: "File"
        Action {
            text: "New"
        }
        Action {
            text: "Open"
        }
        Action {
            text: "Re-Open"
        }
        Menu {
            title: "Recent Files"

            Action {
                text: "Clear List"
            }
            MenuSeparator {
            }
            Action {
                text: "Empty"
                enabled: false
            }
        }
        MenuSeparator {
        }
        Action {
            text: "Save"
        }
        Action {
            text: "Save As"
        }
        Action {
            text: "Save Copy As"
        }
        MenuSeparator {
        }
        Action {
            text: "Wipe Unused Space"
        }
        MenuSeparator {
        }
        Action {
            text: "Exit"
        }
    }
}
