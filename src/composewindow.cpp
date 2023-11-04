#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include <QQmlContext>
#include <QMessageBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QCheckBox>

#include "composewindow.h"
#include "config-conversations.h"
#include "lib/globals.h"

#include "ui_composewindow.h"

Compose * Compose::pCompose = nullptr;

Compose::Compose(Conversations *ctx, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ComposeWindow),
    m_ctx(ctx) {
  pCompose = this;
  ui->setupUi(this);

  setProperty("X-Maemo-StackedWindow", 1);
  setProperty("X-Maemo-Orientation", 2);

  connect(this->ui->btnTo, &QPushButton::clicked, [=]{
    //
  });

  connect(this->ui->btnSend, &QPushButton::clicked, [=]{
    //
  });
}

void Compose::closeEvent(QCloseEvent *event) {
  QWidget::closeEvent(event);
}

Compose::~Compose() {
  delete ui;
}
