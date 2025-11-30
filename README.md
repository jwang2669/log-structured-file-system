# Parse_LFS

`Parse_LFS` is a tool designed to analyze and reconstruct the structure of a **Log-Structured File System (LFS)** from a raw disk image.
It reads metadata and data blocks stored in the image, rebuilds the file system hierarchy, and supports two primary operations:

* Listing all files and directories in the LFS image (`ls`)
* Reading and printing the contents of a specified file (`cat`)

This project demonstrates how an LFS organizes data (inodes, imaps, data blocks) and how to reconstruct files from the log.

## Features

### 🔹 **1. Print LFS Tree (`ls`)**

$ ./bin/parse_lfs ls ./sample_files/a.img
/
         256 2025-04-15 19:00:18 UTC -rw-------    bar.txt
         ...

`ls` recursively prints the entire directory tree, starting from the root `/`.

Output formatting details:

* Indentation reflects directory depth.
* Items are **sorted alphabetically**.
* **Directories are listed after files** at each depth level.

### 🔹 **2. Print File Contents (`cat`)**

$ ./bin/parse_lfs cat /bar.txt ./sample_files/a.img
<file content>

The `cat` operation searches for a given path, locates the corresponding inode, reads its data blocks, and prints only the **valid file content** (excluding any unused padding bytes).

Supports logical path navigation such as:

/dir/b/bar.txt
/dir/./b/bar.txt
/dir/c/../b/bar.txt

### 🔸 Error Handling

If the path does not exist or the file cannot be found:

parse_lfs error: Failed to find file.

Exit code: `1`

## LFS Image Structure

### 🧩 **1. Checkpoint Region (CR)**

Each LFS image begins with a 1 KB checkpoint region:

uint Image_Offset
uint Entry_Count

// Repeated CR entries:
uint inumber_start
uint inumber_end
uint imap_disk_offset

Each CR entry points to an imap and defines which inode range it maps.

### 🧩 **2. Imaps**

Each imap is also a 1 KB metadata block:

uint Entry_Count

// Repeated entries:
uint inumber
uint inode_disk_offset

This table maps inumbers to the disk offsets of their inode structures.

### 🧩 **3. Inodes**

Each inode is stored in a 1 KB region:

uint    file_cursor
uint    size
mode_t  permissions
time_t  mtime
uint    Direct_Block_Count

// Direct block offsets:
uint direct_block_disk_offset
uint direct_block_disk_offset

Only **direct blocks** are supported (no indirect blocks).

### 🧩 **4. Data Blocks**

Each data block is 4 KB.

* Regular files contain raw bytes.
* Directories contain repeated entries in the form:

fileName,    // not null-terminated, ends with comma
uint inumber

Example directory block:

a,5
b,6
c,7
Entries in directory blocks are *not guaranteed to be sorted*.

## Implementation Overview

Typical parsing flow:

1. Read and validate the Checkpoint Region.
2. Load all imaps referenced by the CR.
3. Load all inodes referenced by the imaps.
4. Build the directory tree in memory.
5. Handle `ls` and `cat` operations.

## Build Instructions

Compile with:

bash
make

The resulting executable is located at:

./bin/parse_lfs

## Usage

### **List all files**

./bin/parse_lfs ls <image_file>

### **Print a file’s contents**

./bin/parse_lfs cat <path_in_lfs> <image_file>

Example:

./bin/parse_lfs cat /dir/foo.txt ./sample_files/a.img

## Project Structure

Parse_LFS/
├── src/
│   ├── parse_lfs.c
│   ├── parse_lfs.h
├── sample_files/
├── bin/
└── README.md

## Testing

Sample LFS images are provided under `sample_files/` for basic testing:

./bin/parse_lfs ls ./sample_files/simple.img

Users can create additional LFS images for extended testing.

## License

This project is intended for personal learning and research.
You are free to modify or extend it.
