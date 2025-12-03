#ifndef CONV_GLOBALS_H
#define CONV_GLOBALS_H

#include <QtGlobal>
#include <QRegExp>

#define PATH_CONV INSTALL_PREFIX_QUOTED "/bin/conversations_qml"
#define PATH_CONV_SLIM INSTALL_PREFIX_QUOTED "/bin/conversations_slim"

namespace globals
{
  const static QRegExp reTelMimeHandler = QRegExp(R"(^(?:tel|callto)\:([\d\-+]+))");
  const static QRegExp reConversationsHandler = QRegExp(R"(^conversations\:([\d\-+]+))");
  inline qint64 conversationsSlimExecutableSize = 0;
  inline qint64 conversationsQuickExecutableSize = 0;
}

#endif
