#include <QPixmap>
#include <QWidget>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QFileDialog>

#include "mainwindow.h"
#include "settingswidget.h"
#include "aboutwidget.h"
#include "config-conversations.h"
#include "lib/globals.h"
#include "logger_std/logger_std.h"

#include "ui_mainwindow.h"

MainWindow * MainWindow::pMainWindow = nullptr;

MainWindow::MainWindow(Conversations *ctx, QWidget *parent) :
    QConversationsMainWindow(ctx, parent),
    ui(new Ui::MainWindow),
    m_ctx(ctx) {
  pMainWindow = this;
  ui->setupUi(this);
  ui->menuBar->hide();
  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);

  CLOCK_MEASURE_START(start_add_fonts);
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-Regular.ttf");
  QFontDatabase::addApplicationFont(":/assets/fonts/Roboto-Bold.ttf");
  CLOCK_MEASURE_END(start_add_fonts, "mainwindow::add_fonts");

  m_filters = new QActionGroup(this);
  m_filters->setExclusive(true);

  CLOCK_MEASURE_START(start_get_dimensions);
  this->screenDpiPhysical = QGuiApplication::primaryScreen()->physicalDotsPerInch();
  this->screenRatio = this->screenDpiPhysical / this->screenDpiRef;
  qDebug() << QString("%1x%2 (%3 DPI)").arg(
      this->screenRect.width()).arg(this->screenRect.height()).arg(this->screenDpi);
  CLOCK_MEASURE_END(start_get_dimensions, "mainwindow::start_get_dimensions");

  // messages overview widget
  CLOCK_MEASURE_START(start_overviewwidget);
  m_widgetOverview = new OverviewWidget(ctx, ctx->overviewProxyModel, this);
  ui->mainLayout->addWidget(m_widgetOverview);
  connect(m_ctx, &Conversations::textScalingChanged, m_widgetOverview, &OverviewWidget::onSetColumnStyleDelegate);
  connect(m_ctx, &Conversations::textScalingChanged, m_widgetOverview, &OverviewWidget::onSetTableHeight);
  CLOCK_MEASURE_END(start_overviewwidget, "mainwindow::start_overviewwidget");

  // error messages from Telepathy
  connect(m_ctx->telepathy, &Telepathy::errorMessage, [this](const QString& msg){
    QMessageBox msgBox;
    msgBox.setText(msg);
    msgBox.exec();
  });

  // Setup filter protocol menu items
  CLOCK_MEASURE_START(start_setup_ui_accounts);
  this->onSetupUIAccounts();
  connect(m_ctx->telepathy, &Telepathy::accountAdded, [this](TelepathyAccountPtr ta) {
    m_ctx->onGetAvailableServiceAccounts();
    this->onSetupUIAccounts();  // redraw protocol filter menu
  });
  CLOCK_MEASURE_END(start_setup_ui_accounts, "mainwindow::setup_ui_accounts");

  // js: cfg.get(Config.MaemoTest);  |  cfg.set(Config.MaemoTest , "foo");
#ifdef QUICK
  CLOCK_MEASURE_START(start_register_qml_type);
  qmlRegisterUncreatableMetaObject(
      ConfigKeys::staticMetaObject,
      "MaemoConfig",
      1, 0,
      "Config",
      "Error: only enums"
  );
  CLOCK_MEASURE_END(start_register_qml_type, "mainwindow::register_qml_type");
#endif

  connect(m_ctx, &Conversations::setTitle, this, &QMainWindow::setWindowTitle);
  connect(m_ctx, &Conversations::showApplication, this, &MainWindow::onShowApplication);
  connect(m_ctx, &Conversations::hideApplication, this, &MainWindow::onHideApplication);
  connect(m_ctx, &Conversations::notificationClicked, this, &MainWindow::onNotificationClicked);
  connect(m_ctx, &Conversations::openChatWindow, this, QOverload<QString>::of(&MainWindow::onOpenChatWindow));
  connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onOpenSettingsWindow);
  connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onOpenAboutWindow);
  connect(ui->actionQuit_application, &QAction::triggered, this, &MainWindow::onAskQuitApplication);
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
  connect(m_ctx->telepathy, &Telepathy::onlinenessChanged, this, &MainWindow::onTPAccountOnlinenessChanged);

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

