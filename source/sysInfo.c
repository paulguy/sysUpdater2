#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "sysInfo.h"
#include "log.h"

static const u32 priorities[] = {
        0x00040138, // System Firmware
        0x00040130, // System Modules
        // Make sure NS CFA(0004001B00010702) is installed here, used early in boot.
        // Install System Settings (000400100002{0,1,2,6,7,8}000) here
        // Early applets here too
        0x00040030, // Applets
        0x0004001B, // System Data Archives
        0x00040010, // System Applications
        0x0004009B, // System Data Archives (Shared Archives)
        0x000400DB, // System Data Archives
        0x00048005, // TWL System Applications
        0x0004800F  // TWL System Data Archives
};
static const int nPriorities = sizeof(priorities) / sizeof(priorities[0]);

static const u64 beforeApplets[] = {
  0x0004001B00010702, // NS CFA
  0x0004001000020000, 0x0004001000021000, 0x0004001000022000, 
  0x0004001000026000, 0x0004001000027000, 0x0004001000028000, // System Settings
  // Early Applets
  0x000400300000A102, 0x000400300000A902, 0x000400300000B102, // Home Menu
  0x0004003000008802, 0x0004003000009402, 0x0004003000009D02,
  0x000400300000A602, 0x000400300000AE02, 0x000400300000B602, // Old 3DS Internet Browser
  0x0004003020008802, 0x0004003020009402, 0x0004003020009D02,
  0x000400302000AE02, // New 3DS Internet Browser
  // Stuff might freak out about the wrong software keyboard version...
  0x000400300000C002, 0x000400300000C802, 0x000400300000D002,
  0x000400300000D802, 0x000400300000DE02, 0x000400300000E402, // Software Keyboard
  // This update seems to succeed but always crash immediately afterwards, so it
  // should be safer here
  0x0004003000008A02, 0x0004003000008A03, 0x0004003020008A03, // ErrDisp
  0x0004003000008202, 0x0004003000008F02, 0x0004003000009802
};
static const int nBeforeApplets = sizeof(beforeApplets) / sizeof(beforeApplets[0]);

int getTitleTypePriority(u64 TID) {
  int i;

  for(i = 0; i < nPriorities; i++) {
    if((u32)(TID >> 32) == priorities[i]) {
      return(i);
    }
  }
  
  return(nPriorities);
}

// This function is fun and possibly not even effective.
int titleCompare(const void *t1, const void *t2) {
  AM_TitleEntry *title1 = *(AM_TitleEntry **)t1;
  AM_TitleEntry *title2 = *(AM_TitleEntry **)t2;

  int t1prio, t2prio;
  int t1beforeApplets, t2beforeApplets;
  int i;
  t1beforeApplets = 0;
  t2beforeApplets = 0;
  
  //NULL entries go to the end
  if(title1 == NULL) {
    if(title2 == NULL) {
      return(0);
    }
    return(1);
  }
  if(title2 == NULL) {
    return(-1);
  }

  t1prio = getTitleTypePriority(title1->titleID);
  t2prio = getTitleTypePriority(title2->titleID);
  
  // Weird crap to make sure some early boot items get updated before applets
  // which may depend on these.
  for(i = 0; i < nBeforeApplets; i++) {
    if(title1->titleID == beforeApplets[i]) {
      t1beforeApplets = i;
    }
    if(title2->titleID == beforeApplets[i]) {
      t2beforeApplets = i;
    }
  }
  if(t1beforeApplets > 0) {
    if(t2beforeApplets > 0) {
      if(t1beforeApplets < t2beforeApplets) {
        return(-1);
      } else if(t1beforeApplets > t2beforeApplets) {
        return(1);
      }
      
      return(0); // This shouldn't happen...
    }
    
    if(t2prio >= 2) {
      return(-1);
    } else {
      return(1);
    }
  }
  if(t2beforeApplets == 1) {
    if(t1prio >= 2) {
      return(1);
    } else {
      return(-1);
    }
  }
  
  // higher priorities are "less than" lower priorities
  if(t1prio > t2prio) {
    return(1);
  } else if(t1prio < t2prio) {
    return(-1);
  }
 
  // if they're the same priority, as far as we know, they can install in whatever order
  return(0);
}

