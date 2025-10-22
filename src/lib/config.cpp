// SPDX-License-Identifier: BSD-3-Clause
// Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
// Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
// Copyright (c) 2020-2021, The Monero Project.

#include "config.h"
#include "lib/utils.h"
#include "utils.h"

#define QS QStringLiteral

struct ConfigDirective
{
    QString name;
    QVariant defaultValue;
};

static QHash<ConfigKeys::ConfigKey, ConfigDirective> configStrings = {
  {ConfigKeys::MaemoTest,{QS("MaemoTest"), ""}},
  {ConfigKeys::ChatTheme,{QS("ChatTheme"), "whatsthat"}},
  {ConfigKeys::TextScaling,{QS("TextScaling"), 1.0}},
  {ConfigKeys::EnableNotifications,{QS("EnableNotifications"), true}},
  {ConfigKeys::EnableAutoCloseChatWindows,{QS("EnableAutoCloseChatWindows"), true}},
  {ConfigKeys::EnableInheritSystemTheme,{QS("EnableInheritSystemTheme"), false}},
  {ConfigKeys::EnableDisplayGroupchatJoinLeave,{QS("EnableDisplayGroupchatJoinLeave"), true}},
  {ConfigKeys::EnableDisplayAvatars,{QS("EnableDisplayAvatars"), true}},
  {ConfigKeys::EnableDisplayChatGradient,{QS("EnableDisplayChatGradient"), false}},
  {ConfigKeys::EnterKeySendsChat,{QS("EnterKeySendsChat"), true}},
  {ConfigKeys::GroupChatChannels, {QS("GroupChatChannels"), "{}"}},
  {ConfigKeys::EnableSlim, {QS("EnableSlim"), false}},
  {ConfigKeys::EnableLogWrite, {QS("EnableLogWrite"), false}},
  {ConfigKeys::EnableLogSyslog, {QS("EnableLogSyslog"), false}},
  {ConfigKeys::LinkPreviewEnabled, {QS("LinkPreviewEnabled"), true}},
  {ConfigKeys::LinkPreviewRequiresUserInteraction, {QS("LinkPreviewRequiresUserInteraction"), true}},
  {ConfigKeys::LinkPreviewImageEnabled, {QS("LinkPreviewImageEnabled"), true}},
  {ConfigKeys::LinkPreviewMaxDownloadSize, {QS("LinkPreviewMaxDownloadSize"), 2}}
};

QPointer<Config> Config::m_instance(nullptr);

Q_INVOKABLE QVariant Config::get(unsigned int key) {
  return this->get(ConfigKeys::ConfigKey(key));
}

QVariant Config::get(ConfigKeys::ConfigKey key)
{
    auto cfg = configStrings[key];
    auto defaultValue = configStrings[key].defaultValue;

    return m_settings->value(cfg.name, defaultValue);
}

QString Config::getFileName()
{
    return m_settings->fileName();
}

void Config::set(ConfigKeys::ConfigKey key, const QVariant& value)
{
    if (get(key) == value) {
        return;
    }

    auto cfg = configStrings[key];
    m_settings->setValue(cfg.name, value);

    this->sync();
    emit changed(key);
}

/**
 * Sync configuration with persistent storage.
 *
 * Usually, you don't need to call this method manually, but if you are writing
 * configurations after an emitted \link QCoreApplication::aboutToQuit() signal,
 * use it to guarantee your config values are persisted.
 */
void Config::sync()
{
    m_settings->sync();
}

void Config::resetToDefaults()
{
    m_settings->clear();
}

Config::Config(const QString& fileName, QObject* parent)
        : QObject(parent) {
    init(fileName);
}

Config::Config(QObject* parent)
        : QObject(parent) {
    QString configPath;

    Utils::init_device_type();

    auto gpu = true;
    if(DEVICE_TYPE == DeviceType::PINEPHONE)
      gpu = false;  // @TODO: software rendering is faster on the Pinephone - enable GPU later
    configStrings[ConfigKeys::EnableGPUAccel] = {QS("EnableGPUAccel"), gpu};

    configPath = QDir::homePath();
    configPath += QString("/.config/%1/settings.json").arg(QCoreApplication::applicationName());

    init(QDir::toNativeSeparators(configPath));
}


Config::~Config()
{
}

void Config::init(const QString& configFileName) {
    const QSettings::Format jsonFormat = QSettings::registerFormat("json", Utils::readJsonFile, Utils::writeJsonFile);
    QSettings::setDefaultFormat(jsonFormat);
    m_settings.reset(new QSettings(configFileName, jsonFormat));
}

Config* Config::instance() {
    if (!m_instance) {
        m_instance = new Config(qApp);
    }

    return m_instance;
}
