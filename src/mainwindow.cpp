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

  this->screenDpiPhysical = QGuiApplication::primaryScreen()->physicalDotsPerInch();
  this->screenRatio = this->screenDpiPhysical / this->screenDpiRef;
  qDebug() << QString("%1x%2 (%3 DPI)").arg(
      this->screenRect.width()).arg(this->screenRect.height()).arg(this->screenDpi);

  m_widgetOverview = new OverviewWidget(ctx, this);
  ui->mainLayout->addWidget(m_widgetOverview);
  connect(m_ctx, &Conversations::textScalingChanged, m_widgetOverview, &OverviewWidget::onSetColumnStyleDelegate);

  // Setup filter protocol menu items
  this->onSetupUIAccounts();

  // js: cfg.get(Config.MaemoTest);  |  cfg.set(Config.MaemoTest , "foo");
  qmlRegisterUncreatableMetaObject(
      ConfigKeys::staticMetaObject,
      "MaemoConfig",
      1, 0,
      "Config",
      "Error: only enums"
  );

  connect(m_ctx, &Conversations::setTitle, this, &QMainWindow::setWindowTitle);
  connect(m_ctx, &Conversations::showApplication, this, &MainWindow::onShowApplication);
  connect(m_ctx, &Conversations::hideApplication, this, &MainWindow::onHideApplication);
  connect(m_ctx, &Conversations::notificationClicked, this, &MainWindow::onNotificationClicked);
  connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onOpenSettingsWindow);
  connect(ui->actionCompose, &QAction::triggered, this, &MainWindow::onOpenComposeWindow);
  connect(ui->actionJoinChatRoom, &QAction::triggered, this, &MainWindow::onOpenJoinChatWindow);
  connect(ui->actionSearch, &QAction::triggered, this, &MainWindow::onOpenSearchWindow);

  connect(m_ctx->telepathy, &Telepathy::accountManagerReady, this, &MainWindow::onTPAccountManagerReady);
  connect(m_ctx->telepathy, &Telepathy::openChannelWindow, this, QOverload<QString, QString, QString, QString, QString>::of(&MainWindow::onOpenChatWindow));
  connect(m_ctx->overviewModel, &OverviewModel::overviewRowClicked, this, [this](auto &ptr){ this->onOpenChatWindow(ptr); });
}

void MainWindow::onTPAccountManagerReady() {}

void MainWindow::onOpenChatWindow(int idx) {
  auto msg = m_ctx->chatOverviewModel->chats.at(idx);
  this->onOpenChatWindow(msg);
}

void MainWindow::onOpenChatWindow(QString local_uid, QString remote_uid, QString group_uid, QString channel, QString service) {
  qDebug() << "onOpenChatWindow" << channel;
  if(m_chatWindows.contains(group_uid)) {
    m_chatWindows[group_uid]->setFocus();
    m_chatWindows[group_uid]->activateWindow();
    return;
  }

  auto *window = new ChatWindow(m_ctx, local_uid, remote_uid, group_uid, channel, service, this);
  m_chatWindows[group_uid] = window;
  window->show();

  connect(window, &ChatWindow::sendMessage, this->m_ctx, &Conversations::onSendOutgoingMessage);
  connect(window, &ChatWindow::closed, this, &MainWindow::onChatWindowClosed);
}

void MainWindow::onOpenChatWindow(const QSharedPointer<ChatMessage> &msg) {
  onOpenChatWindow(msg->local_uid(), msg->remote_uid(), msg->group_uid(), msg->channel(), msg->service());
}

void MainWindow::onOpenChatWindowWithHighlight(const QSharedPointer<ChatMessage> &msg) {
  auto group_uid = msg->group_uid();
  onOpenChatWindow(msg->local_uid(), msg->remote_uid(), group_uid, msg->channel(), msg->service());

  if(m_chatWindows.contains(group_uid))
    m_chatWindows[group_uid]->setHighlight(msg->event_id());
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
          SLOT(onOpenChatWindowWithHighlight(QSharedPointer<ChatMessage>)));

  connect(m_searchWindow, &SearchWindow::searchResultClicked, this, &MainWindow::onCloseSearchWindow);
}

void MainWindow::onCloseSearchWindow(const QSharedPointer<ChatMessage> &msg) {
  m_searchWindow->close();
  m_searchWindow->deleteLater();
}

void MainWindow::onOpenJoinChatWindow() {
  m_joinchannel = new JoinChannel(m_ctx, this);
  m_joinchannel->show();

  connect(m_joinchannel, &JoinChannel::joinChannel, [this](QString account, QString channel, bool persistent) {
    m_ctx->telepathy->joinChannel(account, channel, persistent);

    // hack: if we close this window immediately, then the QML overview model will not get
    // updated properly with new changes. The data is updated but the screen wont
    // refresh, only after touch. Dont know why, maybe maemo-qt or Hildon related.
    QTimer::singleShot(300, [this]{
        m_joinchannel->close();
    });
  });
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
  connect(m_settings, &Settings::inheritSystemThemeToggled, this, [this](bool toggled){
    m_ctx->inheritSystemTheme = toggled;
    m_ctx->inheritSystemThemeChanged(toggled);
  });
  connect(m_settings, &Settings::enableDisplayGroupchatJoinLeaveToggled, this, [this](bool toggled){
    m_ctx->displayGroupchatJoinLeave = toggled;
    m_ctx->displayGroupchatJoinLeaveChanged(toggled);
  });
  connect(m_settings, &Settings::enterKeySendsChatToggled, m_ctx, &Conversations::enterKeySendsChatToggled);
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

void MainWindow::onChatWindowClosed(const QString &group_uid) {
  if(!m_chatWindows.contains(group_uid)) return;
  auto *window = m_chatWindows[group_uid];
  if(window != nullptr)
    window->deleteLater();
  m_chatWindows.remove(group_uid);
}

void MainWindow::onShowApplication() {
  this->show();

  m_ctx->isBackground = false;

  // clear notification cache
  m_ctx->notificationMap.clear();
}

void MainWindow::onHideApplication() {
  this->hide();
  m_ctx->isBackground = true;
}

// populate the account filter menu
void MainWindow::onSetupUIAccounts() {
  // the default; All
  this->addProtocol("Show all", "*");

  // accounts from rtcom & tp
  for(const QSharedPointer<ServiceAccount> &acc: m_ctx->serviceAccounts)
    this->addProtocol(acc->title, acc->protocol);
}

void MainWindow::addProtocol(const QString title, const QString service) {
  if(m_filterProtocols.contains(service))
    return;

  auto *item = new FilterProtocolItem;
  item->title = title;
  item->filterKey = service;
  item->action = new QAction(title);
  m_filterProtocols[service] = item;

  connect(item->action, &QAction::triggered, [this, service]{
    this->onProtocolFilterClicked(service);
  });
  ui->menuFilters->addAction(item->action);
}

void MainWindow::onProtocolFilterClicked(const QString service) {
  if(!m_filterProtocols.contains(service)) {
    return;
  }

  auto *item = m_filterProtocols[service];
  m_ctx->overviewProxyModel->setProtocolFilter(item->filterKey);
}

MainWindow *MainWindow::getInstance() {
  return pMainWindow;
}

Conversations *MainWindow::getContext(){
  return pMainWindow->m_ctx;
}

QWidget *MainWindow::getChatWindow(const QString &group_uid)
{
  if (!pMainWindow || !pMainWindow->m_chatWindows.contains(group_uid))
    return nullptr;

  return pMainWindow->m_chatWindows[group_uid];
}

MainWindow::~MainWindow() {
  delete ui;
}
