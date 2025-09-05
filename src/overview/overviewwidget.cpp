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
  QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(
      QScrollerProperties::OvershootAlwaysOff);
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

  // map source->proxy column
  auto proxyColumn = [proxy = m_proxyModel](const int sourceColumn) {
    QModelIndex src;
    if (proxy->sourceModel() && proxy->sourceModel()->rowCount() > 0)
      src = proxy->sourceModel()->index(0, sourceColumn);
    return proxy->mapFromSource(src).column();
  };

  auto applyHeaderConfig = [this, table, header, proxyColumn] {
    // hidden columns
    for (const int col: {
        OverviewModel::COUNT,
        // OverviewModel::MsgStatusIcon,
        OverviewModel::OverviewNameRole,
        // OverviewModel::ChatTypeIcon,
        OverviewModel::ProtocolRole,
        OverviewModel::TimeRole,
        OverviewModel::AvatarIcon}) {
      if (const int pcol = proxyColumn(col); pcol >= 0)
        table->setColumnHidden(pcol, true);
    }

    int col;
    col = proxyColumn(OverviewModel::ChatTypeIcon);
    if (col >= 0)
      header->setSectionResizeMode(col, QHeaderView::ResizeToContents);

    col = proxyColumn(OverviewModel::MsgStatusIcon);
    if (col >= 0)
      header->setSectionResizeMode(col, QHeaderView::ResizeToContents);

    col = proxyColumn(OverviewModel::PresenceIcon);
    if (col >= 0) {
      header->setSectionResizeMode(col, QHeaderView::Fixed);
      table->setColumnWidth(col, 48);
    }

    col = proxyColumn(OverviewModel::ContentRole);
    if (col >= 0)
      header->setSectionResizeMode(col, QHeaderView::Stretch);
  };

  this->onSetColumnStyleDelegate();
  applyHeaderConfig();

  if (m_proxyModel->sourceModel()) {
    connect(m_proxyModel->sourceModel(), &QAbstractItemModel::modelReset, this, applyHeaderConfig);
    connect(m_proxyModel->sourceModel(), &QAbstractItemModel::rowsInserted, this, applyHeaderConfig);
  }

  table->setFocusPolicy(Qt::NoFocus);

  connect(table, &QAbstractItemView::clicked, this, [this](const QModelIndex &idx) {
    // go through proxy model to figure out underlying item
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
  if (m_richContentDelegate != nullptr)
    m_richContentDelegate->deleteLater();
  if (m_centeredIconDelegate != nullptr)
    m_centeredIconDelegate->deleteLater();

  m_centeredIconDelegate = new CenteredIconDelegate(this);
  m_centeredIconDelegate->setIconSize(QSize(24, 24));
  m_richContentDelegate = new RichItemDelegate(this);
  auto css_tmpl = QString(Utils::fileOpen(":/overviewRichDelegate.css"));

  //auto systemFontSize = QApplication::font().pointSize();  // @TODO: returns 18?!
  constexpr auto systemFontSize = 14;
  const unsigned int systemFontSizeScaled = systemFontSize * m_ctx->textScaling;

  //qDebug() << "scalefactor" << m_ctx->textScaling;
  //qDebug() << "size" << QString::number(systemFontSize);

  css_tmpl = css_tmpl.replace("{{ font_size }}", QString::number(systemFontSizeScaled));
  css_tmpl = css_tmpl.replace("{{ font_size_small }}", QString::number(systemFontSizeScaled - 2));
  css_tmpl = css_tmpl.replace("{{ font_size_big }}", QString::number(systemFontSizeScaled + 2));
  m_richContentDelegate->setStyleSheet(css_tmpl);
  ui->tableOverview->setItemDelegateForColumn(OverviewModel::ContentRole, m_richContentDelegate);
  ui->tableOverview->setItemDelegateForColumn(OverviewModel::PresenceIcon, m_centeredIconDelegate);
}

OverviewWidget::~OverviewWidget() {
  delete ui;
}
