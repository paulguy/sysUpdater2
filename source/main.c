#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "sysInfo.h"
#include "cia.h"
#include "file.h"
#include "log.h"
#include "svchax/svchax.h"
#include "logo_data.h"

typedef enum {
  SU2_ACT_NONE,
  SU2_ACT_EXIT,
  SU2_ACT_CHECK,
  SU2_ACT_LIST,
  SU2_ACT_INSTALL,
  SU2_ACT_MODE,
  SU2_ACT_EXPLOIT,
  SU2_ACT_FIRMTRANSFER
} MenuAction;

typedef enum {
  MODE_NEVER = 0,
  MODE_UPGRADE,
  MODE_DOWNGRADE,
  MODE_ALWAYS
} InstallMode;

const char const *installModeNames[] = {
  "Mode:   Never  ",
  "Mode:  Upgrade ",
  "Mode: Downgrade",
  "Mode:  Always  "
};
#define MODE_TEXT_LEN (15)

void printMain(PrintConsole *con, SysInfo *info, InstallMode mode);
void printMenu();
void printRegionModelMain(PrintConsole *con, SysInfo *info);
void printMode(PrintConsole *con, InstallMode mode);
PrintConsole *initializeDisplay();
void uninitializeDisplay();
MenuAction getActionFromKeys(u32 keys);
void listInstalledTitles(PrintConsole *con);
int installCIAs(PrintConsole *con, InstallMode mode);
int testAMuAccess();

// Stuff for libsu's patchServiceAccess()
extern u8 isNew3DS;
extern void patchServiceAccess();

