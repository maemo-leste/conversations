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
  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);

  auto palette = QApplication::palette();
  auto *widget = new QLabel(this);
  m_ctx->colorBase = palette.base().color().name();
  m_ctx->colorWindow = palette.window().color().name();
  m_ctx->colorText = widget->palette().foreground().color().name();
  m_ctx->colorHighlight = widget->palette().highlight().color().name();
  m_ctx->colorButton = palette.button().color().name();
  m_ctx->colorButtonText = palette.buttonText().color().name();
  m_ctx->colorBrightText = palette.brightText().color().name();

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
  connect(m_ctx, &Conversations::notificationClicked, this, &MainWindow::onNotificationClicked);
  connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onOpenSettingsWindow);
  connect(ui->actionCompose, &QAction::triggered, this, &MainWindow::onOpenComposeWindow);
  connect(ui->actionSearch, &QAction::triggered, this, &MainWindow::onOpenSearchWindow);
  connect(ui->actionQuit_conversations, &QAction::triggered, this, &MainWindow::onQuitApplication);

  connect(m_ctx->telepathy, &Telepathy::accountManagerReady, this, &MainWindow::onTPAccountManagerReady);
}

void MainWindow::createQml() {
  if(m_quickWidget != nullptr) return;
  m_quickWidget = new QQuickWidget(this);

  auto *qctx = m_quickWidget->rootContext();
  qctx->setContextProperty("cfg", config());
  qctx->setContextProperty("ctx", m_ctx);
  qctx->setContextProperty("mainWindow", this);
  qctx->setContextProperty("chatOverviewModel", m_ctx->chatOverviewModel);
  qctx->setContextProperty("overviewServiceModel", m_ctx->overviewServiceModel);
  qctx->setContextProperty("colorWindow", m_ctx->colorWindow);
  qctx->setContextProperty("colorBase", m_ctx->colorBase);
  qctx->setContextProperty("colorText", m_ctx->colorText);
  qctx->setContextProperty("colorButton", m_ctx->colorButton);
  qctx->setContextProperty("colorButtonText", m_ctx->colorButtonText);
  qctx->setContextProperty("colorBrightText", m_ctx->colorBrightText);
  qctx->setContextProperty("colorHighlight", m_ctx->colorHighlight);

  m_quickWidget->setSource(QUrl("qrc:/qml/Overview.qml"));
  m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

  connect((QObject*)m_quickWidget->rootObject(), SIGNAL(overviewRowClicked(int)), this, SLOT(onOpenChatWindow(int)));

  ui->centralWidget->layout()->addWidget(m_quickWidget);
}

void MainWindow::onTPAccountManagerReady() {}

void MainWindow::destroyQml() {
  if(m_quickWidget == nullptr) return;
  m_quickWidget->disconnect();
  m_quickWidget->deleteLater();
  m_quickWidget = nullptr;
}

void MainWindow::onOpenChatWindow(int idx) {
  auto msg = m_ctx->chatOverviewModel->chats.at(idx);
  this->onOpenChatWindow(msg);
}

void MainWindow::onOpenChatWindow(const QString &remote_uid) {
  // @TODO: fix
  //this->onOpenChatWindow("", "", remote_uid, "", "");
}

void MainWindow::onOpenChatWindow(const QSharedPointer<ChatMessage> &msg) {
  auto uid = msg->remote_uid();
  if(m_chatWindows.contains(uid)) {
    m_chatWindows[uid]->setFocus();
    return;
  }

  auto *window = new ChatWindow(m_ctx, msg, this);
  m_chatWindows[uid] = window;
  window->show();

  connect(window, &ChatWindow::sendMessage, this->m_ctx, &Conversations::onSendOutgoingMessage);
  connect(window, &ChatWindow::closed, this, &MainWindow::onChatWindowClosed);
}

void MainWindow::onQuitApplication() {
  m_autoHideWindow = false;
  this->close();
}

void MainWindow::onOpenSearchWindow() {
  m_searchWindow = new SearchWindow(m_ctx, "", this);
  m_searchWindow->show();

  connect(m_searchWindow,
          SIGNAL(searchResultClicked(QSharedPointer<ChatMessage>)), this,
          SLOT(onOpenChatWindow(QSharedPointer<ChatMessage>)));

  connect(m_searchWindow, &SearchWindow::searchResultClicked, this, &MainWindow::onCloseSearchWindow);
}

void MainWindow::onCloseSearchWindow(const QSharedPointer<ChatMessage> &msg) {
  m_searchWindow->close();
  m_searchWindow->deleteLater();
}

void MainWindow::onOpenComposeWindow() {
  m_compose = new Compose(m_ctx, this);
  m_compose->show();

  // connect(m_s, &Settings::textScalingChanged, this->m_ctx, &Conversations::onTextScalingChanged);
}


void MainWindow::onOpenSettingsWindow() {
  m_settings = new Settings(m_ctx, this);
  m_settings->show();

  connect(m_settings, &Settings::textScalingChanged, this->m_ctx, &Conversations::onTextScalingChanged);
  connect(m_settings, &Settings::autoCloseChatWindowsChanged, this->m_ctx, &Conversations::autoCloseChatWindowsChanged);
  connect(m_settings, &Settings::inheritSystemThemeToggled, this, [=](bool enabled){
    emit inheritSystemThemeChanged(enabled);
  });
}

void MainWindow::onNotificationClicked(const QSharedPointer<ChatMessage> &msg) {
  this->onShowApplication();
  this->onOpenChatWindow(msg);
}

void MainWindow::closeEvent(QCloseEvent *event) {
  if(m_autoHideWindow) {
    this->onHideApplication();
    event->ignore();
  } else {
    QApplication::quit();
  }
}

void MainWindow::onChatWindowClosed(const QString &remote_uid) {
  if(!m_chatWindows.contains(remote_uid)) return;
  auto *window = m_chatWindows[remote_uid];
  if(window != nullptr)
    window->deleteLater();
  m_chatWindows.remove(remote_uid);
}

void MainWindow::onShowApplication() {
  this->createQml();
  this->show();

  m_ctx->isBackground = false;

  // clear notification cache
  m_ctx->notificationMap.clear();
}

void MainWindow::onHideApplication() {
  this->hide();
  this->destroyQml();
  m_ctx->isBackground = true;
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