void MainWindow::onOpenChatWindow(QString remote_uid) {
  // already have a window?
  for (const auto& window: m_chatWindows) {
    if (window->remote_uid == remote_uid) {
      window->setFocus();
      window->activateWindow();
      return;
    }
  }

  // remote_uid perhaps known to us?
  for (const auto& msg: m_ctx->overviewModel->messages) {
    if (msg->remote_uid() == remote_uid) {
      return onOpenChatWindow(msg);
    }
  }

  qWarning() << "onOpenChatWindow:" << remote_uid << "not found";
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
  connect(window, &ChatWindow::chatCleared, m_ctx->overviewModel, &OverviewModel::loadOverviewMessages);
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

void MainWindow::onAskQuitApplication() {
  if (QMessageBox::question(nullptr, "Confirm", "Quit conversations?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    QApplication::quit();
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

void MainWindow::onOpenAboutWindow() {
  // enforce opening once
  if (m_aboutDialog) {
    m_aboutDialog->raise();
    m_aboutDialog->activateWindow();
    return;
  }

  m_aboutDialog = new QDialog(this);
  m_aboutDialog->setWindowTitle("About");
  m_aboutDialog->setAttribute(Qt::WA_DeleteOnClose);

  connect(m_aboutDialog, &QObject::destroyed, this, [this] {
    m_aboutDialog = nullptr;
  });

  const auto about = new AboutWidget(m_ctx, m_aboutDialog);
  const auto layout = new QVBoxLayout(m_aboutDialog);
  layout->addWidget(about);
  m_aboutDialog->show();
}

void MainWindow::onOpenSettingsWindow() {
  // enforce opening once
  if (m_settingsDialog) {
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
    return;
  }

  m_settingsDialog = new QDialog(this);
  m_settingsDialog->setWindowTitle("Settings");
  m_settingsDialog->setAttribute(Qt::WA_DeleteOnClose);

  connect(m_settingsDialog, &QObject::destroyed, this, [this] {
    m_settingsDialog = nullptr;
  });

  const auto settings = new SettingsWidget(m_ctx, m_settingsDialog);

  connect(settings, &SettingsWidget::textScalingChanged, this->m_ctx, &Conversations::onTextScalingChanged);
  connect(settings, &SettingsWidget::autoCloseChatWindowsChanged, this->m_ctx, &Conversations::autoCloseChatWindowsChanged);
  connect(settings, &SettingsWidget::inheritSystemThemeToggled, this, [this](bool toggled){
    m_ctx->inheritSystemTheme = toggled;
    emit m_ctx->inheritSystemThemeChanged(toggled);
  });
  connect(settings, &SettingsWidget::enableDisplayGroupchatJoinLeaveToggled, this, [this](bool toggled){
    m_ctx->displayGroupchatJoinLeave = toggled;
    emit m_ctx->displayGroupchatJoinLeaveChanged(toggled);
  });
  connect(settings, &SettingsWidget::enableDisplayAvatarsToggled, this, [this](bool toggled){
    m_ctx->displayAvatars = toggled;
    emit m_ctx->displayAvatarsChanged(toggled);
  });
  connect(settings, &SettingsWidget::enableDisplayChatGradientToggled, this, [this](bool toggled){
    m_ctx->displayChatGradient = toggled;
    emit m_ctx->displayChatGradientChanged(toggled);
  });

  connect(settings, &SettingsWidget::enableLinkPreviewEnabledToggled, m_ctx, &Conversations::enableLinkPreviewEnabledToggled);
  connect(settings, &SettingsWidget::enableLinkPreviewImageEnabledToggled, m_ctx, &Conversations::enableLinkPreviewImageEnabledToggled);
  connect(settings, &SettingsWidget::enableLinkPreviewRequiresUserInteractionToggled, m_ctx, &Conversations::enableLinkPreviewRequiresUserInteractionToggled);
  connect(settings, &SettingsWidget::attachmentMaxDownloadSizeChanged, m_ctx, &Conversations::attachmentMaxDownloadSizeChanged);

  connect(settings, &SettingsWidget::enterKeySendsChatToggled, m_ctx, &Conversations::enterKeySendsChatToggled);

  const auto layout = new QVBoxLayout(m_settingsDialog);
  layout->addWidget(settings);
  m_settingsDialog->show();
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

QAction *MainWindow::addProtocol(const QString &title, const QString &service) {
  if(m_filterProtocols.contains(service))
    return nullptr;

  auto *item = new FilterProtocolItem;
  item->title = title;
  item->filterKey = service;
  item->action = new QAction(title, m_filters);

  if (service != "*") {
    bool isOnline = false;
    for (const auto &acc: m_ctx->telepathy->accounts) {
      isOnline = acc->isOnline && acc->protocolName() == service;
      if (service == "tel")
        isOnline = true;
      if (isOnline) break;
    }
    const auto icon = isOnline ? ":/presence_online.png" : ":/presence_offline.png";
    item->action->setIcon(QIcon(icon));
  }

  item->action ->setCheckable(true);
  m_filterProtocols[service] = item;

  connect(item->action, &QAction::triggered, [this, service]{
    this->onProtocolFilterClicked(service);
  });

  ui->menu->addAction(item->action);

  return item->action;
}

// update filter protocols online icons
void MainWindow::onTPAccountOnlinenessChanged(const TelepathyAccountPtr &account, const bool online) const {
  for (auto it = m_filterProtocols.constBegin(); it != m_filterProtocols.constEnd(); ++it) {
    FilterProtocolItem* item = it.value();
    if (item->filterKey == "tel")
      continue;
    if (item->filterKey == account->protocolName()) {
      const auto icon = online ? ":/presence_online.png" : ":/presence_offline.png";
      item->action->setIcon(QIcon(icon));
    }
  }
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
