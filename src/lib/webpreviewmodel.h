#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QUrl>

#include "models/PreviewItem.h"
#include "downloadpreview.h"

class PreviewModel;
extern QMap<unsigned int, QSharedPointer<PreviewModel>> webPreviewCache;

class PreviewModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(State state READ state WRITE setState NOTIFY stateChanged)
  Q_PROPERTY(int totalHeight READ totalHeight NOTIFY totalHeightChanged)
  Q_PROPERTY(int resultsHeight READ resultsHeight NOTIFY resultsHeightChanged)
  Q_PROPERTY(int buttonHeight MEMBER m_buttonHeight NOTIFY buttonHeightChanged)
  Q_PROPERTY(bool displayButton MEMBER m_displayButton NOTIFY displayButtonChanged)
  Q_PROPERTY(int downloadProgress MEMBER m_downloadProgress NOTIFY downloadProgressChanged)
  Q_PROPERTY(bool inlineImages MEMBER m_inlineImages NOTIFY inlineImagesChanged)

public:
  enum State {
    USER_WAIT = 0,
    HEAD,
    HEAD_FINISHED,
    DOWNLOADING,
    RESULT
  };
  Q_ENUM(State)

  enum Roles {
    NameRole = Qt::UserRole + 1,
    ItemHeightRole,
    ItemTypeRole,
    ContentTypeRole,
    ErrorRole,
    UrlRole,
    TitleRole,
    SubtitleRole,
    hasErrorRole,
    RenderRole,
    StateRole,
    IconRole,
    DisplayTypeRole,
    FilePathRole
  };

  struct ItemLookup {
    QSharedPointer<PreviewItem> item = nullptr;
    int idx = -1;
    bool found = false;
  };

  PreviewModel::ItemLookup get_item(const QUrl &url);
  explicit PreviewModel(unsigned int event_id, QObject *parent = nullptr);

  QList<QSharedPointer<PreviewItem>> items;

  [[nodiscard]] State state() const;
  void setState(State state);

  void addLinks(const QStringList& links);

  Q_INVOKABLE void buttonPressed(bool from_user_interaction = false);
  Q_INVOKABLE void itemClicked(unsigned int index, const QPoint point);

  unsigned int totalHeight();
  unsigned int resultsHeight(unsigned int width = 0);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

signals:
  void stateChanged();
  void downloadProgressChanged();
  void totalHeightChanged();
  void resultsHeightChanged();
  void buttonHeightChanged();
  void displayButtonChanged();
  void previewItemClicked(QSharedPointer<PreviewItem> item, const QPoint point);
  void inlineImagesChanged();

private:
  unsigned int m_event_id;

  unsigned int m_buttonHeight = 52;
  State m_state = USER_WAIT;
  WebPreviewHTTP* m_httpHelper = nullptr;

  int m_pendingHeads = 0;
  int m_pendingDownloads = 0;
  bool m_autoDownloadAfterHEAD = false;

  bool m_displayButton = true;
  unsigned int m_totalHeight = 0;

  bool m_inlineImages = false;
  bool m_requires_user_interaction = false;
  int m_downloadProgress = 0;
  QHash<QUrl, QPair<qint64, qint64>> m_progressMap;
};