int main(int argc, char **argv) {
  PrintConsole *con;
  SysInfo *info;
  u32 keys;
  int i;
  int running;
  InstallMode mode = MODE_NEVER; // Default to a safe, disarmed mode.

  con = initializeDisplay();
  if(con == NULL) {
    return(0); //Nothing is initialized at this point, so we should be able to just exit...
  }
  
  if(R_FAILED(fsInit())) {
    printf("Failed to initialize FS.\n");
    printf("Press any key to quit . . .");
    stepFrame();
    waitKey();
    return(0);
  }
  if(sdmcArchiveInit() < 0 ) { // need this early for logging
    printf("Couldn't open SD card.\n");
    printf("Press any key to quit . . .");
    stepFrame();
    waitKey();
    return(0);
  }

  LOG_OPEN();

  printMenu();

  stepFrame();

  if(R_FAILED(cfguInit())) {
    goto finish;
  }
  info = getSysInfo();
  cfguExit();
  
  // Needed for libsu patchServiceAccess()
  isNew3DS = matchModels(info->model, MODEL_N3DS);
  
  printRegionModelMain(con, info);
  printMode(con, mode);
  stepFrame();

  LOG_INFO("Main loop start");

  running = 1;
  while(aptMainLoop() && running) {
    keys = getKeyState();
    
    switch(getActionFromKeys(keys)) {
      case SU2_ACT_EXIT:
        LOG_INFO("Exit requested");
        printf("Exiting...\n");
        stepFrame();
        running = 0;
        break;
      case SU2_ACT_CHECK:
        LOG_INFO("CIA file check requested");
        clearDisplay(con);
        printf("Checking CIA files...\n");
        if(R_FAILED(amInit())) {
          LOG_ERROR("Failed to initialize AM.");
          printf("Failed to initialize AM.\n");
          stepFrame();
          waitKey();
          printMain(con, info, mode);
          break;
        }
        const SysInfo *CIAInfo = checkCIAs(con);
        amExit();
        if(CIAInfo == NULL) {
          LOG_INFO("No match found.");
          printf("No match found, not recommended you continue\n" \
                 "unless you know what you're doing.  Also This\n" \
                 "doesn't necessarily mean your update is bad,\n" \
                 "just that this program doesn't recognize it.\n\n" \
                 "Recognized versions: 9.2.0-20 J/U/E New/Old 3DS\n");
        } else {
          LOG_INFO("Match found.");
          printRegionModel(CIAInfo);
          printf(".\n");
          if(CIAInfo->region == info->region && matchModels(CIAInfo->model, info->model)) {
            printf("This is a match for your detected system.\n");
          } else {
            printf("Which doesn't match your detected system!\nOnly continue if this is what you intend!\n");
          }
        }
        printf("\n");
        printf("Press any key to continue . . .");
        stepFrame();
        waitKey();
        printMain(con, info, mode);
        break;
      case SU2_ACT_LIST:
        LOG_INFO("List installed titles requested.");
        clearDisplay(con);
        printf("Listing installed titles...\n");
        listInstalledTitles(con);
        printf("Press any key to continue . . .");
        stepFrame();
        waitKey();
        printMain(con, info, mode);
        break;
      case SU2_ACT_INSTALL:
        LOG_INFO("Install requested.");
        clearDisplay(con);
        printf("Installing CIAs...\n");

        if(R_FAILED(amInit())) {
          LOG_ERROR("Failed to initialize AM.");
          printf("Failed to initialize AM.\n");
          stepFrame();
          waitKey();
          printMain(con, info, mode);
          break;
        }

        if(installCIAs(con, mode) < 0) {
          LOG_INFO("CIA installation failed.");
          printf("Installation of CIAs failed!\n\n" \
                 "If only some installed successfully, you might\n" \
                 "have a bricked 3DS at this point, sorry...\n\n" \
                 "If the install was canceled or errored before it\n" \
                 "started, there should be no problems.\n");
        } else {
          LOG_INFO("CIA installation succeeded.");
          printf("Installation successful!\n" \
                 "Suggest you shut off the 3DS and power it back on.\n\n");
        }
        amExit();
        printf("Press any key to continue . . .");
        stepFrame();
        waitKey();
        printMain(con, info, mode);
        break;
      case SU2_ACT_MODE:
        LOG_INFO("Mode changed to %d.", mode);
        mode = (mode + 1) % 4;
        printMode(con, mode);
        stepFrame();
        break;
      case SU2_ACT_EXPLOIT:
        LOG_INFO("Activate exploit requested.");
        clearDisplay(con);
        if(testAMuAccess() == 1) {
          LOG_INFO("Exploit already activated or not needed.");
          printf("We already have AM:u access.\n");
        } else {
          printf("This will try to activate the exploit needed to\n" \
                 "actually install something.  However, there is a\n" \
                 "chance that it will just crash, but it shouldn't\n" \
                 "harm anything if it does.  If it does crash,\n" \
                 "just hard shut down the 3DS (Hold Power until\n" \
                 "the screen powers off, then wait until the lights\n" \
                 "turn off.) then power it up again and try again.\n" \
                 "Even if it succeeded, it might still crash!\n\n" \
                 "A tip if you're having trouble: Try running this\n" \
                 "program (Or most others seem to work, really.)\n" \
                 "once, exiting it, then running it second time.\n\n" \
                 "Press any key to continue . . .\n");
          stepFrame();
          waitKey();

          printf("Trying exploit...\n");
          svchax_init();
          if(__ctr_svchax > 0) {
            patchServiceAccess();
            if(testAMuAccess() == 1) {
              LOG_INFO("Exploit success.");
              printf("The exploit appeared to succeed.  Quitting this\n" \
                     "program now will crash, so you'll have to shut\n" \
                     "off the 3DS by holding power when you want to\n" \
                     "quit.\n");
            } else {
              LOG_INFO("Exploit reported failure, but no AM:u access.");
              printf("The exploit reported success but we don't have\n" \
                     "AM:u access, so we can't install anything.\n" \
                     "Suggest shutting off the 3DS and trying again.\n");
            }
          } else {
            LOG_INFO("Exploit failed.");
            printf("The exploit didn't crash, but it failed.\n\n" \
                   "You should probably shut down fully then restart\n" \
                   "before trying again.\n");
          }
        }
        printf("Press any key to continue . . .");
        stepFrame();
        waitKey();
        printMain(con, info, mode);
        break;
      case SU2_ACT_FIRMTRANSFER:
        TitleList *installedTitles;
      
        LOG_INFO("Firmware transfer requested.");
        clearDisplay(con);
        printf("Installing NATIVE_FIRM, this will probably crash but\n" \
               "still succeed, maybe.\n");
        printf("Press any key to continue . . .\n");
        stepFrame();
        waitKey();
        
        if(R_FAILED(amInit())) {
          LOG_ERROR("Failed to initialize AM.");
          printf("Failed to initialize AM.\n");
          stepFrame();
          waitKey();
          printMain(con, info, mode);
          break;
        }

        printf("Getting installed titles...\n");
        installTitles = getInstalledTitles();
        printf("Finding firmware title...\n");
        for(i = 0; i < installedTitles->nTitles; i++) {
          if(installedTitles->title[i]->titleID == 0x0004013800000002 ||
             installedTitles->title[i]->titleID == 0x0004013820000002) {
#ifdef ARMED
            if(R_FAILED(AM_InstallFirm(installedTitles->title[i]->titleID))) {
              LOG_INFO("Firmware transfer failed.");
              printf("Installing NATIVE_FIRM failed.\n");
            } else {
              LOG_INFO("Firmware transfer succeeded.");
              printf("Installing NATIVE_FIRM succeeded.\n");
            }
#else
            printf("Not performed in DISAARMED edition.\n");              
#endif
            break;
          }
        }
        freeTitleList(installedTitles);
        amExit();
        printf("Press any key to continue . . .\n");
        stepFrame();
        waitKey();
        printMain(con, info, mode);
        break;
      default:
        break;
    }
    
    gspWaitForVBlank();
  }

  LOG_INFO("Main loop end");
  
finish:
  free(info);
  LOG_CLOSE();
  sdmcArchiveExit();
  fsExit();
  uninitializeDisplay();
  
  return(0);
}

