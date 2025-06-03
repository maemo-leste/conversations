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

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#include <signal.h>
#endif

// small C binary to launch Conversations
// - if its already running, bring it up via IPC
//   - and forward passed argv arg
// - if it's not running, launch Conversations
//   - check config directory for the presence of
//     a file to determine to launch the slim or qml version

#define PATH_CONV "/usr/bin/conversations_qml"
#define PATH_CONV_SLIM "/usr/bin/conversations_slim"

int file_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

int try_socket(const char *path, const char *message) {
  struct sockaddr_un addr;

  int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("socket");
    return -1;
  }

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

  if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
    perror("connect");
    close(sockfd);
    return -1;
  }

  if (write(sockfd, message, strlen(message)) == -1) {
    perror("write");
    close(sockfd);
    return -1;
  }

  close(sockfd);
  return 0;
}

int main(int argc, char *argv[]) {
  const char *message = argc > 1 ? argv[1] : "makeActive";

  const char *user = getenv("USER");
  if (!user) {
    fprintf(stderr, "could not determine user.\n");
    return 1;
  }

  char socket_path[256];
  snprintf(socket_path, sizeof(socket_path), "/tmp/conversations-%s.sock", user);

  if (file_exists(socket_path)) {
    if (try_socket(socket_path, message) == 0)
      return 0;
  }

  const char *home = getenv("HOME");
  if (!home) {
    fprintf(stderr, "could not determine home directory.\n");
    return 1;
  }

  char slim_config_path[512];
  snprintf(slim_config_path, sizeof(slim_config_path), "%s/.config/conversations/slim", home);

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
    if (file_exists(slim_config_path)) {
      execv(PATH_CONV_SLIM, argv);
    } else {
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