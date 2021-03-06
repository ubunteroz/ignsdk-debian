// ibnu.yahya@toroo.org

#include "cmath"
#include "filesystem.h"
#include "ignsdk.h"
#include "version.h"
#include <QtCore/QVariant>
#include <iostream>

using namespace std;

ign::ign(QObject *parent)
    : QObject(parent), m_sql(0), m_system(0), m_filesystem(0), m_network(0),
      m_json(0) {
  this->version = QStringLiteral(IGNSDK_VERSION);
  frame = web.page()->mainFrame();
  connect(frame, &QWebFrame::javaScriptWindowObjectCleared, this, &ign::ignJS);
  this->dl = new QtDownload;
}

void ign::ignJS() { this->frame->addToJavaScriptWindowObject(QStringLiteral("ign"), this); }

// Config loader
void ign::config(QString path) {
  QString config_path = path + "/ignsdk.json";
  QFile config_file;
  config_file.setFileName(config_path);
  QByteArray config;

  // Default settings
  QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled,
                                               true);
  web.settings()->setAttribute(QWebSettings::PluginsEnabled, true);
  web.settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
  web.settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled,
                               true);
  web.settings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled,
                               true);
  web.settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
  web.settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
  web.settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard,
                               true);
  web.settings()->setAttribute(QWebSettings::JavaEnabled, true);
  web.settings()->setAttribute(QWebSettings::AcceleratedCompositingEnabled,
                               true);
  web.settings()->setAttribute(QWebSettings::WebGLEnabled, true);
  web.settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls,
                               true);
  web.settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls,
                               true);

  // Disable navigation menu by default
  web.page()->action(QWebPage::Back)->setVisible(false);
  web.page()->action(QWebPage::Forward)->setVisible(false);
  web.page()->action(QWebPage::Reload)->setVisible(false);
  web.page()->action(QWebPage::Stop)->setVisible(false);

  // Disable fullscreen mode by default
  fullscreen = false;

  // Application storage
  QString appStorage = QStringLiteral("/tmp/ignsdk-app");

  if (config_file.open(QIODevice::ReadOnly)) {
    qDebug() << "Config file loaded:" << config_path;
    config = config_file.readAll();
    QJsonParseError *err = new QJsonParseError();
    QJsonDocument ignjson = QJsonDocument::fromJson(config, err);

    if (err->error != 0) {
      qDebug() << err->errorString();
      exit(1);
    }

    QJsonObject jObject = ignjson.object();
    QVariantMap result = jObject.toVariantMap();
    QVariantMap configure = result[QStringLiteral("config")].toMap();

    if (configure[QStringLiteral("package")].toString() != QLatin1String("")) {
      appStorage =
          QDir::homePath() + "/.ignsdk/" + configure[QStringLiteral("package")].toString();
    }

    if (configure[QStringLiteral("debug")].toBool()) {
      this->setDev(true);
      this->enableLiveCode = true;
    }

    if (configure[QStringLiteral("debug-port")].toInt()) {
      this->setDevRemote(configure[QStringLiteral("debug-port")].toInt());
    }

    if (configure[QStringLiteral("set-system-proxy")].toBool()) {
      QNetworkProxyFactory::setUseSystemConfiguration(true);
    }

    QVariantMap set_proxy = configure[QStringLiteral("set-proxy")].toMap();

    if (set_proxy[QStringLiteral("type")].toString() != QLatin1String("")) {
      QNetworkProxy proxy;
      QString proxy_type = set_proxy[QStringLiteral("type")].toString();
      if (proxy_type == QLatin1String("http")) {
        proxy.setType(QNetworkProxy::HttpProxy);
      } else if (proxy_type == QLatin1String("socks5")) {
        proxy.setType(QNetworkProxy::Socks5Proxy);
      } else if (proxy_type == QLatin1String("ftp")) {
        proxy.setType(QNetworkProxy::FtpCachingProxy);
      } else if (proxy_type == QLatin1String("httpCaching")) {
        proxy.setType(QNetworkProxy::HttpCachingProxy);
      } else {
        qDebug()
            << "Please input your type proxy (http,socks5,ftp,httpCaching)!";
        exit(0);
      }

      if (set_proxy[QStringLiteral("url")].toString() != QLatin1String("")) {
        QString url = set_proxy[QStringLiteral("url")].toString();
        QStringList url_proxy = url.split(QStringLiteral(":"));
        proxy.setHostName(url_proxy.at(0));
        proxy.setPort(url_proxy.at(1).toInt());
      } else {
        qDebug() << "Please input your hostname:port Ex: 127.0.0.1:8080!";
        exit(0);
      }

      if (set_proxy[QStringLiteral("username")].toString() != QLatin1String("")) {
        proxy.setUser(set_proxy[QStringLiteral("username")].toString());
      }

      if (set_proxy[QStringLiteral("password")].toString() != QLatin1String("")) {
        proxy.setPassword(set_proxy[QStringLiteral("password")].toString());
      }

      QNetworkProxy::setApplicationProxy(proxy);
    }

    if (configure[QStringLiteral("set-ignsdk-proxy")].toBool()) {
      QFile file(QStringLiteral("/etc/ignsdk-proxy.conf"));
      QString data_ign_proxy;

      if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream out(&file);
        data_ign_proxy = out.readLine();
        file.close();

        if (this->debugging) {
          qDebug() << "Enable IGNSDK global proxy setting : " << data_ign_proxy;
        }

        QStringList url_proxy = data_ign_proxy.split(QStringLiteral(":"));
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(url_proxy.at(0));
        proxy.setPort(url_proxy.at(1).toInt());

        if (url_proxy.at(2) != QLatin1String("")) {
          proxy.setUser(url_proxy.at(2));
        }

        if (url_proxy.at(3) != QLatin1String("")) {
          proxy.setPassword(url_proxy.at(3));
        }

        QNetworkProxy::setApplicationProxy(proxy);
      }
    }

    if (configure[QStringLiteral("websecurity")].toBool()) {
      this->websecurity(true);
    }

    if (configure[QStringLiteral("name")].toString() != QLatin1String("")) {
      this->web.setWindowTitle(configure[QStringLiteral("name")].toString());
    }

    QVariantMap window = result[QStringLiteral("window")].toMap();

    if (window[QStringLiteral("transparent")].toBool()) {
      this->widgetTransparent();
    }

    if (window[QStringLiteral("noframe")].toBool()) {
      this->widgetNoFrame();
    }

    if (window[QStringLiteral("notaskbar")].toBool()) {
      this->widgetNoTaskbar();
    }

    if (window[QStringLiteral("fullscreen")].toBool()) {
      this->getToggleFullScreen();
    }

    if (window[QStringLiteral("maximize")].toBool()) {
      this->showMaximized();
    }

    if (window[QStringLiteral("width")].toInt() != 0) {
      if (window[QStringLiteral("height")].toInt() != 0) {
        this->widgetSize(window[QStringLiteral("width")].toInt(), window[QStringLiteral("height")].toInt());
      }
    }

    foreach (QVariant navi, result["navigations"].toList()) {
      if (navi.toString() == QLatin1String("back")) {
        web.page()->action(QWebPage::Back)->setVisible(true);
      }

      if (navi.toString() == QLatin1String("forward")) {
        web.page()->action(QWebPage::Forward)->setVisible(true);
      }

      if (navi.toString() == QLatin1String("stop")) {
        web.page()->action(QWebPage::Stop)->setVisible(true);
      }

      if (navi.toString() == QLatin1String("reload")) {
        web.page()->action(QWebPage::Reload)->setVisible(true);
      }
    }

    config_file.close();
  } else {
    qDebug() << "Warning: Failed to load configuration file" << config_path;
  }

  web.settings()->setLocalStoragePath(appStorage);
  web.settings()->enablePersistentStorage(appStorage);
  web.settings()->setOfflineWebApplicationCachePath(appStorage);
  qDebug() << "Application storage:" << appStorage;
}

