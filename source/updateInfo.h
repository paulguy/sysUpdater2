//
// Created by cpasjuste on 11/01/16, converted to C by paulguy
//

#ifndef __UPDATEINFO_H
#define __UPDATEINFO_H

#include "sysInfo.h"

typedef struct {
  const char md5[33];
  const u64 titleID;
} UpdateItem;

typedef struct {
  const SysInfo info;
  const char version[10]; // XX.Y.Z-NN\0
  const int nUpdates;
  const UpdateItem const updates[130];
} UpdateInfo;

#define UPDATEINFO_COUNT (6)
extern const UpdateInfo const updateInfos[UPDATEINFO_COUNT];

#endif
