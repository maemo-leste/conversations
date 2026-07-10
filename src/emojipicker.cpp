#include <QScreen>
#include <QEvent>

#include "emojipicker.h"
#include "emojis.h"

static constexpr int num_columns_emojis = 8;  // per row
static constexpr int column_emojis_spacing = 6;
static constexpr int column_min_size = 40;

EmojiPicker::EmojiPicker(Conversations *ctx, QWidget *parent) :
  QWidget(parent),
  m_ctx(ctx),
  m_category_group(new QButtonGroup(this)),
  m_grid_container(new QWidget(this)),
  m_gridWidget(new QWidget(m_grid_container)),
  m_grid_layout(new QGridLayout(m_gridWidget)),
  m_btn_prev(new QPushButton(QStringLiteral("‹"), this)),
  m_btn_next(new QPushButton(QStringLiteral("›"), this)),
  m_label_page(new QLabel(this)) {

  if (const QScreen *screen = QGuiApplication::primaryScreen()) {
    const int height = static_cast<int>(screen->geometry().height() * 0.75);
    if (height > 0)
      setMinimumHeight(height);
  }

  const auto layout = new QVBoxLayout(this);

  // emoji categories
  const auto categoryBar = new QHBoxLayout();
  categoryBar->setSpacing(2);
  m_category_group->setExclusive(true);

  // fill
  const auto &categories = emojiCategories();
  for (int i = 0; i < categories.size(); i++) {
    const auto &category = categories.at(i);
    const auto btn = new QPushButton(category.icon, this);
    btn->setCheckable(true);
    btn->setToolTip(category.name);
    btn->setFocusPolicy(Qt::NoFocus);
    m_category_group->addButton(btn, i);
    categoryBar->addWidget(btn);
  }
  layout->addLayout(categoryBar);

  // black pane that fills the available area with the emoji grid centered inside it
  m_grid_container->setAutoFillBackground(true);
  QPalette containerPal = m_grid_container->palette();
  containerPal.setColor(QPalette::Window, Qt::black);
  m_grid_container->setPalette(containerPal);

  // grid itself
  m_grid_layout->setContentsMargins(0, 0, 0, 0);
  m_grid_layout->setSpacing(column_emojis_spacing);

  const auto containerLayout = new QVBoxLayout(m_grid_container);
  containerLayout->setContentsMargins(0, 0, 0, 0);
  containerLayout->addWidget(m_gridWidget, 0, Qt::AlignCenter);
  layout->addWidget(m_grid_container, 1);

  m_grid_container->installEventFilter(this);

  // page navigation bar
  const auto pageBar = new QHBoxLayout();
  m_btn_prev->setFocusPolicy(Qt::NoFocus);
  m_btn_next->setFocusPolicy(Qt::NoFocus);
  m_label_page->setAlignment(Qt::AlignCenter);
  pageBar->addWidget(m_btn_prev);
  pageBar->addWidget(m_label_page, 1);
  pageBar->addWidget(m_btn_next);
  layout->addLayout(pageBar);

  connect(m_category_group, &QButtonGroup::idClicked, this, &EmojiPicker::onShowCategory);
  connect(m_btn_prev, &QPushButton::clicked, this, &EmojiPicker::onPrevPage);
  connect(m_btn_next, &QPushButton::clicked, this, &EmojiPicker::onNextPage);

  if (const auto first = m_category_group->button(0))
    first->setChecked(true);
  m_currentCategory = 0;
}

bool EmojiPicker::eventFilter(QObject *obj, QEvent *event) {
  if (obj == m_grid_container && event->type() == QEvent::Resize) {
    const QSize before = m_last_window_size;
    calc_metrics();
    if (m_last_window_size != before)   // re-calc
      page_populate();
  }
  return QWidget::eventFilter(obj, event);
}

void EmojiPicker::calc_metrics() {
  const QSize vp = m_grid_container->size();
  if (vp.width() <= 0 || vp.height() <= 0)
    return;
  if (vp == m_last_window_size)
    return;
  m_last_window_size = vp;

  m_columns = num_columns_emojis;

  const int cell = qMax(column_min_size, (vp.width() - column_emojis_spacing * (m_columns - 1)) / m_columns);
  m_cellSize = cell;
  m_rows = qMax(1, (vp.height() + column_emojis_spacing) / (cell + column_emojis_spacing));
  m_pageSize = m_columns * m_rows;
}

void EmojiPicker::clear_grid() {
  while (QLayoutItem *item = m_grid_layout->takeAt(0)) {
    if (QWidget *w = item->widget())
      w->deleteLater();
    delete item;
  }
}

void EmojiPicker::page_populate() {
  clear_grid();
  if (m_pageSize <= 0)
    return;

  const auto &categories = emojiCategories();
  if (m_currentCategory < 0 || m_currentCategory >= categories.size())
    return;
  const QStringList &emojis = categories.at(m_currentCategory).emojis;

  const int pages = page_count();
  m_currentPage = qBound(0, m_currentPage, pages - 1);

  const int start = m_currentPage * m_pageSize;
  const int end = qMin(start + m_pageSize, static_cast<int>(emojis.size()));

  QFont emojiFont = m_gridWidget->font();
  emojiFont.setPixelSize(qMax(20, static_cast<int>(m_cellSize * 0.6)));

  for (int i = start; i < end; i++) {
    const int pos = i - start;
    const QString emoji = emojis.at(i);

    const auto btn = new QPushButton(emoji, m_gridWidget);
    btn->setFlat(true);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setFixedSize(m_cellSize, m_cellSize);   // whole cell is clickable
    btn->setFont(emojiFont);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton { border: none; background: transparent; }"
        "QPushButton:pressed { background: #333333; border-radius: 6px; }"));
    connect(btn, &QPushButton::clicked, this, [this, emoji] { emit emojiPicked(emoji); });

    m_grid_layout->addWidget(btn, pos / m_columns, pos % m_columns);
  }

  m_label_page->setText(QStringLiteral("%1 / %2").arg(m_currentPage + 1).arg(pages));
  m_btn_prev->setEnabled(m_currentPage > 0);
  m_btn_next->setEnabled(m_currentPage < pages - 1);
}

int EmojiPicker::page_count() const {
  if (m_pageSize <= 0)
    return 1;
  const auto &categories = emojiCategories();
  if (m_currentCategory < 0 || m_currentCategory >= categories.size())
    return 1;
  const int total = static_cast<int>(categories.at(m_currentCategory).emojis.size());
  return qMax(1, (total + m_pageSize - 1) / m_pageSize);
}

void EmojiPicker::onShowCategory(int index) {
  m_currentCategory = index;
  m_currentPage = 0;
  page_populate();
}

void EmojiPicker::onPrevPage() {
  if (m_currentPage > 0) {
    m_currentPage--;
    page_populate();
  }
}

void EmojiPicker::onNextPage() {
  if (m_currentPage < page_count() - 1) {
    m_currentPage++;
    page_populate();
  }
}
