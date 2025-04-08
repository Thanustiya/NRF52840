/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*       (c) 2014 - 2024    SEGGER Microcontroller GmbH               *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emSSH * Embedded Secure Shell                                *
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
*       emSSH version: V2.56.0                                       *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : SSH_SCP_SINK_FS_FS.c
Purpose     : Adaptation of SSH SCP file system API to emFile.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSH.h"
#include "FS.h"
#include <stdlib.h>

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define ISLEAP(y)       ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)
#define EPOCH_YEAR      1970
#define SECSPERMIN      60L
#define MINSPERHOUR     60L
#define HOURSPERDAY     24L
#define SECSPERHOUR     (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY      (SECSPERHOUR * HOURSPERDAY)

/*********************************************************************
*
*       Local data types
*
**********************************************************************
*/

typedef struct {
  FS_FILE * pFile;
  char      acPath[SSH_SCP_CONFIG_PATH_MAX];
} SSH_SCP_FS_FS_CONTEXT;

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const U8 _SSH_SCP_SINK_FS_FS_aMonLengths[2][12] = {
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static const U16 _SSH_SCP_SINK_FS_FS_aYearLengths[2] = {
  365,
  366
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static struct {
  char                  acRoot  [SSH_SCP_CONFIG_PATH_MAX];
  SSH_SCP_FS_FS_CONTEXT aContext[SSH_SCP_CONFIG_MAX_SESSIONS];
} SSH_SCP_FS_FS_Globals;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_PathNorm()
*
*  Function description
*    Normalize path to emFile directory delimiters.
*
*  Parameters
*    sPath - Pointer to zero-terminated path string.
*/
static void _SSH_SCP_SINK_FS_FS_PathNorm(char *sPath) {
  while (*sPath) {
    if (*sPath == '/' || *sPath == '\\') {
      *sPath = FS_DIRECTORY_DELIMITER;
    }
    ++sPath;
  }
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_PathAddX()
*
*  Function description
*    Append path data to length-limited string.
*
*  Parameters
*    sName   - Pointer to zero-terminated string.
*    NameLen - Capacity of the zero-terminated string.
*    sText   - Pointer to text to append to zero-terminated string.
*    TextLen - Octet length of the text to append to zero-terminated string.
*
*  Return value
*    == 0                       - Appended successfully.
*    == SSH_ERROR_PATH_TOO_LONG - String overflow.
*/
static int _SSH_SCP_SINK_FS_FS_PathAddX(char *sName, unsigned NameLen, const char *sText, unsigned TextLen) {
  if (TextLen > 0 && sText[0] == '/' && sName[0] != 0 && sName[SSH_STRLEN(sName)-1] == '/') {
    ++sText;
    --TextLen;
  }
  if (SSH_STRLEN(sName) + TextLen + 1 >= NameLen) {
    return SSH_ERROR_PATH_TOO_LONG;
  }
  //
  sName = SSH_STRCHR(sName, '\0');
  SSH_MEMCPY(sName, sText, TextLen);
  sName[TextLen] = 0;
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_PathAdd()
*
*  Function description
*    Append path data to length-limited string.
*
*  Parameters
*    sName   - Pointer to zero-terminated string.
*    NameLen - Capacity of the zero-terminated string.
*    sText   - Zero-terminated text to append to zero-terminated string.
*
*  Return value
*    == 0                       - Appended successfully.
*    == SSH_ERROR_PATH_TOO_LONG - String overflow.
*/
static int _SSH_SCP_SINK_FS_FS_PathAdd(char *sName, unsigned NameLen, const char *sText) {
  return _SSH_SCP_SINK_FS_FS_PathAddX(sName, NameLen, sText, SSH_STRLEN(sText));
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_PathTrim()
*
*  Function description
*    Remove all characters from final '/' or '\' to end.
*
*  Parameters
*    sName - Pointer to zero-terminated string.
*/
static void _SSH_SCP_SINK_FS_FS_PathTrim(char *sName) {
  char *s;
  //
  s = SSH_STRRCHR(sName, '/');
  if (s == NULL) {
    s = SSH_STRRCHR(sName, '\\');
  }
  if (s != NULL && s != sName) {
    *s = 0;
  }
}


/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_Config()
*
*  Function description
*    Implementation of Config API call.
*
*  Parameters
*    sRoot - Pointer to zero-terminated virtual SCP root directory.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SINK_FS_FS_Config(const char *sRoot) {
  SSH_SCP_FS_FS_Globals.acRoot[0] = 0;
  return _SSH_SCP_SINK_FS_FS_PathAdd(SSH_SCP_FS_FS_Globals.acRoot,
                                     sizeof(SSH_SCP_FS_FS_Globals.acRoot),
                                     sRoot);
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_Init()
*
*  Function description
*    Implementation of Init API call.
*
*  Parameters
*    Index   - Zero-based SCP session index.
*    pPath   - Pointer to root-relative path string for this transfer.
*    PathLen - Octet length of root-relative path string for this transfer.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SINK_FS_FS_Init(unsigned Index, const char *sPath, unsigned PathLen) {
  SSH_SCP_FS_FS_CONTEXT * pSelf;
  int                     Status;
  //
  SSH_ASSERT(Index < SSH_SCP_CONFIG_MAX_SESSIONS);
  //
  pSelf = &SSH_SCP_FS_FS_Globals.aContext[Index];
  pSelf->pFile     = NULL;
  pSelf->acPath[0] = 0;
  //
  Status = _SSH_SCP_SINK_FS_FS_PathAdd(pSelf->acPath,
                                       sizeof(pSelf->acPath),
                                       SSH_SCP_FS_FS_Globals.acRoot);
  if (Status >= 0) {
    Status = _SSH_SCP_SINK_FS_FS_PathAddX(pSelf->acPath,
                                          sizeof(pSelf->acPath),
                                          sPath,
                                          PathLen);
    if (pSelf->acPath[0] == 0) {
      SSH_STRCPY(pSelf->acPath, "/");
    }
    _SSH_SCP_SINK_FS_FS_PathNorm(pSelf->acPath);
  }
  //
  return Status;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_CvtStamp()
*
*  Function description
*    Convert Unix time to emFile time.
*
*  Parameters
*    UnixTime - Unix format timestamp, number of seconds since
*               1 Jan 1970.
*
*  Return value
*    Timestamp in emFile format.
*/
static U32 _SSH_SCP_SINK_FS_FS_CvtStamp(U32 UnixTime) {
  int        IsLeap;
  const U8 * pMonthLen;
  unsigned   Year;
  unsigned   Day;
  unsigned   Month;
  unsigned   Hour;
  unsigned   Minute;
  unsigned   Second;
  //
  Day    = UnixTime / SECSPERDAY;   UnixTime %= SECSPERDAY;
  Hour   = UnixTime / SECSPERHOUR;  UnixTime %= SECSPERHOUR;
  Minute = UnixTime / SECSPERMIN;   UnixTime %= SECSPERMIN;
  Second = UnixTime;
  //
  Year = EPOCH_YEAR;
  for (;;) {
    IsLeap = ISLEAP(Year);
    if (Day < _SSH_SCP_SINK_FS_FS_aYearLengths[IsLeap]) {
      break;
    }
    Day -= _SSH_SCP_SINK_FS_FS_aYearLengths[IsLeap];
    ++Year;
  }
  //
  pMonthLen = _SSH_SCP_SINK_FS_FS_aMonLengths[IsLeap];
  for (Month = 0; Day >= pMonthLen[Month]; ++Month) {
    Day -= pMonthLen[Month];
  }
  //
  // Convert to conventional day-of-month and month-of-year.
  //
  ++Day;
  ++Month;
  //
  // Now pack components.
  //
  if (Year < 1980) {
    return 0;
  } else {
    return ((Second / 2)  <<  0) +
           (Minute        <<  5) +
           (Hour          << 11) +
           (Day           << 16) +
           (Month         << 21) +
           ((Year - 1980) << 25);
  }
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_CreateFile()
*
*  Function description
*    Implementation of Create File API call.
*
*  Parameters
*    Index   - Zero-based SCP session index.
*    Mode    - Unix file mode.
*    Len     - Length of file.
*    ModTime - Last modification time, Unix format, number of seconds
*              since 1 Jan 1970.  Zero indicates that time was not set.
*    AccTime - Last access time, Unix format, number of seconds since
*              1 Jan 1970.  Zero indicates that time was not set.
*    sName   - Zero-terminated full path name.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SINK_FS_FS_CreateFile(      unsigned Index,
                                                unsigned Mode,
                                                U32      Len,
                                                U32      ModTime,
                                                U32      AccTime,
                                          const char *   sName) {
  SSH_SCP_FS_FS_CONTEXT * pSelf;
  int                     Status;
  //
  SSH_ASSERT(Index < SSH_SCP_CONFIG_MAX_SESSIONS);
  //
  pSelf = &SSH_SCP_FS_FS_Globals.aContext[Index];
  //
  Status = _SSH_SCP_SINK_FS_FS_PathAdd(pSelf->acPath, sizeof(pSelf->acPath), "/");
  if (Status >= 0) {
    Status = _SSH_SCP_SINK_FS_FS_PathAdd(pSelf->acPath, sizeof(pSelf->acPath), sName);
    if (Status >= 0) {
      //
      // Create file.
      //
      _SSH_SCP_SINK_FS_FS_PathNorm(pSelf->acPath);
      Status = FS_FOpenEx(pSelf->acPath, "wb", &pSelf->pFile);
      if (Status >= 0) {
        //
        // Preallocate file data.
        //
        FS_FSeek(pSelf->pFile, Len, FS_SEEK_SET);
        Status = FS_SetEndOfFile(pSelf->pFile);
        FS_FSeek(pSelf->pFile, 0, FS_SEEK_SET);
        //
        // If no Posix owner write permission, make read only.
        //
        if ((Mode & 0200) == 0) {
          (void)FS_ModifyFileAttributes(pSelf->acPath, FS_ATTR_READ_ONLY, 0);
        }
        //
        // If timestamps given, set them.
        //
        if (ModTime != 0) {
          FS_SetFileTimeEx(pSelf->acPath, _SSH_SCP_SINK_FS_FS_CvtStamp(ModTime), FS_FILETIME_CREATE);
          FS_SetFileTimeEx(pSelf->acPath, _SSH_SCP_SINK_FS_FS_CvtStamp(ModTime), FS_FILETIME_MODIFY);
        }
        if (AccTime != 0) {
          FS_SetFileTimeEx(pSelf->acPath, _SSH_SCP_SINK_FS_FS_CvtStamp(AccTime), FS_FILETIME_ACCESS);
        }
      }
      //
      _SSH_SCP_SINK_FS_FS_PathTrim(pSelf->acPath);
    }
  }
  //
  return Status;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_WriteFile()
*
*  Function description
*    Implementation of Write File API call.
*
*  Parameters
*    Index   - Zero-based SCP session index.
*    pData   - Pointer to object to write.
*    DataLen - Octet length of the object to write.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SINK_FS_FS_WriteFile(unsigned Index, const U8 *pData, unsigned DataLen) {
  SSH_SCP_FS_FS_CONTEXT * pSelf;
  //
  pSelf = &SSH_SCP_FS_FS_Globals.aContext[Index];
  //
  return FS_FWrite(pData, 1, DataLen, pSelf->pFile);
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_CloseFile()
*
*  Function description
*    Implementation of Close File API call.
*
*  Parameters
*    Index - Zero-based SCP session index.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SINK_FS_FS_CloseFile(unsigned Index) {
  SSH_SCP_FS_FS_CONTEXT * pSelf;
  int                     Status;
  //
  SSH_ASSERT(Index < SSH_SCP_CONFIG_MAX_SESSIONS);
  //
  pSelf = &SSH_SCP_FS_FS_Globals.aContext[Index];
  if (pSelf->pFile != NULL) {
    Status = FS_FClose(pSelf->pFile);
    pSelf->pFile = NULL;
  } else {
    Status = 0;
  }
  //
  return Status;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_EnterFolder()
*
*  Function description
*    Implementation of Enter Folder API call.
*
*  Parameters
*    Index   - Zero-based SCP session index.
*    Mode    - Folder mode.
*    ModTime - Last modification time, Unix format, number of seconds
*              since 1 Jan 1970.  Zero indicates that time was not set.
*    AccTime - Last access time, Unix format, number of seconds since
*              1 Jan 1970.  Zero indicates that time was not set.
*    sName   - Zero-terminated folder name.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SINK_FS_FS_EnterFolder(      unsigned   Index,
                                                 unsigned   Mode,
                                                 U32        ModTime,
                                                 U32        AccTime,
                                           const char     * sName) {
  SSH_SCP_FS_FS_CONTEXT * pSelf;
  int                     Status;
  //
  SSH_USE_PARA(Mode);
  SSH_ASSERT(Index < SSH_SCP_CONFIG_MAX_SESSIONS);
  //
  pSelf = &SSH_SCP_FS_FS_Globals.aContext[Index];
  Status = _SSH_SCP_SINK_FS_FS_PathAdd(pSelf->acPath, sizeof(pSelf->acPath), "/");
  if (Status >= 0) {
    Status = _SSH_SCP_SINK_FS_FS_PathAdd(pSelf->acPath, sizeof(pSelf->acPath), sName);
    if (Status >= 0) {
      _SSH_SCP_SINK_FS_FS_PathNorm(pSelf->acPath);
      Status = FS_CreateDir(pSelf->acPath);
      if (Status == 1) {
        Status = 0;
        //
        // If timestamps given, set them.
        //
        if (ModTime != 0) {
          FS_SetFileTimeEx(pSelf->acPath, _SSH_SCP_SINK_FS_FS_CvtStamp(ModTime), FS_FILETIME_CREATE);
          FS_SetFileTimeEx(pSelf->acPath, _SSH_SCP_SINK_FS_FS_CvtStamp(ModTime), FS_FILETIME_MODIFY);
        }
        if (AccTime != 0) {
          FS_SetFileTimeEx(pSelf->acPath, _SSH_SCP_SINK_FS_FS_CvtStamp(AccTime), FS_FILETIME_ACCESS);
        }
      }
    }
  }
  //
  return Status;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_ExitFolder()
*
*  Function description
*    Implementation of Exit Folder API call.
*
*  Parameters
*    Index - Zero-based SCP session index.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SINK_FS_FS_ExitFolder(unsigned Index) {
  SSH_ASSERT(Index < SSH_SCP_CONFIG_MAX_SESSIONS);
  //
  _SSH_SCP_SINK_FS_FS_PathTrim(SSH_SCP_FS_FS_Globals.aContext[Index].acPath);
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_FS_DecodeStatus()
*
*  Function description
*    Decode error status.
*
*  Parameters
*    Status - Error status.
*
*  Return value
*    Pointer to zero-terminate string if error, or NULL if success.
*/
static const char * _SSH_SCP_SINK_FS_FS_DecodeStatus(int Status) {
  return Status == 0 ? 0 : FS_ErrorNo2Text(Status);
}

/*********************************************************************
*
*       Public data
*
**********************************************************************
*/

const SSH_SCP_SINK_FS_API SSH_SCP_SINK_FS_FS = {
  _SSH_SCP_SINK_FS_FS_Config,
  _SSH_SCP_SINK_FS_FS_Init,
  _SSH_SCP_SINK_FS_FS_CreateFile,
  _SSH_SCP_SINK_FS_FS_WriteFile,
  _SSH_SCP_SINK_FS_FS_CloseFile,
  _SSH_SCP_SINK_FS_FS_EnterFolder,
  _SSH_SCP_SINK_FS_FS_ExitFolder,
  _SSH_SCP_SINK_FS_FS_DecodeStatus,
};

/*************************** End of file ****************************/
