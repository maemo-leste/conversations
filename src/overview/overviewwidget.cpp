#include "overview/overviewwidget.h"
#include "overview/ui_overviewwidget.h"

OverviewWidget::OverviewWidget(Conversations *ctx, QWidget *parent) :
    QWidget(parent),
    m_ctx(ctx),
    ui(new Ui::OverviewWidget)
{
  ui->setupUi(this);
  this->setupUIAccounts();
  this->setupUITable();
}

void OverviewWidget::setupUITable() {
  auto *table = ui->tableOverview;

  // enable kinetic scrolling, disable overshoot
  QScroller::grabGesture(table, QScroller::LeftMouseButtonGesture);
  QScroller *scroller = QScroller::scroller(table);
  QScrollerProperties properties = QScroller::scroller(scroller)->scrollerProperties();
  QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff);
  properties.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, overshootPolicy);
  scroller->setScrollerProperties(properties);
  table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // row height
  const auto header = table->horizontalHeader();
  QHeaderView *verticalHeader = table->verticalHeader();
  verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
  verticalHeader->setDefaultSectionSize(m_ctx->overviewModel->itemHeight);

  table->setModel(m_ctx->overviewProxyModel);

  auto richItemDelegate = new RichItemDelegate(this);
  richItemDelegate->setStyleSheet(QString(Utils::fileOpen(":/overviewRichDelegate.css")));
  table->setItemDelegateForColumn(OverviewModel::ContentRole, richItemDelegate);

  header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(1, QHeaderView::Stretch);
  header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  table->setFocusPolicy(Qt::NoFocus);

  // table click handler
  connect(table, &QAbstractItemView::clicked, this, [this](const QModelIndex& idx) {
    // need to go through the proxy model to figure out the underlying 
    // item in the base model. Register to OverviewModel::overviewRowClicked for 
    // the actual signal.
    m_ctx->overviewProxyModel->onOverviewRowClicked(idx.row());
  });
}

// populate the account 'filter' buttons
void OverviewWidget::setupUIAccounts() {
  // @TODO: clear layout
  // the default; All
  this->addAccountButton("All", "*");
  m_accountButtons["*"]->setChecked(true);

  // accounts from rtcom & tp
  for(const QSharedPointer<ServiceAccount> &acc: m_ctx->serviceAccounts)
    this->addAccountButton(acc->title, acc->protocol);
}

void OverviewWidget::addAccountButton(const QString title, const QString service) {
  if(m_accountButtons.contains(service))
    return;

  const auto item = new OverviewBtnWidget(title, service);
  connect(item, &OverviewBtnWidget::clicked, this, &OverviewWidget::onAccountButtonClicked);
  auto insert_pos = m_accountButtons.keys().size();
  m_accountButtons[service] = item;

  // add to UI
  ui->protocolLayout->insertWidget(insert_pos, item);
}

void OverviewWidget::onAccountButtonClicked(const QString service) {
  for(const auto &k: m_accountButtons.keys())  // reset all to False
    m_accountButtons[k]->setChecked(false);

  if(!m_accountButtons.contains(service)) {
    return;
  }

  auto *item = m_accountButtons[service];
  item->setChecked(true);

  // filter overview table
  m_ctx->overviewProxyModel->setProtocolFilter(item->service);
}

OverviewWidget::~OverviewWidget() {
  delete ui;
}
