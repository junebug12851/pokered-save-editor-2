#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QSettings>

#include "ui_mainwindow.h"

class FileManagement;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

    static MainWindow* getInstance();

    FileManagement* file = nullptr;

private slots:
    void reUpdateRecentFiles(QList<QString> files);
    void onRecentFileClick();
    void onPathChanged(QString path);

private:
    Ui::MainWindow ui;
    QSettings settings;
    void closeEvent(QCloseEvent* event);

    void saveState();
    void loadState();

    static MainWindow* instance;
};

#endif // MAINWINDOW_H
