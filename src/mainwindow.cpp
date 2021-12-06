#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QFileDialog>
#include <QQuickWidget>
#include <QQuickView>
#include <QQmlContext>

#include "mainwindow.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_mainwindow.h"

MainWindow * MainWindow::pMainWindow = nullptr;

MainWindow::MainWindow(Conversations *ctx, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_ctx(ctx) {
  pMainWindow = this;
  ui->setupUi(this);

#ifdef MAEMO
  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);
#endif

  this->screenDpiRef = 128;
  this->screenGeo = QApplication::primaryScreen()->availableGeometry();
  this->screenRect = QGuiApplication::primaryScreen()->geometry();
  this->screenDpi = QGuiApplication::primaryScreen()->logicalDotsPerInch();
  this->screenDpiPhysical = QGuiApplication::primaryScreen()->physicalDotsPerInch();
  this->screenRatio = this->screenDpiPhysical / this->screenDpiRef;
  qInfo()
      << QString("%1x%2 (%3 DPI)").arg(this->screenRect.width()).arg(this->screenRect.height()).arg(this->screenDpi);

  // js: cfg.get(Config.MaemoTest);  |  cfg.set(Config.MaemoTest , "foo");
  qmlRegisterUncreatableMetaObject(
      ConfigKeys::staticMetaObject,
      "MaemoConfig",
      1, 0,
      "Config",
      "Error: only enums"
  );

  auto *qctx = ui->quick->rootContext();
  qctx->setContextProperty("cfg", config());
  qctx->setContextProperty("ctx", m_ctx);
  qctx->setContextProperty("chatOverviewModel", m_ctx->chatOverviewModel);

  ui->quick->setSource(QUrl("qrc:/overview.qml"));

  ui->menuBar->hide();
  connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::openSettingsWindow);
  connect((QObject*)ui->quick->rootObject(), SIGNAL(rowClicked(QString, QString, QString)), this, SLOT(openChatWindow(QString, QString, QString)));

  connect(m_ctx, &Conversations::setTitle, this, &QMainWindow::setWindowTitle);

  this->show();
}

void MainWindow::openSettingsWindow() {
  m_settings = new Settings(m_ctx, this);
  m_settings->show();

  connect(m_settings, &Settings::textScalingChanged, this->m_ctx, &Conversations::onTextScalingChanged);
}

void MainWindow::openChatWindow(const QString &group_uid, const QString &local_uid, const QString &remote_uid) {
  m_chatWindow = new ChatWindow(m_ctx, group_uid, local_uid, remote_uid, this);
  m_chatWindow->show();

  connect(m_chatWindow, &ChatWindow::sendMessage, this->m_ctx, &Conversations::onSendMessage);
}

void MainWindow::showDebugInfo() {
//  auto *dialog = new DebugInfoDialog(m_ctx, this);
//  dialog->exec();
//  dialog->deleteLater();
  printf("oki\n");
}

MainWindow *MainWindow::getInstance() {
  return pMainWindow;
}

Conversations *MainWindow::getContext(){
  return pMainWindow->m_ctx;
}

void MainWindow::closeEvent(QCloseEvent *event) {
  QWidget::closeEvent(event);
}

MainWindow::~MainWindow() {
  delete ui;
}
