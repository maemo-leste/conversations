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

#include "previewwindow.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_previewwindow_widgets.h"

PreviewWindow * PreviewWindow::pPreviewWindow = nullptr;
PreviewWindow::PreviewWindow(Conversations *ctx, QSharedPointer<PreviewItem> item, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PreviewWindow),
    m_item(item),
    m_ctx(ctx) {
  setAttribute(Qt::WA_DeleteOnClose);
  setAttribute(Qt::WA_AcceptTouchEvents);
  setAttribute(Qt::WA_TouchPadAcceptSingleTouchEvents);
  pPreviewWindow = this;
  ui->setupUi(this);
  ui->menuBar->hide();

  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);

  connect(ui->actionSave_file, &QAction::triggered, this, [this] {
    onMenuClickedSaveFile();
  });

  connect(ui->actionCopy_url, &QAction::triggered, this, [this] {
    onMenuClickedCopyURL();
  });

  m_imageWidget = new ImageWidget(this);
  m_imageWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  QVector<Qt::GestureType> gestures;
  gestures << Qt::PanGesture;
  gestures << Qt::PinchGesture;
  gestures << Qt::SwipeGesture;
  m_imageWidget->grabGestures(gestures);
  setCentralWidget(m_imageWidget);

  if (m_item && !m_item->filePath.isEmpty()) {
    m_imageWidget->appendImage(m_item);
  }
}

void PreviewWindow::onMenuClickedCopyURL(const QSharedPointer<PreviewItem> &item) {
  QClipboard *clipboard = QGuiApplication::clipboard();
  clipboard->setText(item->url.toString());

  QMessageBox _msgBox;
  _msgBox.setText(QString("URL copied."));
  _msgBox.exec();
}

void PreviewWindow::onMenuClickedSaveFile(const QSharedPointer<PreviewItem> &item) {
  const auto info = QFileInfo(item->filePath);
  const auto fn = QString("%1/MyDocs/%2").arg(QDir::homePath()).arg(info.fileName());

  QDir dir(QDir::homePath() + "/MyDocs");
  if (!dir.exists())
    dir.mkpath(".");

  auto result = item->saveFile(fn, false);

  if (result == PreviewItem::SaveResult::AlreadyExists) {
    const auto reply = QMessageBox::question(
      this,
      "Overwrite File?",
      QString("The file \"%1\" already exists.\nDo you want to overwrite it?").arg(fn),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
      result = item->saveFile(fn, true);
    } else {
      return;
    }
  }

  if (result == PreviewItem::SaveResult::Success) {
    QMessageBox::information(this, "Success", QString("File written to %1").arg(fn));
  } else {
    QMessageBox::warning(this, "Error", item->lastError());
  }
}

void PreviewWindow::onMenuClickedCopyURL() {
  const auto item = m_imageWidget->items.at(m_imageWidget->pos);
  onMenuClickedCopyURL(item);
}

void PreviewWindow::onMenuClickedSaveFile() {
  const auto item = m_imageWidget->items.at(m_imageWidget->pos);
  onMenuClickedSaveFile(item);
}

void PreviewWindow::grabGestures(const QVector<Qt::GestureType> &gestures) {
  m_imageWidget->grabGestures(gestures);
}

Conversations *PreviewWindow::getContext(){
  return pPreviewWindow->m_ctx;
}

void PreviewWindow::resizeEvent(QResizeEvent *event) {
  // QMainWindow::resizeEvent(event);
  //
  // if (!m_originalPixmap.isNull()) {
  //   ui->lbl_image->setPixmap(m_originalPixmap.scaled(
  //       ui->lbl_image->size(),
  //       Qt::KeepAspectRatio,
  //       Qt::SmoothTransformation
  //   ));
  // }
}

void PreviewWindow::closeEvent(QCloseEvent *event) {
  qDebug() << "closing";
  QWidget::closeEvent(event);
}

PreviewWindow::~PreviewWindow() {
  qDebug() << "destroying PreviewWindow";
  delete ui;
}