void printMain(PrintConsole *con, SysInfo *info, InstallMode mode) {
  clearDisplay(con);
  printMenu();
  printRegionModelMain(con, info);
  printMode(con, mode);
  stepFrame();
}  

void printMenu() {
  printf("sysUpdater2 by paulguy\n" \
         "based heavily on SafeSysUpdater\n" \
         "by cpasjuste\n\n" \
         "Press (A) to verify CIAs\n" \
         "Press (X) to list installed titles\n" \
         "Press (Y) to install CIAs\n" \
         "Press (B) to exit\n" \
         "Press [SELECT] to change mode\n" \
         "Press [START] to try exploit.  This will be needed" \
         "to actually install anything.\n" \
         "Press [L) to transfer 00040138{0,2}0000002 to\n" \
         "NATIVE_FIRM.  Needed for if an update including\n" \
         "one of those failed partway through.  Avoids a\n" \
         "frankenfirmware.  Will probably crash after but\n" \
         "it should be OK.\n\n"
         "CIAs will be installed from SDMC:" CIAS_PATH "\n\n");
}

void printRegionModelMain(PrintConsole *con, SysInfo *info) {
  int oldx, oldy;
  
  oldx = con->cursorX;
  oldy = con->cursorY;
  con->cursorX = 0;
  con->cursorY = con->consoleHeight - 1;
  
  printRegionModel(info);
  
  con->cursorX = oldx;
  con->cursorY = oldy;
}

void printMode(PrintConsole *con, InstallMode mode) {
  int oldfg, oldbg;
  int oldx, oldy;
  oldfg = con->fg;
  oldbg = con->bg;
  oldx = con->cursorX;
  oldy = con->cursorY;
  
  con->cursorX = con->consoleWidth - MODE_TEXT_LEN;
  con->cursorY = 0;
  
  if(mode > 0) {
    con->fg == 15;
    con->bg == 12;
  }
    
  printf("%s", installModeNames[mode]);
  
  con->cursorX = oldx;
  con->cursorY = oldy;
  con->fg = oldfg;
  con->bg = oldbg;
}

