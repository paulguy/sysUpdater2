#include <stdio.h>
#include <string.h>

#include "util.h"

u32 getKeyState() {
  hidScanInput();
  return(hidKeysDown());
}

void stepFrame() {
  gfxFlushBuffers();
  gfxSwapBuffers();
  gspWaitForVBlank();
}

void clearDisplay(PrintConsole *con) {
  int i;

  u16 fbW, fbH;
  u8 *fb;
  
  fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &fbW, &fbH);
  
  memset(fb, 0, fbW * fbH * 2);

  con->cursorX = 0;
  con->cursorY = 0;
}

void waitKey() {
  while(getKeyState() == 0) {
    gspWaitForVBlank();
  }
}

void printRegionModel(const SysInfo *info) {
  if(info == NULL) {
    printf("Couldn't get region/model!");
    return;
  }
  
  switch(info->region) {
    case CFG_REGION_JPN:
      printf("Japanese ");
      break;
    case CFG_REGION_USA:
      printf("North American ");
      break;
    case CFG_REGION_EUR:
      printf("European ");
      break;
    case CFG_REGION_AUS:
      printf("Australian ");
      break;
    case CFG_REGION_CHN:
      printf("Chinese ");
      break;
    case CFG_REGION_KOR:
      printf("Korean ");
      break;
    case CFG_REGION_TWN:
      printf("Taiwanese ");
      break;
    default:
      printf("UnknownRegion ");
  }
  switch(info->model) {
    case 0:
      printf("Old 3DS");
      break;
    case 1:
      printf("Old 3DS XL");
      break;
    case 2:
      printf("New 3DS");
      break;
    case 3:
      printf("2DS");
      break;
    case 4:
      printf("New 3DS XL");
      break;
    default:
      printf("UnknownModel");
  }
}

int yesNoCancel() {
  u32 keys;
  
  printf("(A) Yes/(B) No/(X) Cancel");
  stepFrame();
  for(;;) {
    keys = getKeyState();
    
    switch(keys) {
      case KEY_A:
        return(1);
        break;
      case KEY_B:
        return(0);
        break;
      case KEY_X:
        return(-1);
        break;
    }
    
    gspWaitForVBlank();
  }
  
  return(0);
}

void printTitlesMore(TitleList *titles, PrintConsole *con) {
  int titlesPerScreen;
  int titlesThisScreen;
  int titlenum;
  int printed;
  int lines;
  int oldfg;
  int i, j;

  oldfg = con->fg;
  titlesPerScreen = (con->consoleHeight - 1) * 2;
  
  printed = 0;
  for(i = 0; i < titles->nTitles; i += titlesPerScreen) {
    printf("\n");
    
    titlesThisScreen = titles->nTitles - i < titlesPerScreen ?
                       titles->nTitles - i : titlesPerScreen;
    lines = (titlesThisScreen / 2) + (titlesThisScreen % 2); // ceiling

    for(j = 0; j < lines; j++) {
      titlenum = i + j;
      con->fg = getTitleTypePriority(titles->title[titlenum]->titleID) + 1;
      printf("%02X:%016llX %04X\n", titlenum,
        titles->title[titlenum]->titleID, titles->title[titlenum]->version);
    }
    con->cursorY -= lines;
    for(j = lines; j < titlesThisScreen; j++) {
      titlenum = i + j;
      con->fg = getTitleTypePriority(titles->title[titlenum]->titleID) + 1;
      con->cursorX = con->consoleWidth / 2;
      printf("%02X:%016llX %04X\n", titlenum,
        titles->title[titlenum]->titleID, titles->title[titlenum]->version);
    }
    
    if(i + titlesThisScreen < titles->nTitles) {
      con->cursorX = 0;
      con->cursorY = con->consoleHeight - 1;
      con->fg = 7;
      con->flags |= CONSOLE_COLOR_REVERSE;
      printf(" MORE ");
      stepFrame();
      con->flags &= ~CONSOLE_COLOR_REVERSE;
      waitKey();
    } else if(titlesThisScreen % 2 == 1) {
      printf("\n");
    }
  }

  con->fg = oldfg;
}
