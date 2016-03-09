#include <stdio.h>
#include <malloc.h>

#include "file.h"

static FS_Archive sdmcArchive;

Handle openFileHandle(const char *path, u32 openFlags) {
  Handle fileHandle = 0;

  FS_Path filePath = fsMakePath(PATH_ASCII, path);
  if(R_FAILED(FSUSER_OpenFile(&fileHandle, sdmcArchive, filePath, openFlags, 0))) {
    return(0);
  }
  
  return fileHandle;
}

void closeFileHandle(const Handle handle) {
  FSFILE_Close(handle);
}

// Opens the SD card so files can be opened from it.
int sdmcArchiveInit() {
  Result response;
  
  sdmcArchive = (FS_Archive) {ARCHIVE_SDMC, (FS_Path) {PATH_EMPTY, 1, (u8 *) ""}};
  if(R_FAILED(response = FSUSER_OpenArchive(&sdmcArchive))) {
    return(-1);
  }
  
  return(0);
}

void sdmcArchiveExit() {
  FSUSER_CloseArchive(&sdmcArchive);
}

Handle openDirectory(const char *path) {
  Handle dir;

  FS_Path filePath = fsMakePath(PATH_ASCII, path);
  if(R_FAILED(FSUSER_OpenDirectory(&dir, sdmcArchive, filePath))) {
    return(0);
  }
  
  return(dir);
}

int getNextFile(Handle dir, FS_DirectoryEntry *ent) {
  u32 count;
  
  if(R_FAILED(FSDIR_Read(dir, &count, 1, ent))) {
    return(-1);
  }
  
  if(count == 0) {
    return(0);
  }
  
  return(1);
}

void closeDirectory(Handle dir) {
  FSDIR_Close(dir);
}
