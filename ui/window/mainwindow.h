#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QSettings>

#include "ui_mainwindow.h"
#include "../../src/data/file/filemanagement.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    static MainWindow* instance();

private slots:
    void reUpdateRecentFiles(QList<QString>* files);
    void onRecentFileClick();
    void onPathChanged(QString path);

private:
    Ui::MainWindow ui;
    FileManagement file;
    QSettings settings;
    void closeEvent(QCloseEvent* event);

    void saveState();
    void loadState();

    static MainWindow* _instance;
};

#endif // MAINWINDOW_H
