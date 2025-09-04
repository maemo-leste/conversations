#pragma once

#include <QStyledItemDelegate>
#include <QIcon>
#include <QPainter>

// delegate that centers icons inside table cells
class CenteredIconDelegate final : public QStyledItemDelegate {
Q_OBJECT

public:
  explicit CenteredIconDelegate(QObject *parent = nullptr);

  void paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const override;

  QSize sizeHint(
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const override;

  void setIconSize(const QSize &size);

private:
  QSize m_iconSize;
};
