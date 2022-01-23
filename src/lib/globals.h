#ifndef CONV_GLOBALS_H
#define CONV_GLOBALS_H

#include <QtGlobal>
#include <QRegExp>

namespace globals
{
  const static QRegExp reTelMimeHandler = QRegExp(R"(^(?:tel|callto)\:([\d\-+]+))");
  const static QRegExp reConversationsHandler = QRegExp(R"(^conversations\:([\d\-+]+))");
  const static QRegExp reRemoteUID = QRegExp(R"(^([\d\-+]+))");
}

#endif
