#include "parse_lfs.h"
CR *cr; char *imgMap; int valid = -1;
Inode* findInode(uint inumber);
Directory* buildDirectory(Inode *dirInode);
void LS(Directory *dir, int depth);
void CAT(Directory *currDir, char *filePath[100], int pathIndex, int numPath);

int main(int argc, char **argv) {
  int imgFd = 0; off_t imgSize = 0;
  if (argc == 3 && strcmp(argv[1], "ls") == 0) {
    imgFd = open(argv[2], O_RDONLY);
    imgSize = lseek(imgFd, 0, SEEK_END);
    imgMap = mmap(NULL, imgSize, PROT_READ, MAP_PRIVATE, imgFd, 0);
    cr = (CR*)imgMap; //
    LS(buildDirectory(findInode(0)), 1);
  } else if (argc == 4 && strcmp(argv[1], "cat") == 0) {
    imgFd = open(argv[3], O_RDONLY);
    imgSize = lseek(imgFd, 0, SEEK_END);
    imgMap = mmap(NULL, imgSize, PROT_READ, MAP_PRIVATE, imgFd, 0);
    cr = (CR*)imgMap; //

    char *filePath[1000]; int numPath = 0;
    char *token = strtok(argv[2], "/");
    while (token != NULL) {
      filePath[numPath++] = token;
      token = strtok(NULL, "/");
    }
    for (int i = 0; i < numPath;) {
      if (strcmp(filePath[i], ".") == 0) {
        numPath--;
        for (int j = i; j < numPath; j++) filePath[j] = filePath[j+1];
      } else i++;
    }
    for (int i = 0; i < numPath;) {
      if (strcmp(filePath[i], "..") == 0) {
        if (i == 0) {
          numPath--;
          for (int j = i; j < numPath; j++) filePath[j] = filePath[j + 1];
        } else {
          numPath -= 2;
          for (int j = i - 1; j < numPath; j++) filePath[j] = filePath[j + 2];
          i--;
        }
      } else i++;
    }
    CAT(buildDirectory(findInode(0)), filePath, 0, numPath);
    munmap(imgMap, imgSize);
    close(imgFd);
    if (valid == -1) { fprintf(stderr, "parse_lfs error: Failed to find file.\n"); return 1; }
  }

  munmap(imgMap, imgSize);
  close(imgFd);
  return 0;
}

Inode* findInode(uint inumber) {
  for (uint i = 0; i < cr->num_entries; i++) {
    if (inumber >= cr->entries[i].inumber_start && inumber <= cr->entries[i].inumber_end) {
      Imap *imap = (Imap*)(imgMap + cr->entries[i].imap_disk_offset);
      for (uint j = 0; j < imap->num_entries; j++) if (imap->entries[j].inumber == inumber) return (Inode*)(imgMap + imap->entries[j].inode_disk_offset);
    }
  }
  return NULL;
}

Directory* buildDirectory(Inode *dirInode) {
  Directory *dir = malloc(sizeof(Directory)); dir->num_entries = 0; dir->entries = malloc(0);
  char buffer[1000]; int bufferSize = 0;
  int offset1 = dirInode->entries[0].direct_block_disk_offset, offset2 = dirInode->entries[0].direct_block_disk_offset; uint i = 0;
  while (i < dirInode->size) {
    offset1 = dirInode->entries[i / 4096].direct_block_disk_offset;
    offset2 = dirInode->entries[(i + 1) / 4096].direct_block_disk_offset;
    if (imgMap[offset1 + i % 4096] == ',') {
      dir->num_entries++;
      dir->entries = realloc(dir->entries, dir->num_entries * sizeof(DirEntry));
      dir->entries[dir->num_entries - 1].fileName = malloc(bufferSize + 1); //////
      strncpy(dir->entries[dir->num_entries - 1].fileName, buffer, bufferSize); //
      dir->entries[dir->num_entries - 1].fileName[bufferSize] = '\0'; ////////////
      memset(buffer, 0, sizeof(buffer)); bufferSize = 0; //
      dir->entries[dir->num_entries - 1].inumber = *((int*)(imgMap + offset2 + (i + 1) % 4096)); i += 5;
    } else buffer[bufferSize++] = imgMap[offset1 + (i++ % 4096)];
  }
  return dir;
}

void LS(Directory *dir, int depth) {
  DirEntry *dirEntries[dir->num_entries], *fileEntries[dir->num_entries], *temp;
  int dirEntrySize = 0, fileEntrySize = 0, nextDepth = depth + 1;
  for (uint i = 0; i < dir->num_entries; i++) {
    Inode *inode = findInode(dir->entries[i].inumber);
    if (S_ISDIR(inode->permissions)) dirEntries[dirEntrySize++] = dir->entries + i;
    else fileEntries[fileEntrySize++] = dir->entries + i;
  }
  for (int i = 0; i < fileEntrySize - 1; i++) {
    for (int j = i + 1; j < fileEntrySize; j++) {
      if (strcmp(fileEntries[i]->fileName, fileEntries[j]->fileName) > 0) {
        temp = fileEntries[i];
        fileEntries[i] = fileEntries[j];
        fileEntries[j] = temp;
      }
    }
  }
  for (int i = 0; i < dirEntrySize - 1; i++) {
    for (int j = i + 1; j < dirEntrySize; j++) {
      if (strcmp(dirEntries[i]->fileName, dirEntries[j]->fileName) > 0) {
        temp = dirEntries[i];
        dirEntries[i] = dirEntries[j];
        dirEntries[j] = temp;
      }
    }
  }

  if (depth == 1) printf("/\n");
  for (int i = 0; i < fileEntrySize; i++) {
    Inode *inode = findInode(fileEntries[i]->inumber);
    ls_print_file(fileEntries[i]->fileName, inode->size, inode->permissions, inode->mtime, depth);
  }
  for (int i = 0; i < dirEntrySize; i++) {
    Inode *inode = findInode(dirEntries[i]->inumber);
    ls_print_file(dirEntries[i]->fileName, inode->size, inode->permissions, inode->mtime, depth);
    LS(buildDirectory(findInode(dirEntries[i]->inumber)), nextDepth);
  }

  for (uint i = 0; i < dir->num_entries; i++) free(dir->entries[i].fileName); // free
  free(dir->entries);
  free(dir);
}

void CAT(Directory *currDir, char *filePath[100], int pathIndex, int numPath) {
  for (uint i = 0; i < currDir->num_entries; i++) {
    if (strcmp(currDir->entries[i].fileName, filePath[pathIndex]) == 0) {
      if (pathIndex == numPath - 1) {
        valid = 1;
        Inode *inode = findInode(currDir->entries[i].inumber);
        for (uint j = 0; j < inode->size; j++) printf("%c", *(imgMap + inode->entries[j/4096].direct_block_disk_offset + j % 4096));
      } else CAT(buildDirectory(findInode(currDir->entries[i].inumber)), filePath, ++pathIndex, numPath);
    }
  }

  for (uint i = 0; i < currDir->num_entries; i++) free(currDir->entries[i].fileName); // free
  free(currDir->entries);
  free(currDir);
}