void rearrangeTitles(TitleList *titles) {
  LOG_VERBOSE("Rearranging titles.");
  qsort(titles->title, titles->nTitles, sizeof(AM_TitleEntry *), titleCompare);
}

// CFGU should be initialized for this
SysInfo *getSysInfo() {
  LOG_VERBOSE("Getting system info.");
  Result ret;
  SysInfo *sysInfo = (SysInfo *) malloc(sizeof(SysInfo));
  if(sysInfo == NULL) {
    LOG_ERROR("getSysInfo: Couldn't allocate memory.");
    return(NULL);
  }
  
  ret = CFGU_GetSystemModel(&sysInfo->model);
  if(R_FAILED(ret)) {
    LOG_ERROR("getSysInfo: CFGU_GetSystemModel failed.");
    free(sysInfo);
    return(NULL);
  }
    
  ret = CFGU_SecureInfoGetRegion(&sysInfo->region);
  if(R_FAILED(ret)) {
    LOG_ERROR("getSysInfo: CFGU_SecureInfoGetRegion failed.");
    free(sysInfo);
    return(NULL);
  }

  return sysInfo;
}

TitleList *initTitleList(int count, int pointersOnly) {
  TitleList *titles;
  int i;

  LOG_VERBOSE("initTitleList");
  titles = malloc(sizeof(TitleList));
  if(titles == NULL) {
    goto error0;
  }
  
  titles->nTitles = count;

  titles->title = malloc(sizeof(AM_TitleEntry *) * titles->nTitles);
  if(titles->title == NULL) {
    goto error1;
  }
  if(pointersOnly == 0) {
    titles->__buffer = malloc(sizeof(AM_TitleEntry) * titles->nTitles);
    if(titles->__buffer == NULL) {
      goto error2;
    }
    for(i = 0; i < titles->nTitles; i++) {
      titles->title[i] = (AM_TitleEntry *)&(((char *)(titles->__buffer))[i * sizeof(AM_TitleEntry)]);
    }
  } else {
    titles->__buffer = NULL;
  }

  LOG_VERBOSE("initTitleList succeeded");
  return(titles);
  
error2:
  free(titles->title);
error1:
  free(titles);
error0:
  LOG_ERROR("initTitleList failed");
  return(NULL);
}

void freeTitleList(TitleList *titles) {
  LOG_VERBOSE("freeTitleList");
  if(titles->__buffer != NULL) {
    free(titles->__buffer);
  }
  free(titles->title);
  free(titles);
}

// AM should be initialized before calling this
TitleList *getInstalledTitles() {
  TitleList *titles;
  u32 count;

  LOG_VERBOSE("Getting installed titles.");
  if (R_FAILED(AM_GetTitleCount(MEDIATYPE_NAND, &count))) {
      return(NULL);
  }

  titles = initTitleList(count, 0);

  u64 titlesId[titles->nTitles];
  if (R_FAILED(AM_GetTitleIdList(MEDIATYPE_NAND, titles->nTitles, titlesId))) {
    LOG_ERROR("getInstalledTitles: AM_GetTitleIdList failed.");
    goto error1;
  }

  if (R_FAILED(AM_ListTitles(MEDIATYPE_NAND, count, titlesId, titles->__buffer))) {
    LOG_ERROR("getInstalledTitles: AM_ListTitles failed.");
    goto error1;
  }

  LOG_VERBOSE("Succeeded getting installed titles.");
  return titles;

error1:
  freeTitleList(titles);
error0:
  LOG_ERROR("Failed getting installed titles.");
  return(NULL);
}

int matchModels(u8 model1, u8 model2) {
  if(model1 == MODEL_O3DS || model1 == MODEL_O3DSXL || model1 == MODEL_2DS) {
    model1 = MODEL_O3DS;
  } else if(model1 == MODEL_N3DS || model1 == MODEL_N3DSXL) {
    model1 = MODEL_N3DS;
  } else {
    return(0);
  }

  if(model2 == MODEL_O3DS || model2 == MODEL_O3DSXL || model2 == MODEL_2DS) {
    model2 = MODEL_O3DS;
  } else if(model2 == MODEL_N3DS || model2 == MODEL_N3DSXL) {
    model2 = MODEL_N3DS;
  } else {
    return(0);
  }

  return(model1 == model2);
}
