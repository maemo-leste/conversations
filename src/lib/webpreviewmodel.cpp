#include <cmath>

#include "webpreviewmodel.h"

#include "conversations.h"

QMap<unsigned int, QSharedPointer<PreviewModel>> webPreviewCache = {};

PreviewModel::ItemLookup PreviewModel::get_item(const QUrl &url) {
  if (const auto it = std::ranges::find_if(items, [&](const auto &item) { return item->url == url; });
    it != items.end()) {
    return {*it, static_cast<int>(std::distance(items.begin(), it)), true};
  }
  return {};
}

PreviewModel::PreviewModel(const unsigned int event_id, QObject *parent) :
    m_event_id(event_id), QAbstractListModel(parent) {
  const auto *ctx = Conversations::instance();
  m_httpHelper = new WebPreviewHTTP(this);
  m_state = USER_WAIT;

  m_requires_user_interaction = config()->get(ConfigKeys::LinkPreviewRequiresUserInteraction).toBool();
  if (!m_requires_user_interaction)
    m_displayButton = false;

  m_inlineImages = config()->get(ConfigKeys::LinkPreviewImageEnabled).toBool();

  connect(m_httpHelper, &WebPreviewHTTP::headFinished, this, [this](const HttpHeadResult &result) {
    auto [item, idx, found] = get_item(result.url);
    if (!found)
      return;

    if (!result.valid) {
      item->state = PreviewItem::State::Head;
      item->error = result.errorString;
      item->subtitle = result.errorString;
    } else {
      auto type = PreviewItem::ItemType::Unknown;
      if (result.contentType.startsWith("image/"))
        type = PreviewItem::ItemType::Image;
      else if (result.contentType.startsWith("audio/"))
        type = PreviewItem::ItemType::Audio;
      else if (result.contentType.contains("html"))
        type = PreviewItem::ItemType::Html;
      else if (result.contentType.startsWith("video/"))
        type = PreviewItem::ItemType::Video;
      else if (result.contentType.contains("application/pdf"))
        type = PreviewItem::ItemType::Pdf;

      item->state = PreviewItem::State::Head;
      item->itemType = type;
      item->contentType = result.contentType;
      item->contentLength = result.contentLength;
      item->subtitle = result.contentType;
      item->error = "";
    }

    const QModelIndex modelIndex = this->index(idx);
    emit dataChanged(modelIndex, modelIndex);
    emit totalHeightChanged();

    m_pendingHeads--;
    if (m_pendingHeads == 0) {
      setState(HEAD_FINISHED);
      buttonPressed();
    }
  });

  connect(m_httpHelper, &WebPreviewHTTP::downloadFinished, this, [this](const QString &filePath, const QUrl &url) {
    qDebug() << "Download complete:" << filePath;

    auto [item, idx, found] = get_item(url);
    if (!found)
      return;

    item->state = PreviewItem::State::Downloaded;
    item->filePath = filePath;
    if (item->itemType == PreviewItem::ItemType::Image)
      item->displayType = PreviewItem::DisplayType::Image;

    // get download info
    if (item->itemType == PreviewItem::ItemType::Html) {
      item->title = item->previewHTMLTitle();
      item->subtitle = item->name;
    } else if (item->displayType == PreviewItem::DisplayType::Image) {
      item->title = item->name;
      item->subtitle = "Image";
#ifdef QUICK
      PreviewImageProvider::generateSize(item->filePath);
#endif
    } else {
      item->title = item->name;
      item->subtitle = "Downloaded";
#ifdef QUICK
      PreviewImageProvider::generateSize(item->filePath);
#endif
    }

    const QModelIndex modelIndex = this->index(idx);
    emit dataChanged(modelIndex, modelIndex);

    m_pendingDownloads -= 1;
    if (m_pendingDownloads == 0)
      setState(RESULT);
  });

  connect(m_httpHelper, &WebPreviewHTTP::downloadProgress, this, [this](qint64 received, qint64 total, const QUrl &url) {
    // account for total being `-1` when server does not supply content-length
    if (total > 0) {
      m_progressMap[url] = qMakePair(received, total);
    }

    qint64 total_received = 0;
    qint64 total_size = 0;

    for (auto it = m_progressMap.constBegin(); it != m_progressMap.constEnd(); ++it) {
      if (it.value().second > 0) {
        total_received += it.value().first;
        total_size += it.value().second;
      }
    }

    if (total_size > 0) {
      int pct = static_cast<int>(std::ceil(
          static_cast<double>(total_received) /
          static_cast<double>(total_size) * 100.0
          ));

      if (pct < 0)
        pct = 0;
      if (pct > 100)
        pct = 100;

      if (m_downloadProgress != pct) {
        m_downloadProgress = pct;
        emit downloadProgressChanged();
      }
    }
  });

  connect(m_httpHelper, &WebPreviewHTTP::downloadError, this, [this](const QString &message, const QUrl &url) {
    qWarning() << "Download failed for" << url << ":" << message;

    auto [item, idx, found] = get_item(url);
    if (!found)
      return;

    item->title = item->name;
    item->subtitle = message;
    item->state = PreviewItem::State::Head;
    item->displayType = PreviewItem::DisplayType::Text;
    item->itemType = PreviewItem::ItemType::Error;
    item->error = message;

    const QModelIndex modelIndex = this->index(idx);
    emit dataChanged(modelIndex, modelIndex);

    m_pendingDownloads--;
    if (m_pendingDownloads == 0)
      setState(RESULT);
  });

  connect(ctx, &Conversations::enableLinkPreviewImageEnabledToggled, [=](bool enabled) {
    m_inlineImages = enabled;
    emit inlineImagesChanged();
    setState(USER_WAIT);
  });

  connect(ctx, &Conversations::enableLinkPreviewRequiresUserInteractionToggled, [=](bool enabled) {
    m_requires_user_interaction = enabled;
    if (m_state != RESULT) {
      m_displayButton = true;
      emit displayButtonChanged();
    }
  });
}

