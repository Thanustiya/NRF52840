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

File        : SSH_SCP_SOURCE_FS_FS.c
Purpose     : SSH-SCP source file system access for emFile.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSH.h"
#include "FS.h"

/*********************************************************************
*
*       Local types
*
**********************************************************************
*/

typedef struct {
  FS_FILE      * pFile;
  FS_FIND_DATA   FindData;
  int            FindActive;
  char           aPath[SSH_SCP_CONFIG_PATH_MAX];      // Folder to search.
  const char   * sPattern;                            // Pattern to match
  char           aFileName[SSH_SCP_CONFIG_PATH_MAX];  // Found file name.
} SSH_SCP_SOURCE_FS_FS_CONTEXT;

typedef struct {
  SSH_SCP_SOURCE_FS_FS_CONTEXT aContext [SSH_SCP_CONFIG_MAX_SESSIONS];
  char                         aRootPath[SSH_SCP_CONFIG_PATH_MAX];
} SSH_SCP_SOURCE_GLOBALS;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static SSH_SCP_SOURCE_GLOBALS _SSH_SCP_SOURCE_FS_FS_Globals;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _Upper()
*
*  Function description
*    Convert character to uppercase.
*
*  Parameters
*    Ch - Character to convert.
*
*  Return value
*    uppercased character.
*/
static int _Upper(int Ch) {
  if ('a' <= Ch && Ch <= 'z') {
    Ch = Ch - 'a' + 'A';
  }
  return Ch;
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_Match()
*
*  Function description
*    Match string against pattern.
*
*  Parameters
*    sPat - Zero-terminated pattern string.
*    sStr - Zero-terminated string to match.
*
*  Return value
*    0 -- No match.
*    1 -- Match.
*/
static int _SSH_SCP_SOURCE_Match(const char *sPat, const char *sStr) {
  const char   * s;
  const char   * p;
  int            Star;
  //
  Star = 0;
  //
Again:
  for (s = sStr, p = sPat; *s; ++s, ++p) {
    switch (*p) {
    case '?':
      break;
      //
    case '*':
      Star = 1;
      sStr = s;
      sPat = p;
      if (*++sPat == 0) {
        return 1;
      }
      goto Again;
      //
    default:
      if (_Upper(*s) != _Upper(*p)) {
        if (!Star) {
          return 0;
        }
        ++sStr;
        goto Again;
      }
      break;
    }
  }
  if (*p == '*') {
    ++p;
  }
  return *p == 0;
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_IsPathSep()
*
*  Function description
*    Is character a path separator?
*
*  Parameters
*    Ch - Character to test.
*
*  Return value
*    == 0 - Not a path separator.
*    != 0 - Is a path separator.
*/
static int _SSH_SCP_SOURCE_IsPathSep(int Ch) {
  return Ch == '/' || Ch == '\\';
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_FS_FS_PathNorm()
*
*  Function description
*    Normalize path to emFile directory delimiters.
*
*  Parameters
*    sPath - Pointer to zero-terminated path string.
*/
static void _SSH_SCP_SOURCE_FS_FS_PathNorm(char *sPath) {
  while (*sPath) {
    if (_SSH_SCP_SOURCE_IsPathSep(*sPath)) {
      *sPath = FS_DIRECTORY_DELIMITER;
    }
    ++sPath;
  }
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_PathTrim()
*
*  Function description
*    Remove all characters from final '/' or '\' to end.
*
*  Parameters
*    sName - Pointer to zero-terminated string.
*/
static void _SSH_SCP_SOURCE_PathTrim(char *sName) {
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
*       _SSH_SCP_SOURCE_PathAdd()
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
static int _SSH_SCP_SOURCE_PathAdd(char *sName, unsigned NameLen, const char *sText) {
  if (_SSH_SCP_SOURCE_IsPathSep(sText[0]) && sName[0] != 0 && _SSH_SCP_SOURCE_IsPathSep(sName[SSH_STRLEN(sName)-1])) {
    ++sText;
  }
  if (SSH_STRLEN(sName) + SSH_STRLEN(sText) + 1 >= NameLen) {
    return SSH_ERROR_PATH_TOO_LONG;
  }
  //
  SSH_STRNCAT(sName, sText, NameLen);
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_FS_ConstructData()
*
*  Function description
*    Fill in file information structure.
*
*  Parameters
*    pContext  - Pointer to context.
*    pInfo     - Pointer to object that receives the file information.
*
*  Return value
*    >= 0                       - File information filled in successfully.
*    == SSH_ERROR_PATH_TOO_LONG - Full path name is too long.
*/
static int _SSH_SCP_SOURCE_FS_ConstructData(SSH_SCP_SOURCE_FS_FS_CONTEXT * pContext,
                                            SSH_SCP_SOURCE_FILE_INFO     * pInfo) {
  int Status;
  //
  pInfo->Length   = pContext->FindData.FileSize;
  pInfo->IsFolder = (pContext->FindData.Attributes & FS_ATTR_DIRECTORY) != 0;
  pInfo->Mode     = (pContext->FindData.Attributes & FS_ATTR_READ_ONLY) ? 0444 : 0666;
  //
  pInfo->aPathName[0] = 0;
  Status = _SSH_SCP_SOURCE_PathAdd(pInfo->aPathName, sizeof(pInfo->aPathName), pContext->aPath);
  if (Status >= 0) {
    Status = _SSH_SCP_SOURCE_PathAdd(pInfo->aPathName, sizeof(pInfo->aPathName), "/");
    if (Status >= 0) {
      Status = _SSH_SCP_SOURCE_PathAdd(pInfo->aPathName, sizeof(pInfo->aPathName), pContext->FindData.sFileName);
      if (Status >= 0) {
        _SSH_SCP_SOURCE_FS_FS_PathNorm(pInfo->aPathName);
      }
    }
  }
  return Status;
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_FS_FS_Config()
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
static int _SSH_SCP_SOURCE_FS_FS_Config(const char *sRoot) {
  _SSH_SCP_SOURCE_FS_FS_Globals.aRootPath[0] = 0;
  return _SSH_SCP_SOURCE_PathAdd(_SSH_SCP_SOURCE_FS_FS_Globals.aRootPath,
                                sizeof(_SSH_SCP_SOURCE_FS_FS_Globals.aRootPath),
                                sRoot);
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_FS_FS_Init()
*
*  Function description
*    Implementation of Init API call.
*
*  Parameters
*    Index - Zero-based SCP session index.
*    sPath - Pointer to zero-terminated initial path from SCP command line.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SOURCE_FS_FS_Init(unsigned Index, const char *sPath) {
  SSH_SCP_SOURCE_FS_FS_CONTEXT * pContext;
  int                               Status;
  //
  pContext = &_SSH_SCP_SOURCE_FS_FS_Globals.aContext[Index];
  //
  SSH_MEMSET(pContext, 0, sizeof(*pContext));
  pContext->FindActive = 0;
  //
  pContext->aPath[0] = 0;
  Status = _SSH_SCP_SOURCE_PathAdd(pContext->aPath, sizeof(pContext->aPath), _SSH_SCP_SOURCE_FS_FS_Globals.aRootPath);
  if (Status >= 0) {
    Status = _SSH_SCP_SOURCE_PathAdd(pContext->aPath, sizeof(pContext->aPath), "/");
    if (Status >= 0) {
      Status = _SSH_SCP_SOURCE_PathAdd(pContext->aPath, sizeof(pContext->aPath), sPath);
      if (Status >= 0 && pContext->aPath[SSH_STRLEN(pContext->aPath)-1] == '/') {
        Status = _SSH_SCP_SOURCE_PathAdd(pContext->aPath, sizeof(pContext->aPath), "*");
      }
      if (Status >= 0) {
        _SSH_SCP_SOURCE_FS_FS_PathNorm(pContext->aPath);
        _SSH_SCP_SOURCE_PathTrim(pContext->aPath);
        pContext->sPattern = SSH_STRCHR(pContext->aPath, 0) + 1;
      }
    }
  }
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_FS_FS_FindFirst()
*
*  Function description
*    Implementation of Find First API call.
*
*  Parameters
*    Index - Zero-based SCP session index.
*    pInfo - Pointer to object that receives file information.
*
*  Return value
*    >= 0                       - Success.
*    == SSH_ERROR_NO_MORE_FILES - Folder enumeration complete (i.e. empty folder).
*    <  0                       - Error status.
*/
static int _SSH_SCP_SOURCE_FS_FS_FindFirst(unsigned Index, SSH_SCP_SOURCE_FILE_INFO *pInfo) {
  SSH_SCP_SOURCE_FS_FS_CONTEXT * pContext;
  int                            Status;
  //
  pContext = &_SSH_SCP_SOURCE_FS_FS_Globals.aContext[Index];
  //
  // Close down any open handle; this is just belts and braces.
  //
  if (pContext->FindActive) {
    FS_FindClose(&pContext->FindData);
    pContext->FindActive = 0;
  }
  //
  // FS_FindFirstFile() and FS_FindNextFile() produce opposite "success" and "no
  // files" results.  FS_FindFirstFile() returns 0 for "found" and 1 for "not found".
  // FS_FindNextFile() returns 1 for "found" and 0 for "not found / error".
  //
  // We normalize FindFirstFile() to results of FindNextFile(), i.e. 1 is "found"
  // and 0 as "not found".
  //
  Status = FS_FindFirstFile(&pContext->FindData, pContext->aPath, &pContext->aFileName[0], sizeof(pContext->aFileName));
  if (Status == 0 || Status == 1) {
    Status = 1-Status;
  }
  while (Status == 1 && (!_SSH_SCP_SOURCE_Match(pContext->sPattern, pContext->FindData.sFileName) ||
                         (pContext->FindData.Attributes & (FS_ATTR_HIDDEN | FS_ATTR_SYSTEM | FS_ATTR_DIRECTORY)) != 0)) {
    Status = FS_FindNextFile(&pContext->FindData);
  }
  if (Status == 0) {
    Status = SSH_ERROR_NO_MORE_FILES;
  } else if (Status < 0) {
    Status = SSH_ERROR_CANT_READ_FOLDER;
  } else {
    Status = _SSH_SCP_SOURCE_FS_ConstructData(pContext, pInfo);
  }
  if (Status >= 0) {
    pContext->FindActive = 1;
  }
  return Status;
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_FS_FS_FindNext()
*
*  Function description
*    Implementation of Find Next API call.
*
*  Parameters
*    Index - Zero-based SCP session index.
*    pInfo - Pointer to object that receives file information.
*
*  Return value
*    >= 0                       - Success.
*    == SSH_ERROR_NO_MORE_FILES - Folder enumeration complete, no additional information.
*    <  0                       - Error status.
*/
static int _SSH_SCP_SOURCE_FS_FS_FindNext(unsigned Index, SSH_SCP_SOURCE_FILE_INFO *pInfo) {
  SSH_SCP_SOURCE_FS_FS_CONTEXT * pContext;
  int                            Status;
  //
  pContext = &_SSH_SCP_SOURCE_FS_FS_Globals.aContext[Index];
  //
  if (!pContext->FindActive) {
    return SSH_ERROR_NO_MORE_FILES;
  }
  //
  Status = FS_FindNextFile(&pContext->FindData);
  while (Status == 1 && !_SSH_SCP_SOURCE_Match(pContext->sPattern, pContext->aFileName)) {
    Status = FS_FindNextFile(&pContext->FindData);
  }
  if (Status == 0) {
    FS_FindClose(&pContext->FindData);
    pContext->FindActive = 0;
    return SSH_ERROR_NO_MORE_FILES;
  } else {
    return _SSH_SCP_SOURCE_FS_ConstructData(pContext, pInfo);
  }
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_FS_FS_FindClose()
*
*  Function description
*    Implementation of Find Close API call.
*
*  Parameters
*    Index - Zero-based SCP session index.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SOURCE_FS_FS_FindClose(unsigned Index) {
  SSH_SCP_SOURCE_FS_FS_CONTEXT *pContext;
  //
  pContext = &_SSH_SCP_SOURCE_FS_FS_Globals.aContext[Index];
  //
  if (pContext->FindActive) {
    FS_FindClose(&pContext->FindData);
    pContext->FindActive = 0;
  }
  //
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_FS_FS_OpenFile()
*
*  Function description
*    Implementation of Open File API call.
*
*  Parameters
*    Index - Zero-based SCP session index.
*    sName - Pointer to zero-terminated full path name to file.
*            Note that this is writable to allow in-place modification
*            of path separators.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SOURCE_FS_FS_OpenFile(unsigned Index, char *sName) {
  SSH_SCP_SOURCE_FS_FS_CONTEXT *pContext;
  //
  pContext = &_SSH_SCP_SOURCE_FS_FS_Globals.aContext[Index];
  //
  if (pContext->pFile != NULL) {
    FS_FClose(pContext->pFile);
  }
  //
  // Close down any open file; this is just belts and braces.
  //
  pContext->pFile = FS_FOpen(sName, "rb");
  if (pContext->pFile == NULL) {
    return SSH_ERROR_CANT_READ_FILE;
  } else {
    return 0;
  }
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_FS_FS_ReadFile()
*
*  Function description
*    Implementation of Read File API call.
*
*  Parameters
*    Index   - Zero-based SCP session index.
*    pData   - Pointer to object that receives the file data.
*    DataLen - Octet length of the object that receives the file data.
*
*  Return value
*    >  0 - Number of octets read from file (with more data to read from file).
*    == 0 - End of file indication (with no more data to read from file).
*    <  0 - Error status.
*/
static int _SSH_SCP_SOURCE_FS_FS_ReadFile(unsigned Index, U8 *pData, unsigned DataLen) {
  SSH_SCP_SOURCE_FS_FS_CONTEXT * pContext;
  unsigned                          L;
  //
  pContext = &_SSH_SCP_SOURCE_FS_FS_Globals.aContext[Index];
  //
  if (pContext->pFile == NULL) {
    return SSH_ERROR_CANT_READ_FILE;
  } else {
    L = FS_FRead(pData, 1, DataLen, pContext->pFile);
    if (FS_FError(pContext->pFile) != FS_ERRCODE_OK && FS_FError(pContext->pFile) != FS_ERRCODE_EOF) {
      return SSH_ERROR_CANT_READ_FILE;
    } else {
      return L;
    }
  }
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_FS_FS_CloseFile()
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
static int _SSH_SCP_SOURCE_FS_FS_CloseFile(unsigned Index) {
  SSH_SCP_SOURCE_FS_FS_CONTEXT * pContext;
  //
  pContext = &_SSH_SCP_SOURCE_FS_FS_Globals.aContext[Index];
  //
  if (pContext->pFile) {
    FS_FClose(pContext->pFile);
  }
  pContext->pFile = NULL;
  //
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SOURCE_FS_FS_DecodeStatus()
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
static const char * _SSH_SCP_SOURCE_FS_FS_DecodeStatus(int Status) {
  return Status == 0 ? NULL : "General failure";
}

/*********************************************************************
*
*       Public data
*
**********************************************************************
*/

const SSH_SCP_SOURCE_FS_API SSH_SCP_SOURCE_FS_FS = {
  _SSH_SCP_SOURCE_FS_FS_Config,
  _SSH_SCP_SOURCE_FS_FS_Init,
  _SSH_SCP_SOURCE_FS_FS_FindFirst,
  _SSH_SCP_SOURCE_FS_FS_FindNext,
  _SSH_SCP_SOURCE_FS_FS_FindClose,
  _SSH_SCP_SOURCE_FS_FS_OpenFile,
  _SSH_SCP_SOURCE_FS_FS_ReadFile,
  _SSH_SCP_SOURCE_FS_FS_CloseFile,
  _SSH_SCP_SOURCE_FS_FS_DecodeStatus,
};

/*************************** End of file ****************************/
