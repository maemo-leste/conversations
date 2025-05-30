#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QCheckBox>

#include "settings.h"
#include "config-conversations.h"
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
    emit enableGPUAccel(toggled);
  });

  // chat avatars
  ui->checkBox_enableDisplayAvatars->setChecked(config()->get(ConfigKeys::EnableDisplayAvatars).toBool());
  connect(ui->checkBox_enableDisplayAvatars, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::EnableDisplayAvatars, toggled);
    emit enableDisplayAvatarsToggled(toggled);
  });

  // chat bg gradient shader
  ui->checkBox_enableDisplayChatGradient->setChecked(config()->get(ConfigKeys::EnableDisplayChatGradient).toBool());
  connect(ui->checkBox_enableDisplayChatGradient, &QCheckBox::toggled, [this](bool toggled){
    config()->set(ConfigKeys::EnableDisplayChatGradient, toggled);
    emit enableDisplayChatGradientToggled(toggled);
  });

  // text scaling
  float textScaling = config()->get(ConfigKeys::TextScaling).toFloat();
  if(textScaling < 1.0)
    textScaling = 1.0;
  ui->label_textScalingValue->setText("x" + QString::number(textScaling));
  ui->sliderTextScaling->setValue(round(textScaling / 0.25 - 4.0));
  connect(ui->sliderTextScaling, &QSlider::valueChanged, this, &Settings::onTextScalingValueChanged);
  emit textScalingChanged();

  //connect(this->ui->btnSend, &QPushButton::clicked, this, &Settings::onGatherMessage);
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