PrintConsole *initializeDisplay() {
  u8 *screen;
  u16 width, height;
  
  if(R_FAILED(hidInit())) {
    return(NULL);
  }
  gfxInitDefault();
  
  gfxSetScreenFormat(GFX_BOTTOM, GSP_RGB565_OES);
  screen = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &width, &height);
  memcpy(screen, logo_data, logo_data_size < width * height * 2 ? logo_data_size : width * height * 2);
  gfxSwapBuffers();  // copy in to both buffers
  screen = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &width, &height);
  memcpy(screen, logo_data, logo_data_size < width * height * 2 ? logo_data_size : width * height * 2);
  
  return(consoleInit(GFX_TOP, NULL));
}

void uninitializeDisplay() {
  gfxExit();
  hidExit();
}

MenuAction getActionFromKeys(u32 keys) {
  switch(keys) {
    case KEY_B:
      return(SU2_ACT_EXIT);
    case KEY_A:
      return(SU2_ACT_CHECK);
    case KEY_X:
      return(SU2_ACT_LIST);
    case KEY_Y:
      return(SU2_ACT_INSTALL);
    case KEY_SELECT:
      return(SU2_ACT_MODE);
    case KEY_START:
      return(SU2_ACT_EXPLOIT);
    case KEY_L:
      return(SU2_ACT_FIRMTRANSFER);
    default:
      return(SU2_ACT_NONE);
  }
  
  return(SU2_ACT_NONE);
}

void listInstalledTitles(PrintConsole *con) {
  TitleList *installedTitles;
  LOG_VERBOSE("listInstalledTitles");
  
  if(R_FAILED(amInit())) {
    printf("Couldn't initialize AM.\n");
    LOG_ERROR("Couldn't initialize AM.");
    return;
  }
  installedTitles = getInstalledTitles();
  amExit();
  if(installedTitles == NULL) {
    printf("Failed to get installed titles!\n");
    return;
  }
  
  printTitlesMore(installedTitles, con);

  freeTitleList(installedTitles);
}