void ign::render(QString w) {
  QString pwd(QLatin1String(""));
  QString url_fix;
  char *PWD;
  PWD = getenv("PWD");
  pwd.append(PWD);
  QStringList url_exp = w.split(QStringLiteral("/"));

  if (url_exp.at(0) == QLatin1String("http:") || url_exp.at(0) == QLatin1String("https:")) {
    url_fix = w;
  } else if (url_exp.at(0) == QLatin1String("..") || url_exp.at(0) == QLatin1String(".")) {
    url_fix = "file://" + pwd + "/" + w;
    this->pathLive = pwd + "/" + w;
  } else if (url_exp.at(0) == QLatin1String("")) {
    url_fix = "file://" + w;
    this->pathLive = w;
  } else {
    url_fix = "file://" + pwd + "/" + w;
    this->pathLive = pwd + "/" + w;
  }

  if (this->enableLiveCode) {
    this->liveCode();
  }

  this->web.load(url_fix);
}

void ign::setUrl(const QString &url) { this->web.setUrl(QUrl(url)); }

void ign::show() { this->web.show(); }

void ign::showMaximized() { this->web.showMaximized(); }

void ign::showMinimized() { this->web.showMinimized(); }

QString ign::showMessageBox(const QVariant &config) {
  QVariantMap configuration = m_json->jsonParser(config).toVariantMap();
  QString title = configuration[QStringLiteral("title")].toString();
  QString message = configuration[QStringLiteral("message")].toString();
  QString buttons = configuration[QStringLiteral("buttons")].toString();

  QMessageBox msgBox;
#ifdef Q_OS_MAC
  msgBox.setText(title);
#else
  msgBox.setWindowTitle(title);
#endif
  msgBox.setInformativeText(message);

  QStringList btnlist = buttons.split(QStringLiteral(":"));
  btnlist.removeDuplicates();

  for (int n = 0; n < btnlist.size(); n++) {
    if (btnlist[n] == QLatin1String("ok")) {
      msgBox.addButton(QMessageBox::Ok);
    } else if (btnlist[n] == QLatin1String("open")) {
      msgBox.addButton(QMessageBox::Open);
    } else if (btnlist[n] == QLatin1String("save")) {
      msgBox.addButton(QMessageBox::Save);
    } else if (btnlist[n] == QLatin1String("cancel")) {
      msgBox.addButton(QMessageBox::Cancel);
    } else if (btnlist[n] == QLatin1String("close")) {
      msgBox.addButton(QMessageBox::Close);
    } else if (btnlist[n] == QLatin1String("discard")) {
      msgBox.addButton(QMessageBox::Discard);
    } else if (btnlist[n] == QLatin1String("apply")) {
      msgBox.addButton(QMessageBox::Apply);
    } else if (btnlist[n] == QLatin1String("reset")) {
      msgBox.addButton(QMessageBox::Reset);
    } else if (btnlist[n] == QLatin1String("yes")) {
      msgBox.addButton(QMessageBox::Yes);
    } else if (btnlist[n] == QLatin1String("no")) {
      msgBox.addButton(QMessageBox::No);
    } else if (btnlist[n] == QLatin1String("abort")) {
      msgBox.addButton(QMessageBox::Abort);
    } else if (btnlist[n] == QLatin1String("retry")) {
      msgBox.addButton(QMessageBox::Retry);
    } else if (btnlist[n] == QLatin1String("ignore")) {
      msgBox.addButton(QMessageBox::Ignore);
    } else {
      qDebug() << "Warning: Unknown message box button:" << btnlist[n];
    }
  }

  int ret = msgBox.exec();
  QString response;

  switch (ret) {
  case QMessageBox::Ok:
    response = QLatin1String("ok");
    break;
  case QMessageBox::Open:
    response = QLatin1String("open");
    break;
  case QMessageBox::Save:
    response = QLatin1String("save");
    break;
  case QMessageBox::Cancel:
    response = QLatin1String("cancel");
    break;
  case QMessageBox::Close:
    response = QLatin1String("close");
    break;
  case QMessageBox::Discard:
    response = QLatin1String("discard");
    break;
  case QMessageBox::Apply:
    response = QLatin1String("apply");
    break;
  case QMessageBox::Reset:
    response = QLatin1String("reset");
    break;
  case QMessageBox::Yes:
    response = QLatin1String("yes");
    break;
  case QMessageBox::No:
    response = QLatin1String("no");
    break;
  case QMessageBox::Abort:
    response = QLatin1String("abort");
    break;
  case QMessageBox::Retry:
    response = QLatin1String("retry");
    break;
  case QMessageBox::Ignore:
    response = QLatin1String("ignore");
    break;
  default:
    break;
  }

  return response;
}

