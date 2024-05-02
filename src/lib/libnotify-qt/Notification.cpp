/* libnotify-qt - library for sending notifications implemented in Qt
 * Copyright (C) 2010-2011 Vojtech Drbohlav <vojta.d@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Notification.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDebug>

#include "OrgFreedesktopNotificationsInterface.h"

bool Notification::s_isInitted = false;
QString Notification::s_appName;
org::freedesktop::Notifications * Notification::s_notifications = 0;

bool Notification::init(const QString & appName)
{
	if(!QDBusConnection::sessionBus().isConnected())
		return false;

	s_notifications = new org::freedesktop::Notifications("org.freedesktop.Notifications",
														  "/org/freedesktop/Notifications",
														  QDBusConnection::sessionBus());

	s_appName = appName;
	s_isInitted = s_notifications->isValid();
	return s_isInitted;
}

void Notification::uninit()
{
	s_isInitted = false;

	delete s_notifications;
}

bool Notification::isInitted()
{
	return s_isInitted;
}

Notification* Notification::issue(QString title, QString message, const QSharedPointer<ChatMessage> &msg) {
  bool is_sms = msg->event_type() == "RTCOM_EL_EVENTTYPE_SMS_MESSAGE"; // Make this prettier
  Notification* notification;

  if (is_sms) {
    notification = new Notification(msg, title, message, "general_sms", 0);
    notification->setCategory("sms-message");
    notification->setHintString("led-pattern", "PatternCommunicationSMS");
  } else {
    notification = new Notification(msg, title, message, "general_chat", 0);
    notification->setCategory("chat-message");
    notification->setHintString("led-pattern", "PatternCommunicationIM");
  }

  // Currently this will group all notifications together, which is weird
  notification->setHintString("group", "_grouped_messages");
  notification->setHintString("conversations-groupuid", msg->group_uid());

  notification->setHintByte("persistent", 1);

  notification->show();
  return notification;
}

const QString & Notification::getAppName()
{
	return s_appName;
}

QStringList Notification::getServerCaps()
{
	QDBusPendingReply<QStringList> reply = s_notifications->getCapabilities();
	reply.waitForFinished();

	if(reply.isValid())
	{
		return reply.argumentAt(0).toStringList();
	}

	return QStringList();
}

bool Notification::getServerInfo(QString & name, QString & vendor, QString & version)
{
	QDBusPendingReply<QString, QString, QString> reply = s_notifications->getServerInformation();
	reply.waitForFinished();

	if(reply.isValid())
	{
		name = reply.argumentAt(SERVER_INFO_NAME).toString();
		vendor = reply.argumentAt(SERVER_INFO_VENDOR).toString();
		version = reply.argumentAt(SERVER_INFO_VERSION).toString();
		return true;
	}

	return false;
}

Notification::Notification(const QSharedPointer<ChatMessage> &msg, const QString &summary, const QString &body, const QString &iconName, int timeout, QObject * parent) :
	QObject(parent),
	m_id(0),
	m_summary(summary),
	m_body(body),
  m_context(msg),
	m_iconName(iconName),
	m_timeout(timeout),
	m_autoDelete(true)
{
	setUrgency(NOTIFICATION_URGENCY_NORMAL);

	// TODO: i should do this somehow in org::freedesktop::Notifications imho
//	QDBusConnection::sessionBus().connect(QString(), QString(), org::freedesktop::Notifications::staticInterfaceName(),
//										  "NotificationClosed", this, SLOT(onNotificationClosed(quint32, quint32)));


  QDBusConnection::sessionBus().connect(QString(), QString(), org::freedesktop::Notifications::staticInterfaceName(),
										  "NotificationClosed", this, SLOT(onNotificationClosed(quint32)));

	QDBusConnection::sessionBus().connect(QString(), QString(), org::freedesktop::Notifications::staticInterfaceName(),
										  "ActionInvoked", this, SLOT(onActionInvoked(quint32,QString)));
}

Notification::~Notification()
{
	disconnect(s_notifications);
}

bool Notification::show()
{
	QDBusPendingReply<quint32> reply = s_notifications->notify(getAppName(), m_id, m_iconName, m_summary, m_body,
															   m_actions, m_hints, m_timeout);
	if(m_id == 0)
	{
		reply.waitForFinished();
		if(!reply.isValid())
			return false;

		m_id = reply.argumentAt(0).toInt();
	}

	return true;
}

void Notification::setSummary(const QString & summary)
{
	m_summary = summary;
}

void Notification::setBody(const QString & body)
{
	m_body = body;
}

void Notification::setIconName(const QString & iconName)
{
	m_iconName = iconName;
}

void Notification::setTimeout(qint32 timeout)
{
	m_timeout = timeout;
}

void Notification::setUrgency(NotificationUrgency urgency)
{
	setHintByte("urgency", urgency);
}

void Notification::setCategory(const QString & category)
{
	setHintString("category", category);
}

/*void Notification::setIconFromPixmap(const QPixmap & pixmap)
{
	setHintByteArray("image_data",pixmap.t);
}*/

void Notification::setLocation(qint32 x, qint32 y)
{
	setHintInt32("x", x);
	setHintInt32("y", y);
}

void Notification::setHint(const QString & key, const QVariant & value)
{
	m_hints.insert(key, value);
}

void Notification::setHintInt32(const QString & key, qint32 value)
{
	m_hints.insert(key, value);
}

void Notification::setHintDouble(const QString & key, double value)
{
	m_hints.insert(key, value);
}

void Notification::setHintString(const QString & key, const QString & value)
{
	m_hints.insert(key, value);
}

void Notification::setHintBoolean(const QString & key, bool value)
{
    m_hints.insert(key, value);
}

void Notification::setHintByte(const QString & key, uchar value)
{
    auto var = QVariant::fromValue(value);
	m_hints.insert(key, var);
}

void Notification::setHintByteArray(const QString & key, const QByteArray & value)
{
	m_hints.insert(key, value);
}

void Notification::clearHints()
{
	m_hints.clear();
}

void Notification::addAction(const QString & actionKey, const QString & label)
{
	m_actions << actionKey << label;
}

void Notification::clearActions()
{
	m_actions.clear();
}

bool Notification::close()
{
	QDBusPendingReply<> reply = s_notifications->closeNotification(m_id);
	reply.waitForFinished();
	return reply.isValid();
}

bool Notification::autoDelete() const
{
	return m_autoDelete;
}

void Notification::setAutoDelete(bool autoDelete)
{
	m_autoDelete = autoDelete;
}

void Notification::onNotificationClosed(quint32 id) {
	if(m_id == id && m_autoDelete) {
    this->deleteLater();
	}
}

void Notification::onActionInvoked(quint32 id, const QString & actionKey) {
  // actionKey == "default" on notification click
	if(m_id == id) {
		emit clicked(m_context);
	}
}
