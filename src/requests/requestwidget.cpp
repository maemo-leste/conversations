#include "requests/requestwidget.h"
#include "requests/ui_requestwidget.h"

RequestWidget::RequestWidget(Conversations *ctx, QWidget *parent) :
    QWidget(parent),
    m_ctx(ctx),
    ui(new Ui::RequestWidget)
{
  ui->setupUi(this);
  this->setupUITable();
}

void RequestWidget::setupUITable() {
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
  this->onSetTableHeight();

  table->setModel(m_ctx->requestModel);
  table->setColumnHidden(RequestModel::COUNT, true);
  table->setColumnHidden(RequestModel::ButtonRole, true);

  this->onSetContentDelegate();

  header->setSectionResizeMode(RequestModel::MsgStatusIcon, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(RequestModel::ContentRole, QHeaderView::Stretch);
  table->setFocusPolicy(Qt::NoFocus);

  // // table click handler
  connect(table, &QAbstractItemView::clicked, this, [this](const QModelIndex& idx) {
    QSharedPointer<ContactItem> contact_item = m_ctx->requestModel->requests.at(idx.row());
    emit openDialog(contact_item);
  });
}

void RequestWidget::onSetTableHeight() {
  m_itemHeight = 66;
  if(m_ctx->textScaling == 1.0) {
    m_itemHeight = 66;
  } else if (m_ctx->textScaling <= 1.25) {
    m_itemHeight = 80;
  } else if (m_ctx->textScaling <= 1.50) {
    m_itemHeight = 92;
  } else if (m_ctx->textScaling <= 1.75) {
    m_itemHeight = 100;
  } else {
    m_itemHeight = 112;
  }

  QHeaderView *verticalHeader = ui->tableOverview->verticalHeader();
  verticalHeader->setDefaultSectionSize(m_itemHeight);
}

void RequestWidget::onSetContentDelegate() {
  if(m_richItemDelegate != nullptr)
    m_richItemDelegate->deleteLater();

  m_richItemDelegate = new RichItemDelegate(this);
  auto css_tmpl = QString(Utils::fileOpen(":/overviewRichDelegate.css"));

  //auto systemFontSize = QApplication::font().pointSize();  // @TODO: returns 18?!
  auto systemFontSize = 14;
  unsigned int systemFontSizeScaled = systemFontSize * m_ctx->textScaling;

  //qDebug() << "scalefactor" << m_ctx->textScaling;
  //qDebug() << "size" << QString::number(systemFontSize);

  css_tmpl = css_tmpl.replace("{{ font_size }}", QString::number(systemFontSizeScaled));
  css_tmpl = css_tmpl.replace("{{ font_size_small }}", QString::number(systemFontSizeScaled - 2));
  css_tmpl = css_tmpl.replace("{{ font_size_big }}", QString::number(systemFontSizeScaled + 2));
  m_richItemDelegate->setStyleSheet(css_tmpl);
  ui->tableOverview->setItemDelegateForColumn(RequestModel::ContentRole, m_richItemDelegate);
}

RequestWidget::~RequestWidget() {
  delete ui;
}
