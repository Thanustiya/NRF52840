/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2021  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support_emfile@segger.com        *
*                                                                    *
**********************************************************************
*                                                                    *
*       emFile * File system for embedded applications               *
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
---------------------------END-OF-HEADER------------------------------
File        : FS_Conf.h
Purpose     : Configuration for file system
*/

#ifndef _FS_CONF_H_
#define _FS_CONF_H_     // Avoid multiple inclusion

//
// The ESPRO library has been built with following defines set
//
#ifndef FS_SUPPORT_SECTOR_BUFFER_CACHE
  #define FS_SUPPORT_SECTOR_BUFFER_CACHE 1
#endif
#ifndef FS_MMC_SUPPORT_UHS
  #define FS_MMC_SUPPORT_UHS             1
#endif
#ifndef FS_NOR_SUPPORT_CRC
  #define FS_NOR_SUPPORT_CRC             1
#endif
#ifndef FS_MULTI_HANDLE_SAFE
  #define FS_MULTI_HANDLE_SAFE           1
#endif


#ifndef DEBUG
  #define DEBUG 0
#endif

#if DEBUG
  #ifndef  FS_DEBUG_LEVEL
    #define FS_DEBUG_LEVEL                     5  // Errors, Warnings and Messages are recorded.
    #define FS_SUPPORT_PROFILE                 1  // Enable SystemView in debug builds.
    #define FS_SUPPORT_PROFILE_END_CALL        1  // Profiling instrumentation of function return events.
  #endif
#else
  #ifndef  FS_DEBUG_LEVEL
    #define FS_DEBUG_LEVEL                     0  // No run time checks are performed
    #define FS_SUPPORT_PROFILE                 0  // Disable SystemView (Default)
    #define FS_SUPPORT_PROFILE_END_CALL        0  // No profiling instrumentation of function return events.
  #endif
#endif

#define FS_SUPPORT_CHECK_MEMORY            1
#define FS_SUPPORT_DEINIT                  1
#define FS_SUPPORT_ENCRYPTION              1
#define FS_SUPPORT_EXT_ASCII               1
#define FS_SUPPORT_FILE_NAME_ENCODING      1
#define FS_SUPPORT_MBCS                    1

#endif  // Avoid multiple inclusion


/*************************** End of file ****************************/