// Very basic print support
bool ign::print() {
  QPrintDialog *printDialog = new QPrintDialog(&printer);

  if (printDialog->exec() == QDialog::Accepted) {
    this->web.print(&printer);
    return true;
  } else {
    return false;
  }
}

bool ign::print(const QVariant &config) {
  return this->m_system->print(config);
}

// Action trigger
// TODO: Clean exit
void ign::quit() { exit(0); }

// Navigate history back
void ign::back() { this->web.page()->triggerAction(QWebPage::Back, true); }

// Navigate history forward
void ign::forward() {
  this->web.page()->triggerAction(QWebPage::Forward, true);
}

// Stop loading url
void ign::stop() { this->web.page()->triggerAction(QWebPage::Stop, true); }

// Reload current url
void ign::reload() { this->web.page()->triggerAction(QWebPage::Reload, true); }

// Cut selection to clipboard
void ign::cut() { this->web.page()->triggerAction(QWebPage::Cut, true); }

// Copy selection to clipboard
void ign::copy() { this->web.page()->triggerAction(QWebPage::Copy, true); }

// Paste from clipboard
void ign::paste() { this->web.page()->triggerAction(QWebPage::Paste, true); }

// Undo action
void ign::undo() { this->web.page()->triggerAction(QWebPage::Undo, true); }

