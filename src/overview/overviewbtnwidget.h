#pragma once
#include <QCompleter>
#include <QPushButton>
#include <QStringListModel>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QWidget>
#include <QMenu>

#include "conversations.h"
#include "lib/utils.h"

namespace Ui {
  class OverviewBtnWidget;
}

class OverviewBtnWidget : public QWidget
{
Q_OBJECT

public:
  explicit OverviewBtnWidget(const QString title, const QString service, QWidget *parent = nullptr);
  ~OverviewBtnWidget() override;
  void setChecked(bool status);
  bool checked = false;
  QString service;

signals:
  void clicked(QString service);

private:
  QString m_title;
  Ui::OverviewBtnWidget *ui;
};
