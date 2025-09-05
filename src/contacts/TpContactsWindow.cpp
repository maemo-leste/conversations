#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#ifdef QUICK
#include <QQmlContext>
#endif
#include <QMessageBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QTextEdit>

#include "TpContactsWindow.h"

#include <QVBoxLayout>

#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_TpContactsWindow.h"

TpContactsWindow *TpContactsWindow::pTpContactsWindow = nullptr;

TpContactsWindow::TpContactsWindow(const TelepathyChannelPtr& channel, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TpContactsWindow),
    channel(channel) {
  setAttribute(Qt::WA_DeleteOnClose);
  auto *ctx = Conversations::instance();
  pTpContactsWindow = this;
  ui->setupUi(this);
  ui->menuBar->hide();

  m_model = new TpContactsModel(channel, this);
  connect(m_model, &TpContactsModel::contactClicked, [this](Tp::ContactPtr ptr) {
    this->contactClicked(ptr);
    this->close();
  });

  m_proxyModel = new TpContactsProxyModel(this);
  m_proxyModel->setSourceModel(m_model);
  m_proxyModel->setDynamicSortFilter(true);
  m_widgetContacts = new TpContactsWidget(m_proxyModel, nullptr);

  connect(ui->filterContacts, &QLineEdit::textChanged, this, [this](const QString &text) {
    m_proxyModel->setNameFilter(text);
  });

  // add to layout
  if (ui->centralWidget->layout()) {
    ui->centralWidget->layout()->addWidget(m_widgetContacts);
  } else {
    // If centralWidget has no layout, create one
    auto *layout = new QVBoxLayout(ui->centralWidget);
    layout->addWidget(m_widgetContacts);
    ui->centralWidget->setLayout(layout);
  }

  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);

  onSetWindowTitle();

  connect(channel.data(), &TelepathyChannel::room_contact_count_changed, this, &TpContactsWindow::onSetWindowTitle);
  connect(channel.data(), &TelepathyChannel::room_contact_count_changed, m_model, &TpContactsModel::onReload);
}

void TpContactsWindow::onSetWindowTitle() {
  if (channel.isNull()) return;
  QString title;
  const int member_count = channel->room_contacts.size();
  QString member_title = member_count != 1 ? "members" : "member";

  title += channel->room_name;
  title += " " + QString("- %1 %2").arg(QString::number(member_count), member_title);

  this->setWindowTitle(title);
}

void TpContactsWindow::closeEvent(QCloseEvent *event) {
  qDebug() << "closing";
  QWidget::closeEvent(event);
}

TpContactsWindow::~TpContactsWindow() {
  qDebug() << "destroying TpContactsWindow";
  m_widgetContacts->deleteLater();
  delete ui;
}
