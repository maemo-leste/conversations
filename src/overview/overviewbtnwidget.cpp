#include "overview/overviewbtnwidget.h"
#include "overview/ui_overviewbtnwidget.h"

OverviewBtnWidget::OverviewBtnWidget(const QString title, const QString service, QWidget *parent) :
    QWidget(parent),
    m_title(title),
    service(service),
    ui(new Ui::OverviewBtnWidget)
{
  ui->setupUi(this);
  ui->lbl_title->setText(m_title);

  connect(ui->clickFrame, &ClickFrame::clicked, [this, service] {
    emit clicked(service);
  });

  setChecked(false);
}

void OverviewBtnWidget::setChecked(bool status) {
  checked = status;
  if(checked) {
    ui->dotsFrame->setStyleSheet("border:0; background-image: url(:/overviewdots.png); background-repeat: repeat-x;");
  } else {
    ui->dotsFrame->setStyleSheet("border:0;background-image:");
  }
}

OverviewBtnWidget::~OverviewBtnWidget() {
  delete ui;
}
