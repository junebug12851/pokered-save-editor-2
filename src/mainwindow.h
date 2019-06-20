#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "filemanagement.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void reUpdateRecentFiles(QList<QString>* files);
    void onRecentFileClick();
    void onPathChanged(QString path);

private:
    Ui::MainWindow ui;
    FileManagement file;
};

#endif // MAINWINDOW_H
