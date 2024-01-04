#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QCheckBox>

#ifdef QUICK
#include <QQmlContext>
#endif

#include "settings.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_settings.h"

Settings * Settings::pSettings = nullptr;

Settings::Settings(Conversations *ctx, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Settings),
    m_ctx(ctx) {
  pSettings = this;
  ui->setupUi(this);

#ifdef MAEMO
  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);
#endif
  QPixmap p_theme_whatsthat(":whatsthat/whatsthat.png");
  QPixmap p_theme_chatty(":chatty/chatty.png");
  QPixmap p_theme_irssi(":irssi/irssi.png");

  ui->label_chatty_img->setPixmap(p_theme_chatty);
  ui->label_whatsthat_img->setPixmap(p_theme_whatsthat);
  ui->label_irssi_img->setPixmap(p_theme_irssi);

  auto theme = config()->get(ConfigKeys::ChatTheme).toString();
  if(theme == "chatty")
    ui->radio_theme_chatty->setChecked(true);
  else if(theme == "irssi")
    ui->radio_theme_irssi->setChecked(true);
  else
    ui->radio_theme_whatsthat->setChecked(true);

  connect(ui->themeRadioGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [=](QAbstractButton *button) {
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
  connect(ui->checkBox_enterKeySendsChat, &QCheckBox::toggled, [](bool toggled){
    config()->set(ConfigKeys::EnterKeySendsChat, toggled);
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
  connect(ui->checkBox_enableInheritSystemTheme, &QCheckBox::toggled, [=](bool toggled){
    config()->set(ConfigKeys::EnableInheritSystemTheme, toggled);
    emit inheritSystemThemeToggled(toggled);
  });

  // text scaling
  float textScaling = config()->get(ConfigKeys::TextScaling).toFloat();
  if(textScaling < 1) textScaling = 1;
  auto scaling = 100*(textScaling-1);
  ui->label_textScalingValue->setText("x" + QString::number(textScaling));
  ui->sliderTextScaling->setValue(scaling);
  connect(ui->sliderTextScaling, &QSlider::valueChanged, this, &Settings::onTextScalingValueChanged);
  emit textScalingChanged();

  //connect(this->ui->btnSend, &QPushButton::clicked, this, &Settings::onGatherMessage);
}

void Settings::onTextScalingValueChanged(int val) {
  float scaling;
  if(val == 0) scaling = 1.0;
  else if(val <= 25) scaling = 1.25;
  else if(val <= 50) scaling = 1.50;
  else if(val <= 75) scaling = 1.75;
  else if(val <= 100) scaling = 2.0;
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
