#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  /* indicates the offset of the byte that is going to be read or written */
  size_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_SERIAL, FD_EVENTS, FD_FB, FD_DISPINFO};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write, 0},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write, 0},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write, 0},
  [FD_SERIAL] = {"/dev/serial", 0, 0, invalid_read, serial_write, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0, events_read, invalid_write, 0},
  /* file size of frame buffer should be initialized */
  [FD_FB] = {"/dev/fb", 0, 0, invalid_read, fb_write, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write, 0},
#include "files.h"
};

#define NR_FILE sizeof(file_table) / sizeof(Finfo)

void init_fs() {
  // TODO: initialize the size of /dev/fb
  /* initialize the size of /dev/fb */


}

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

int fs_open(const char *pathname, int flags, int mode) {
  /* for simplicity, we do not use flags and mode */
  int i;
  for (i = 0; i < NR_FILE; ++ i) {
    if (strcmp(file_table[i].name, pathname) == 0)  break;
  }
  if (i == NR_FILE)  panic("Can't find file %s\n", pathname);

  return i;
}

size_t fs_read(int fd, void *buf, size_t len) {
  if (len == 0)  return 0;
  /* invalid file descriptor */
  if (fd < 0)  {
    Log("Attemp to read a negative fd");
    return -1;
  }

  if (file_table[fd].read != NULL) {
    switch (fd) {
      /* for events device */
      case FD_EVENTS: {
        /* no offset specified because events is not block device */
        size_t ret = file_table[FD_EVENTS].read(buf, 0, len);
        return ret;
      }
      case FD_DISPINFO: {
        size_t ret = file_table[FD_DISPINFO].read(buf, 0, len);
        return ret;
      }
    }

    /* call function to trigger panic */
    file_table[fd].read(buf, 0, len);
  }

  /* for regular file */
  size_t open_offset = file_table[fd].open_offset;
  size_t disk_offset = file_table[fd].disk_offset;
  size_t file_size = file_table[fd].size;
  /* if exceed file size, read what is remained */
  if (open_offset + len >= file_size) {
    Log("read for fd %d reaches the end of file %s", fd, file_table[fd].name);
    len = file_size - open_offset;
  }
  size_t ret = ramdisk_read(buf, disk_offset + open_offset, len);
  file_table[fd].open_offset += ret;

  return ret;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (len == 0)  return 0;
  size_t written = 0;

  if (file_table[fd].write != NULL) {
    switch (fd) {
      /* for stdout and stderr */
      case FD_STDOUT: case FD_STDERR: case FD_SERIAL: {
        written = file_table[fd].write(buf, 0, len);
        return written;
      }
    }

    /* call function to trigger panic */
    file_table[fd].write(buf, 0, len);
  }
  
  /* for regular file */
  size_t open_offset = file_table[fd].open_offset;
  size_t disk_offset = file_table[fd].disk_offset;
  size_t file_size = file_table[fd].size;
  /* if exceed file size, write to the remained space */
  if (open_offset + len >= file_size) {
    Log("write for fd %d reaches the end of file %s", fd, file_table[fd].name);
    len = file_size - open_offset;
  }
  size_t ret = ramdisk_write(buf, disk_offset + open_offset, len);
  file_table[fd].open_offset += ret;

  written = ret;
  return written;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  if (fd < 0) {
    Log("Attemp to lseek a nagative file descriptor");
    return -1;
  }
  /* in sfs, all operations except write to stdout and stderr are not supported */
  if (fd >= 0 && fd <= 2) {
    Log("Not support lseek on stdin, stdout or stderr");
    return -1;
  }

  /* for regular file */
  switch (whence) {
    case SEEK_CUR: {
      if (file_table[fd].open_offset + offset > file_table[fd].size) {
        Log("lseek for fd %d exceed the size of %s", fd, file_table[fd].name);
        return -1;
      }
      file_table[fd].open_offset += offset;
      break;
    }
    case SEEK_END: {
      if (file_table[fd].size + offset > file_table[fd].size) {
        Log("lseek for fd %d exceed the size of %s", fd, file_table[fd].name);
        return -1;
      }
      file_table[fd].open_offset = file_table[fd].size + offset;
      break;
    }
    case SEEK_SET: {
      if (offset > file_table[fd].size) {
        Log("lseek for fd %d exceed the size of %s", fd, file_table[fd].name);
        return -1;
      }
      file_table[fd].open_offset = offset;
      break;
    }
    default: {
      Log("Not supported whence in lseek: %d", whence);
      return -1;
    }
  }

  return file_table[fd].open_offset;
}

int fs_close(int fd) {
  if (fd < 0)  {
    Log("Attempt to close a negative file descriptor");
    return -1;
  }
  /* reset the open offset */
  file_table[fd].open_offset = 0;
  /* in sfs, close always returns 0 */
  return 0;
}
