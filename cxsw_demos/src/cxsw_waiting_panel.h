/**
 * @file cxsw_waiting_panel.h
 * @author Feng
 *
 */

#ifndef CXSW_WAITING_PANEL_H
#define CXSW_WAITING_PANEL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
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
lv_obj_t *LvCreateWaitingPanel(lv_obj_t *parent);
lv_obj_t *LvDeleteWaitingPanel(lv_obj_t *parent);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*CXSW_WAITING_PANEL_H*/
