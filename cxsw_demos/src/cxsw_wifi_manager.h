/**
 * @file cxsw_wifi_manager.h
 * @author Feng
 *
 */

#ifndef CXSW_WIFI_MANAGER_H
#define CXSW_WIFI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>

#include "../cxsw_lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define WIFI_INFO_SIZE_MAX          5
#define WIFI_STATE_INQUIRE_TIME     1000
#define WIFI_CONNECT_INFO_PATH      "/etc/wifi/wpa_supplicant.conf"

/**********************
 *      TYPEDEFS
 **********************/
enum Wifi_Info_Type{
    WIFI_INFO_TYPE_BSSID,
    WIFI_INFO_TYPE_FREQUER,
    WIFI_INFO_TYPE_SIGNAL,
    WIFI_INFO_TYPE_FLAGS,
    WIFI_INFO_TYPE_SSID,
    WIFI_INFO_TYPE_SIZE,
};

enum Wifi_Info_Use{
    WIFI_INFO_USE_NET_ID,
    WIFI_INFO_USE_SSID,
    WIFI_INFO_USE_BSSID,
    WIFI_INFO_USE_FLAGS,
    WIFI_INFO_USE_SIZE,
};

enum Wifi_Use_State{
    WIFI_USE_STATE_UNUSED,
    WIFI_USE_STATE_USED,
    WIFI_USE_STATE_IS_USING,
    WIFI_USE_STATE_SIZE,
};

struct Wifi_Str_t{
    char *str;
    unsigned char str_len;
};

struct Wifi_Info_t{
    int info_size;
    struct Wifi_Str_t single[WIFI_INFO_SIZE_MAX];
    lv_obj_t *btn_hand;
    enum Wifi_Info_Use WiFi_Usage;
};

struct Wifi_Manager_t{
    unsigned long wifi_num;
    struct Wifi_Info_t **info;
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t *LvCreateWifiManager(lv_obj_t *parent);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*CXSW_WIFI_MANAGER_H*/
