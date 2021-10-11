/**
 * @file cxsw_directory.h
 * @author Feng
 *
 */

#ifndef CXSW_DIRECTORY_H
#define CXSW_DIRECTORY_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>  
#include <stdio.h>
#include <stdlib.h>

#include "../cxsw_lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define DEF_FILE_NAME_LEN   50

/**********************
 *      TYPEDEFS
 **********************/
struct LvFileList_t{
    char d_name[DEF_FILE_NAME_LEN];
    __mode_t st_mode;
    __off_t st_size;
    struct timespec st_mtim;
    lv_obj_t *btn_hand;
};

struct LvDirInfo_t {
    char *PathParam;
    unsigned long FileNum;
    struct LvFileList_t **FileList;
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void LvGetDirInfo(const char *name, struct LvDirInfo_t *info);// 返回参数(info)在接口内申请内存,使用(LvFreeDirInfo)释放
void LvFreeDirInfo(struct LvDirInfo_t *free_list);

lv_obj_t * LvCreateDirManager(lv_obj_t *parent, char *path, struct LvDirInfo_t *dirinfo, void (*event_cb)(lv_event_t * e));

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*CXSW_DIRECTORY_H*/
