/**
 * @file cxsw_popup_prompt.h
 * @author Feng
 *
 */

#ifndef CXSW_POPUP_PROMPT_H
#define CXSW_POPUP_PROMPT_H

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
lv_obj_t * LvCreateErrorPopup(const char * txt);
lv_obj_t * LvCreateWarnPopup(const char * txt);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*CXSW_POPUP_PROMPT_H*/
