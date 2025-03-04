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

  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-Regular.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-Bold.ttf");

  m_filters = new QActionGroup(this);
  m_filters->setExclusive(true);

  this->screenDpiPhysical = QGuiApplication::primaryScreen()->physicalDotsPerInch();
  this->screenRatio = this->screenDpiPhysical / this->screenDpiRef;
  qDebug() << QString("%1x%2 (%3 DPI)").arg(
      this->screenRect.width()).arg(this->screenRect.height()).arg(this->screenDpi);

  // messages overview widget
  m_widgetOverview = new OverviewWidget(ctx, this);
  ui->mainLayout->addWidget(m_widgetOverview);
  connect(m_ctx, &Conversations::textScalingChanged, m_widgetOverview, &OverviewWidget::onSetColumnStyleDelegate);
  connect(m_ctx, &Conversations::textScalingChanged, m_widgetOverview, &OverviewWidget::onSetTableHeight);

  // error messages from Telepathy
  connect(m_ctx->telepathy, &Telepathy::errorMessage, [this](const QString& msg){
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();
  });

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

  // determine if we show the welcome screen
  connect(m_ctx->telepathy, &Telepathy::channelJoined, [this](QString local_uid, QString channel) { this->onDeterminePage(); });
  connect(m_ctx->telepathy, &Telepathy::channelLeft, [this](QString local_uid, QString channel) { this->onDeterminePage(); });
  connect(m_ctx->telepathy, &Telepathy::channelDeleted, [this](QString local_uid, QString channel) { this->onDeterminePage(); });
  connect(m_ctx->telepathy, &Telepathy::accountAdded, [this](TelepathyAccountPtr ta) { this->onDeterminePage(); });
  connect(m_ctx->telepathy, &Telepathy::accountRemoved, [this] { this->onDeterminePage(); });
  connect(m_ctx->telepathy, &Telepathy::accountManagerReady, this, &MainWindow::onDeterminePage);

  this->onDeterminePage();
}

void MainWindow::onTPAccountManagerReady() {}

void MainWindow::onDeterminePage() const {
  bool hasTpAccounts = m_ctx->telepathy->accounts.length() > 0;
  bool hasDatabase = m_ctx->overviewModel->messages.length() > 0;
  qDebug() << "tp accounts" << m_ctx->telepathy->accounts.length();
  qDebug() << "overview model length" << m_ctx->overviewModel->messages.length();

  if(hasTpAccounts && !hasDatabase) {
    this->onShowEmptyDbPage();
  } else if(hasTpAccounts || hasDatabase) {
    this->onShowOverviewPage();
  } else {
    this->onShowWelcomePage();
  }
}

void MainWindow::onShowOverviewPage() const {
  qDebug() << "onShowOverviewPage";
  ui->overviewPages->setCurrentIndex(0);
}

void MainWindow::onShowEmptyDbPage() const {
  QFont robotoRegular(QFontDatabase::applicationFontFamilies(0).at(0));
  robotoRegular.setPointSize(16);
  ui->label_emptyDb->setFont(robotoRegular);
  ui->overviewPages->setCurrentIndex(2);
}

