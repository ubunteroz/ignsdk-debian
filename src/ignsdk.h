// ibnu.yahya@toroo.org
#ifndef IGNSDK_H
#define IGNSDK_H

#include "download.h"
#include "filesystem.h"
#include "json.h"
#include "network.h"
#include "sql.h"
#include "system.h"
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkProxy>
#include <QObject>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QSize>
#include <QThread>
#include <QTimer>
#include <QVariant>
#include <QtNetwork/QNetworkInterface>
#include <QtWebKitWidgets/QtWebKitWidgets>
#include <QtWidgets/QLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSplashScreen>
#include <QtWidgets/QWidget>

class ign : public QObject {
  Q_OBJECT

private:
  QWebView web;
  QWebFrame *frame;
  QFileSystemWatcher live;
  QPrinter printer;
  QString pathLive;
  QString version;
  QtDownload *dl;
  QPoint offset;
  ignsql *m_sql;
  ignsystem *m_system;
  ignfilesystem *m_filesystem;
  ignnetwork *m_network;
  ignjson *m_json;
  bool mMoving;
  bool fullscreen = false;
  bool enableLiveCode = false;
  bool debugging = false;
  void liveCode();

public:
  ign(QObject *parent = 0);
  void render(QString w);
  void show();
  QString pathApp;

signals:
  void downloadProgress(qint64 received, qint64 total);

private slots:
  void fileChanged(const QString &path) {
    qDebug() << "Changes detected on" << path;
    QThread::msleep(50);
    this->web.page()->triggerAction(QWebPage::Reload, true);
  }

public slots:
  void ignJS();
  void setUrl(const QString &url);

  // Message box
  QString showMessageBox(const QVariant &config);

  // Print support
  bool print();
  bool print(const QVariant &config);

  // Development mode
  void setDev(bool v);
  void setDevRemote(int port);

  // Action
  void quit();
  void back();
  void forward();
  void reload();
  void stop();
  void cut();
  void copy();
  void paste();
  void undo();
  void redo();

  // Window manipulation
  void widgetSizeMax(int w, int h);
  void widgetSizeMin(int w, int h);
  void widgetSize(int w, int h);
  void widgetNoTaskbar();
  void widgetNoFrame();
  void widgetTransparent();
  void getToggleFullScreen();
  void getFullScreen(bool screen);
  void showMaximized();
  void showMinimized();

  // Load executable file in bin/
  QString loadBin(const QString &script);

  // Load app configuration
  void config(QString path);

  // Websecurity
  void websecurity(bool c);

  // Network
  void saveFile(const QByteArray &data, QString filename, QString path);
  void download(QString data, QString path);
  void download_signal(qint64 received, qint64 total);

  // Evaluate external JS
  void include(QString path);

  // Filesystem
  QObject *filesystem();

  // SQL
  QObject *sql();

  // System
  QObject *system();
  QObject *sys();

  // Network
  QObject *network();
  QObject *net();

  // IGNSDK Version
  QString sdkVersion();
};

#endif // IGN_H
