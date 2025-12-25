#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Missing Doom Globals
int drone = 0;
int net_client_connected = 0;

volatile uintptr_t doom_virt_addr = 0;

// Import the WAD data from w_file_zephyr.c / doom_wad.c
extern unsigned char src_doomgeneric_doom1_wad[];
extern unsigned int src_doomgeneric_doom1_wad_len;

// Simulated file offset for the single "open" file (doom1.wad)
static off_t dummy_wad_offset = 0;

void *I_ZoneBase(int *size) {
  *size = 6 * 1024 * 1024;
  return (void *)doom_virt_addr;
}

// POSIX Stubs for Zephyr/Picolibc
// These are needed because Doom uses fopen/etc which Picolibc maps to these
// syscalls.

#define DOOM_FD 666

int open(const char *pathname, int flags, ...) {
  if (strstr(pathname, "doom1.wad")) {
    dummy_wad_offset = 0;
    return DOOM_FD;
  }
  errno = EACCES;
  return -1;
}

int close(int fd) {
  if (fd == DOOM_FD) {
    return 0;
  }
  errno = EBADF;
  return -1;
}

ssize_t read(int fd, void *buf, size_t count) {
  if (fd == DOOM_FD) {
    if (dummy_wad_offset >= src_doomgeneric_doom1_wad_len) {
      return 0;
    }
    size_t remaining = src_doomgeneric_doom1_wad_len - dummy_wad_offset;
    size_t to_read = (count < remaining) ? count : remaining;
    memcpy(buf, src_doomgeneric_doom1_wad + dummy_wad_offset, to_read);
    dummy_wad_offset += to_read;
    return to_read;
  }
  errno = EBADF;
  return -1;
}

ssize_t write(int fd, const void *buf, size_t count) {
  // Might want to redirect stdout/stderr to log?
  // But for now stub it.
  errno = EBADF;
  return -1;
}

off_t lseek(int fd, off_t offset, int whence) {
  if (fd == DOOM_FD) {
    off_t new_offset = dummy_wad_offset;
    if (whence == SEEK_SET) {
      new_offset = offset;
    } else if (whence == SEEK_CUR) {
      new_offset += offset;
    } else if (whence == SEEK_END) {
      new_offset = src_doomgeneric_doom1_wad_len + offset;
    }
    if (new_offset < 0)
      new_offset = 0;
    if (new_offset > src_doomgeneric_doom1_wad_len)
      new_offset = src_doomgeneric_doom1_wad_len;
    dummy_wad_offset = new_offset;
    return dummy_wad_offset;
  }
  errno = EBADF;
  return -1;
}

int unlink(const char *pathname) {
  errno = EACCES;
  return -1;
}

int rename(const char *oldpath, const char *newpath) {
  errno = EACCES;
  return -1;
}

int mkdir(const char *pathname, mode_t mode) {
  errno = EACCES;
  return -1;
}

#include <string.h>

int fstat(int fd, struct stat *statbuf) {
  statbuf->st_mode = S_IFREG | 0777;
  return 0;
}

int stat(const char *path, struct stat *statbuf) {
  if (strstr(path, "doom1.wad")) {
    statbuf->st_size =
        4196020; // Exact size of shareware wad usually, or just > 0
    statbuf->st_mode = S_IFREG | 0777;
    return 0;
  }
  errno = EACCES;
  return -1;
}

int access(const char *pathname, int mode) {
  if (strstr(pathname, "doom1.wad")) {
    return 0;
  }
  errno = ENOENT;
  return -1;
}

int isatty(int fd) { return 0; }
