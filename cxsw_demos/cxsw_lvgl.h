/**
 * @file cxsw_lvgl.h
 *
 */

#ifndef CXSW_LVGL_H
#define CXSW_LVGL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "../lvgl/lvgl.h"
#endif

#include "src/cxsw_popup_prompt.h"
#include "src/cxsw_directory.h"
#include "src/cxsw_quick_operation.h"
#include "src/cxsw_wifi_manager.h"
#include "src/cxsw_waiting_panel.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
// cxsw_waiting_panel
#define WAITING_PANEL_TEXT          "Please wait..."

// cxsw_popup_prompt
#define POPUP_TITLE_ERROE           "Error!!!"
#define POPUP_TITLE_WARN            "Warning!!!"
#define POPUP_TITLE_BG_COLOR        0x42A5F5    // 蓝亮-1

// cxsw_quick_operation
#define OPERATION_LIST_COPY         "copy"
#define OPERATION_LIST_PASTE        "paste"
#define OPERATION_LIST_DELETE       "delete"

#define OPERATION_DEL_MSG_TITLE     "Please confirm"
#define OPERATION_DEL_MSG_TXT       "Are you sure you want to delete the file?"
#define OPERATION_DEL_BTN_YES       "Yes"
#define OPERATION_DEL_BTN_CANCEL    "Cancel"

// cxsw_directory
#define DIR_MANAGER_ITEM_FILE       "Name"
#define DIR_MANAGER_ITEM_DATE       "Date"
#define DIR_MANAGER_ITEM_TYPE       "Type"
#define DIR_MANAGER_ITEM_SIZE       "Size"

#define DIR_MANAGER_FILE_DEF_NAME   "default_name"

//cxsw_wifi_manager
#define WIFI_MANAGER_TITLE          "Wifi Manager"
#define WIFI_MANAGER_EXIT           "Exit"
#define WIFI_MANAGER_IP_STR         "IP Addr:  "

#define WIFI_MANAGER_ADD_WIFI_BTN   "Add wifi"
#define WIFI_MANAGER_ADD_TITLE      "Add WIFI"
#define WIFI_MANAGER_ADD_NAME       "Network name"
#define WIFI_MANAGER_ADD_SAFETY     "Safety"
#define WIFI_MANAGER_ADD_PASS       "password"
#define WIFI_MANAGER_ADD_CANCEL     "Cancel"
#define WIFI_MANAGER_ADD_SAVE       "Save"

#define WIFI_MANAGER_JOIN_PASS      "password"
#define WIFI_MANAGER_JOIN_CANCEL    "Cancel"
#define WIFI_MANAGER_JOIN_CONNECT   "Connect"

#define WIFI_MANAGER_CON_DISCON     "Disconnect"
#define WIFI_MANAGER_CON_CON        "Connect"
#define WIFI_MANAGER_CON_REMOVE     "Remove"

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*CXSW_LVGL_H*/
