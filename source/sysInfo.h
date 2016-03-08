#ifndef __SYSINFO_H
#define __SYSINFO_H

#include <3ds.h>

typedef enum {
  MODEL_O3DS = 0,
  MODEL_O3DSXL,
  MODEL_N3DS,
  MODEL_2DS,
  MODEL_N3DSXL
} Model;

typedef struct {
  Model model; //  (0 = O3DS, 1 = O3DSXL, 2 = N3DS, 3 = 2DS, 4 = N3DSXL)
  CFG_Region region;
} SysInfo;

typedef struct {
  int nTitles;
  AM_TitleEntry **title;
  void *__buffer;
} TitleList;

int getTitleTypePriority(u64 TID);
void rearrangeTitles(TitleList *titles);
SysInfo *getSysInfo();
TitleList *initTitleList(int count, int pointersOnly);
void freeTitleList(TitleList *titles);
TitleList *getInstalledTitles();
int matchModels(u8 model1, u8 model2);

#endif
