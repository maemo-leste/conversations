#include "overview/overviewwidget.h"
#include "overview/ui_overviewwidget.h"

OverviewWidget::OverviewWidget(Conversations *ctx, QWidget *parent) :
    QWidget(parent),
    m_ctx(ctx),
    ui(new Ui::OverviewWidget)
{
  ui->setupUi(this);
  this->setupUITable();

  // avatars
  connect(m_ctx, &Conversations::displayAvatarsChanged, this, &OverviewWidget::onAvatarDisplayChanged);
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

  // row height

  const auto header = table->horizontalHeader();
  QHeaderView *verticalHeader = table->verticalHeader();
  verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
  this->onSetTableHeight();

  table->setModel(m_ctx->overviewProxyModel);
  table->setColumnHidden(OverviewModel::ProtocolRole, true);
  table->setColumnHidden(OverviewModel::TimeRole, true);
  table->setColumnHidden(OverviewModel::COUNT, true);

  this->onSetColumnStyleDelegate();

  header->setSectionResizeMode(OverviewModel::MsgStatusIcon, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(OverviewModel::ContentRole, QHeaderView::Stretch);
  header->setSectionResizeMode(OverviewModel::PresenceIcon, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(OverviewModel::ChatTypeIcon, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(OverviewModel::AvatarIcon, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(OverviewModel::AvatarPadding, QHeaderView::ResizeToContents);
  table->setFocusPolicy(Qt::NoFocus);

  // table click handler
  connect(table, &QAbstractItemView::clicked, this, [this](const QModelIndex& idx) {
    // need to go through the proxy model to figure out the underlying 
    // item in the base model. Register to OverviewModel::overviewRowClicked for 
    // the actual signal.
    m_ctx->overviewProxyModel->onOverviewRowClicked(idx.row());
  });

  onAvatarDisplayChanged();
}

void OverviewWidget::onAvatarDisplayChanged() {
  bool enableDisplayAvatars = config()->get(ConfigKeys::EnableDisplayAvatars).toBool();
  ui->tableOverview->setColumnHidden(OverviewModel::AvatarIcon, !enableDisplayAvatars);
  ui->tableOverview->setColumnHidden(OverviewModel::AvatarPadding, !enableDisplayAvatars);
}

void OverviewWidget::onSetTableHeight() {
  auto itemHeight = 66;
  if(m_ctx->textScaling == 1.0) {
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
  if(m_richItemDelegate != nullptr)
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
