#ifndef PARSE_LFS_H
#define PARSE_LFS_H

// See `man inode` for more on bitmasks.
#include <sys/stat.h> // Provides mode_t, S_ISDIR, etc.
#include <time.h> // Provides time_t
#include "../lib/lib.h" // Provides ls_print_file(...)

// You may need more includes!
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// Can add more structs, change, extend, or delete these.
typedef struct {
  char *fileName;
  uint inumber;
} __attribute__((packed)) DirEntry;

typedef struct {
  uint num_entries;
  DirEntry *entries;
} __attribute__((packed)) Directory;

typedef struct {
  uint file_cursor;
  uint size;
  mode_t permissions;
  time_t mtime;

  uint num_direct_blocks;
  struct {
    uint direct_block_disk_offset;
  } entries[];
} __attribute__((packed)) Inode;

typedef struct {
  uint num_entries;
  struct {
    uint inumber;
    uint inode_disk_offset;
  } entries[];
} __attribute__((packed)) Imap;

typedef struct {
  uint image_offset;
  uint num_entries;
  struct {
    uint inumber_start;
    uint inumber_end;
    uint imap_disk_offset;
  } entries[];
} __attribute__((packed)) CR;

#endif