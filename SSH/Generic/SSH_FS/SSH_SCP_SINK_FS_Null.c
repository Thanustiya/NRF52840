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

File        : SSH_SCP_SINK_FS_Null.c
Purpose     : Null file system that always writes correctly.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSH_Int.h"

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_Null_Config()
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
static int _SSH_SCP_SINK_FS_Null_Config(const char *sRoot) {
  SSH_USE_PARA(sRoot);
  //
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_Null_Init()
*
*  Function description
*    Implementation of Init API call.
*
*  Parameters
*    Index - Zero-based SCP session index.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SINK_FS_Null_Init(unsigned Index, const char *pPath, unsigned PathLen) {
  SSH_USE_PARA(Index);
  SSH_USE_PARA(pPath);
  SSH_USE_PARA(PathLen);
  //
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_Null_CreateFile()
*
*  Function description
*    Implementation of Create File API call.
*
*  Parameters
*    Index   - Zero-based SCP session index.
*    Mode    - Unix file mode.
*    Len     - Length of file.
*    ModTime - Last modification time, Unix format, number of seconds
*              since 1 Jan 1970.  Zero indicates time is not set.
*    AccTime - Last access time, Unix format, number of seconds since
*              1 Jan 1970.  Zero indicates time is not set.
*    sName   - Zero-terminated full path name.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SINK_FS_Null_CreateFile(unsigned Index, unsigned Mode, U32 Len, U32 ModTime, U32 AccTime, const char *sName) {
  SSH_USE_PARA(Index);
  SSH_USE_PARA(Mode);
  SSH_USE_PARA(Len);
  SSH_USE_PARA(ModTime);
  SSH_USE_PARA(AccTime);
  SSH_USE_PARA(sName);
  //
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_Null_WriteFile()
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
static int _SSH_SCP_SINK_FS_Null_WriteFile(unsigned Index, const U8 *pData, unsigned DataLen) {
  SSH_USE_PARA(Index);
  SSH_USE_PARA(pData);
  SSH_USE_PARA(DataLen);
  //
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_Null_CloseFile()
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
static int _SSH_SCP_SINK_FS_Null_CloseFile(unsigned Index) {
  SSH_USE_PARA(Index);
  //
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_Null_EnterFolder()
*
*  Function description
*    Implementation of Enter Folder API call.
*
*  Parameters
*    Index   - Zero-based SCP session index.
*    Mode    - Folder mode.
*    ModTime - Last modification time, Unix format, number of seconds
*              since 1 Jan 1970.  Zero indicates time is not set.
*    AccTime - Last access time, Unix format, number of seconds since
*              1 Jan 1970.  Zero indicates time is not set.
*    sName   - Zero-terminated folder name.
*
*  Return value
*    >= 0 - Success.
*    <  0 - Error status.
*/
static int _SSH_SCP_SINK_FS_Null_EnterFolder(unsigned Index, unsigned Mode, U32 ModTime, U32 AccTime, const char *sName) {
  SSH_USE_PARA(Index);
  SSH_USE_PARA(Mode);
  SSH_USE_PARA(ModTime);
  SSH_USE_PARA(AccTime);
  SSH_USE_PARA(sName);
  //
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_Null_ExitFolder()
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
static int _SSH_SCP_SINK_FS_Null_ExitFolder(unsigned Index) {
  SSH_USE_PARA(Index);
  //
  return 0;
}

/*********************************************************************
*
*       _SSH_SCP_SINK_FS_Null_DecodeStatus()
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
static const char * _SSH_SCP_SINK_FS_Null_DecodeStatus(int Status) {
  return Status == 0 ? NULL : "General failure";
}

/*********************************************************************
*
*       Public data
*
**********************************************************************
*/

const SSH_SCP_SINK_FS_API SSH_SCP_SINK_FS_Null = {
  _SSH_SCP_SINK_FS_Null_Config,
  _SSH_SCP_SINK_FS_Null_Init,
  _SSH_SCP_SINK_FS_Null_CreateFile,
  _SSH_SCP_SINK_FS_Null_WriteFile,
  _SSH_SCP_SINK_FS_Null_CloseFile,
  _SSH_SCP_SINK_FS_Null_EnterFolder,
  _SSH_SCP_SINK_FS_Null_ExitFolder,
  _SSH_SCP_SINK_FS_Null_DecodeStatus,
};

/*************************** End of file ****************************/
