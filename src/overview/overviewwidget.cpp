#include "overview/overviewwidget.h"
#include "overview/ui_overviewwidget.h"

OverviewWidget::OverviewWidget(Conversations *ctx, OverviewProxyModel *proxyModel, QWidget *parent) :
  QWidget(parent),
  m_ctx(ctx),
  m_proxyModel(proxyModel),
  ui(new Ui::OverviewWidget) {
  ui->setupUi(this);
  this->setupUITable();

  // avatars
  connect(m_ctx, &Conversations::displayAvatarsChanged, this, &OverviewWidget::onAvatarDisplayChanged);
}

int proxyColumn(QSortFilterProxyModel *proxy, int sourceColumn) {
  return proxy->mapFromSource(proxy->sourceModel()->index(0, sourceColumn)).column();
}

void OverviewWidget::setupUITable() {
  auto *table = ui->tableOverview;
  table->setIconSize(QSize(58, 58));

  // enable kinetic scrolling, disable overshoot
  QScroller::grabGesture(table, QScroller::LeftMouseButtonGesture);
  QScroller *scroller = QScroller::scroller(table);
  QScrollerProperties properties = QScroller::scroller(scroller)->scrollerProperties();
  QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff);
  properties.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, overshootPolicy);
  scroller->setScrollerProperties(properties);
  table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  const auto header = table->horizontalHeader();
  header->setMinimumSectionSize(0);
  QHeaderView *verticalHeader = table->verticalHeader();
  verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
  this->onSetTableHeight();

  table->setModel(m_proxyModel);

  auto proxyColumn = [proxy = m_proxyModel](const int sourceColumn) {
    return proxy->mapFromSource(proxy->sourceModel()->index(0, sourceColumn)).column();
  };

  // hidden columns
  for (const int col: {
      OverviewModel::COUNT,
      // OverviewModel::MsgStatusIcon,
      OverviewModel::OverviewNameRole,
      // OverviewModel::ChatTypeIcon,
      OverviewModel::ProtocolRole,
      OverviewModel::TimeRole,
      OverviewModel::AvatarIcon}) {
    table->setColumnHidden(proxyColumn(col), true);
  }

  this->onSetColumnStyleDelegate();

  header->setSectionResizeMode(proxyColumn(OverviewModel::ChatTypeIcon), QHeaderView::ResizeToContents);
  header->setSectionResizeMode(proxyColumn(OverviewModel::MsgStatusIcon), QHeaderView::ResizeToContents);
  header->setSectionResizeMode(proxyColumn(OverviewModel::PresenceIcon), QHeaderView::Fixed);
  table->setColumnWidth(proxyColumn(OverviewModel::PresenceIcon), 32);
  header->setSectionResizeMode(proxyColumn(OverviewModel::ContentRole), QHeaderView::Stretch);

  table->setFocusPolicy(Qt::NoFocus);

  // table click handler
  connect(table, &QAbstractItemView::clicked, this, [this](const QModelIndex &idx) {
    // need to go through the proxy model to figure out the underlying 
    // item in the base model. Register to OverviewModel::overviewRowClicked for 
    // the actual signal.
    m_proxyModel->onOverviewRowClicked(idx.row());
  });

  onAvatarDisplayChanged();
}

void OverviewWidget::onAvatarDisplayChanged() {
  // @TODO: implement
}

void OverviewWidget::onSetTableHeight() {
  auto itemHeight = 66;
  if (m_ctx->textScaling == 1.0) {
    itemHeight = 66;
  } else if (m_ctx->textScaling <= 1.25) {
    itemHeight = 80;
  } else if (m_ctx->textScaling <= 1.50) {
    itemHeight = 92;
  } else if (m_ctx->textScaling <= 1.75) {
    itemHeight = 100;
  } else {
    itemHeight = 112;
  }

  QHeaderView *verticalHeader = ui->tableOverview->verticalHeader();
  verticalHeader->setDefaultSectionSize(itemHeight);
}

void OverviewWidget::onSetColumnStyleDelegate() {
  if (m_richItemDelegate != nullptr)
    m_richItemDelegate->deleteLater();

  m_richItemDelegate = new RichItemDelegate(this);
  auto css_tmpl = QString(Utils::fileOpen(":/overviewRichDelegate.css"));

  //auto systemFontSize = QApplication::font().pointSize();  // @TODO: returns 18?!
  constexpr auto systemFontSize = 14;
  const unsigned int systemFontSizeScaled = systemFontSize * m_ctx->textScaling;

  //qDebug() << "scalefactor" << m_ctx->textScaling;
  //qDebug() << "size" << QString::number(systemFontSize);

  css_tmpl = css_tmpl.replace("{{ font_size }}", QString::number(systemFontSizeScaled));
  css_tmpl = css_tmpl.replace("{{ font_size_small }}", QString::number(systemFontSizeScaled - 2));
  css_tmpl = css_tmpl.replace("{{ font_size_big }}", QString::number(systemFontSizeScaled + 2));
  m_richItemDelegate->setStyleSheet(css_tmpl);
  ui->tableOverview->setItemDelegateForColumn(OverviewModel::ContentRole, m_richItemDelegate);
}

OverviewWidget::~OverviewWidget() {
  delete ui;
}
