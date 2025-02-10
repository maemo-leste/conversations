#include <unistd.h>

#include <QObject>
#include <QFile>
#include <QTextCodec>

void clion_debug_setup() {
  // For remote debugging (QEMU) with CLion, the environment variables need
  // to be correctly set such that e.g. dbus will work. We can execute
  // this hack to dump the environ to a file, then reads it below.
  // This snippet runs when `-D CMAKE_DEBUG_TYPE=Debug`

  // /bin/sh -c 'nohup /tmp/tmp.wB7WLVouUV/cmake-build-debug/bin/example >/dev/null 2>&1 &'; sleep 2; cat "/proc/`pidof example`/environ" | tr "\0" "\n" > /home/user/env.sh; kill -9 "`pidof example`"
  setuid(1000);
  QString path_env_file = "/home/user/env.sh";
  qDebug() << "trying to read ENV from" << path_env_file << ", if it exists";

  if(QFile::exists(path_env_file)) {
    QFile file(path_env_file);
    if(!file.open(QFile::ReadOnly | QFile::Text))
      throw "File could not be opened";

    const auto env_file = file.readAll();
    const auto env_file_str = QString(QTextCodec::codecForMib(106)->toUnicode(env_file));

    for(auto &line: env_file_str.split("\n")) {
      line = line.replace("export ", "");
      const int pos = line.indexOf("=");
      auto key = line.left(pos);
      auto val = line.remove(0, pos + 1);
      if(key == "DISPLAY")
        val = ":0";
      if(key.startsWith("QT"))  // we set Qt related variables manually later
        continue;

      if(val.startsWith("\""))
        val = val.mid(1);
      if(val.endsWith("\""))
        val = val.mid(0, val.length() - 1);

      if(val.isEmpty() || key.isEmpty()) continue;
      qputenv(key.toStdString().c_str(), val.toStdString().c_str());
    }
  }
}