#include "QRichItemDelegate.h"

#include "overview/OverviewModel.h"

RichItemDelegate::RichItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void RichItemDelegate::setStyleSheet(const QString &sheet) {
  m_styleSheet = sheet;
}

// debug: red
// void RichItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
//   painter->save();
//   painter->fillRect(option.rect, Qt::red);
//   painter->restore();
// }

void RichItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  QStyleOptionViewItem options = option;
  initStyleOption(&options, index);

  painter->save();

  QTextDocument doc;
  if (!m_styleSheet.isEmpty())
    doc.setDefaultStyleSheet(m_styleSheet);

  doc.setHtml(options.text);

  options.text = "";
  options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);

  QSize iconSize = options.icon.actualSize(options.rect.size());
  painter->translate(options.rect.left() + iconSize.width(), options.rect.top());
  QRect clip(0, 0, options.rect.width() + iconSize.width(), options.rect.height());

  // avatar rectangle
  QModelIndex avatarIndex = index.sibling(index.row(), OverviewModel::AvatarIcon);
  QPixmap avatarPixmap = avatarIndex.data(Qt::DecorationRole).value<QPixmap>();

  QRect avatarRect;
  if (!avatarPixmap.isNull()) {
    const int padding_r = 12;
    QSize avatar_size = avatarPixmap.size();
    avatarRect = QRect(
        option.rect.right() - avatar_size.width() - padding_r,
        option.rect.top() + (option.rect.height() - avatar_size.height()) / 2,
        avatar_size.width(),
        avatar_size.height()
        );

    // exclude avatar region from text
    QRegion textRegion(clip);
    textRegion = textRegion.subtracted(avatarRect.translated(-option.rect.left() - iconSize.width(),
                                                             -option.rect.top()));
    painter->setClipRegion(textRegion);
  } else {
    painter->setClipRect(clip);
  }

  // draw text
  QAbstractTextDocumentLayout::PaintContext ctx;
  ctx.clip = clip;

  // vcenter alignment
  painter->translate(0, 0.5 * (options.rect.height() - doc.size().height()));
  doc.documentLayout()->draw(painter, ctx);

  painter->restore();

  // draw avatar last
  if (!avatarPixmap.isNull()) {
    painter->save();
    painter->drawPixmap(avatarRect, avatarPixmap);
    painter->restore();
  }
}

QSize RichItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
  QStyleOptionViewItem options = option;
  initStyleOption(&options, index);

  QTextDocument doc;
  doc.setHtml(options.text);
  doc.setTextWidth(options.rect.width());
  return QSize(doc.idealWidth(), doc.size().height());
}

QString RichItemDelegate::anchorAt(QString html, const QPoint &point) const {
  QTextDocument doc;
  doc.setHtml(html);

  auto textLayout = doc.documentLayout();
  Q_ASSERT(textLayout != 0);
  return textLayout->anchorAt(point);
}
