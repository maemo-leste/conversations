#include "QRichItemDelegate.h"

#include <QTextCursor>
#include <QTextBlock>

#include "lib/utils.h"
#include "overview/OverviewModel.h"

static constexpr int CACHE_ENTRIES = 16;

RichItemDelegate::RichItemDelegate(QObject *parent) : QStyledItemDelegate(parent), m_cache(CACHE_ENTRIES) {}

void RichItemDelegate::setStyleSheet(const QString &sheet) {
  m_styleSheet = sheet;
  m_cache.clear();
}

void RichItemDelegate::setFont(const QFont &font) {
  m_font = font;
  m_cache.clear();
}

void RichItemDelegate::setColorEmojiFamily(const QString &family) {
  m_colorEmojiFamily = family;
  m_cache.clear();
}

void RichItemDelegate::applyColorEmoji(QTextDocument *doc, const QString &html) const {
  if (m_colorEmojiFamily.isEmpty() || !Utils::mayContainEmoji(html))
    return;

  QList<QPair<int, int>> ranges;
  for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
    for (auto it = block.begin(); !it.atEnd(); ++it) {
      const QTextFragment frag = it.fragment();
      if (!frag.isValid())
        continue;
      const QString text = frag.text();
      if (!Utils::mayContainEmoji(text))
        continue;
      const int base = frag.position();
      for (const auto &r : Utils::emojiRanges(text))
        ranges.append({base + r.first, base + r.second});
    }
  }

  if (ranges.isEmpty())
    return;

  QTextCharFormat fmt;
  fmt.setFontFamilies({m_colorEmojiFamily});

  QTextCursor cursor(doc);
  cursor.beginEditBlock();
  for (const auto &r : ranges) {
    cursor.setPosition(r.first);
    cursor.setPosition(r.second, QTextCursor::KeepAnchor);
    cursor.mergeCharFormat(fmt);
  }
  cursor.endEditBlock();
}

RichItemDelegate::Entry *RichItemDelegate::entry(const QString &html, int textWidth) const {
  const QString key = textWidth < 0 ? html : QString::number(textWidth) + QLatin1Char('\n') + html;
  if (Entry *cached = m_cache.object(key))
    return cached;

  auto *e = new Entry;
  e->doc = new QTextDocument;
  e->doc->setUndoRedoEnabled(false);
  if (!m_styleSheet.isEmpty())
    e->doc->setDefaultStyleSheet(m_styleSheet);
  if (!m_font.family().isEmpty())
    e->doc->setDefaultFont(m_font);

  e->doc->setHtml(html);
  applyColorEmoji(e->doc, html);
  if (textWidth >= 0)
    e->doc->setTextWidth(textWidth);

  e->size = e->doc->size();

  if (!m_cache.insert(key, e))
    return nullptr;
  return e;
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

  Entry *e = entry(options.text, -1);
  if (e == nullptr)
    return;

  painter->save();

  options.text = "";
  options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);

  QSize iconSize = options.icon.actualSize(options.rect.size());
  painter->translate(options.rect.left() + iconSize.width(), options.rect.top());
  QRect clip(0, 0, options.rect.width() + iconSize.width(), options.rect.height());

  // avatar rectangle
  QModelIndex avatarIndex = index.sibling(index.row(), static_cast<int>(OverviewModel::Columns::AvatarColumn));
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
  painter->translate(0, 0.5 * (options.rect.height() - e->size.height()));
  e->doc->documentLayout()->draw(painter, ctx);

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

  Entry *e = entry(options.text, options.rect.width());
  if (e == nullptr)
    return {};
  return QSize(e->doc->idealWidth(), e->size.height());
}

QString RichItemDelegate::anchorAt(QString html, const QPoint &point) const {
  QTextDocument doc;
  doc.setHtml(html);

  auto textLayout = doc.documentLayout();
  Q_ASSERT(textLayout != 0);
  return textLayout->anchorAt(point);
}
