#include "QCenteredIconDelegate.h"

CenteredIconDelegate::CenteredIconDelegate(QObject *parent) :
    QStyledItemDelegate(parent), m_iconSize(16, 16) {}

void CenteredIconDelegate::paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const {
  painter->save();

  QStyleOptionViewItem opt(option);
  initStyleOption(&opt, index);

  // debug: green bg
  // if (opt.state & QStyle::State_Selected) {
  //   // selection color
  //   painter->fillRect(opt.rect, opt.palette.highlight());
  // } else {
  //   // green
  //   painter->fillRect(opt.rect, QColor(0, 200, 0));
  // }

  // centered icon
  QVariant value = index.data(Qt::DecorationRole);
  if (value.canConvert<QIcon>()) {
    auto icon = qvariant_cast<QIcon>(value);

    QRect rect = opt.rect;
    int x = rect.x() + (rect.width() - m_iconSize.width()) / 2;
    int y = rect.y() + (rect.height() - m_iconSize.height()) / 2;
    QRect iconRect(QPoint(x, y), m_iconSize);

    icon.paint(
      painter, iconRect, Qt::AlignCenter,
      opt.state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled);
  }

  painter->restore();
}

QSize CenteredIconDelegate::sizeHint(
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const {

  if (index.data(Qt::DecorationRole).canConvert<QIcon>()) {
    return m_iconSize;
  }

  return QStyledItemDelegate::sizeHint(option, index);
}

void CenteredIconDelegate::setIconSize(const QSize &size) {
  m_iconSize = size;
}
