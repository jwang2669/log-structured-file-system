
---

# Parse_LFS

`Parse_LFS` is a tool for reading and interpreting a Log-Structured File System (LFS) disk image.
It supports:

* Printing the full LFS directory tree (`ls`)
* Extracting and printing the contents of a file inside the LFS image (`cat`)

The tool reconstructs filesystem metadata and structure by parsing Checkpoint Regions, Imaps, Inodes, and data blocks inside the image.

---

## ✨ Features

### ✔ List the LFS directory tree (`ls`)

```
$ ./bin/parse_lfs ls ./sample_files/a.img
/
         256 2025-04-15 19:00:18 UTC -rw-------    bar.txt
           0 2025-04-15 19:00:18 UTC -rw-------    foo.txt
           ...
```

* Displays a hierarchical file tree starting from the root (`/`)
* Files are sorted alphabetically; directories appear *after* files at each depth
* Indentation reflects directory depth
* Uses the provided `ls_print_file()` helper for consistent formatting

---

### ✔ Print file contents (`cat`)

```
$ ./bin/parse_lfs cat /bar.txt ./sample_files/a.img
revised to the end of modernizing
and improving them"...
```

* Follows the given absolute path and outputs the file content
* Supports `.` and `..` logically (though they do not exist in the image)
* Paths must start with `/`
* If the file cannot be found, the program prints:

```
parse_lfs error: Failed to find file.
```

and returns exit code `1`.

---

## 📚 Background

LFS (Log-Structured File System) writes metadata and data in sequential log segments to optimize write performance and reduce random I/O.
This project focuses on understanding how LFS organizes its structures and reconstructing filesystem information from a raw image.

---

## 🗂 LFS Image Format Overview

LFS images contain:

* **1 KB metadata blocks** (Checkpoint Region, Imap, Inode)
* **4 KB data blocks** (file and directory data)

### 1. Checkpoint Region (CR, 1 KB)

```
uint Image_Offset
uint Entry_Count

// repeated Entry_Count times
uint inumber_start
uint inumber_end
uint imap_disk_offset
```

Each entry maps a range of inumbers to the location of an Imap block.

---

### 2. Imap (1 KB)

```
uint Entry_Count

// repeated
uint inumber
uint inode_disk_offset
```

Each entry points to the location of one inode.

---

### 3. Inode (1 KB)

```
uint    file_cursor
uint    size
mode_t  permissions
time_t  mtime

uint    Direct_Block_Count

uint direct_block_disk_offset
...
```

* Includes only direct block pointers (no indirects)
* `size` is the actual file size—not necessarily equal to total data block space

---

### 4. Data Blocks (4 KB)

**Regular file:** raw bytes
**Directory:** sequence of entries

```
char name[]   // ends with ',', not null-terminated
uint inumber
```

For example:

```
a,5
b,6
c,7
```

Entries in directories are **not guaranteed to be sorted**.

---

## 🔍 How Files Are Located

To find the data block for an inode:

1. Read the CR and locate the Imap responsible for the inumber
2. Read that Imap to find the inode’s location
3. Read the inode
4. Use direct block offsets to access data blocks

---

## 🧩 Implementation Structure

A practical development path:

1. Parse the Checkpoint Region
2. Parse each Imap listed in the CR
3. Parse each referenced Inode
4. Build in-memory structures for easy traversal
5. Implement `ls`
6. Implement `cat`

Start with the simplest sample image (`simple.img`) before testing more complex cases.

---

## ▶ Usage

### List filesystem tree

```
./bin/parse_lfs ls <image_file>
```

### Print file contents

```
./bin/parse_lfs cat <absolute_path> <image_file>
```

---

## ❗ Error Handling

If the specified file cannot be found:

```
parse_lfs error: Failed to find file.
```

Program exits with code **1**.

---

## 🧪 Testing

Sample tests are included:

```
cd tests/
./run-tests.sh
```

Use the `-h` flag for additional options.

---
