#include "QRichItemDelegate.h"

RichItemDelegate::RichItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{}

void RichItemDelegate::setStyleSheet(const QString &sheet) {
  m_styleSheet = sheet;
}

void RichItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  QStyleOptionViewItem options = option;
  initStyleOption(&options, index);

  painter->save();

  QTextDocument doc;
  if(!m_styleSheet.isEmpty())
    doc.setDefaultStyleSheet(m_styleSheet);

  doc.setHtml(options.text);

  options.text = "";
  options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);

  // right shift the icon, if present
  QSize iconSize = options.icon.actualSize(options.rect.size());
  painter->translate(options.rect.left() + iconSize.width(), options.rect.top());
  QRect clip(0, 0, options.rect.width() + iconSize.width(), options.rect.height());

  painter->setClipRect(clip);
  QAbstractTextDocumentLayout::PaintContext ctx;

  // adjust palette on cell selection
  // if (option.state & QStyle::State_Selected)
  //   ctx.palette.setColor(QPalette::Text, option.palette.color(QPalette::Active, QPalette::HighlightedText));
  ctx.clip = clip;

  // vcenter alignment
  painter->translate(0, 0.5 * (options.rect.height() - doc.size().height()));

  doc.documentLayout()->draw(painter, ctx);
  painter->restore();
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