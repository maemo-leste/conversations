#pragma once
#include <QObject>
#include <QSize>
#include <QFont>
#include <QTextDocument>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QAbstractTextDocumentLayout>

class RichItemDelegate : public QStyledItemDelegate {
Q_OBJECT

public:
  explicit RichItemDelegate(QObject *parent = nullptr);
  void setStyleSheet(const QString &sheet);
  void setFont(const QFont &font);
  void setColorEmojiFamily(const QString &family);
  QString anchorAt(QString html, const QPoint &point) const;

protected:
  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
  void applyColorEmoji(QTextDocument *doc) const;

  QString m_styleSheet;
  QFont m_font;
  QString m_colorEmojiFamily;
};