void MainWindow::onShowWelcomePage() const {
  qDebug() << "onShowWelcomePage";
  ui->overviewPages->setCurrentIndex(1);

  QPixmap p("/usr/share/icons/hicolor/48x48/hildon/general_sms.png");
  ui->label_welcomeIcon->setText("");
  ui->label_welcomeIcon->setPixmap(p);

  QFont robotoBold(QFontDatabase::applicationFontFamilies(1).at(0));
  robotoBold.setPointSize(28);
  ui->label_welcomeTitle->setFont(robotoBold);

  QFont robotoBoldSubTitle(QFontDatabase::applicationFontFamilies(1).at(0));
  robotoBoldSubTitle.setPointSize(16);
  robotoBoldSubTitle.setBold(true);
  ui->label_subtitle->setFont(robotoBoldSubTitle);

  QFont robotoRegular(QFontDatabase::applicationFontFamilies(0).at(0));
  robotoRegular.setPointSize(14);
  ui->label_description->setFont(robotoRegular);
}

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
  connect(window, &ChatWindow::chatCleared, m_ctx->overviewModel, &OverviewModel::onLoad);
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

  connect(m_joinchannel, &JoinChannel::joinChannel, [this](QString account, QString channel) {
    m_ctx->telepathy->joinChannel(account, channel);

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

  connect(m_compose, &Compose::message, [this](QString account, QString to, QString msg) {
    m_ctx->telepathy->sendMessage(account, to, msg);

    m_compose->close();
    m_compose->deleteLater();
    m_compose = nullptr;
  });
}

// message box ask
void MainWindow::onFriendRequest(QSharedPointer<ContactItem> item) {
  QString remote_uid = item->remote_uid();
  QString local_uid = item->local_uid();
  QString message = QString("Accept friend request from %1?").arg(remote_uid);

  QWidget window;
  QMessageBox msgBox(&window);
  msgBox.setWindowTitle(remote_uid);
  msgBox.setText(message);

  //QPushButton *btn_add = msgBox.addButton("Add contact", QMessageBox::ActionRole);
  //QPushButton *btn_deny = msgBox.addButton("Deny contact", QMessageBox::ActionRole);
  msgBox.setText("Use the address book to accept/deny incoming friend request");
  QPushButton *btn_nothing = msgBox.addButton("Do nothing", QMessageBox::ActionRole);

  msgBox.exec();
  // if (msgBox.clickedButton() == btn_add) {
  //   m_ctx->telepathy->authorizeContact(local_uid, remote_uid);
  // } else if (msgBox.clickedButton() == btn_deny) {
  //   m_ctx->telepathy->removeContact(local_uid, remote_uid);
  // }
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
  connect(m_settings, &Settings::enableDisplayAvatarsToggled, this, [this](bool toggled){
    m_ctx->displayAvatars = toggled;
    m_ctx->displayAvatarsChanged(toggled);
  });
  connect(m_settings, &Settings::enableDisplayChatGradientToggled, this, [this](bool toggled){
    m_ctx->displayChatGradientChanged(toggled);
  });
  connect(m_settings, &Settings::enterKeySendsChatToggled, m_ctx, &Conversations::enterKeySendsChatToggled);
}

void MainWindow::onNotificationClicked(const QSharedPointer<ChatMessage> &msg) {
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
  if(!m_chatWindows.contains(group_uid))
    return;

  auto *window = m_chatWindows[group_uid];
  if(window != nullptr)
    window->deleteLater();
  m_chatWindows.remove(group_uid);

  // // after closing a chatwindow, we'll attempt to
  // // malloc_trim as usually *some* memory can be forced
  // // freed, maybe it helps.
  QTimer::singleShot(500, [] {
    if (malloc_trim(0) == 0){  // returns 1 if memory was actually released
      qWarning() << "malloc_trim(0); no memory can be returned";
    }
  });
}

void MainWindow::onShowApplication() {
  qDebug() << "raising mainwindow";
  this->show();
  this->raise();

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
  if (QAction *a = addProtocol(tr("All"), "*"))
      a->setChecked(true);

  // accounts from rtcom & tp
  for(const QSharedPointer<ServiceAccount> &acc: m_ctx->serviceAccounts)
    this->addProtocol(acc->title, acc->protocol);
}

QAction *MainWindow::addProtocol(const QString title, const QString service) {
  if(m_filterProtocols.contains(service))
    return nullptr;

  auto *item = new FilterProtocolItem;
  item->title = title;
  item->filterKey = service;
  item->action = new QAction(title, m_filters);
  item->action ->setCheckable(true);
  m_filterProtocols[service] = item;

  connect(item->action, &QAction::triggered, [this, service]{
    this->onProtocolFilterClicked(service);
  });

  ui->menu->addAction(item->action);

  return item->action;
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
