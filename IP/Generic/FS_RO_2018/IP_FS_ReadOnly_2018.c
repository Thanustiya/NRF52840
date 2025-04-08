/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2007 - 2024    SEGGER Microcontroller GmbH               *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emNet * TCP/IP stack for embedded applications               *
*                                                                    *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product for in-house use.         *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       emNet version: V3.60.0                                       *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

Purpose : Implementation of read only file system layer.
*/

#include <stdio.h>   // For NULL.
#include <string.h>  // For memcpy(), strcmp().

#include "IP_FS.h"
#include "WEBS_Conf.h"

#ifndef   WEBS_USE_SAMPLE_2018
  #define WEBS_USE_SAMPLE_2018  0
#endif

#if (WEBS_USE_SAMPLE_2018 != 0)

/*********************************************************************
*
*       Includes for read only files
*
**********************************************************************
*/

#include "index_2018.h"
#include "Error404_2018.h"
#include "Samples_2018.h"
#include "Shares_2018.h"
#include "OSInfo_2018.h"
#include "FormGET_2018.h"
#include "FormPOST_2018.h"
#include "IPConfig_2018.h"
#include "VirtFile_2018.h"
#include "Authen_2018.h"
#include "Custom_2018.h"
#include "bootstrap_min_js_2018.h"
#include "jquery_min_js_2018.h"
#include "tether_min_js_2018.h"
#include "bootstrap_min_css_2018.h"
#include "emWeb_Logo_2018.h"
#include "Logo_2018.h"
#include "favicon_2018.h"
#include "events_2018.h"
#include "RGraphCC_2018.h"
#include "RGraphCE_2018.h"
#include "RGraphLi_2018.h"
#include "GreenRUp_2018.h"
#include "RedRDown_2018.h"
#include "WhiteR_2018.h"

#if INCLUDE_PRESENTATION_SAMPLE
#include "Products_2018.h"
#include "Empty_2018.h"
//#include "embOS_Icon_2018.h"
#include "embOSMPU_Icon_2018.h"
#include "emNet_Icon_2018.h"
//#include "emCompress_Icon_2018.h"
//#include "emCrypt_Icon_2018.h"
#include "emFile_Icon_2018.h"
//#include "emLib_Icon_2018.h"
#include "emLoad_Icon_2018.h"
//#include "emModbus_Icon_2018.h"
//#include "emSecure_Icon_2018.h"
#include "emSSH_Icon_2018.h"
#include "emSSL_Icon_2018.h"
#include "emUSBD_Icon_2018.h"
#include "emUSBH_Icon_2018.h"
#include "emWin_Icon_2018.h"
#endif

/*********************************************************************
*
*       typedefs
*
**********************************************************************
*/

typedef struct {
  const IP_FS_READ_ONLY_FILE_ENTRY* pEntry;
  unsigned                          DirLength;
  unsigned                          LastFolderLength;
  const IP_FS_READ_ONLY_FILE_ENTRY* pPrevious;
} IP_FS_READ_ONLY_CONTEXT;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static IP_FS_READ_ONLY_FILE_HOOK* _pFirstFileHook;

