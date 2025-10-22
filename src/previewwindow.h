#pragma once

#include <QtGlobal>
#include <QResource>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QMenu>
#include <QMainWindow>
#include <QObject>
#ifdef QUICK
#include <QQuickItem>
#endif
#include <QtCore>
#include <QtGui>
#include <QFileInfo>

#include <iostream>

#include "conversations.h"
#include "lib/config.h"
#include "models/ChatModel.h"
#include "models/ChatMessage.h"
#include "overview/overviewwidget.h"
#include "models/PreviewItem.h"
#include "lib/imagepinch.h"

namespace Ui {
  class PreviewWindow;
}

class PreviewWindow : public QMainWindow {
  Q_OBJECT

public:
  Ui::PreviewWindow *ui;
  explicit PreviewWindow(Conversations *ctx, QSharedPointer<PreviewItem> item, QWidget *parent = nullptr);
  static Conversations *getContext();
  ~PreviewWindow() override;

  void grabGestures(const QVector<Qt::GestureType> &gestures);

signals:
    void closed();

protected:
  void closeEvent(QCloseEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

public slots:
  void onMenuClickedSaveFile();
  void onMenuClickedSaveFile(const QSharedPointer<PreviewItem> &item);
  void onMenuClickedCopyURL();
  void onMenuClickedCopyURL(const QSharedPointer<PreviewItem> &item);

private:
  Conversations *m_ctx;
  static PreviewWindow *pPreviewWindow;
  QSharedPointer<PreviewItem> m_item;

  QPixmap m_originalPixmap;
  ImageWidget *m_imageWidget;
};
