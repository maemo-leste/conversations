#include "aboutwidget.h"
#include "ui_aboutwidget.h"

#include "config-conversations.h"
#include "changelog.h"
#include "lib/globals.h"


AboutWidget::AboutWidget(Conversations *ctx, QWidget *parent) :
  QWidget(parent),
  m_ctx(ctx),
  ui(new Ui::AboutWidget) {
  ui->setupUi(this);

  ui->label_version->setText(QString("Conversations version: ") + CONVERSATIONS_VERSION);

  // populate debian changelog @ About
  auto changelog = Utils::parseDebianChangelog(QString::fromStdString(globals::debian_changelog));

  QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->widget_changelog->layout());

  QLabel *titleLabel = new QLabel("Changelog", ui->widget_changelog);
  titleLabel->setStyleSheet("padding-bottom:12px;");
  QFont titleFont = titleLabel->font();
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);
  layout->addWidget(titleLabel);

  for (int i = 0; i < changelog.size(); ++i) {
    const QMap<QString, QString> &map = changelog.at(i);

    if (map.contains("version")) {
      QLabel *versionLabel = new QLabel(map.value("version"), ui->widget_changelog);
      versionLabel->setStyleSheet("padding-bottom:12px;");
      QFont versionFont = versionLabel->font();
      int currentSize = versionFont.pointSize();
      versionFont.setPointSize(currentSize + 2);
      versionLabel->setFont(versionFont);
      versionFont.setBold(true);
      versionLabel->setFont(versionFont);
      layout->addWidget(versionLabel);
    }

    if (map.contains("body")) {
      QLabel *bodyLabel = new QLabel(map.value("body"), ui->widget_changelog);
      bodyLabel->setWordWrap(true);
      layout->addWidget(bodyLabel);
    }

    if (map.contains("meta")) {
      QLabel *metaLabel = new QLabel(map.value("meta"), ui->widget_changelog);
      QPalette palette = metaLabel->palette();
      palette.setColor(QPalette::WindowText, palette.color(QPalette::Disabled, QPalette::WindowText));
      metaLabel->setPalette(palette);
      metaLabel->setStyleSheet("padding-top: 6px; padding-bottom:10px;");
      metaLabel->setWordWrap(true);
      layout->addWidget(metaLabel);

      QFrame *line = new QFrame(ui->widget_changelog);
      line->setFrameShape(QFrame::HLine);
      line->setFrameShadow(QFrame::Sunken);
      layout->addWidget(line);
    }

    if (i>7) break;
  }

  layout->addStretch();
}

AboutWidget::~AboutWidget() {
  delete ui;
}