//
// List of static read only files.
// For FTP server, all the entries under the same folder need to be regrouped.
//
static const IP_FS_READ_ONLY_FILE_ENTRY _aFile[] = {
//  path                               file data                               file size
//  -------------------                --------------                          -------------
  { "/index.htm",                      index_2018_file,                        INDEX_2018_SIZE                  },
  { "/Error404.htm",                   error404_2018_file,                     ERROR404_2018_SIZE               },
  { "/Samples.htm",                    samples_2018_file,                      SAMPLES_2018_SIZE                },
  { "/Shares.htm",                     shares_2018_file,                       SHARES_2018_SIZE                 },
  { "/OSInfo.htm",                     osinfo_2018_file,                       OSINFO_2018_SIZE                 },
  { "/FormGET.htm",                    formget_2018_file,                      FORMGET_2018_SIZE                },
  { "/FormPOST.htm",                   formpost_2018_file,                     FORMPOST_2018_SIZE               },
  { "/IPConfig.htm",                   ipconfig_2018_file,                     IPCONFIG_2018_SIZE               },
  { "/VirtFile.htm",                   virtfile_2018_file,                     VIRTFILE_2018_SIZE               },
  { "/conf/Authen.htm",                authen_2018_file,                       AUTHEN_2018_SIZE                 },
  { "/Custom.css.gz",                  custom_2018_file,                       CUSTOM_2018_SIZE                 },
  { "/emWeb_Logo.png",                 emweb_logo_2018_file,                   EMWEB_LOGO_2018_SIZE             },
  { "/Logo.gif",                       logo_2018_file,                         LOGO_2018_SIZE                   },
  { "/favicon.ico",                    favicon_2018_file,                      FAVICON_2018_SIZE                },
  { "/css/bootstrap_min_css.css.gz",   bootstrap_min_css_2018_file,            BOOTSTRAP_MIN_CSS_2018_SIZE      },
  { "/js/bootstrap_min_js.js.gz",      bootstrap_min_js_2018_file,             BOOTSTRAP_MIN_JS_2018_SIZE       },
  { "/js/jquery_min_js.js.gz",         jquery_min_js_2018_file,                JQUERY_MIN_JS_2018_SIZE          },
  { "/js/tether_min_js.js.gz",         tether_min_js_2018_file,                TETHER_MIN_JS_2018_SIZE          },
  { "/js/events.js.gz",                events_2018_file,                       EVENTS_2018_SIZE                 },
#if INCLUDE_SHARE_SAMPLE
  { "/GreenRUp.gif",                   greenrup_2018_file,                     GREENRUP_2018_SIZE               },
  { "/RedRDown.gif",                   redrdown_2018_file,                     REDRDOWN_2018_SIZE               },
  { "/WhiteR.gif",                     whiter_2018_file,                       WHITER_2018_SIZE                 },
  { "/js/RGraphCC.js.gz",              rgraphcc_2018_file,                     RGRAPHCC_2018_SIZE               },
  { "/js/RGraphCE.js.gz",              rgraphce_2018_file,                     RGRAPHCE_2018_SIZE               },
  { "/js/RGraphLi.js.gz",              rgraphli_2018_file,                     RGRAPHLI_2018_SIZE               },
#endif
#if INCLUDE_PRESENTATION_SAMPLE
  { "/Products.htm",                   products_2018_file,                     PRODUCTS_2018_SIZE               },
  { "/Empty.gif",                      empty_2018_file,                        EMPTY_2018_SIZE                  },
  { "/embOSMPU_Icon.gif",              embosmpu_icon_2018_file,                EMBOSMPU_ICON_2018_SIZE          },
  { "/emFile_Icon.gif",                emfile_icon_2018_file,                  EMFILE_ICON_2018_SIZE            },
  { "/emLoad_Icon.gif",                emload_icon_2018_file,                  EMLOAD_ICON_2018_SIZE            },
  { "/emNet_Icon.gif",                 emnet_icon_2018_file,                   EMNET_ICON_2018_SIZE             },
  { "/emSSH_Icon.gif",                 emssh_icon_2018_file,                   EMSSH_ICON_2018_SIZE             },
  { "/emSSL_Icon.gif",                 emssl_icon_2018_file,                   EMSSL_ICON_2018_SIZE             },
  { "/emUSBD_Icon.gif",                emusbd_icon_2018_file,                  EMUSBD_ICON_2018_SIZE            },
  { "/emUSBH_Icon.gif",                emusbh_icon_2018_file,                  EMUSBH_ICON_2018_SIZE            },
  { "/emWin_Icon.gif",                 emwin_icon_2018_file,                   EMWIN_ICON_2018_SIZE             },
#endif
  { 0,                                 0,                                      0                                }
};

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _CompareDir()
*/
static int _CompareDir(const char* sDir, const char* sPath) {
  char c0;
  char c1;

  for (;;) {
    c0 = *sDir++;
    if (c0 == 0) {
      break;
    }
    c1 = *sPath++;
    if (c0 != c1) {
      return 1;  // No match, file not in this directory.
    }
  }
  return 0;      // Match.
}

/*********************************************************************
*
*       _GetFolderLength()
*/
static int _GetFolderLength(const char* sPath) {
  const char     *pEnd;
        unsigned  r;

  pEnd = strstr(sPath, "/");
  if (pEnd == NULL) {
    r = 0u;
  } else {
    r = (unsigned)(pEnd - sPath);
  }
  return r;
}

