#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    ui.setupUi(this);
    MainWindow::_instance = this;

    connect(ui.actionNew, &QAction::triggered, &file, &FileManagement::newFile);
    connect(ui.actionOpen, &QAction::triggered, &file, &FileManagement::openFile);
    connect(ui.actionRe_Open, &QAction::triggered, &file, &FileManagement::reopenFile);
    connect(ui.actionClear_Recent_Files, &QAction::triggered, &file, &FileManagement::clearRecentFiles);
    connect(ui.actionSave, &QAction::triggered, &file, &FileManagement::saveFile);
    connect(ui.actionSave_As, &QAction::triggered, &file, &FileManagement::saveFileAs);
    connect(ui.actionSave_Copy_As, &QAction::triggered, &file, &FileManagement::saveFileCopy);
    connect(ui.actionWipe_Unused_Space, &QAction::triggered, &file, &FileManagement::wipeUnusedSpace);
    connect(ui.actionExit, &QAction::triggered, this, &MainWindow::close);

    connect(&file, &FileManagement::pathChanged, this, &MainWindow::onPathChanged);
    connect(&file, &FileManagement::recentFilesChanged, this, &MainWindow::reUpdateRecentFiles);

    this->reUpdateRecentFiles(file.recentFiles());
    this->onPathChanged(file.path());
}

MainWindow* MainWindow::_instance = nullptr;

MainWindow *MainWindow::instance()
{
    return MainWindow::_instance;
}

void MainWindow::reUpdateRecentFiles(QList<QString>* files)
{
    // Grab Recent Files Menu
    QMenu* filesMenu = ui.menuRecent_Files;

    // Disconnect Signal Slot connections from the 3rd onward
    for(int i = 3; i < filesMenu->actions().size(); i++) {
        QAction* action = filesMenu->actions()[i];
        disconnect(action, &QAction::triggered, this, &MainWindow::onRecentFileClick);
    }

    // Remove all but first 3 menu items which are
    // clear recents, the seperator, and no files msg
    // hide no files msg
    while(filesMenu->actions().size() > 3) {
        filesMenu->removeAction(filesMenu->actions().last());
    }
    filesMenu->actions()[2]->setVisible(false);

    // Grab list of recent files and loop through them
    // Add actions for each one of them
    for(int i = 0; i < MAX_RECENT_FILES && i < files->size(); i++) {
        QString file = files->at(i);
        if(file == "")
            continue;

        QAction* recentFile = new QAction(file);

        // Key_0 is 0x30 - add i to 0x30 to get shortcut offset
        int shortcutKey = 0x30 + i;
        recentFile->setShortcut(Qt::CTRL + Qt::SHIFT + shortcutKey);
        recentFile->setData(i);

        connect(recentFile, &QAction::triggered, this, &MainWindow::onRecentFileClick);
        filesMenu->addAction(recentFile);
    }

    // Re-show no-files message if no recents
    if(filesMenu->actions().size() <= 3)
        filesMenu->actions()[2]->setVisible(true);
}

void MainWindow::onRecentFileClick()
{
    QAction* action = qobject_cast<QAction*>(sender());
    int index = action->data().toInt();
    file.openFileRecent(index);
}

void MainWindow::onPathChanged(QString path)
{
    if(path == "")
        this->setWindowTitle("Pokered Save Editor - New File");
    else
        this->setWindowTitle("Pokered Save Editor - " + path);
}
