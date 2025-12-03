#pragma once

#include <QObject>
#include <QSettings>
#include <QPointer>
#ifdef QUICK
#include <QQmlEngine>
#endif
#include <QDir>

namespace ConfigKeys {
    Q_NAMESPACE
    enum ConfigKey {
        MaemoTest,
        ChatTheme,
        TextScaling,
        EnableNotifications,
        EnableAutoCloseChatWindows,
        EnterKeySendsChat,
        EnableInheritSystemTheme,
        EnableDisplayGroupchatJoinLeave,
        EnableDisplayAvatars,
        EnableNewVersionMessage,
        EnableDisplayChatGradient,
        GroupChatChannels,
        EnableGPUAccel,
        EnableSlim,
        EnableLogWrite,
        EnableLogSyslog,
        LinkPreviewEnabled,
        LinkPreviewRequiresUserInteraction,
        LinkPreviewImageEnabled,
        LinkPreviewMaxDownloadSize,
        EnableMatrixRainBackground
    };
    Q_ENUM_NS(ConfigKey)
}

class Config : public QObject {
Q_OBJECT

public:
    Q_DISABLE_COPY(Config)

    ~Config() override;
    Q_INVOKABLE QVariant get(unsigned int configKey);
    QVariant get(ConfigKeys::ConfigKey key);
    QString getFileName();
    void set(ConfigKeys::ConfigKey key, const QVariant& value);
    void sync();
    void resetToDefaults();

    static Config* instance();

signals:
    void changed(ConfigKeys::ConfigKey key);

private:
    Config(const QString& fileName, QObject* parent = nullptr);
    explicit Config(QObject* parent);
    void init(const QString& configFileName);

    static QPointer<Config> m_instance;

    QScopedPointer<QSettings> m_settings;
    QHash<QString, QVariant> m_defaults;
};

inline Config* config() {
    return Config::instance();
}
