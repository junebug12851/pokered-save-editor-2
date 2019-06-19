#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    ui.setupUi(this);

    // Funnily enough we can't enter this into the shortcut capturer
    // Because it exits all of Qt lol
    ui.actionExit->setShortcut(Qt::ALT + Qt::Key_F4);
}
