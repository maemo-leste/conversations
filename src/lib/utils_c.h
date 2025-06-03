#pragma once
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

#ifdef __cplusplus
extern "C" {
#endif

int file_exists(const char *path);
int ipc_try_socket(const char *path, const char *message);
int ipc_try_wakeup(const char* ipc_message);
int active_proc_count_by_path(const char* abs_path);
int active_proc_count_self();

#ifdef __cplusplus
}
#endif