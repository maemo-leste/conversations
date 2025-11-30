#include <QObject>
#include <QDebug>

#include "lib/abook/abook_public.h"
#include "lib/abook/abook_roster.h"
#include "lib/logger_std/logger_std.h"
#include "contacts/TpContactsModel.h"
#include "conversations.h"

TpContactsProxyModel::TpContactsProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {

}

void TpContactsProxyModel::setNameFilter(const QString& name) {
  if(name.toLower() == m_nameFilter) return;
  qDebug() << "name filter" << name;

  this->m_nameFilter = name.toLower();
  if (this->sourceModel() != nullptr) {
    auto mdl = dynamic_cast<TpContactsModel*>(this->sourceModel());
    this->invalidateFilter();
  }
}

bool TpContactsProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
  if (this->sourceModel() == nullptr)
    return false;

  const auto mdl = dynamic_cast<TpContactsModel*>(this->sourceModel());
  const auto index = this->sourceModel()->index(source_row, 0, source_parent);

  if (!index.isValid())
    return false;

  // filtering
  if (m_nameFilter.isEmpty())
    return true;

  Tp::ContactPtr contact = mdl->tpchannel->room_contacts[index.row()];
  const auto name = contact->alias().toLower();
  return name.contains(m_nameFilter);
}

int TpContactsProxyModel::rowCount(const QModelIndex &parent) const {
  return QSortFilterProxyModel::rowCount(parent);
}

void TpContactsProxyModel::onTpContactsRowClicked(uint32_t idx) {
  QModelIndex source_idx = this->mapToSource(this->index(idx, 0));
  uint32_t row_idx = source_idx.row();
  const auto mdl = dynamic_cast<TpContactsModel*>(this->sourceModel());
  const Tp::ContactPtr contact = mdl->tpchannel->room_contacts[row_idx];
  emit mdl->contactClicked(contact);
}

TpContactsModel::TpContactsModel(const TelepathyChannelPtr &channel, QObject *parent) :
    tpchannel(channel),
    QAbstractListModel(parent) {}

int TpContactsModel::rowCount(const QModelIndex & parent) const {
  Q_UNUSED(parent);
  const auto count = tpchannel->room_contacts.count();
  return count;
}

int TpContactsModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return COUNT;
}

QVariant TpContactsModel::data(const QModelIndex &index, int role) const {
  auto ctx = Conversations::instance();
  const int row = index.row();
  if (row < 0 || row >= tpchannel->room_contacts.count())
    return {};

  const auto &contact = tpchannel->room_contacts[row];

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
      case ContentRole:
        return generateTpContactsItemDelegateRichText(contact->alias(), contact->id());
      default:
        return {};
    }
  }

  if (role == Qt::TextAlignmentRole) {
    switch (index.column()) {
      case MsgStatusIcon:
      case ChatTypeIcon:
          return Qt::AlignHCenter;
      default:
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    }
  }

  if (role == Qt::DecorationRole) {
    switch (index.column()) {
      case MsgStatusIcon: {
        return QPixmap("/usr/share/icons/hicolor/48x48/hildon/general_default_avatar.png");
      }
      default:
        return {};
    }
  }

  if (role == Qt::SizeHintRole) {
    switch (index.column()) {
      case MsgStatusIcon:
      case ChatTypeIcon:
        return QSize(58, 54);
      default:
        return {};
    }
  }

  return {};
}

QHash<int, QByteArray> TpContactsModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[MsgStatusIcon] = "msg_status_icon";
  roles[ChatTypeIcon] = "chat_type_icon";
  roles[ContentRole] = "content";
  return roles;
}

QString TpContactsModel::generateTpContactsItemDelegateRichText(QString title, QString subtitle) const {
  title = title.replace("<", "");

  QString richtext;
  // Stylesheet: overview/overviewRichDelegate.css
  richtext += QString("<span class=\"header\">%1</b>").arg(title);
  richtext += QString("<span class=\"small\">&nbsp;&nbsp;%1</span>").arg("room participant");
  richtext += QString("<span class=\"small text-muted\"></span>");
  richtext += "<br>";

  const QString textSnippet = subtitle.replace("<", "");
  richtext += QString("<span class=\"text-muted\">%1</span>").arg(textSnippet);
  return richtext;
}

void TpContactsModel::onReload() {
  beginResetModel();
  endResetModel();
}
