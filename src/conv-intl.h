// osso-intl.h - OSSO intl
// Copyright (C) 2011  Pali Rohár <pali.rohar@gmail.com>

#ifndef CONV_INTL_H
#define CONV_INTL_H

#include <QByteArray>
#include <QString>

#include <libintl.h>

static inline void intl(const char * package) {

  setlocale(LC_ALL, "");
  bindtextdomain(package, "/usr/share/locale");
  bind_textdomain_codeset(package, "UTF-8");
  textdomain(package);

}

static inline const QString _(const char * str) {

  return QString::fromUtf8(gettext(str)).replace("%%", "%");

}

static inline const QString _(const QString &str) {

  return _(str.toUtf8().data());

}

#endif
