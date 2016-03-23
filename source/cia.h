#ifndef __CIA_H
#define __CIA_H

#include "sysInfo.h"

// Uncomment to enable real install capability
//#define ARMED

#define CIAS_PATH "/updates/"

const SysInfo *checkCIAs();
int deleteTitle(u64 titleID);
int installTitleFromCIA(const char *path, PrintConsole *con);
TitleList *getUpdateCIAs();

#endif
