#include "settingswidget.h"
#include "ui_settingswidget.h"

// #include "config-conversations.h"
// ui->label_version->setText(QString("Version: ") + CONVERSATIONS_VERSION);

SettingsWidget::SettingsWidget(Conversations *ctx, QWidget *parent) :
  QWidget(parent),
  m_ctx(ctx),
  ui(new Ui::SettingsWidget) {
  ui->setupUi(this);

  auto theme = config()->get(ConfigKeys::ChatTheme).toString();
  if (theme == "irssi")
    ui->radio_theme_irssi->setChecked(true);
  else
    ui->radio_theme_whatsthat->setChecked(true);

  connect(ui->themeRadioGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [this](QAbstractButton *button) {
    auto name = button->objectName();
    if (name == "radio_theme_whatsthat") {
      config()->set(ConfigKeys::ChatTheme, "whatsthat");
    } else if (name == "radio_theme_irssi") {
      config()->set(ConfigKeys::ChatTheme, "irssi");
    }
  });

  // Enter key sends chat message toggle
  ui->checkBox_enterKeySendsChat->setChecked(config()->get(ConfigKeys::EnterKeySendsChat).toBool());
  connect(ui->checkBox_enterKeySendsChat, &QCheckBox::toggled, [this](bool toggled) {
    config()->set(ConfigKeys::EnterKeySendsChat, toggled);
    emit enterKeySendsChatToggled(toggled);
  });

  // Enable notifications
  ui->checkBox_enableNotifications->setChecked(config()->get(ConfigKeys::EnableNotifications).toBool());
  connect(ui->checkBox_enableNotifications, &QCheckBox::toggled, [](bool toggled) {
    config()->set(ConfigKeys::EnableNotifications, toggled);
  });

  // Auto-close chat windows on inactivity
  ui->checkBox_enableAutoCloseChatWindows->setChecked(config()->get(ConfigKeys::EnableAutoCloseChatWindows).toBool());
  connect(ui->checkBox_enableAutoCloseChatWindows, &QCheckBox::toggled, [](bool toggled) {
    config()->set(ConfigKeys::EnableAutoCloseChatWindows, toggled);
  });

  // respect system themes
  ui->checkBox_enableInheritSystemTheme->setChecked(config()->get(ConfigKeys::EnableInheritSystemTheme).toBool());
  connect(ui->checkBox_enableInheritSystemTheme, &QCheckBox::toggled, [this](bool toggled) {
    config()->set(ConfigKeys::EnableInheritSystemTheme, toggled);
    emit inheritSystemThemeToggled(toggled);
  });

  // groupchat join/leave
  ui->checkBox_enableDisplayGroupchatJoinLeave->setChecked(
      config()->get(ConfigKeys::EnableDisplayGroupchatJoinLeave).toBool());
  connect(ui->checkBox_enableDisplayGroupchatJoinLeave, &QCheckBox::toggled, [this](bool toggled) {
    config()->set(ConfigKeys::EnableDisplayGroupchatJoinLeave, toggled);
    emit enableDisplayGroupchatJoinLeaveToggled(toggled);
  });

  // GPU acceleration
  ui->checkBox_enableGPUAccel->setChecked(config()->get(ConfigKeys::EnableGPUAccel).toBool());
  connect(ui->checkBox_enableGPUAccel, &QCheckBox::toggled, [this](bool toggled) {
    config()->set(ConfigKeys::EnableGPUAccel, toggled);
    if (const auto reply = QMessageBox::question(this, "Quit", "Quit application now?",
                                                 QMessageBox::Yes | QMessageBox::No); reply == QMessageBox::Yes)
      QApplication::quit();
    emit enableGPUAccel(toggled);
  });

  // chat avatars
  ui->checkBox_enableDisplayAvatars->setChecked(config()->get(ConfigKeys::EnableDisplayAvatars).toBool());
  connect(ui->checkBox_enableDisplayAvatars, &QCheckBox::toggled, [this](bool toggled) {
    config()->set(ConfigKeys::EnableDisplayAvatars, toggled);
    emit enableDisplayAvatarsToggled(toggled);
  });

  // Log: writing
  ui->checkBox_logWriteToDisk->setChecked(config()->get(ConfigKeys::EnableLogWrite).toBool());
  connect(ui->checkBox_logWriteToDisk, &QCheckBox::toggled, [this](bool toggled) {
    config()->set(ConfigKeys::EnableLogWrite, toggled);
  });

  // Log: syslog
  ui->checkBox_logWriteToSyslog->setChecked(config()->get(ConfigKeys::EnableLogSyslog).toBool());
  connect(ui->checkBox_logWriteToSyslog, &QCheckBox::toggled, [this](bool toggled) {
    config()->set(ConfigKeys::EnableLogSyslog, toggled);
  });

  // Log: TP
  const QString path_log_tp = m_ctx->configDirectory + ".log_tp";
  ui->checkBox_logTP->setChecked(QFile::exists(path_log_tp));
  connect(ui->checkBox_logTP, &QCheckBox::toggled, [this, path_log_tp](const bool toggled) {
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
  connect(ui->checkBox_logGlib, &QCheckBox::toggled, [this, path_log_glib](const bool toggled) {
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

  // attachment preview options
  ui->checkBox_enablePreview->setChecked(config()->get(ConfigKeys::LinkPreviewEnabled).toBool());
  ui->checkBox_enablePreviewInlineImages->setChecked(config()->get(ConfigKeys::LinkPreviewImageEnabled).toBool());
  ui->checkBox_enablePreviewUserInteraction->setChecked(
      config()->get(ConfigKeys::LinkPreviewRequiresUserInteraction).toBool());

  connect(ui->checkBox_enablePreview, &QCheckBox::toggled, [this](bool toggled) {
    config()->set(ConfigKeys::LinkPreviewEnabled, toggled);
    emit enableLinkPreviewEnabledToggled(toggled);
  });

  connect(ui->checkBox_enablePreviewInlineImages, &QCheckBox::toggled, [this](bool toggled) {
    config()->set(ConfigKeys::LinkPreviewImageEnabled, toggled);
    emit enableLinkPreviewImageEnabledToggled(toggled);
  });

  connect(ui->checkBox_enablePreviewUserInteraction, &QCheckBox::toggled, [this](bool toggled) {
    config()->set(ConfigKeys::LinkPreviewRequiresUserInteraction, toggled);
    emit enableLinkPreviewRequiresUserInteractionToggled(toggled);
  });

  // attachment max download size
  int attachmentMaxDownloadSize = config()->get(ConfigKeys::LinkPreviewMaxDownloadSize).toInt();
  QString maxAttachmentDownloadSize_text = "Max. download size: " + QString::number(attachmentMaxDownloadSize) + "mb";
  ui->label_attachmentMaxDownloadSize->setText(maxAttachmentDownloadSize_text);
  ui->sliderMaxAttachmentSize->setValue(attachmentMaxDownloadSize);
  connect(ui->sliderMaxAttachmentSize, &QSlider::valueChanged, [this, attachmentMaxDownloadSize](int value) {
    config()->set(ConfigKeys::LinkPreviewMaxDownloadSize, value);
    QString _maxAttachmentDownloadSize_text = "Max. download size: " + QString::number(value) + "mb";
    ui->label_attachmentMaxDownloadSize->setText(_maxAttachmentDownloadSize_text);
    emit attachmentMaxDownloadSizeChanged(attachmentMaxDownloadSize * 1024 * 1024);
  });
  emit attachmentMaxDownloadSizeChanged(attachmentMaxDownloadSize * 1024 * 1024);

  // chat bg gradient shader
  ui->checkBox_enableDisplayChatGradient->setChecked(config()->get(ConfigKeys::EnableDisplayChatGradient).toBool());
  connect(ui->checkBox_enableDisplayChatGradient, &QCheckBox::toggled, [this](bool toggled) {
    config()->set(ConfigKeys::EnableDisplayChatGradient, toggled);
    emit enableDisplayChatGradientToggled(toggled);
  });

  ui->checkBox_enableSlim->setChecked(config()->get(ConfigKeys::EnableSlim).toBool());
  connect(ui->checkBox_enableSlim, &QCheckBox::toggled, [this](const bool toggled) {
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

    if (const auto reply = QMessageBox::question(
        this, "Quit", "Quit application now?",
        QMessageBox::Yes | QMessageBox::No); reply == QMessageBox::Yes)
      QApplication::quit();
    emit enableSlimToggled(toggled);
  });

  // text scaling
  float textScaling = config()->get(ConfigKeys::TextScaling).toFloat();
  if (textScaling < 1.0)
    textScaling = 1.0;
  ui->label_textScalingValue->setText("x" + QString::number(textScaling));
  ui->sliderTextScaling->setValue(round(textScaling / 0.25 - 4.0));
  connect(ui->sliderTextScaling, &QSlider::valueChanged, this, &SettingsWidget::onTextScalingValueChanged);
  emit textScalingChanged();
}

void SettingsWidget::onAttachmentMaxDownloadSize(float val) {

}

void SettingsWidget::onTextScalingValueChanged(int val) {
  float scaling = 1.0 + (val * 0.25);
  ui->label_textScalingValue->setText("x" + QString::number(scaling));
  config()->set(ConfigKeys::TextScaling, scaling);
  emit textScalingChanged();
}

SettingsWidget::~SettingsWidget() {
  delete ui;
}
