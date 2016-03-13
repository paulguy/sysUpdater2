#ifndef __LOG_H
#define __LOG_H

#include <3ds.h>

#define LOG_BUFFER_SIZE (256)

typedef enum {
  LOG_LEVEL_VERBOSE = 0,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_NEVER
} LogLevel;

typedef struct {
  Handle hLog;
  char buffer[LOG_BUFFER_SIZE];
  LogLevel level;
  u64 fileOffset;
} Log;

extern const char LogFilename[17];
extern const char LogLevelNames[4][8];

extern Log *globalLog;

Log *openLog(const char *filename, LogLevel level);
void closeLog(Log *l);
void writeLog(Log *l, const char *file, int line, const LogLevel level, const char *format, ...);

#define LOG_OPEN() globalLog = openLog(LogFilename, LOG_LEVEL_VERBOSE)
#define LOG_CLOSE() closeLog(globalLog);
#define LOG_VERBOSE(format, ...) writeLog(globalLog, __FILE__, __LINE__, LOG_LEVEL_VERBOSE, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) writeLog(globalLog, __FILE__, __LINE__, LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) writeLog(globalLog, __FILE__, __LINE__, LOG_LEVEL_WARNING, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) writeLog(globalLog, __FILE__, __LINE__, LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#endif
