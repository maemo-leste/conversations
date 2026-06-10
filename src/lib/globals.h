#ifndef CONV_GLOBALS_H
#define CONV_GLOBALS_H

#include <QtGlobal>
#include <QRegularExpression>

#define PATH_CONV INSTALL_PREFIX_QUOTED "/bin/conversations_qml"
#define PATH_CONV_SLIM INSTALL_PREFIX_QUOTED "/bin/conversations_slim"

namespace globals
{
  const static QRegularExpression reTelMimeHandler = QRegularExpression(R"(^(?:tel|callto)\:([\d\-+]+))");
  const static QRegularExpression reConversationsHandler = QRegularExpression(R"(^conversations\:([\d\-+]+))");
  inline QString appDataDirectory = {};
  inline QString configDirectory = {};
  inline QString appDataDownloadDirectory = {};
  inline qint64 conversationsSlimExecutableSize = 0;
  inline qint64 conversationsQuickExecutableSize = 0;
}

#endif
