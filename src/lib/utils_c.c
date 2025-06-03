#include "utils_c.h"

int file_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

int ipc_try_socket(const char *path, const char *message) {
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

int ipc_try_wakeup(const char* ipc_message) {
  const char *user = getenv("USER");
  if (!user) {
    fprintf(stderr, "could not determine user.\n");
    return 1;
  }

  char socket_path[256];
  snprintf(socket_path, sizeof(socket_path), "/tmp/conversations-%s.sock", user);

  if (file_exists(socket_path)) {
    if (ipc_try_socket(socket_path, ipc_message) == 0)
      return 0;
  }

  return 1;
}

int active_proc_count_by_path(const char* abs_path) {
#ifdef DEBUG
  printf("checking running instances count of %s\n", abs_path);
#endif

  DIR *proc = opendir("/proc");
  if (!proc) {
    perror("opendir /proc");
    return -1;
  }

  char real_target[PATH_MAX];
  if (realpath(abs_path, real_target) == NULL) {
    perror("failed to resolve real path of abs_path");
    closedir(proc);
    return -1;
  }

  struct dirent *entry;
  char exe_path[PATH_MAX];
  char resolved_path[PATH_MAX];
  int count = 0;

  while ((entry = readdir(proc)) != NULL) {
    if (!isdigit(entry->d_name[0]))
      continue;

    snprintf(exe_path, sizeof(exe_path), "/proc/%s/exe", entry->d_name);
    ssize_t len = readlink(exe_path, resolved_path, sizeof(resolved_path) - 1);
    if (len == -1)
      continue;

    resolved_path[len] = '\0';

    if (strcmp(resolved_path, abs_path) == 0) {
      count++;
    }
  }

  closedir(proc);
  return count;
}

int active_proc_count_self() {
  char abs_path[PATH_MAX];
  const ssize_t len = readlink("/proc/self/exe", abs_path, sizeof(abs_path) - 1);
  if (len == -1) {
    perror("Failed to read /proc/self/exe");
    return 1;
  }
  abs_path[len] = '\0';

  return active_proc_count_by_path(abs_path);
}