// Redo action
void ign::redo() { this->web.page()->triggerAction(QWebPage::Redo, true); }

// Debugging mode
void ign::setDev(bool v) {
  this->web.settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, v);
  this->debugging = true;
}

// Set port for development purpose (Only accessible from WebKit/Chrome-based
// browser)
void ign::setDevRemote(int port) {
  QString host;
  Q_FOREACH (QHostAddress address, QNetworkInterface::allAddresses()) {
    if (!address.isLoopback() &&
        (address.protocol() == QAbstractSocket::IPv4Protocol)) {
      host = address.toString();
      break;
    }
  }
  QString server;

  if (host.isEmpty()) {
    server = QString::number(port);
  } else {
    server = QStringLiteral("%1:%2").arg(host, QString::number(port));
  }

  qDebug() << "Remote debugging is enable : " << server.toUtf8();
  qputenv("QTWEBKIT_INSPECTOR_SERVER", server.toUtf8());
  this->web.page()->setProperty("_q_webInspectorServerPort", port);
}

// Web security
void ign::websecurity(bool c) {
  web.settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls,
                               c);
}

// Window configuration
// Set widget max size
void ign::widgetSizeMax(int w, int h) { this->web.setMaximumSize(w, h); }

// Set widget min size
void ign::widgetSizeMin(int w, int h) { this->web.setMinimumSize(w, h); }

// Set widget size
void ign::widgetSize(int w, int h) { this->web.resize(w, h); }

// Frameless widget
void ign::widgetNoFrame() { this->web.setWindowFlags(Qt::FramelessWindowHint); }

// Don't show application on taskbar
void ign::widgetNoTaskbar() {
  this->web.setWindowFlags(this->web.windowFlags() | Qt::Tool);
}

