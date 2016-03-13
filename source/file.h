#ifndef __FILE_H
#define __FILE_H

#include <3ds.h>

Handle openFileHandle(const char *path, u32 openFlags);
void closeFileHandle(const Handle handle);
int sdmcArchiveInit();
void sdmcArchiveExit();
Handle openDirectory(const char *path);
int getNextFile(Handle dir, FS_DirectoryEntry *ent);
void closeDirectory(Handle dir);
int deleteFile(const char *path);

#endif
