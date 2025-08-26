#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QCheckBox>
#include <QPushButton>

#include "settings.h"
#include "config-conversations.h"
#include "changelog.h"
#include "lib/globals.h"

#include "config-conversations.h"
#include "ui_settings.h"

Settings * Settings::pSettings = nullptr;

Settings::Settings(Conversations *ctx, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Settings),
    m_ctx(ctx) {
  pSettings = this;
  ui->setupUi(this);

  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);

  ui->label_version->setText(QString("Version: ") + CONVERSATIONS_VERSION);

  auto theme = config()->get(ConfigKeys::ChatTheme).toString();
  if(theme == "chatty")
    ui->radio_theme_chatty->setChecked(true);
  else if(theme == "irssi")
    ui->radio_theme_irssi->setChecked(true);
  else
    ui->radio_theme_whatsthat->setChecked(true);

  connect(ui->themeRadioGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [this](QAbstractButton *button) {
      auto name = button->objectName();
      if(name == "radio_theme_whatsthat") {
        config()->set(ConfigKeys::ChatTheme, "whatsthat");
      } else if(name == "radio_theme_chatty") {
        config()->set(ConfigKeys::ChatTheme, "chatty");
      } else if(name == "radio_theme_irssi") {
        config()->set(ConfigKeys::ChatTheme, "irssi");
      }
  });

  // Enter key sends chat message toggle
  ui->checkBox_enterKeySendsChat->setChecked(config()->get(ConfigKeys::EnterKeySendsChat).toBool());
  connect(ui->checkBox_enterKeySendsChat, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::EnterKeySendsChat, toggled);
    emit enterKeySendsChatToggled(toggled);
  });

  // Enable notifications
  ui->checkBox_enableNotifications->setChecked(config()->get(ConfigKeys::EnableNotifications).toBool());
  connect(ui->checkBox_enableNotifications, &QCheckBox::toggled, [](bool toggled){
    config()->set(ConfigKeys::EnableNotifications, toggled);
  });

  // Auto-close chat windows on inactivity
  ui->checkBox_enableAutoCloseChatWindows->setChecked(config()->get(ConfigKeys::EnableAutoCloseChatWindows).toBool());
  connect(ui->checkBox_enableAutoCloseChatWindows, &QCheckBox::toggled, [](bool toggled){
    config()->set(ConfigKeys::EnableAutoCloseChatWindows, toggled);
  });

  // respect system themes
  ui->checkBox_enableInheritSystemTheme->setChecked(config()->get(ConfigKeys::EnableInheritSystemTheme).toBool());
  connect(ui->checkBox_enableInheritSystemTheme, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::EnableInheritSystemTheme, toggled);
    emit inheritSystemThemeToggled(toggled);
  });

  // groupchat join/leave
  ui->checkBox_enableDisplayGroupchatJoinLeave->setChecked(config()->get(ConfigKeys::EnableDisplayGroupchatJoinLeave).toBool());
  connect(ui->checkBox_enableDisplayGroupchatJoinLeave, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::EnableDisplayGroupchatJoinLeave, toggled);
    emit enableDisplayGroupchatJoinLeaveToggled(toggled);
  });

  // GPU acceleration
  ui->checkBox_enableGPUAccel->setChecked(config()->get(ConfigKeys::EnableGPUAccel).toBool());
  connect(ui->checkBox_enableGPUAccel, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::EnableGPUAccel, toggled);
    if(const auto reply = QMessageBox::question(this, "Quit", "Quit application now?", QMessageBox::Yes | QMessageBox::No); reply == QMessageBox::Yes)
      QApplication::quit();
    emit enableGPUAccel(toggled);
  });

  // chat avatars
  ui->checkBox_enableDisplayAvatars->setChecked(config()->get(ConfigKeys::EnableDisplayAvatars).toBool());
  connect(ui->checkBox_enableDisplayAvatars, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::EnableDisplayAvatars, toggled);
    emit enableDisplayAvatarsToggled(toggled);
  });

  // Log: writing
  ui->checkBox_logWriteToDisk->setChecked(config()->get(ConfigKeys::EnableLogWrite).toBool());
  connect(ui->checkBox_logWriteToDisk, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::EnableLogWrite, toggled);
  });

  // Log: syslog
  ui->checkBox_logWriteToSyslog->setChecked(config()->get(ConfigKeys::EnableLogSyslog).toBool());
  connect(ui->checkBox_logWriteToSyslog, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::EnableLogSyslog, toggled);
  });

  // Log: TP
  const QString path_log_tp = m_ctx->configDirectory + ".log_tp";
  ui->checkBox_logTP->setChecked(QFile::exists(path_log_tp));
  connect(ui->checkBox_logTP, &QCheckBox::toggled, [this, path_log_tp](const bool toggled){
    if (toggled) {
      if (!QFile::exists(path_log_tp)) {
          QFile file(path_log_tp);
          file.open(QIODevice::WriteOnly);
          file.close();
      }
    } else {
      if (QFile::exists(path_log_tp))
          QFile::remove(path_log_tp);
    }
  });

  // Log: GLib/osso
  const QString path_log_glib = m_ctx->configDirectory + ".log_glib";
  ui->checkBox_logGlib->setChecked(QFile::exists(path_log_glib));
  connect(ui->checkBox_logGlib, &QCheckBox::toggled, [this, path_log_glib](const bool toggled){
    if (toggled) {
      if (!QFile::exists(path_log_glib)) {
          QFile file(path_log_glib);
          file.open(QIODevice::WriteOnly);
          file.close();
      }
    } else {
      if (QFile::exists(path_log_glib))
          QFile::remove(path_log_glib);
    }
  });

  connect(ui->btn_quitConversations, &QPushButton::clicked, [this]() {
    if(const auto reply = QMessageBox::question(this, "Quit", "Quit application now?", QMessageBox::Yes | QMessageBox::No); reply == QMessageBox::Yes)
      QApplication::quit();
  });

  // attachment preview options
  ui->checkBox_enablePreview->setChecked(config()->get(ConfigKeys::LinkPreviewEnabled).toBool());
  ui->checkBox_enablePreviewInlineImages->setChecked(config()->get(ConfigKeys::LinkPreviewImageEnabled).toBool());
  ui->checkBox_enablePreviewUserInteraction->setChecked(config()->get(ConfigKeys::LinkPreviewRequiresUserInteraction).toBool());

  connect(ui->checkBox_enablePreview, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::LinkPreviewEnabled, toggled);
    emit enableLinkPreviewEnabledToggled(toggled);
  });

  connect(ui->checkBox_enablePreviewInlineImages, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::LinkPreviewImageEnabled, toggled);
    emit enableLinkPreviewImageEnabledToggled(toggled);
  });

  connect(ui->checkBox_enablePreviewUserInteraction, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::LinkPreviewRequiresUserInteraction, toggled);
    emit enableLinkPreviewRequiresUserInteractionToggled(toggled);
  });

  // chat bg gradient shader
  ui->checkBox_enableDisplayChatGradient->setChecked(config()->get(ConfigKeys::EnableDisplayChatGradient).toBool());
  connect(ui->checkBox_enableDisplayChatGradient, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::EnableDisplayChatGradient, toggled);
    emit enableDisplayChatGradientToggled(toggled);
  });

  ui->checkBox_enableSlim->setChecked(config()->get(ConfigKeys::EnableSlim).toBool());
  connect(ui->checkBox_enableSlim, &QCheckBox::toggled, [this](const bool toggled){
    config()->set(ConfigKeys::EnableSlim, toggled);

    const QString path = m_ctx->configDirectory + "slim";
    if (toggled) {
      if (!QFile::exists(path)) {
          QFile file(path);
          file.open(QIODevice::WriteOnly);
          file.close();
      }
    } else {
      if (QFile::exists(path)) {
          QFile::remove(path);
      }
    }

    if(const auto reply = QMessageBox::question(this, "Quit", "Quit application now?", QMessageBox::Yes | QMessageBox::No); reply == QMessageBox::Yes)
      QApplication::quit();

    emit enableSlimToggled(toggled);
  });

  // text scaling
  float textScaling = config()->get(ConfigKeys::TextScaling).toFloat();
  if(textScaling < 1.0)
    textScaling = 1.0;
  ui->label_textScalingValue->setText("x" + QString::number(textScaling));
  ui->sliderTextScaling->setValue(round(textScaling / 0.25 - 4.0));
  connect(ui->sliderTextScaling, &QSlider::valueChanged, this, &Settings::onTextScalingValueChanged);
  emit textScalingChanged();

  // populate debian changelog @ About
  auto changelog = Utils::parseDebianChangelog(QString::fromStdString(globals::debian_changelog));

  QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->widget_changelog->layout());

  QLabel *titleLabel = new QLabel("Changelog", ui->widget_changelog);
  titleLabel->setStyleSheet("padding-bottom:12px;");
  QFont titleFont = titleLabel->font();
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);
  layout->addWidget(titleLabel);

  for (int i = 0; i < changelog.size(); ++i) {
    const QMap<QString, QString> &map = changelog.at(i);

    if (map.contains("version")) {
      QLabel *versionLabel = new QLabel(map.value("version"), ui->widget_changelog);
      versionLabel->setStyleSheet("padding-bottom:12px;");
      QFont versionFont = versionLabel->font();
      int currentSize = versionFont.pointSize();
      versionFont.setPointSize(currentSize + 2);
      versionLabel->setFont(versionFont);
      versionFont.setBold(true);
      versionLabel->setFont(versionFont);
      layout->addWidget(versionLabel);
    }

    if (map.contains("body")) {
      QLabel *bodyLabel = new QLabel(map.value("body"), ui->widget_changelog);
      bodyLabel->setWordWrap(true);
      layout->addWidget(bodyLabel);
    }

    if (map.contains("meta")) {
      QLabel *metaLabel = new QLabel(map.value("meta"), ui->widget_changelog);
      QPalette palette = metaLabel->palette();
      palette.setColor(QPalette::WindowText, palette.color(QPalette::Disabled, QPalette::WindowText));
      metaLabel->setPalette(palette);
      metaLabel->setStyleSheet("padding-top: 6px; padding-bottom:10px;");
      metaLabel->setWordWrap(true);
      layout->addWidget(metaLabel);

      QFrame *line = new QFrame(ui->widget_changelog);
      line->setFrameShape(QFrame::HLine);
      line->setFrameShadow(QFrame::Sunken);
      layout->addWidget(line);
    }

    if (i>7) break;
  }

  layout->addStretch();
}

void Settings::onTextScalingValueChanged(int val) {
  float scaling = 1.0 + (val * 0.25);
  ui->label_textScalingValue->setText("x" + QString::number(scaling));
  config()->set(ConfigKeys::TextScaling, scaling);
  emit textScalingChanged();
}

Conversations *Settings::getContext(){
  return pSettings->m_ctx;
}

void Settings::closeEvent(QCloseEvent *event) {
  QWidget::closeEvent(event);
}

Settings::~Settings() {
  delete ui;
}
