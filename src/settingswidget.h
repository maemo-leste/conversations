#pragma once
#include <QCompleter>
#include <QCheckBox>
#include <QPushButton>
#include <QStringList>
#include <QClipboard>
#include <QScroller>
#include <QStringListModel>
#include <QTimer>
#include <QEasingCurve>
#include <QMessageBox>
#include <QWidget>
#include <QMenu>

#include "conversations.h"
#include "lib/utils.h"

namespace Ui {
  class SettingsWidget;
}

class SettingsWidget : public QWidget
{
Q_OBJECT

public:
  explicit SettingsWidget(Conversations *ctx, QWidget *parent = nullptr);
  ~SettingsWidget() override;
signals:
  void textScalingChanged();
  void attachmentMaxDownloadSizeChanged(int val);
  void autoCloseChatWindowsChanged(bool enabled);
  void inheritSystemThemeToggled(bool enabled);
  void enableDisplayGroupchatJoinLeaveToggled(bool enabled);
  void enableDisplayAvatarsToggled(bool enabled);
  void enableNewVersionMessageToggled(bool enabled);
  void enableGPUAccel(bool enabled);
  void enableDisplayChatGradientToggled(bool enabled);
  void enableSlimToggled(bool enabled);
  void enterKeySendsChatToggled(bool enabled);
  void enableLinkPreviewEnabledToggled(bool enabled);
  void enableLinkPreviewImageEnabledToggled(bool enabled);
  void enableLinkPreviewRequiresUserInteractionToggled(bool enabled);
  void bgMatrixRainEnabledToggled(bool enabled);
private slots:
  void onTextScalingValueChanged(int val);
  void onAttachmentMaxDownloadSize(float val);
  void onAskQuitApplication();
private:
  Ui::SettingsWidget *ui;
  Conversations *m_ctx;
};
