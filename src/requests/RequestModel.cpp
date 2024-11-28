#include <QObject>
#include <QDebug>

#include "requests/RequestModel.h"
#include "lib/tp/tp.h"

RequestModel::RequestModel(Telepathy *tp, QObject *parent) :
    m_tp(tp),
    QAbstractListModel(parent) {
  this->preloadPixmaps();
}

int RequestModel::rowCount(const QModelIndex & parent) const {
  Q_UNUSED(parent);
  return requests.count();
}

int RequestModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return RequestModelRoles::COUNT;
}

void RequestModel::onLoad() {
  beginResetModel();

  this->requests.clear();
  for(const auto& contact: abook_roster_cache) {
    if(contact->published() == "local-pending") {
      requests << contact;
    }
  }

  endResetModel();
  emit changed();
}

void RequestModel::addEntry(QSharedPointer<ContactItem> roster_item) {
  beginResetModel();
  requests << roster_item;
  endResetModel();
  emit changed();
}

void RequestModel::removeEntry(QSharedPointer<ContactItem> roster_item) {
  QList<QSharedPointer<ContactItem>> _requests;
  beginResetModel();

  for(auto &r: requests) {
    if(r->local_uid() == roster_item->local_uid() &&
        r->id() == roster_item->id())
      continue;

    _requests << r;
  }

  requests = _requests;
  endResetModel();
  emit changed();
}

QVariant RequestModel::data(const QModelIndex &index, int role) const {
  const auto row = index.row();
  if (row < 0 || row >= requests.count())
    return QVariant();
  const auto request = requests[row];

  if(role == Qt::DisplayRole) {
    switch(index.column()) {
      case RequestModel::ContentRole: {
        return this->generateOverviewItemDelegateRichText(request);
      }
      default:
        return QVariant();
    }
  }
  else if(role == Qt::DecorationRole) {
    switch (index.column()) {
      case RequestModel::MsgStatusIcon: {
        const auto icon_default = "qgn_list_addressbook";
        if(m_pixmaps.contains(icon_default))
          return m_pixmaps[icon_default];
      }
      default: {
        return QVariant();
      }
    }
  } else if(role == Qt::SizeHintRole) {
    switch (index.column()) {
      case RequestModel::MsgStatusIcon: {
        return QSize(58, 54);
      }
      default: {
        return QVariant();
      }
    }
  }

  return {};
}

QHash<int, QByteArray> RequestModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[ContentRole] = "content";
  roles[ButtonRole] = "button_role";
  roles[MsgStatusIcon] = "msg_status_icon";
  return roles;
}

QString RequestModel::generateOverviewItemDelegateRichText(QSharedPointer<ContactItem> roster_item) const {
  // Stylesheet: overview/overviewRichDelegate.css
  auto richtext = QString("<span class=\"header\">%1</b>").arg(roster_item->remote_uid());
  richtext += "<br>";
  richtext += QString("<span class=\"text-muted\">%1</span>").arg("incoming friend request");
  return richtext;
}

void RequestModel::preloadPixmaps() {
  // const auto basepath = QDir::currentPath() + "/src/assets/icons/";
  const auto basepath = "/usr/share/icons/hicolor/48x48/hildon/";
  const QStringList icons = {
    "qgn_list_addressbook"
  };

  for(const auto &icon: icons) {
    const auto fn = QString("%1/%2.png").arg(basepath, icon);
    if(!Utils::fileExists(fn))
      continue;
    m_pixmaps[icon] = QPixmap(fn);
  }
}

void RequestModel::onClear() {
  beginResetModel();
  this->requests.clear();
  endResetModel();

  emit changed();
}
