/*
  * Copyright 2020 Twilight
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/
#pragma once
#include <QString>
#include <QHash>
#include <QShortcut>
#include <QSettings>
#include <QQmlEngine>

#include "ui_mainwindow.h"

class FileManagement;
class RecentFilesModel;
class QAbstractItemModel;
class Bridge;

class MainWindow : public QMainWindow
{
  Q_OBJECT

  Q_PROPERTY(FileManagement* file MEMBER file NOTIFY fileChanged)

public:
  MainWindow(QWidget* parent = nullptr);
  virtual ~MainWindow();

  static MainWindow* getInstance();
  static Bridge* bridge;
  static QQmlEngine* engine;

  FileManagement* file = nullptr;

  // MAX_RECENT_FILES
  QShortcut* recentFileShortcuts[5];
  QHash<QString, QShortcut*> otherShortcuts;

signals:
  void fileChanged();

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

  void setupShortcuts();
  void setupProviders();
  void injectIntoQML();
  void ssConnect();

  static MainWindow* instance;
};

