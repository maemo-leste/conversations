#pragma once
#include <QObject>
#include <QMetaType>

#include "utils.h"

class HildonThemeColors : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString defaultTextColor MEMBER defaultTextColor NOTIFY defaultTextColorChanged);
  Q_PROPERTY(QString secondaryTextColor MEMBER secondaryTextColor NOTIFY secondaryTextColorChanged);
  Q_PROPERTY(QString activeTextColor MEMBER activeTextColor NOTIFY activeTextColorChanged);
  Q_PROPERTY(QString disabledTextColor MEMBER disabledTextColor NOTIFY disabledTextColorChanged);
  Q_PROPERTY(QString paintedTextColor MEMBER paintedTextColor NOTIFY paintedTextColorChanged);
  Q_PROPERTY(QString reversedTextColor MEMBER reversedTextColor NOTIFY reversedTextColorChanged);
  Q_PROPERTY(QString reversedSecondaryTextColor MEMBER reversedSecondaryTextColor NOTIFY reversedSecondaryTextColorChanged);
  Q_PROPERTY(QString reversedActiveTextColor MEMBER reversedActiveTextColor NOTIFY reversedActiveTextColorChanged);
  Q_PROPERTY(QString reversedDisabledTextColor MEMBER reversedDisabledTextColor NOTIFY reversedDisabledTextColorChanged);
  Q_PROPERTY(QString reversedPaintedTextColor MEMBER reversedPaintedTextColor NOTIFY reversedPaintedTextColorChanged);
  Q_PROPERTY(QString defaultBackgroundColor MEMBER defaultBackgroundColor NOTIFY defaultBackgroundColorChanged);
  Q_PROPERTY(QString darkerBackgroundColor MEMBER darkerBackgroundColor NOTIFY darkerBackgroundColorChanged);
  Q_PROPERTY(QString reversedBackgroundColor MEMBER reversedBackgroundColor NOTIFY reversedBackgroundColorChanged);
  Q_PROPERTY(QString selectionColor MEMBER selectionColor NOTIFY selectionColorChanged);
  Q_PROPERTY(QString reversedSelectionColor MEMBER reversedSelectionColor NOTIFY reversedSelectionColorChanged);
  Q_PROPERTY(QString contentBackgroundColor MEMBER contentBackgroundColor NOTIFY contentBackgroundColorChanged);
  Q_PROPERTY(QString contentFrameColor MEMBER contentFrameColor NOTIFY contentFrameColorChanged);
  Q_PROPERTY(QString contentSelectionColor MEMBER contentSelectionColor NOTIFY contentSelectionColorChanged);
  Q_PROPERTY(QString titleTextColor MEMBER titleTextColor NOTIFY titleTextColorChanged);
  Q_PROPERTY(QString buttonTextColor MEMBER buttonTextColor NOTIFY buttonTextColorChanged);
  Q_PROPERTY(QString buttonTextPressedColor MEMBER buttonTextPressedColor NOTIFY buttonTextPressedColorChanged);
  Q_PROPERTY(QString buttonTextDisabledColor MEMBER buttonTextDisabledColor NOTIFY buttonTextDisabledColorChanged);
  Q_PROPERTY(QString accentColor1 MEMBER accentColor1 NOTIFY accentColor1Changed);
  Q_PROPERTY(QString accentColor2 MEMBER accentColor2 NOTIFY accentColor2Changed);
  Q_PROPERTY(QString accentColor3 MEMBER accentColor3 NOTIFY accentColor3Changed);
  Q_PROPERTY(QString attentionColor MEMBER attentionColor NOTIFY attentionColorChanged);
  Q_PROPERTY(QString notificationBackgroundColor MEMBER notificationBackgroundColor NOTIFY notificationBackgroundColorChanged);
  Q_PROPERTY(QString notificationTextColor MEMBER notificationTextColor NOTIFY notificationTextColorChanged);
  Q_PROPERTY(QString notificationSecondaryTextColor MEMBER notificationSecondaryTextColor NOTIFY notificationSecondaryTextColorChanged)

public:
  explicit HildonThemeColors(QObject *parent = nullptr);
  void load(const QString &theme_name);
  ~HildonThemeColors() override;

  QString defaultTextColor = "red";
  QString secondaryTextColor;
  QString activeTextColor;
  QString disabledTextColor;
  QString paintedTextColor;

  QString reversedTextColor;
  QString reversedSecondaryTextColor;
  QString reversedActiveTextColor;
  QString reversedDisabledTextColor;
  QString reversedPaintedTextColor;
  QString defaultBackgroundColor;
  QString darkerBackgroundColor;
  QString reversedBackgroundColor;
  QString selectionColor;
  QString reversedSelectionColor;

  QString contentBackgroundColor;
  QString contentFrameColor;
  QString contentSelectionColor;

  QString titleTextColor;
  QString buttonTextColor;
  QString buttonTextPressedColor;
  QString buttonTextDisabledColor;

  QString accentColor1;
  QString accentColor2;
  QString accentColor3;
  QString attentionColor;

  QString notificationBackgroundColor;
  QString notificationTextColor;
  QString notificationSecondaryTextColor;

signals:
  void defaultTextColorChanged();
  void secondaryTextColorChanged();
  void activeTextColorChanged();
  void disabledTextColorChanged();
  void paintedTextColorChanged();
  void reversedTextColorChanged();
  void reversedSecondaryTextColorChanged();
  void reversedActiveTextColorChanged();
  void reversedDisabledTextColorChanged();
  void reversedPaintedTextColorChanged();
  void defaultBackgroundColorChanged();
  void darkerBackgroundColorChanged();
  void reversedBackgroundColorChanged();
  void selectionColorChanged();
  void reversedSelectionColorChanged();
  void contentBackgroundColorChanged();
  void contentFrameColorChanged();
  void contentSelectionColorChanged();
  void titleTextColorChanged();
  void buttonTextColorChanged();
  void buttonTextPressedColorChanged();
  void buttonTextDisabledColorChanged();
  void accentColor1Changed();
  void accentColor2Changed();
  void accentColor3Changed();
  void attentionColorChanged();
  void notificationBackgroundColorChanged();
  void notificationTextColorChanged();
  void notificationSecondaryTextColorChanged();
};


class HildonTheme : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString title MEMBER title NOTIFY titleChanged)
  Q_PROPERTY(QString name MEMBER name NOTIFY nameChanged)
  Q_PROPERTY(QString icon MEMBER icon NOTIFY iconChanged)
  Q_PROPERTY(QString icon_theme MEMBER icon_theme NOTIFY icon_themeChanged)
  Q_PROPERTY(HildonThemeColors* colors MEMBER colors)

public:
  explicit HildonTheme(QObject *parent = nullptr);
  ~HildonTheme() override;

  QString title;  // Matrix Theme
  QString name;   // hildon-theme-matrix
  QString icon;   // /path/to/foo.png
  QString icon_theme;  // default
  HildonThemeColors* colors = nullptr;

  void load();
  Q_INVOKABLE void printText();

signals:
  void titleChanged();
  void nameChanged();
  void iconChanged();
  void icon_themeChanged();
};