// Transparent background (Only works if background is not styled)
void ign::widgetTransparent() {
  QPalette pal = this->web.palette();
  pal.setBrush(QPalette::Base, Qt::transparent);
  this->web.setPalette(pal);
  this->web.setAttribute(Qt::WA_OpaquePaintEvent, false);
  this->web.setAttribute(Qt::WA_TranslucentBackground, true);
}

// Toggle fullscreen mode
void ign::getToggleFullScreen() {
  if (this->fullscreen) {
    this->web.showNormal();
    this->fullscreen = false;
  } else {
    this->web.showFullScreen();
    this->fullscreen = true;
  }
}

// Set application to fullscreen mode
void ign::getFullScreen(bool screen) {
  if (screen) {
    this->web.showFullScreen();
    this->fullscreen = true;
  } else {
    this->web.showNormal();
    this->fullscreen = false;
  }
}

// Load executable file in bin/
QString ign::loadBin(const QString &script) {
  QStringList list = this->pathApp.split(QStringLiteral("/"));

  QString pwd(QLatin1String(""));
  char *PWD;
  PWD = getenv("PWD");
  pwd.append(PWD);

  QString path_bin;

  if (list.at(0) != QLatin1String("")) {
    path_bin = pwd + "/" + this->pathApp;
  } else {
    path_bin = this->pathApp;
  }

  return path_bin + "/bin/" + script;
}

void ign::saveFile(const QByteArray &data, QString filename, QString path) {
  QByteArray byteArray = QByteArray::fromBase64(data);
  QString home;
  home = path + "/" + filename;
  QFile localFile(home);

  if (!localFile.open(QIODevice::WriteOnly))
    return;

  localFile.write(byteArray);
  localFile.close();
}

void ign::download(QString data, QString path) {
  this->dl = new QtDownload;
  this->dl->setTarget(data);
  this->dl->save(path);
  this->dl->download();
  connect(this->dl, &QtDownload::download_signal, this,
          &ign::download_signal);
}

void ign::download_signal(qint64 received, qint64 total) {
  emit downloadProgress(received, total);
}

// Evaluate external JS
void ign::include(QString path) {
  QString script = this->m_filesystem->fileRead(path);
  this->web.page()->mainFrame()->evaluateJavaScript(script);
}

// SQL
QObject *ign::sql() {
  if (!m_sql) {
    m_sql = new ignsql;
  }

  return m_sql;
}

// System
QObject *ign::system() {
  if (!m_system) {
    m_system = new ignsystem;
  }

  return m_system;
}

QObject *ign::sys() {
  if (!m_system) {
    m_system = new ignsystem;
  }

  return m_system;
}

// Filesystem
QObject *ign::filesystem() {
  if (!m_filesystem) {
    m_filesystem = new ignfilesystem;
  }

  return m_filesystem;
}

// Network
QObject *ign::network() {
  if (!m_network) {
    m_network = new ignnetwork;
  }

  return m_network;
}

QObject *ign::net() {
  if (!m_network) {
    m_network = new ignnetwork;
  }

  return m_network;
}

// Live coding
void ign::liveCode() {
  QString file = this->pathLive;
  QDir dirApp = QFileInfo(file).absoluteDir();
  QStringList list = this->m_filesystem->list(dirApp.absolutePath());
  QRegularExpression regex(QStringLiteral("(./[\\.\\.\\-])"));
  int dirCount = 0;
  int fileCount = 0;
  foreach (const QString &file, list) {
    if ((!regex.match(file).hasMatch()) ||
        (file == dirApp.absolutePath() + "/.")) {
      if (this->m_filesystem->isDirectory(file)) {
        this->live.addPath(file);
        dirCount = dirCount + 1;
      } else {
        this->live.addPath(file);
        fileCount = fileCount + 1;
      }
    }
  }
  connect(&live, &QFileSystemWatcher::directoryChanged, this,
          &ign::fileChanged);
  connect(&live, &QFileSystemWatcher::fileChanged, this,
          &ign::fileChanged);
  printf("Monitoring changes in %i files and %i directories\n", fileCount,
         dirCount);
}

// Show IGNSDK version
QString ign::sdkVersion() { return this->version; }
