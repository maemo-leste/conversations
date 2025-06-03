// small C binary to launch conversations
// - if its already running, bring it up via IPC
//   - and forward passed argv[1] arg
// - if it's not running, launch conversations
//   - check config directory for the presence of
//     a file to determine to launch the slim or qml version
//   - refuses to execv when /usr/bin/conversations (this launcher)
//     is already running (checks /proc) to prevent race condition
//     conversations itself (slim/qml) has a similar check
//   - when this launcher dies, child also dies to prevent orphans

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#include <signal.h>
#endif

#include "lib/utils_c.h"

#define PATH_CONV INSTALL_PREFIX_QUOTED "/bin/conversations_qml"
#define PATH_CONV_SLIM INSTALL_PREFIX_QUOTED "/bin/conversations_slim"

int main(int argc, char *argv[]) {
  const char *ipc_message = argc > 1 ? argv[1] : "makeActive";
  if (ipc_try_wakeup(ipc_message) == 0)
    return 0;

  const char *home = getenv("HOME");
  if (!home) {
    fprintf(stderr, "could not determine home directory.\n");
    return 1;
  }

  // the presence of this file determines what version of conversations we'll launch
  char slim_config_path[512];
  snprintf(slim_config_path, sizeof(slim_config_path), "%s/.config/conversations/slim", home);

  // enforce not running twice
  const int instances_count = active_proc_count_self();
  if (instances_count <= 0) {
    fprintf(stderr, "error detecting running instance(s).\n");
    return 1;
  } else if (instances_count != 1) {
    fprintf(stderr, "we are already running.\n");
    return 1;
  }

  // ensure paths exist
  if (access(PATH_CONV_SLIM, X_OK) != 0) {
    fprintf(stderr, "error: %s is not accessible, or not executable.\n", PATH_CONV_SLIM);
    exit(EXIT_FAILURE);
  }

  if (access(PATH_CONV, X_OK) != 0) {
    fprintf(stderr, "error: %s is not accessible, or not executable.\n", PATH_CONV);
    exit(EXIT_FAILURE);
  }

  const pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  }

  if (pid == 0) {
#ifdef HAVE_SYS_PRCTL_H
    // kill child when parent dies  ▄︻̷̿┻̿═━一
    if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1) {
      perror("prctl");
      _exit(1);
    }
#endif
    // pass correct argv[0] to this fork
    if (file_exists(slim_config_path)) {
      argv[0] = PATH_CONV_SLIM;
      execv(PATH_CONV_SLIM, argv);
    } else {
      argv[0] = PATH_CONV;
      execv(PATH_CONV, argv);
    }
    perror("execl");
    _exit(1);
  }

  int status;
  if (waitpid(pid, &status, 0) < 0) {
    perror("waitpid");
    return 1;
  }

  return 0;
}