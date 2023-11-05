#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QtNetwork>
#include <QDebug>

#include "lib/colors.h"
#include "lib/utils.h"
#include "lib/globals.h"

HildonThemeColors::HildonThemeColors(QObject *parent) : QObject(parent) {}

void HildonThemeColors::load(const QString &theme_name) {
  const auto path = QString("/usr/share/themes/%1/colors.config").arg(theme_name);
  auto kvmap = Utils::readSystemConfig(path);
  if(kvmap.isEmpty()) {
    qWarning() << "could not read: " << path;
    return;
  }

  for(const auto &key: kvmap.keys()) {
    const auto &val = kvmap.value(key);

    if(key == "DefaultTextColor") {
      this->defaultTextColor = val;
      emit defaultTextColorChanged();
    } else if(key == "SecondaryTextColor") {
      this->secondaryTextColor = val;
      emit secondaryTextColorChanged();
    } else if(key == "ActiveTextColor") {
      this->activeTextColor = val;
      emit activeTextColorChanged();
    } else if(key == "DisabledTextColor") {
      this->disabledTextColor = val;
      emit disabledTextColorChanged();
    } else if(key == "PaintedTextColor") {
      this->paintedTextColor = val;
      emit paintedTextColorChanged();
    } else if(key == "ReversedTextColor") {
      this->reversedTextColor = val;
      emit reversedTextColorChanged();
    } else if(key == "ReversedSecondaryTextColor") {
      this->reversedSecondaryTextColor = val;
      emit reversedSecondaryTextColorChanged();
    } else if(key == "ReversedActiveTextColor") {
      this->reversedActiveTextColor = val;
      emit reversedActiveTextColorChanged();
    } else if(key == "ReversedDisabledTextColor") {
      this->reversedDisabledTextColor = val;
      emit reversedDisabledTextColorChanged();
    } else if(key == "ReversedPaintedTextColor") {
      this->reversedPaintedTextColor = val;
      emit reversedPaintedTextColorChanged();
    } else if(key == "DefaultBackgroundColor") {
      this->defaultBackgroundColor = val;
      emit defaultBackgroundColorChanged();
    } else if(key == "DarkerBackgroundColor") {
      this->darkerBackgroundColor = val;
      emit darkerBackgroundColorChanged();
    } else if(key == "ReversedBackgroundColor") {
      this->reversedBackgroundColor = val;
      emit reversedBackgroundColorChanged();
    } else if(key == "SelectionColor") {
      this->selectionColor = val;
      emit selectionColorChanged();
    } else if(key == "ReversedSelectionColor") {
      this->reversedSelectionColor = val;
      emit reversedSelectionColorChanged();
    } else if(key == "ContentBackgroundColor") {
      this->contentBackgroundColor = val;
      emit contentBackgroundColorChanged();
    } else if(key == "ContentFrameColor") {
      this->contentFrameColor = val;
      emit contentFrameColorChanged();
    } else if(key == "ContentSelectionColor") {
      this->contentSelectionColor = val;
      emit contentSelectionColorChanged();
    } else if(key == "TitleTextColor") {
      this->titleTextColor = val;
      emit titleTextColorChanged();
    } else if(key == "ButtonTextColor") {
      this->buttonTextColor = val;
      emit buttonTextColorChanged();
    } else if(key == "ButtonTextPressedColor") {
      this->buttonTextPressedColor = val;
      emit buttonTextPressedColorChanged();
    } else if(key == "ButtonTextDisabledColor") {
      this->buttonTextDisabledColor = val;
      emit buttonTextDisabledColorChanged();
    } else if(key == "AccentColor1") {
      this->accentColor1 = val;
      emit accentColor1Changed();
    } else if(key == "AccentColor2") {
      this->accentColor2 = val;
      emit accentColor2Changed();
    } else if(key == "AccentColor3") {
      this->accentColor3 = val;
      emit accentColor3Changed();
    } else if(key == "AttentionColor") {
      this->attentionColor = val;
      emit attentionColorChanged();
    } else if(key == "NotificationBackgroundColor") {
      this->notificationBackgroundColor = val;
      emit notificationBackgroundColorChanged();
    } else if(key == "NotificationTextColor") {
      this->notificationTextColor = val;
      emit notificationTextColorChanged();
    } else if(key == "NotificationSecondaryTextColor") {
      this->notificationSecondaryTextColor = val;
      emit notificationSecondaryTextColorChanged();
    }

  }
}

HildonThemeColors::~HildonThemeColors() {

}

HildonTheme::HildonTheme(QObject *parent) : colors(new HildonThemeColors()), QObject(parent) {
  this->load();
}

void HildonTheme::load() {
  const auto path = "/etc/hildon/theme/index.theme";
  auto kvmap = Utils::readSystemConfig(path);
  if(kvmap.isEmpty()) {
    qWarning() << "could not read: " << path;
    return;
  }

  for(const auto &key: kvmap.keys()) {
    const auto &val = kvmap.value(key);
    if(key == "Name") {
      this->title = val;
      emit titleChanged();
    } else if(key == "GtkTheme") {
      this->name = val;
      emit nameChanged();
    } else if(key == "IconTheme") {
      this->icon_theme = val;
      emit icon_themeChanged();
    }
    else if(key == "Icon") {
      this->icon = val;
      emit iconChanged();
    }
  }

  if(this->name.isEmpty()) {
    qWarning() << "could not parse 'name' from " << path;
    return;
  }

  this->colors->load(this->name);
}

void HildonTheme::printText() {
}

HildonTheme::~HildonTheme() {
}
