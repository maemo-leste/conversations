#include "contacts/TpContactsWidget.h"
#include "contacts/ui_TpContactsWidget.h"

TpContactsWidget::TpContactsWidget(TpContactsProxyModel *proxyModel, QWidget *parent) :
  QWidget(parent),
  m_proxyModel(proxyModel),
  ui(new Ui::TpContactsWidget) {
  ui->setupUi(this);
  this->setupUITable();
}

int TpContactsWidget::proxyColumn(QSortFilterProxyModel *proxy, int sourceColumn) {
  return proxy->mapFromSource(proxy->sourceModel()->index(0, sourceColumn)).column();
}

void TpContactsWidget::setupUITable() {
  auto *table = ui->tableTpContacts;
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
      TpContactsModel::COUNT,
      TpContactsModel::TpContactsNameRole
  }) {
    table->setColumnHidden(proxyColumn(col), true);
  }

  this->onSetColumnStyleDelegate();

  header->setSectionResizeMode(proxyColumn(TpContactsModel::ChatTypeIcon), QHeaderView::ResizeToContents);
  header->setSectionResizeMode(proxyColumn(TpContactsModel::MsgStatusIcon), QHeaderView::ResizeToContents);
  header->setSectionResizeMode(proxyColumn(TpContactsModel::ContentRole), QHeaderView::Stretch);

  table->setFocusPolicy(Qt::NoFocus);

  // table click handler
  connect(table, &QAbstractItemView::clicked, this, [this](const QModelIndex &idx) {
    // need to go through the proxy model to figure out the underlying 
    // item in the base model. Register to TpContactsModel::overviewRowClicked for
    // the actual signal.
    m_proxyModel->onTpContactsRowClicked(idx.row());
  });
}

void TpContactsWidget::onSetTableHeight() {
  const auto ctx = Conversations::instance();
  auto itemHeight = 66;
  if (ctx->textScaling == 1.0) {
    itemHeight = 66;
  } else if (ctx->textScaling <= 1.25) {
    itemHeight = 80;
  } else if (ctx->textScaling <= 1.50) {
    itemHeight = 92;
  } else if (ctx->textScaling <= 1.75) {
    itemHeight = 100;
  } else {
    itemHeight = 112;
  }

  QHeaderView *verticalHeader = ui->tableTpContacts->verticalHeader();
  verticalHeader->setDefaultSectionSize(itemHeight);
}

void TpContactsWidget::onSetColumnStyleDelegate() {
  const auto ctx = Conversations::instance();
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
  const unsigned int systemFontSizeScaled = systemFontSize * ctx->textScaling;

  //qDebug() << "scalefactor" << m_ctx->textScaling;
  //qDebug() << "size" << QString::number(systemFontSize);

  css_tmpl = css_tmpl.replace("{{ font_size }}", QString::number(systemFontSizeScaled));
  css_tmpl = css_tmpl.replace("{{ font_size_small }}", QString::number(systemFontSizeScaled - 2));
  css_tmpl = css_tmpl.replace("{{ font_size_big }}", QString::number(systemFontSizeScaled + 2));
  m_richContentDelegate->setStyleSheet(css_tmpl);
  ui->tableTpContacts->setItemDelegateForColumn(TpContactsModel::ContentRole, m_richContentDelegate);
}

TpContactsWidget::~TpContactsWidget() {
  delete ui;
}