/*********************************************************************
*
*       _FS_RO_FS_Open()
*/
static void* _FS_RO_FS_Open(const char* sPath) {
  const IP_FS_READ_ONLY_FILE_ENTRY* pEntry;
        IP_FS_READ_ONLY_FILE_HOOK*  pHook;

  //
  // Some applications/samples have been designed with a real
  // filesystem in mind which does not use a root slash "/"
  // at the beginning to describe that the file to open is actually
  // in the root of the medium. The FS/RO layer however has been
  // designed with URIs in mind, which typically start with it.
  // To unify both versions we now skip the root slash as everything
  // this FS/RO layer uses a path relative to root. This then works
  // with paths with and without root slash as both do the same,
  // access a path starting at root, whether this is explicitly
  // stated or not.
  //
  if (*sPath == '/') {
    sPath++;
  }
  //
  // Use dynamically added files list first.
  //
  for (pHook = _pFirstFileHook; pHook != NULL; pHook = pHook->pNext) {
    pEntry = &pHook->FileEntry;
    if (strcmp(sPath, pEntry->sPath + 1) == 0) {
      return (void*)pEntry;
    }
  }
  //
  // Use fixed list.
  //
  pEntry = &_aFile[0];
  for (;;) {
    if (pEntry->sPath == NULL) {
      break;
    }
    if (strcmp(sPath, pEntry->sPath + 1) == 0) {
      return (void*)pEntry;
    }
    pEntry++;
  }
  return NULL;
}

/*********************************************************************
*
*       _FS_RO_Close()
*/
static int _FS_RO_Close (void* hFile) {
  (void)hFile;

  return 0;
}

/*********************************************************************
*
*       _FS_RO_ReadAt()
*/
static int _FS_RO_ReadAt(void* hFile, void* pDest, U32 Pos, U32 NumBytes) {
  IP_FS_READ_ONLY_FILE_ENTRY* pEntry;

  pEntry = (IP_FS_READ_ONLY_FILE_ENTRY*)hFile;
  memcpy(pDest, pEntry->pData + Pos, NumBytes);
  return NumBytes;
}

/*********************************************************************
*
*       _FS_RO_GetLen()
*/
static long _FS_RO_GetLen(void* hFile) {
  IP_FS_READ_ONLY_FILE_ENTRY* pEntry;

  pEntry = (IP_FS_READ_ONLY_FILE_ENTRY*)hFile;
  return pEntry->FileSize;
}

/*********************************************************************
*
*       _FS_RO_ForEachDirEntry()
*/
static int _FS_RO_ForEachDirEntry(void* pContext, const char* sDir, void (*pf)(void* pContext, void* pFileEntry)) {
  IP_FS_READ_ONLY_CONTEXT     Context;
  IP_FS_READ_ONLY_FILE_ENTRY* pEntry;
  IP_FS_READ_ONLY_FILE_HOOK*  pHook;
  int i;

  Context.DirLength        = (unsigned)strlen(sDir);
  Context.LastFolderLength = 0u;
  Context.pPrevious        = NULL;
  //
  // Use dynamically added files list first.
  //
  for (pHook = _pFirstFileHook; pHook != NULL; pHook = pHook->pNext) {
    pEntry = &pHook->FileEntry;
    if (_CompareDir(sDir, pEntry->sPath) == 0) {
      //
      // Check if this entry corresponds to the folder previously reported.
      //
      if (Context.LastFolderLength != 0u) {
        if (memcmp(Context.pPrevious->sPath, pEntry->sPath, Context.LastFolderLength) == 0) {
          continue;
        }
      }
      Context.pEntry = pEntry;
      pf(pContext, (void*)&Context);
    }
  }
  //
  // Use fixed list.
  // Might report a filename for the second time as
  // we do not check if a filename has been overwritten
  // using the dynamic file list.
  //
  i = 0;
  while (1) {
    if (_aFile[i].sPath == NULL) {
      break;
    }
    if (_CompareDir(sDir, _aFile[i].sPath) == 0) {
      //
      // Check if this entry corresponds to the folder previously reported.
      //
      if (Context.LastFolderLength != 0u) {
        if (memcmp(Context.pPrevious->sPath, _aFile[i].sPath, Context.LastFolderLength) == 0) {
          i++;
          continue;
        }
      }
      Context.pEntry = &_aFile[i];
      pf(pContext, (void*)&Context);
    }
    i++;
  }
  return 0;
}

/*********************************************************************
*
*       _FS_RO_GetFileName()
*/
static void _FS_RO_GetFileName(void* pFileEntry, char* pBuffer, U32 BufferSize) {
        IP_FS_READ_ONLY_CONTEXT*    pContext;
  const IP_FS_READ_ONLY_FILE_ENTRY* pEntry;
  const char* pStart;
  unsigned FilenameLen;
  unsigned n;

  BufferSize--;                             // Reserve one byte for string termination.
  pContext    = (IP_FS_READ_ONLY_CONTEXT*)pFileEntry;
  pEntry      = pContext->pEntry;
  n           = 0u;
  //
  // Check the presence of a folder.
  //
  pStart = pEntry->sPath + pContext->DirLength;
  if (*pStart == '/') {
    pStart++;
    n++;
  }
  FilenameLen = _GetFolderLength(pStart);
  if (FilenameLen == 0u) {
    FilenameLen = (unsigned)strlen(pStart);
  } else {
    pContext->LastFolderLength = pContext->DirLength + n + FilenameLen + 1u; // Add 1 to add the '/' at the end.
    pContext->pPrevious        = pEntry;
  }
  FilenameLen = SEGGER_MIN(FilenameLen, BufferSize);
  memcpy(pBuffer, pStart, FilenameLen);
  *(pBuffer + FilenameLen) = 0;             // Terminate string.
}

