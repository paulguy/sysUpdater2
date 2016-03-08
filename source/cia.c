#include <string.h>
#include <stdio.h>

#include "sysInfo.h"
#include "updateInfo.h"
#include "file.h"
#include "libmd5-rfc/md5.h"
#include "util.h"
#include "buffer.h"
#include "cia.h"

// /updates/ + 16 digit hex + .cia + \0
#define UPDATE_FILENAME_LEN (9 + 16 + 4 + 1)

const char const *anim = ".oOo";

int checkUpdates(const UpdateInfo *info, TitleList *updateCIAs, PrintConsole *con) {
  Handle CIAHandle;
  char filename[UPDATE_FILENAME_LEN];
  md5_state_t md5;
  u32 bytesread;
  u64 totalread;
  char digest[16];
  char strdigest[33]; // 32 hex digits + \0
  TitleList *extraTitles;
  int i, j;
  int animpos;
  
  extraTitles = initTitleList(updateCIAs->nTitles, 1);
  if(extraTitles == NULL) {
    printf("Couldn't create title list.\n");
    goto error0;
  }
  memcpy(extraTitles->title, updateCIAs->title, sizeof(AM_TitleEntry *) * updateCIAs->nTitles);
  if(extraTitles->nTitles == 0) {
    printf("No titles found to check.\nThere is nothing to do.\n");
    goto error1;
  }
  
  for(i = 0; i < info->nUpdates; i++) {
    snprintf(filename, UPDATE_FILENAME_LEN, CIAS_PATH "%016llX.cia",
             info->updates[i].titleID);
    printf("Checking %s... ", filename);
    stepFrame();

    CIAHandle = openFileHandle(filename, FS_OPEN_READ);
    if(CIAHandle == 0) {
      if(info->info.region == CFG_REGION_JPN && info->updates[i].titleID == 0x000400102002CA00) {
        printf("Bad update missing, this is good.\n");
      } else {
        printf("\nOpen Failed!\n", filename);
        goto error1;
      }
    }

    md5_init(&md5);
    bytesread = BUFFERSIZE;
    totalread = 0;
    animpos = 0;
    while(bytesread == BUFFERSIZE) {
      printf("%c", anim[animpos]);
      con->cursorX--;
      animpos = (animpos + 1) % 4;
      
      if(R_FAILED(FSFILE_Read(CIAHandle, &bytesread, totalread, buffer, BUFFERSIZE))) {
        printf("\nFailed to read %s around %s!\n", filename, totalread);
        stepFrame();
        freeTitleList(extraTitles);
        goto error2;
      }
      md5_append(&md5, buffer, bytesread);
      
      totalread += bytesread;
    }
    closeFileHandle(CIAHandle);
    md5_finish(&md5, digest);

    printf(" \n");
    // Cheezy bin2hex
    snprintf(strdigest, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x \n",
      digest[0], digest[1], digest[2], digest[3], digest[4], digest[5],
      digest[6], digest[7], digest[8], digest[9], digest[10], digest[11], 
      digest[12], digest[13], digest[14], digest[15]);
      
    printf("%s ", strdigest);
    stepFrame();

    if(strncmp(info->updates[i].md5, strdigest, 33) != 0) {
      printf("!= \n%s FAIL\n", info->updates[i].md5);
      goto error1;
    }

    if(info->info.region == CFG_REGION_JPN && info->updates[i].titleID == 0x000400102002CA00) {
      printf("Firmware file known to brick Japanese 3DS\n" \
             "present!  It is recommended you delete the\n" \
             "file named " CIAS_PATH "000400102002CA00.cia\n" \
             "before installing the updates!\n" \
             "Press any key to continue . . .\n");
      stepFrame();
      waitKey();
    } else {
      printf("OK\n");
      stepFrame();

      for(j = 0; j < extraTitles->nTitles; j++) {
        if(extraTitles->title[j] == NULL) {
          continue;
        }
        if(extraTitles->title[j]->titleID == info->updates[i].titleID) {
          extraTitles->title[j] = NULL;
          break;
        }
      }
    }
  }

  rearrangeTitles(extraTitles);
  for(i = 0; i < extraTitles->nTitles; i++) {
    if(extraTitles->title[i] == NULL) {
      extraTitles->nTitles = i;
      break;
    }
  }
  if(extraTitles->nTitles > 0) {
    printf("These are the titles that were found but weren't\n" \
           "in the internal hash tables, meaning they aren't\n" \
           "part of the official update.  They will be\n" \
           "installed if you choose to install now.  Look\n" \
           "over them to see if they are what you want before\n" \
           "choosing to install.");
    waitKey();
    printTitlesMore(extraTitles, con);
    printf("Press any key to continue . . .");
    waitKey();
    printf("\n");
  } else {
    printf("No excess titles.\n");
  }
  freeTitleList(extraTitles);

  return(0);
  
error2:
  closeFileHandle(CIAHandle);
error1:
  freeTitleList(extraTitles);
error0:
  return(-1);
}

const SysInfo *checkCIAs(PrintConsole *con) {
  TitleList *updateCIAs;
  
  int i;
  printf("Gathering CIAs, this may take a while...\n");
  updateCIAs = getUpdateCIAs();

  for(i = 0; i < UPDATEINFO_COUNT; i++) {
    printf("Checking for ");
    printRegionModel(&(updateInfos[i].info));
    printf(" %s", updateInfos[i].version);
    printf("...\n");
    if(checkUpdates(&(updateInfos[i]), updateCIAs, con) == 0) {
      stepFrame();
      freeTitleList(updateCIAs);
      return(&(updateInfos[i].info));
    }
    stepFrame();
  }
  
  freeTitleList(updateCIAs);
  return(NULL);
}

