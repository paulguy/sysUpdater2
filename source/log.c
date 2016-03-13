#include "log.h"
#include "file.h"

#include <stdarg.h>
#include <malloc.h>
#include <stdio.h>

const char LogFilename[17] = "/sysUpdater2.txt";
const char LogLevelNames[4][8] = {"VERBOSE", "INFO", "WARNING", "ERROR"};
Log *globalLog = NULL;

Log *openLog(const char *filename, LogLevel level) {
  Log *l;
  
  l = malloc(sizeof(Log));
  if(l == NULL) {
    goto error0;
  }
  
  // we don't care if it fails, and there's nothing we can do about it anyway
  deleteFile(filename);
  
  l->hLog = openFileHandle(filename, FS_OPEN_WRITE | FS_OPEN_CREATE);
  if(l->hLog == 0) {
    goto error1;
  }
  
  l->level = level;
  l->fileOffset = 0;
  
  return(l);
  
error1:
  free(l);
error0:
  return(NULL);
}

void closeLog(Log *l) {
  closeFileHandle(l->hLog);
  free(l);
}

void writeLog(Log *l, const char *file, int line, const LogLevel level, const char *format, ...) {
  u64 offset;
  u32 written;
  int bytes;
  va_list ap;
  
  if(l == NULL) {
    return;
  }
  
  if(level < l->level) {
    return;
  }

  offset = 0;
  if(level < 0 || level >= LOG_LEVEL_NEVER) {
    bytes = snprintf(l->buffer, LOG_BUFFER_SIZE, "[UNKNOWN] %s:%d: ", file, line);
    if(bytes < 0) { // Nothing more we can do
      return;
    }
  } else {
    bytes = snprintf(l->buffer, LOG_BUFFER_SIZE, "[%s] %s:%d: ",
                     LogLevelNames[level], file, line);
    if(bytes < 0) {
      return;
    }
  }
  FSFILE_Write(l->hLog, &written, l->fileOffset + offset, l->buffer, bytes, FS_WRITE_FLUSH);
  offset += written;
  
  va_start(ap, format);
  bytes = vsnprintf(l->buffer, LOG_BUFFER_SIZE, format, ap);
  if(bytes < 0) {
    return;
  }
  va_end(ap);
  FSFILE_Write(l->hLog, &written, l->fileOffset + offset, l->buffer, bytes, FS_WRITE_FLUSH);
  offset += written;
  
  bytes = snprintf(l->buffer, LOG_BUFFER_SIZE, "\n");
  if(bytes < 0) {
    return;
  }
  FSFILE_Write(l->hLog, &written, l->fileOffset + offset, l->buffer, bytes, FS_WRITE_FLUSH);
  offset += written;
  
  l->fileOffset += offset;
}