/*********************************************************************
*
*       _FS_RO_GetFileSize()
*/
static U32 _FS_RO_GetFileSize(void * pFileEntry, U32 * pFileSizeHigh) {
  const IP_FS_READ_ONLY_CONTEXT*    pContext;
  const IP_FS_READ_ONLY_FILE_ENTRY* pEntry;
  const char* pStart;
  unsigned DirLen;
  U32 r;

  (void)pFileSizeHigh;

  pContext    = (IP_FS_READ_ONLY_CONTEXT*)pFileEntry;
  pEntry      = pContext->pEntry;
  r           = 0uL;  // folder, indicate no length.
  //
  // Check the presence of a folder.
  //
  pStart = pEntry->sPath + pContext->DirLength;
  if (*pStart == '/') {
    pStart++;
  }
  DirLen = _GetFolderLength(pStart);
  if (DirLen == 0u) {
    r = (U32)pEntry->FileSize;
  }
  return r;
}

/*********************************************************************
*
*       _FS_RO_GetFileAttributes()
*/
static int _FS_RO_GetFileAttributes(void* pFileEntry) {
  const IP_FS_READ_ONLY_CONTEXT*    pContext;
  const IP_FS_READ_ONLY_FILE_ENTRY* pEntry;
  const char* pStart;
  unsigned FilenameLen;
  int r;

  pContext    = (IP_FS_READ_ONLY_CONTEXT*)pFileEntry;
  pEntry      = pContext->pEntry;
  r           = 0;
  //
  // Check the presence of a folder.
  //
  pStart = pEntry->sPath + pContext->DirLength;
  if (*pStart == '/') {
    pStart++;
  }
  FilenameLen = _GetFolderLength(pStart);
  if (FilenameLen != 0u) {
    r = 1;  // folder.
  }
  return r;
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       IP_FS_READ_ONLY_ClrFileHooks()
*
*  Function description
*    Clears all files that have been dynamically added.
*/
void IP_FS_READ_ONLY_ClrFileHooks(void) {
  _pFirstFileHook = NULL;
}

/*********************************************************************
*
*       IP_FS_READ_ONLY_AddFileHook()
*
*  Function description
*    Adds a file to the list of static read only files.
*
*  Parameters
*    pHook   : Management element of type IP_FS_READ_ONLY_FILE_HOOK.
*    sPath   : Path (filename) of the file to add.
*    pData   : Content of the file.
*    FileSize: Size of content.
*/
void IP_FS_READ_ONLY_AddFileHook(IP_FS_READ_ONLY_FILE_HOOK* pHook, const char* sPath, const unsigned char* pData, unsigned int FileSize) {
  IP_FS_READ_ONLY_FILE_ENTRY* pEntry;

  pHook->pNext     = _pFirstFileHook;
  pEntry           = &pHook->FileEntry;
  pEntry->sPath    = sPath;
  pEntry->pData    = pData;
  pEntry->FileSize = FileSize;
  _pFirstFileHook  = pHook;
}

/*********************************************************************
*
*       Public API structures
*
**********************************************************************
*/

const IP_FS_API IP_FS_ReadOnly = {
  _FS_RO_FS_Open,            // pfOpenFile
  _FS_RO_Close,              // pfCloseFile
  _FS_RO_ReadAt,             // pfReadAt
  _FS_RO_GetLen,             // pfGetLen
  NULL,
  _FS_RO_GetFileName,        // pfGetDirEntryFileName
  _FS_RO_GetFileSize,        // pfGetDirEntryFileSize
  NULL,                      // pfGetDirEntryFileTime
  _FS_RO_GetFileAttributes,  // pfGetDirEntryAttributes
  NULL,                      // pfCreate
  NULL,                      // pfDeleteFile
  NULL,                      // pfRenameFile
  NULL,                      // pfWriteAt
  NULL,                      // pfMKDir
  NULL,                      // pfRMDir
  NULL,                      // pfIsFolder
  NULL,                      // pfMove
  _FS_RO_ForEachDirEntry     // pfForEachDirEntry
};

#endif

/*************************** End of file ****************************/