int installCIAs(PrintConsole *con, InstallMode mode) {
  TitleList *installedTitles;
  TitleList *titlesToInstall;
  TitleList *skipped;
  int proceed;
  int giveUp;
  int response;
  char ciaPath[9 + 16 + 4 + 1]; // /updates/ + 16 hex digits + .cia + \0
  int i, j;

  LOG_INFO("Installing CIAs.");
  
  printf("Your 3DS shouldn't be bricked by failures until\n" \
         "you're told it can, but there's always a risk!\n\n");
  
  printf("Getting installed titles...\n");
  installedTitles = getInstalledTitles();
  if(installedTitles == NULL) {
    LOG_ERROR("Couldn't get installed titles.");
    printf("Failed to get list of installed titles.\n");
    return(-1);
  }
  
  printf("Gathering CIAs, this may take several minutes...\n");
  stepFrame();
  titlesToInstall = getUpdateCIAs();
  if(titlesToInstall == NULL) {
    printf("Failed to get list of CIAs from SD card.\n");
    LOG_ERROR("Failed to get list of CIAs from SD card.");
    freeTitleList(installedTitles);
    return(-1);
  }
  if(titlesToInstall->nTitles == 0) {
    printf("There are no CIAs found.\nThere is nothing to do.\n");
    LOG_VERBOSE("No CIAs on SD card.");
    freeTitleList(installedTitles);
    freeTitleList(titlesToInstall);
    return(-1);
  }

  printf("Determining which to install...\n");
  skipped = initTitleList(titlesToInstall->nTitles, 1);
  for(i = 0; i < titlesToInstall->nTitles; i++) {
    // Find title in installedTitles
    switch(mode) {
      case MODE_NEVER: // always skip
        skipped->title[i] = titlesToInstall->title[i];
        titlesToInstall->title[i] = NULL;
        break;
      case MODE_ALWAYS: // always install, don't do anything
        skipped->title[i] = NULL;
        break;
      case MODE_DOWNGRADE: // only install if the version to install is older
        for(j = 0; j < installedTitles->nTitles; j++) {
          if(titlesToInstall->title[i]->titleID == installedTitles->title[j]->titleID) {
            // Skip if newer or equal
            if(titlesToInstall->title[i]->version >= installedTitles->title[j]->version) {
              skipped->title[i] = titlesToInstall->title[i];
              titlesToInstall->title[i] = NULL;
              break;
            } else {
              skipped->title[i] = NULL;
            }
          }
        }
        break;
      case MODE_UPGRADE: // only install if the version to install is newer
        for(j = 0; j < installedTitles->nTitles; j++) {
          if(titlesToInstall->title[i]->titleID == installedTitles->title[j]->titleID) {
            // Skip if newer or equal
            if(titlesToInstall->title[i]->version <= installedTitles->title[j]->version) {
              skipped->title[i] = titlesToInstall->title[i];
              titlesToInstall->title[i] = NULL;
              break;
            } else {
              skipped->title[i] = NULL;
            }
          }
        }
        break;
    }
  }

  // sort titles and move NULLs to the end then pretend they don't exist
  rearrangeTitles(titlesToInstall);
  for(i = 0; i < titlesToInstall->nTitles; i++) {
    if(titlesToInstall->title[i] == NULL) {
      titlesToInstall->nTitles = i;
      break;
    }
  }
  rearrangeTitles(skipped);
  for(i = 0; i < skipped->nTitles; i++) {
    if(skipped->title[i] == NULL) {
      skipped->nTitles = i;
      break;
    }
  }
  
  if(titlesToInstall->nTitles == 0) {
    printf("There are no CIAs to install based on mode.\nThere is nothing to do.\n");
    LOG_VERBOSE("No CIAs meet mode criteria.");
    freeTitleList(installedTitles);
    freeTitleList(titlesToInstall);
    freeTitleList(skipped);
    return(0);
  }
  
  LOG_VERBOSE("Titles to install");
  for(i = 0; i < titlesToInstall->nTitles; i++) {
    LOG_VERBOSE("%016llX %04X", titlesToInstall->title[i]->titleID,
                                titlesToInstall->title[i]->version);
  }
  LOG_VERBOSE("Titles skipped");
  for(i = 0; i < skipped->nTitles; i++) {
    LOG_VERBOSE("%016llX %04X", skipped->title[i]->titleID,
                                skipped->title[i]->version);
  }
  
  //Print packages to be installed
  printf("These are the titles which WILL be installed.\n");
  printf("Press any key to continue . . .");
  stepFrame();
  waitKey();
  printTitlesMore(titlesToInstall, con);
  printf("Press any key to continue . . .");
  stepFrame();
  waitKey();
  printf("\n");
  
  //Print packages that will be skipped
  if(skipped->nTitles > 0) {
    printf("These are the titles which will NOT be installed.\n");
    printf("Press any key to continue . . .");
    stepFrame();
    waitKey();
    printTitlesMore(skipped, con);
    printf("Press any key to continue . . .");
    stepFrame();
    waitKey();
    printf("\n");
  }

  printf("##################################################");
  printf("A failure past this point may brick your 3DS!\nContinue? ");
  stepFrame();
  response = yesNoCancel();
  if(response != 1) {
    freeTitleList(installedTitles);
    freeTitleList(titlesToInstall);
    freeTitleList(skipped);
    return(-1);
  }
  printf("\n");
  
  giveUp = 0;
  for(i = 0; i < titlesToInstall->nTitles; i++) {
    LOG_VERBOSE("Installing %016llX", titlesToInstall->title[i]->titleID);
    snprintf(ciaPath, 9 + 16 + 4 + 1, CIAS_PATH "%016llX.cia",
             titlesToInstall->title[i]->titleID);
    printf("Installing %s... ", ciaPath);
    stepFrame();
    proceed = 1;
    for(j = 0; j < installedTitles->nTitles; j++) {
      if(titlesToInstall->title[i]->titleID == installedTitles->title[j]->titleID) {      
        LOG_VERBOSE("Deleting already installed %016llX.",
                    installedTitles->title[j]->titleID);
        while(proceed == 1) {
          if(deleteTitle(titlesToInstall->title[i]->titleID) < 0) {
            LOG_ERROR("Failed deleting %016llX.", installedTitles->title[i]->titleID);
            printf("Failed to delete title %016llX, retry?\n",
                   titlesToInstall->title[i]->titleID);
            response = yesNoCancel();
            if(response == 1) {
              continue;
            } else if(response == 0) {
              proceed = 0;
              break;
            } else if(response == -1) {
              proceed = 0;
              giveUp = 1;
              break;
            }
          } else {
            break;
          }
        }
        break;
      }
    }
    while(proceed == 1) {
      LOG_VERBOSE("Installing title %016llX.", titlesToInstall->title[i]->titleID);
      if(installTitleFromCIA(ciaPath, con) < 0) {
        LOG_ERROR("Failed installing %016llX.", titlesToInstall->title[i]->titleID);
        printf(" \nInstall failed, retry?\n");
        response = yesNoCancel();
        printf("\n");
        if(response == 1) {
          continue;
        } else if(response == 0) {
          proceed = 0;
          break;
        } else if(response == -1) {
          proceed = 0;
          giveUp = 1;
          break;
        }
      } else {
        break;
      }
    }
    
    if(giveUp == 1) {
      LOG_ERROR("Given up.");
      printf("Giving up.\n");
      stepFrame();
      break;
    }
    
    LOG_INFO("Installing %016llX succeeded.", titlesToInstall->title[i]->titleID);
    printf(" \nSuccess!\n");
    stepFrame();
  }

  // This seems to actually install to the real NAND0/NAND1 through rxTools...
  // Also generally causes a crash a few seconds later, so do this last.
  for(i = 0; i < titlesToInstall->nTitles; i++) {
    if(titlesToInstall->title[i]->titleID == 0x0004013800000002 ||
       titlesToInstall->title[i]->titleID == 0x0004013820000002) {
      LOG_INFO("Installing firmware to NATIVE_FIRM.");
      printf(" \nInstalling NATIVE_FIRM...");
#ifdef ARMED
      while(proceed == 1) {
        // AM_InstallNativeFirm() does not work.
        if(R_FAILED(AM_InstallFirm(titlesToInstall->title[i]->titleID))) {
          LOG_ERROR("Installing firmware to NATIVE_FIRM failed.");
          printf("\nInstalling NATIVE_FIRM failed, retry?\n");
          response = yesNoCancel();
          printf("\n");
          if(response == 1) {
            continue;
          } else if(response == 0) {
            proceed = 0;
            break;
          } else if(response == -1) {
            proceed = 0;
            giveUp = 1;
            break;
          }
        } else {
          break;
        }
      }
#endif
    }
  }

  freeTitleList(installedTitles);
  freeTitleList(titlesToInstall);
  freeTitleList(skipped);
  
  if(giveUp == 1) {
    return(-1);
  }
  
  LOG_INFO("Installing CIAs succeeded.");
  return(0);
}

int testAMuAccess() {
    Handle amHandle = 0;

    // verify am:u access
    srvGetServiceHandleDirect(&amHandle, "am:u");
    if (amHandle) {
        svcCloseHandle(amHandle);
        return(1);
    }
    
    return(0);
}