#ifdef ARMED
int deleteTitle(u64 titleID) {
  if(R_FAILED(AM_DeleteTitle(MEDIATYPE_NAND, titleID))) {
    return(-1);
  }
  
  return(0);
}

int installTitleFromCIA(const char *path, PrintConsole *con) {
  Handle CIAIn, CIAOut;
  u32 bytesread;
  u32 byteswritten;
  u64 totalread;
  int animpos;
  
  CIAIn = openFileHandle(path, FS_OPEN_READ);
  if(CIAIn == 0) {
    printf("Failed to open file %s.\n", path);
    goto error0;
  }
  
  if(R_FAILED(AM_StartCiaInstall(MEDIATYPE_NAND, &CIAOut))) {
    printf("Failed to start CIA install.\n");
    goto error1;
  }
  
  bytesread = BUFFERSIZE;
  totalread = 0;
  while(bytesread == BUFFERSIZE) {
    printf("%c", anim[animpos]);
    con->cursorX--;
    animpos = (animpos + 1) % 4;
    
    if(R_FAILED(FSFILE_Read(CIAIn, &bytesread, totalread, buffer, BUFFERSIZE))) {
      printf("\nFailed to read %s around %llu!\n", path, totalread);
      stepFrame();
      goto error2;
    }
    if(R_FAILED(FSFILE_Write(CIAOut, &byteswritten, totalread, buffer, bytesread, FS_WRITE_FLUSH))) {
      printf("\nFailed to write %s around %llu!\n", path, totalread);
      stepFrame();
      goto error2;
    }
    if(byteswritten < bytesread) {
      printf("\nIncompelete write to %s around %llu!\n", path, totalread);
      stepFrame();
      goto error2;
    }

    totalread += bytesread;
  }
  
  if(R_FAILED(AM_FinishCiaInstall(MEDIATYPE_NAND, &CIAOut))) {
    printf("Failed to finalize CIA install.\n");
    goto error2;
  }
  
  closeFileHandle(CIAIn);

  return(0);
  
error2:
  if(R_FAILED(AM_CancelCIAInstall(&CIAOut))) {
    printf("Couldn't cancel unsuccessful CIA install.\n");
  }
error1:
  closeFileHandle(CIAIn);
error0:
  return(-1);
}
#endif

int isCIAName(const char *name) {
  int i;
  
  for(i = 0; i < 16; i++) {
    if(!((name[i] >= '0' && name[i] <= '9') ||
         (name[i] >= 'a' && name[i] <= 'f') ||
         (name[i] >= 'A' && name[i] <= 'F'))) {
      return(0);
    }
  }
  if(name[16] != '.') {
    return(0);
  }
  if(name[17] != 'c' && name[17] != 'C') {
    return(0);
  }
  if(name[18] != 'i' && name[18] != 'I') {
    return(0);
  }
  if(name[19] != 'a' && name[19] != 'A') {
    return(0);
  }
  if(name[20] != '\0') {
    return(0);
  }
  
  return(1);
}

void simpleUTF16toASCII(char *in) {
  int i;
  
  for(i = 0; in[i * 2] != '\0'; i++) {
    in[i] = in[i * 2] & 0x7F;
  }

  in[i] = '\0';
}

TitleList *getUpdateCIAs() {
  Handle dir, CIAFile;
  FS_DirectoryEntry ent;
  int ret;
  int ciafiles;
  TitleList *updateCIAs;
  char ciaPath[9 + 16 + 4 + 1]; // /updates/ + 16 hex digits + .cia + \0
  
  // Run through first and count .cia files.
  dir = openDirectory(CIAS_PATH);
  if(dir == 0) {
    printf("Failed to open SDMC:" CIAS_PATH ".\n");
    goto error0;
  }
  ciafiles = 0;
  for(;;) {
    ret = getNextFile(dir, &ent);
    
    if(ret < 1) {
      if(ret == -1) {
        printf("Error reading directory.\n");
        goto error2;
      }
      break;
    }
    
    simpleUTF16toASCII((char *)(ent.name));
    if(isCIAName((char *)(ent.name)) == 1) {
      ciafiles++;
    }
  }
  closeDirectory(dir);
  
  updateCIAs = initTitleList(ciafiles, 0);
  
  // Run through a second time and add CIA info.
  dir = openDirectory(CIAS_PATH);
  if(dir == 0) {
    printf("Failed to open SDMC:" CIAS_PATH ".\n");
    goto error1;
  }
  ciafiles = 0;
  for(;;) {
    ret = getNextFile(dir, &ent);
    if(ret < 1) {
      break;
    }
    
    simpleUTF16toASCII((char *)(ent.name));
    if(isCIAName((char *)(ent.name)) == 1) {
      snprintf(ciaPath, 9 + 16 + 4 + 1, CIAS_PATH "%s", ent.name);
      CIAFile = openFileHandle(ciaPath, FS_OPEN_READ);
      if(CIAFile == 0) {
        printf("Failed to open %s for read.\n", ciaPath);
        goto error2;
      }
      
      if(R_FAILED(AM_GetCiaFileInfo(MEDIATYPE_NAND, updateCIAs->title[ciafiles], CIAFile))) {
        printf("Failed to get information on %s.\n", ciaPath);
        goto error3;
      }
      
      closeFileHandle(CIAFile);
      
      ciafiles++;
    }
  }
  closeDirectory(dir);
  
  return(updateCIAs);

error3:
  closeFileHandle(CIAFile);
error2:
  closeDirectory(dir);
error1:
  freeTitleList(updateCIAs);
error0:
  return(NULL);
}