unsigned int PreviewModel::totalHeight() {
  unsigned int total_height = 0;
  if (m_displayButton)
    total_height += m_buttonHeight;

  total_height += resultsHeight();
  // qDebug() << "totalHeight()" << total_height;
  return total_height;
}

unsigned int PreviewModel::resultsHeight(unsigned int width) {
  unsigned int total_height = 0;
  if (m_state == USER_WAIT) {
    return 0;
  }

  for (const auto &item: items)
    total_height += item->height(width);

  if (total_height > 0) {
    // note: + listview spacing
    total_height += 2 * (items.length() - 1);
  }

  // qDebug() << "resultsHeight()" << total_height;
  return total_height;
}

void PreviewModel::setState(const PreviewModel::State state) {
  if (m_state == state)
    return;

  m_state = state;
  qDebug() << "new state:" << m_state;

  if (m_state == RESULT) {
    m_displayButton = std::ranges::any_of(items, [](const auto& item) {
      return item->hasError();
    });
  } else {
    m_displayButton = true;
  }

  emit totalHeightChanged();
  emit stateChanged();
  emit displayButtonChanged();
  emit resultsHeightChanged();
}

void PreviewModel::buttonPressed(const bool from_user_interaction) {
  const bool linkPreviewImageEnabled = config()->get(ConfigKeys::LinkPreviewImageEnabled).toBool();

  if (m_state == USER_WAIT) {
    setState(HEAD);
    m_pendingHeads = items.size();

    for (int idx = 0; idx < items.size(); idx++) {
      const auto item = items.at(idx);
      item->title = item->name;
      item->state = PreviewItem::State::Head;
      item->subtitle = "fetching HEAD";

      const QModelIndex modelIndex = this->index(idx);
      emit dataChanged(modelIndex, modelIndex);

      m_httpHelper->head(item);
    }
    return;
  }

  if (m_state == HEAD_FINISHED) {
    m_pendingDownloads = 0;

    QList<QSharedPointer<PreviewItem>> downloadQueue;

    for (int idx = 0; idx < items.size(); idx++) {
      const auto item = items.at(idx);
      // skip errors
      if (item->itemType == PreviewItem::ItemType::Error)
        continue;

      if (item->itemType != PreviewItem::ItemType::Error &&
          item->itemType != PreviewItem::ItemType::Unknown) {

        if (item->itemType == PreviewItem::ItemType::Image && !linkPreviewImageEnabled)
          continue;

        item->title = "Fetching";
        item->subtitle = item->name;
        const QModelIndex modelIndex = this->index(idx);
        emit dataChanged(modelIndex, modelIndex);

        m_pendingDownloads++;
        downloadQueue << item;
      }
    }

    // async download
    for (const auto &item: downloadQueue)
      m_httpHelper->download(item);

    setState(DOWNLOADING);

    if (m_pendingDownloads == 0)
      setState(RESULT);
  }

  if (m_state == RESULT) {
    const bool has_errors = std::ranges::any_of(items, [](const auto& item) {
      return item->hasError();
    });

    if (has_errors && from_user_interaction) {
      setState(USER_WAIT);
      buttonPressed();
    }
  }
}

