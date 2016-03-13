#include <stdio.h>
#include <malloc.h>

#include "file.h"
#include "log.h"

static FS_Archive sdmcArchive;

Handle openFileHandle(const char *path, u32 openFlags) {
  Handle fileHandle = 0;
  
  LOG_VERBOSE("openFileHandle: %s, %d", path, openFlags);
  FS_Path filePath = fsMakePath(PATH_ASCII, path);
  if(R_FAILED(FSUSER_OpenFile(&fileHandle, sdmcArchive, filePath, openFlags, 0))) {
    LOG_VERBOSE("FSUSER_OpenFile failed.");
    return(0);
  }
  
  LOG_VERBOSE("File opened successfully.");
  return(fileHandle);
}

void closeFileHandle(const Handle handle) {
  LOG_VERBOSE("closeFileHandle");
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

  LOG_VERBOSE("openDirectory: %s", path);
  FS_Path filePath = fsMakePath(PATH_ASCII, path);
  if(R_FAILED(FSUSER_OpenDirectory(&dir, sdmcArchive, filePath))) {
    LOG_VERBOSE("FSUSER_OpenDirectory failed.");
    return(0);
  }
  
  LOG_VERBOSE("Directory opened successfully.");
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
  LOG_VERBOSE("closeDirectory");
  FSDIR_Close(dir);
}

// don't bother with log messages, this is only used before the logging is started
int deleteFile(const char *path) {
  FS_Path filePath = fsMakePath(PATH_ASCII, path);
  if(R_FAILED(FSUSER_DeleteFile(sdmcArchive, filePath))) {
    return(-1);
  }
  
  return(0);
}
