#ifndef CONV_SETTINGS_H
#define CONV_SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QPointer>
#include <QQmlEngine>
#include <QDir>

namespace ConfigKeys
{
    Q_NAMESPACE
    enum ConfigKey {
        MaemoTest,
        ChatTheme,
        TextScaling,
        EnableNotifications,
        EnterKeySendsChat
    };
    Q_ENUM_NS(ConfigKey)
}

class Config : public QObject
{
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

inline Config* config()
{
    return Config::instance();
}

#endif
