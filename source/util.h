#ifndef __UTIL_H
#define __UTIL_H

#include <3ds.h>
#include "sysInfo.h"

u32 getKeyState();
void stepFrame();
void clearDisplay(PrintConsole *con);
void waitKey();
void printRegionModel(const SysInfo *info);
int yesNoCancel();
void printTitlesMore(TitleList *titles, PrintConsole *con);

#endif
