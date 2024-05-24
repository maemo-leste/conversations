#pragma once
#include <QObject>
#include <QSize>
#include <QTextDocument>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QAbstractTextDocumentLayout>

class RichItemDelegate : public QStyledItemDelegate {
Q_OBJECT

public:
  explicit RichItemDelegate(QObject *parent = nullptr);
  void setStyleSheet(const QString &sheet);
  QString anchorAt(QString html, const QPoint &point) const;

protected:
  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
  QString m_styleSheet;
};