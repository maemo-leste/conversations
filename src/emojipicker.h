#pragma once
#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "conversations.h"

class EmojiPicker : public QWidget {
Q_OBJECT

public:
  explicit EmojiPicker(Conversations *ctx, QWidget *parent = nullptr);
  ~EmojiPicker() override = default;

signals:
  void emojiPicked(const QString &emoji);

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
  void onShowCategory(int index);
  void onPrevPage();
  void onNextPage();

private:
  void calc_metrics();
  void page_populate();
  void clear_grid();
  int page_count() const;

  Conversations *m_ctx;
  QButtonGroup *m_category_group;
  QWidget *m_grid_container;
  QWidget *m_gridWidget;
  QGridLayout *m_grid_layout;
  QPushButton *m_btn_prev;
  QPushButton *m_btn_next;
  QLabel *m_label_page;

  int m_currentCategory = 0;
  int m_currentPage = 0;

  QSize m_last_window_size;
  int m_cellSize = 0;
  int m_columns = 8;
  int m_rows = 0;
  int m_pageSize = 0;
};
