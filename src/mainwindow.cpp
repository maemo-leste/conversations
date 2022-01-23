#include <QPixmap>
#include <QWidget>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QFileDialog>

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
  ui->menuBar->hide();
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
  qDebug() << QString("%1x%2 (%3 DPI)").arg(
      this->screenRect.width()).arg(this->screenRect.height()).arg(this->screenDpi);

  // js: cfg.get(Config.MaemoTest);  |  cfg.set(Config.MaemoTest , "foo");
  qmlRegisterUncreatableMetaObject(
      ConfigKeys::staticMetaObject,
      "MaemoConfig",
      1, 0,
      "Config",
      "Error: only enums"
  );

  connect(m_ctx, &Conversations::setTitle, this, &QMainWindow::setWindowTitle);
  connect(m_ctx, SIGNAL(openChatWindow(QString)), this, SLOT(onOpenChatWindow(QString)));
  connect(m_ctx, &Conversations::showApplication, this, &MainWindow::onShowApplication);
  connect(m_ctx, &Conversations::hideApplication, this, &MainWindow::onHideApplication);
  connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onOpenSettingsWindow);
}

void MainWindow::createQml() {
  if(m_quickWidget != nullptr) return;
  m_quickWidget = new QQuickWidget(this);

  auto *qctx = m_quickWidget->rootContext();
  qctx->setContextProperty("cfg", config());
  qctx->setContextProperty("ctx", m_ctx);
  qctx->setContextProperty("chatOverviewModel", m_ctx->chatOverviewModel);

  m_quickWidget->setSource(QUrl("qrc:/overview.qml"));
  m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

  connect((QObject*)m_quickWidget->rootObject(), SIGNAL(rowClicked(QString, QString, QString)),
      this, SLOT(onOpenChatWindow(QString, QString, QString)));

  ui->centralWidget->layout()->addWidget(m_quickWidget);
}

void MainWindow::destroyQml() {
  if(m_quickWidget == nullptr) return;
  m_quickWidget->disconnect();
  m_quickWidget->deleteLater();
  m_quickWidget = nullptr;
}

void MainWindow::onOpenChatWindow(const QString &remote_uid) {
  this->onOpenChatWindow("", "", remote_uid);
}

void MainWindow::onOpenChatWindow(const QString &group_uid, const QString &local_uid, const QString &remote_uid) {
  m_chatWindow = new ChatWindow(m_ctx, group_uid, local_uid, remote_uid, this);
  m_chatWindow->show();

  connect(m_chatWindow, &ChatWindow::sendMessage, this->m_ctx, &Conversations::onSendOutgoingMessage);
  connect(m_chatWindow, &ChatWindow::closed, this, &MainWindow::onChatWindowClosed);
}

void MainWindow::onOpenSettingsWindow() {
  m_settings = new Settings(m_ctx, this);
  m_settings->show();

  connect(m_settings, &Settings::textScalingChanged, this->m_ctx, &Conversations::onTextScalingChanged);
}

void MainWindow::closeEvent(QCloseEvent *event) {
  // override close event, force hide window
  this->onHideApplication();
  event->ignore();
}

void MainWindow::onChatWindowClosed() {
  if(m_chatWindow == nullptr) return;
  m_chatWindow->deleteLater();
  m_chatWindow = nullptr;
}

void MainWindow::onShowApplication() {
  this->createQml();
  this->show();
}

void MainWindow::onHideApplication() {
  this->hide();
  this->destroyQml();
}

MainWindow *MainWindow::getInstance() {
  return pMainWindow;
}

Conversations *MainWindow::getContext(){
  return pMainWindow->m_ctx;
}

MainWindow::~MainWindow() {
  delete ui;
}
