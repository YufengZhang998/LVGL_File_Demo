/**
 * @file cxsw_quick_operation.h
 * @author Feng
 *
 */

#ifndef CXSW_QUICK_OPERATION_H
#define CXSW_QUICK_OPERATION_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdlib.h>

#include "../cxsw_lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void LvCloseQuickOperation();
lv_obj_t * LvCreateQuickOperation(char *path, void (*event_cb)(void *para));

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*CXSW_QUICK_OPERATION_H*/