void PreviewModel::addLinks(const QStringList &links) {
  if (m_state != USER_WAIT) {
    qWarning() << "addLinks called *after* USER_WAIT, aborting";
    return;
  }

  beginInsertRows(QModelIndex(), items.size(), items.size());

  for (const auto &link: links) {
    auto _url = QUrl(link);

    auto item = QSharedPointer<PreviewItem>(new PreviewItem(this));
    item->name = _url.host();
    item->url = _url;
    item->state = PreviewItem::State::Idle;

    items.push_back(item);
  }

  endInsertRows();
  emit resultsHeightChanged();
}

int PreviewModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return items.size();
}

QVariant PreviewModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= items.size())
    return {};

  const auto &item = items.at(index.row());
  switch (role) {
    case NameRole:
      return item->name;
    case ItemHeightRole:
      return item->height();
    case ItemTypeRole:
      return static_cast<int>(item->itemType);
    case ContentTypeRole:
      return item->contentType;
    case ErrorRole:
      return item->error;
    case UrlRole:
      return item->url.toString();
    case hasErrorRole:
      return item->hasError();
    case StateRole:
      return static_cast<int>(item->state);
    case DisplayTypeRole:
      return static_cast<int>(item->displayType);
    case TitleRole:
      return item->title;
    case FilePathRole:
      return item->filePath;
    case SubtitleRole:
      return item->subtitle;
    case IconRole:
      return item->icon();
    case RenderRole: {
      if (item->state == PreviewItem::State::Idle)
        return false;
      return true;
    }
    default:
      return {};
  }
}

QHash<int, QByteArray> PreviewModel::roleNames() const {
  return {
      {NameRole, "name"},
      {ItemHeightRole, "itemHeight"},
      {ItemTypeRole, "itemType"},
      {ContentTypeRole, "contentType"},
      {ErrorRole, "error"},
      {UrlRole, "url"},
      {hasErrorRole, "hasError"},
      {StateRole, "state"},
      {RenderRole, "display"},
      {TitleRole, "titleRole"},
      {SubtitleRole, "subtitleRole"},
      {DisplayTypeRole, "displayType"},
      {IconRole, "icon"},
      {FilePathRole, "filepath"}
  };
}

// int PreviewModel::itemHeight(const int index, const int width) const {
//   const auto item = items.at(index);
//   return item->height(width);
// }

void PreviewModel::itemClicked(unsigned int index, const QPoint point) {
  qDebug() << "item clicked" << index << point;
  const auto item = items.at(index);
  emit previewItemClicked(item, point);
}

PreviewModel::State PreviewModel::state() const {
  return m_state;
}
