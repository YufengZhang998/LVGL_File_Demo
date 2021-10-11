/**
 * @file cxsw_wifi_manager.c
 * @author Feng
 *
 */

#if 1
/*********************
 *      INCLUDES
 *********************/
#include "cxsw_wifi_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "wifid_cmd.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static int FreeWifiInfo(struct Wifi_Manager_t *wifi_info);
static int GetWifiInfo(char *buff, struct Wifi_Manager_t *wifi_info);
static void WifiEventEb(lv_event_t * e);
static void AddWifiEventEb(lv_event_t * e);
static void UpdateDisplayIp(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static const char list_safety[][10] = {"WEP", "WPA", "WPA2/WPA3", "WPA3", "WAPI"};
static const char list_using[][10] = {WIFI_MANAGER_CON_DISCON};
static const char list_used[][10] = {WIFI_MANAGER_CON_CON, WIFI_MANAGER_CON_REMOVE};

static lv_style_t style_blur_background;
static lv_style_t style_wifi_manager;
static lv_style_t style_wifi_manager_list;
static lv_style_t style_wifi_list;
static lv_style_t style_quick_operation;

static lv_obj_t *WifiManager = NULL;
static lv_obj_t *WifiList = NULL;
static lv_obj_t *keyboard = NULL;
static lv_obj_t *LabelIpAddr = NULL;

static lv_obj_t *Connect_cont = NULL;
static lv_obj_t *Add_wifi_cont = NULL;

static lv_timer_t *WifiStateInquireTimer = NULL;

struct wifi_status LastWifiStatus = {
    .state = STATE_UNKNOWN,
    .ssid = {'\0'},
};

struct Wifi_Manager_t GetWifiManagerInfo = {
    .info = NULL,
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/***********************************************
 *   Name：WifiManagerInit
 * Depict：WiFi管理器初始化
 ***********************************************/
static void WifiManagerInit(void)
{
    // 设置模糊背景样式
    lv_style_init(&style_blur_background);
    lv_style_set_pad_top(&style_blur_background, 0);
    lv_style_set_pad_bottom(&style_blur_background, 0);
    lv_style_set_pad_left(&style_blur_background, 0);
    lv_style_set_pad_right(&style_blur_background, 0);
    lv_style_set_pad_row(&style_blur_background, 0);
    lv_style_set_pad_column(&style_blur_background, 0);
    lv_style_set_radius(&style_blur_background, 0);

    // 设置文件管理器样式
    lv_style_init(&style_wifi_manager);

    // 设置文件管理器子类样式
    lv_style_init(&style_wifi_manager_list);
    lv_style_set_radius(&style_wifi_manager_list, 0);

    // 设置列表字体样式
    lv_style_init(&style_wifi_list);
    lv_style_set_text_font(&style_wifi_list, &lv_font_montserrat_30);

    // 设置快捷操作样式
    lv_style_init(&style_quick_operation);
    lv_style_set_pad_left(&style_quick_operation, 0);
    lv_style_set_pad_right(&style_quick_operation, 0);
    lv_style_set_pad_row(&style_quick_operation, 0);
    lv_style_set_pad_column(&style_quick_operation, 0);
    lv_style_set_text_font(&style_quick_operation, &lv_font_montserrat_22);
}

/***********************************************
 *   Name：WifiManageResetInit
 * Depict：WiFi管理器初始化
 ***********************************************/
static void WifiManageResetInit(void)
{
    // 复位样式
    lv_style_reset(&style_blur_background);
    lv_style_reset(&style_wifi_manager);
    lv_style_reset(&style_wifi_manager_list);
    lv_style_reset(&style_wifi_list);
    lv_style_reset(&style_quick_operation);

    lv_timer_del(WifiStateInquireTimer);
    WifiStateInquireTimer = NULL;

    LastWifiStatus.state = STATE_UNKNOWN;
    memset(LastWifiStatus.ssid, 0, sizeof(LastWifiStatus.ssid));

    FreeWifiInfo(&GetWifiManagerInfo);
    keyboard = NULL;
}

/***********************************************
 *   Name：CustomizeFileRead
 * Depict：自定义文件读取
 ***********************************************/
static int CustomizeFileRead(char *Path, char *buffer)
{
    FILE *fpRead;
    int readCount = 0;
    int readSize;
    int bufferLen = 1024*4;

    if( (fpRead = fopen(Path, "rb")) == NULL)
    {
        LV_LOG_WARN("Cannot open file, press any key to exit!\n");
        return -1;
    }

    while ((readSize = fread(&buffer[readCount], 1, bufferLen, fpRead)) > 0)
    {
        readCount += readSize;
    }

    fclose(fpRead);
    return readCount;
}

/***********************************************
 *   Name：DisdlayWifiInfo
 * Depict：显示WiFi信息
 ***********************************************/
static int DisdlayWifiInfo(struct Wifi_Manager_t *List)
{
    for (int i = 0; i < List->wifi_num; i++)
    {
        char dis_str[100] = {0,};
        for (int j = 0; j < List->info[i]->info_size; j++)
        {
            sprintf(dis_str, "%s\t%s", dis_str, List->info[i]->single[j].str);
        }
        sprintf(dis_str, "%s", dis_str);
        LV_LOG_USER("%s", dis_str);
    }
}

/***********************************************
 *   Name：FilterSSIDSameOrEmpty
 * Depict：筛选SSID是否相同或者为空
 ***********************************************/
static int FilterSSIDSameOrEmpty(struct Wifi_Manager_t *List)
{
    int filte_size = 0;
    // 申请空间储存wifi信息
    struct Wifi_Info_t **FilterList = (struct Wifi_Info_t **)lv_mem_alloc(sizeof(struct Wifi_Info_t *) * List->wifi_num);
    for (int i = 0; i < List->wifi_num; i++)
    {
        if (List->info[i]->single[WIFI_INFO_TYPE_SSID].str_len == 0) continue;
        bool ssid_same_flag = false;
        for (int j = 0; j < filte_size; j++)
        {
            if (strcmp(FilterList[j]->single[WIFI_INFO_TYPE_SSID].str, List->info[i]->single[WIFI_INFO_TYPE_SSID].str) == 0)
            {
                if (strcmp(FilterList[j]->single[WIFI_INFO_TYPE_SIGNAL].str, List->info[i]->single[WIFI_INFO_TYPE_SIGNAL].str) > 0)
                {
                    FilterList[j]->info_size = List->info[i]->info_size;
                    for (int k = 0; k < List->info[i]->info_size; k++)
                    {
                        FilterList[j]->single[k].str_len = List->info[i]->single[k].str_len;
                        lv_mem_free(FilterList[j]->single[k].str);
                        FilterList[j]->single[k].str = (char *)lv_mem_alloc(List->info[i]->single[k].str_len + 1);
                        memset(FilterList[j]->single[k].str, 0, List->info[i]->single[k].str_len + 1);
                        memcpy(FilterList[j]->single[k].str, List->info[i]->single[k].str, List->info[i]->single[k].str_len);
                    }
                }
                ssid_same_flag = true;
                break;
            }
        }
        if (ssid_same_flag == false)
        {
            FilterList[filte_size] = (struct Wifi_Info_t *)lv_mem_alloc(sizeof(struct Wifi_Info_t));
            memset(FilterList[filte_size], 0, sizeof(struct Wifi_Info_t));

            FilterList[filte_size]->info_size = List->info[i]->info_size;
            for(int j = 0; j < List->info[i]->info_size; j++)
            {
                FilterList[filte_size]->single[j].str_len = List->info[i]->single[j].str_len;
                FilterList[filte_size]->single[j].str = (char *)lv_mem_alloc(List->info[i]->single[j].str_len + 1);
                memset(FilterList[filte_size]->single[j].str, 0, List->info[i]->single[j].str_len + 1);
                memcpy(FilterList[filte_size]->single[j].str, List->info[i]->single[j].str, List->info[i]->single[j].str_len);
            }
            filte_size++;
        }
    }
    FreeWifiInfo(List);
    LV_LOG_USER("Filter the remaining number after the SSID is empty %d", filte_size);
    
    // 申请空间储存wifi信息
    struct Wifi_Info_t **ConfirmList = (struct Wifi_Info_t **)lv_mem_alloc(sizeof(struct Wifi_Info_t *) * filte_size);
    for (int i = 0; i < filte_size; i++)
    {
        ConfirmList[i] = FilterList[i];
    }
    lv_mem_free(FilterList);

    // 保存筛选后的数据
    List->wifi_num = filte_size;
    List->info = ConfirmList;
    LV_LOG_USER("Filter SSID is empty or the same name is completed");
}

/***********************************************
 *   Name：WifiSignalStrengthSorting
 * Depict：按照wifi信号强度排序
 ***********************************************/
static int WifiSignalStrengthSorting(struct Wifi_Manager_t *wifi_info)
{
    struct Wifi_Info_t *temp;
    for (int i = 0; i < (wifi_info->wifi_num - 1); i++)
    {
        for (int j = 0; j < (wifi_info->wifi_num - i - 1); j++)
        {
            if (strcmp(wifi_info->info[j]->single[WIFI_INFO_TYPE_SIGNAL].str,
                        wifi_info->info[j+1]->single[WIFI_INFO_TYPE_SIGNAL].str) > 0)
            {
                temp = wifi_info->info[j];
                wifi_info->info[j] = wifi_info->info[j+1];
                wifi_info->info[j+1] = temp;
            }
        }
    }
    LV_LOG_USER("Sort by signal strength");
}

/***********************************************
 *   Name：GetWifiInfoFromStr
 * Depict：从字符串获取WiFi信息
 ***********************************************/
static int GetWifiInfoFromStr(char *buff, struct Wifi_Info_t ***wifi_info, int info_size)
{
    int wifi_size = 0;
    char *Recv_p = NULL;
    char *p_Source = NULL;

    // 去除信息头部字符串
    if (buff == NULL || strlen(buff) == 0) return -1;
    Recv_p = strchr(buff, '\n') + 1;
    LV_LOG_USER("get scan wifi info str valid len %d", strlen(Recv_p));

    // 计算热点数量
    p_Source = Recv_p;
    while (1)
    {
        p_Source = strchr(p_Source, '\n');
        if (p_Source == NULL)
        {
            wifi_size++;
            break;
        }
        p_Source = p_Source + 1;
        wifi_size++;
    }
    LV_LOG_USER("get scan wifi num = %d", wifi_size);

    // 定义空间,提取热点信息
    struct Wifi_Info_t List[wifi_size];
    char *p_t = NULL, *p_n = NULL;
    p_Source = Recv_p;
    for (int i = 0; i < wifi_size; i++)
    {
        p_n = strchr(p_Source, '\n');
        for (int j = 0; j < info_size; j++)
        {
            p_t = strchr(p_Source, '\t');
            if (p_n == NULL)
            {
                if (p_t == NULL)
                {
                    List[i].single[j].str = p_Source;
                    List[i].single[j].str_len = strlen(p_Source);
                    break;
                }
                else
                {
                    List[i].single[j].str = p_Source;
                    List[i].single[j].str_len = p_t - p_Source;
                    p_Source = p_t + 1;
                }
            }
            else
            {
                if (p_t < p_n)
                {
                    List[i].single[j].str = p_Source;
                    List[i].single[j].str_len = p_t - p_Source;
                    p_Source = p_t + 1; 
                }
                else 
                {
                    List[i].single[j].str = p_Source;
                    List[i].single[j].str_len = p_n - p_Source;
                    break;
                }
            }
        }
        p_Source = p_n + 1;
    }

    // 申请空间储存wifi信息
    struct Wifi_Info_t **FilterList = (struct Wifi_Info_t **)lv_mem_alloc(sizeof(struct Wifi_Info_t *) * wifi_size);
    for(int i = 0; i < wifi_size; i++)
    {
        FilterList[i] = (struct Wifi_Info_t *)lv_mem_alloc(sizeof(struct Wifi_Info_t));
        memset(FilterList[i], 0, sizeof(struct Wifi_Info_t));

        FilterList[i]->info_size = info_size;
        for(int j = 0; j < info_size; j++)
        {
            FilterList[i]->single[j].str_len = List[i].single[j].str_len;
            FilterList[i]->single[j].str = (char *)lv_mem_alloc(List[i].single[j].str_len + 1);
            memset(FilterList[i]->single[j].str, 0, List[i].single[j].str_len + 1);
            memcpy(FilterList[i]->single[j].str, List[i].single[j].str, List[i].single[j].str_len);
        }
    }
    LV_LOG_USER("Number of hotspot information %d", FilterList[0]->info_size);
    LV_LOG_USER("Point one parameter of the first hot spot %s", FilterList[0]->single[0].str);

    *wifi_info = FilterList;
    return wifi_size;
}

/***********************************************
 *   Name：MarkHotspotsSavingPasswords
 * Depict：标记保存密码的热点
 ***********************************************/
static int MarkHotspotsSavingPasswords(struct Wifi_Manager_t *wifi_info)
{
    LV_LOG_USER("Mark hotspots for saving passwords");
    // 查询保存密码的热点
    char list_net_results[LIST_NETWORK_MAX];
    int ret = aw_wifid_list_networks(list_net_results, LIST_NETWORK_MAX);
    if(ret == -1)
    {
        LV_LOG_WARN("list networks results:\n%s", list_net_results);
    }

    // 将字符串转换成结构数据
    struct Wifi_Info_t **info = NULL;
    int wifi_size = GetWifiInfoFromStr(list_net_results, &info, WIFI_INFO_USE_SIZE);
    if (wifi_size == -1)
    {
        LV_LOG_WARN("list networks results fail");
        return -1;
    }

    // 标记热点是否保存密码
    for (int i = 0; i < wifi_info->wifi_num; i++)
    {
        bool save_pass_flag = false;
        for (int j = 0; j < wifi_size; j++)
        {
            if (strcmp(wifi_info->info[i]->single[WIFI_INFO_TYPE_SSID].str, info[j]->single[WIFI_INFO_USE_SSID].str) == 0)
            {
                if (info[j]->single[WIFI_INFO_USE_FLAGS].str_len == 0)
                {
                    wifi_info->info[i]->WiFi_Usage = WIFI_USE_STATE_USED;
                }
                else
                {
                    wifi_info->info[i]->WiFi_Usage = WIFI_USE_STATE_IS_USING;
                }
                save_pass_flag = true;
                break;
            }
        }
        if (save_pass_flag == false)
        {
            wifi_info->info[i]->WiFi_Usage = WIFI_USE_STATE_UNUSED;
        }
    }
    struct Wifi_Manager_t temp;
    temp.wifi_num = wifi_size;
    temp.info = info;
    FreeWifiInfo(&temp);

    // 申请空间储存wifi信息
    int count = 0;
    struct Wifi_Info_t **ConfirmList = (struct Wifi_Info_t **)lv_mem_alloc(sizeof(struct Wifi_Info_t *) * wifi_info->wifi_num);
    for (int i = 0; i < wifi_info->wifi_num; i++)
    {
        if (wifi_info->info[i]->WiFi_Usage == WIFI_USE_STATE_IS_USING)
        {
            ConfirmList[count++] = wifi_info->info[i];
            break;
        }
    }
    for (int i = 0; i < wifi_info->wifi_num; i++)
    {
        if (wifi_info->info[i]->WiFi_Usage == WIFI_USE_STATE_USED)
        {
            ConfirmList[count++] = wifi_info->info[i];
        }
    }
    for (int i = 0; i < wifi_info->wifi_num; i++)
    {
        if ((wifi_info->info[i]->WiFi_Usage != WIFI_USE_STATE_IS_USING) && (wifi_info->info[i]->WiFi_Usage != WIFI_USE_STATE_USED))
        {
            ConfirmList[count++] = wifi_info->info[i];
        }
    }
    lv_mem_free(wifi_info->info);

    // 保存筛选后的数据
    wifi_info->info = ConfirmList;
    wifi_info->wifi_num = count;
    return count;
}

/***********************************************
 *   Name：GetWifiInfo
 * Depict：获取WiFi信息
 ***********************************************/
static int GetWifiInfo(char *buff, struct Wifi_Manager_t *wifi_info)
{
    // 获取WiFi信息
    struct Wifi_Info_t **info = NULL;
    int wifi_size = GetWifiInfoFromStr(buff, &info, WIFI_INFO_TYPE_SIZE);
    wifi_info->wifi_num = wifi_size;
    wifi_info->info = info;

    // 筛选SSID是否相同或者为空
    FilterSSIDSameOrEmpty(wifi_info);
    // 按照信号强度排序
    WifiSignalStrengthSorting(wifi_info);
    // 判断热点是否有保存密码或正在连接
    MarkHotspotsSavingPasswords(wifi_info);
    return wifi_info->wifi_num;
}

/***********************************************
 *   Name：FreeWifiInfo
 * Depict：释放WiFi信息
 ***********************************************/
static int FreeWifiInfo(struct Wifi_Manager_t *wifi_info)
{
    if (wifi_info->info != NULL)
    {
        for (int i = 0; i < wifi_info->wifi_num; i++)
        {
            for (int j = 0; j < wifi_info->info[i]->info_size; j++)
            {
                lv_mem_free(wifi_info->info[i]->single[j].str);
                wifi_info->info[i]->single[j].str = NULL;
            }
            lv_mem_free(wifi_info->info[i]);
            wifi_info->info[i] = NULL;
        }
        lv_mem_free(wifi_info->info);
        wifi_info->info = NULL;
    }
}

/***********************************************
 *   Name：LvDispWifiInfo
 * Depict：显示WiFi信息
 ***********************************************/
static void LvDispWifiInfo(struct Wifi_Info_t *list_info)
{
    static lv_coord_t grid_col_dsc[] = {60, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    // 创建单个文件显示容器
    lv_obj_t * cont = lv_obj_create(WifiList);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_grid_dsc_array(cont, grid_col_dsc, grid_row_dsc);

    // 容器绑定按键
    list_info->btn_hand = lv_btn_create(cont);
    lv_obj_set_size(list_info->btn_hand, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(list_info->btn_hand, lv_color_white(), 0);
    lv_obj_set_style_radius(list_info->btn_hand, 0, 0);
    lv_obj_set_style_opa(list_info->btn_hand, LV_OPA_100, 0);
    lv_obj_add_event_cb(list_info->btn_hand, &WifiEventEb, LV_EVENT_SHORT_CLICKED, cont);

    // 显示文件信息
    lv_obj_t * label;
    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_wifi_list, 0);
    lv_label_set_text(label, list_info->single[WIFI_INFO_TYPE_SIGNAL].str);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);

    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_wifi_list, 0);
    lv_label_set_text(label, list_info->single[WIFI_INFO_TYPE_SSID].str);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 0, 1);

    label = lv_label_create(cont);
    lv_obj_add_style(label, &style_wifi_list, 0);
    char use_state[10] = {0,};
    sprintf(use_state, "%d", list_info->WiFi_Usage);
    lv_label_set_text(label, use_state);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_END, 0, 1);
}

/***********************************************
 *   Name：LvRefreshWifiDispList
 * Depict：刷新WiFi显示列表
 ***********************************************/
static void LvRefreshWifiDispList()
{
    // 清空文件列表显示
    lv_obj_clean(WifiList);
    if (GetWifiManagerInfo.info != NULL)
    {
        LV_LOG_USER("The number of hotspots shown in the WiFi list = %d", GetWifiManagerInfo.wifi_num);
        for (int i = 0; i < GetWifiManagerInfo.wifi_num; i++)
        {
            struct Wifi_Info_t *info = GetWifiManagerInfo.info[i];
            LvDispWifiInfo(info);
        }
    }

    lv_obj_t *btn_add = lv_btn_create(WifiList);
    lv_obj_set_width(btn_add, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(btn_add, 0, 0);
    lv_obj_set_style_opa(btn_add, LV_OPA_80, 0);
    lv_obj_add_event_cb(btn_add, &AddWifiEventEb, LV_EVENT_SHORT_CLICKED, NULL);

    lv_obj_t *label_btn_add = lv_label_create(btn_add);
    lv_obj_add_style(label_btn_add, &style_wifi_list, 0);
    lv_obj_align_to(label_btn_add, btn_add, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label_btn_add, WIFI_MANAGER_ADD_WIFI_BTN);

    LV_LOG_USER("WiFi list display completed");
}

/***********************************************
 *   Name：LvSetWifiList
 * Depict：设置WiFi列表
 ***********************************************/
static void LvSetWifiList()
{
    // char scan_results[4*1024] = {0,};
    // CustomizeFileRead("/home/feng/littleVGL/lv_sim_vscode_sdl/host.txt", scan_results);
    char scan_results[SCAN_MAX];
    lv_obj_t *WaitingPanel = LvCreateWaitingPanel(WifiManager);
    int ret = aw_wifid_get_scan_results(scan_results, SCAN_MAX);
    LvDeleteWaitingPanel(WaitingPanel);

    if(ret == -1)
    {
        LV_LOG_WARN("scan wifi error!");
        return;
    }
    LV_LOG_USER("get scan wifi info str len %d", strlen(scan_results));
    printf("%s\n", scan_results);
    FreeWifiInfo(&GetWifiManagerInfo);
    GetWifiInfo(scan_results, &GetWifiManagerInfo);

    LvRefreshWifiDispList();
}

/***********************************************
 *   Name：bg_event_cb
 * Depict：背景板事件响应
 ***********************************************/
static void bg_event_cb(lv_event_t * e)
{
    if(keyboard != NULL)
    {
        lv_obj_del(keyboard);
        keyboard = NULL;
    }
}

/***********************************************
 *   Name：pwd_mode_event_cb
 * Depict：修改密码模式事件响应
 ***********************************************/
static void pwd_mode_event_cb(lv_event_t * e)
{
    lv_obj_t *pwd_mode = lv_event_get_user_data(e);
    bool pwd_mode_value = lv_textarea_get_password_mode(pwd_mode);

    if (pwd_mode_value) lv_textarea_set_password_mode(pwd_mode, false);
    else lv_textarea_set_password_mode(pwd_mode, true);
}

/***********************************************
 *   Name：cancel_event_cb
 * Depict：响应取消事件
 ***********************************************/
static void cancel_event_cb(lv_event_t * e)
{
    lv_obj_t *cont = lv_event_get_user_data(e);
    keyboard = NULL;
    lv_obj_clean(cont);
    lv_obj_del(cont);
}

/***********************************************
 *   Name：start_connect_wifi
 * Depict：开始连接WiFi
 ***********************************************/
static void start_connect_wifi(const char *name_str, const char *pass_str)
{
    enum cn_event event = DA_UNKNOWN;
    LV_LOG_USER("==============================================");
    LV_LOG_USER("connecting ssid:%s passward:%s", name_str, pass_str);

    lv_obj_t *WaitingPanel = LvCreateWaitingPanel(WifiManager);
    aw_wifid_connect_ap(name_str, pass_str, &event);
    LvDeleteWaitingPanel(WaitingPanel);
    if(event == DA_CONNECTED)
    {
        LV_LOG_USER("connected ap successful");
    }else
    {
        LV_LOG_USER("connected ap failed:%s", connect_event_txt(event));
    }
    LV_LOG_USER("==============================================");
    // 刷新WiFi列表显示
    LvSetWifiList();
    UpdateDisplayIp();
}

/***********************************************
 *   Name：connect_event_cb
 * Depict：响应连接WiFi事件
 ***********************************************/
static void connect_event_cb(lv_event_t * e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *cont = lv_obj_get_parent(btn);
    lv_obj_t *cont_manager = lv_obj_get_parent(cont);
    lv_obj_t *child_cont = lv_obj_get_child(cont_manager, 0);
    struct Wifi_Info_t *info = lv_event_get_user_data(e);

    if (cont_manager == Connect_cont)
    {
        char name_str[50] = {0,};
        memcpy(name_str, info->single[WIFI_INFO_TYPE_SSID].str, info->single[WIFI_INFO_TYPE_SSID].str_len);
        lv_obj_t *pass = lv_obj_get_child(child_cont, 2);
        const char *pass_str = lv_textarea_get_text(pass);

        // 开始连接
        start_connect_wifi(name_str, pass_str);
        // 关闭弹窗
        keyboard = NULL;
        lv_obj_clean(cont_manager);
        lv_obj_del(cont_manager);
    }
    else if (cont_manager == Add_wifi_cont)
    {
        lv_obj_t *name = lv_obj_get_child(child_cont, 2);
        lv_obj_t *safe = lv_obj_get_child(child_cont, 4);
        lv_obj_t *pass = lv_obj_get_child(child_cont, 6);

        const char *name_str = lv_textarea_get_text(name);
        const char *pass_str = lv_textarea_get_text(pass);
        char safe_str[20] = {0,};
        lv_dropdown_get_selected_str(safe, safe_str, 20);

        LV_LOG_USER("name %s", name_str);
        LV_LOG_USER("safe %s", safe_str);
        LV_LOG_USER("pass %s", pass_str);

        if (strlen(name_str) == 0)
        {
            LvCreateWarnPopup("Please enter a name");
        }
        else if (strlen(pass_str) == 0)
        {
            LvCreateWarnPopup("Please enter the password");
        }
        else
        {
            // 开始连接
            start_connect_wifi(name_str, pass_str);
            // 关闭弹窗
            keyboard = NULL;
            lv_obj_clean(cont_manager);
            lv_obj_del(cont_manager);
        }
    }
    else
    {
        LV_LOG_WARN("Operation error");
        keyboard = NULL;
        lv_obj_clean(cont_manager);
        lv_obj_del(cont_manager);
    }
}

/***********************************************
 *   Name：keyboard_event_cb
 * Depict：调出键盘事件
 ***********************************************/
static void keyboard_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    lv_obj_t *obj_bg = lv_event_get_user_data(e);
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED)
    {
        if(keyboard == NULL)
        {
            keyboard = lv_keyboard_create(obj_bg);
        }
        lv_obj_set_size(keyboard,  LV_PCT(100), LV_PCT(30));
        lv_keyboard_set_textarea(keyboard, ta);
    }
    else if (code == LV_EVENT_READY)
    {
        LV_LOG_USER("Ready, current text: %s", lv_textarea_get_text(ta));
    }
}

/***********************************************
 *   Name：GetWiFiPassConnected
 * Depict：获取已连接WiFi的密码
 ***********************************************/
static void GetWiFiPassConnected(char **pass, struct Wifi_Info_t *info, char *save)
{
    char *source = save;
    char *find = NULL;
    while (1)
    {
        source = strstr(source, "network");
        if(source != NULL)
        {
            source = strstr(source, "ssid=\"");
            source = source + strlen("ssid=\"");
            find = strstr(source, "\"");

            int ssid_len = find - source;
            char wifi_ssid[ssid_len + 1];
            memcpy(wifi_ssid, source, ssid_len);
            wifi_ssid[ssid_len] = '\0';

            if (strcmp(info->single[WIFI_INFO_TYPE_SSID].str, wifi_ssid) == 0)
            {
                LV_LOG_USER("Get the password of %s", wifi_ssid);

                source = strstr(source, "psk=\"");
                source = source + strlen("psk=\"");
                find = strstr(source, "\"");

                int pass_len = find - source;
                char pass_ssid[pass_len + 1];
                memcpy(pass_ssid, source, pass_len);
                pass_ssid[pass_len] = '\0';
                LV_LOG_USER("The password obtained is = %s", pass_ssid);

                memcpy(pass, pass_ssid, pass_len);
                break;
            }
        }
        else break;
    }
}

/***********************************************
 *   Name：FileEventEb
 * Depict：文件按键事件响应
 ***********************************************/
static void ListEventEb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *btn_list = lv_obj_get_parent(btn);
    lv_obj_t *bg = lv_obj_get_parent(btn_list);
    struct Wifi_Info_t *info = lv_event_get_user_data(e);

    if (code == LV_EVENT_SHORT_CLICKED) 
    {
        const char *btn_text = lv_list_get_btn_text(btn_list, btn);
        LV_LOG_USER("Connect button selection = %s", btn_text);
        if (strcmp(WIFI_MANAGER_CON_DISCON, btn_text) == 0)
        {
            LV_LOG_USER("User actively disconnected");
            aw_wifid_disconnect_ap();
            // 刷新WiFi列表显示
            LvSetWifiList();
            UpdateDisplayIp();
        }
        else if (strcmp(WIFI_MANAGER_CON_CON, btn_text) == 0)
        {
            // 获取设备保存的WiFi信息
            char save_results[4*1024] = {0,};
            CustomizeFileRead(WIFI_CONNECT_INFO_PATH, save_results);
            char password[100] = {0,};
            GetWiFiPassConnected((char **)&password, info, save_results);

            LV_LOG_USER("Reconnect to new hotspot ssid--%s, pass--%s", info->single[WIFI_INFO_TYPE_SSID].str, password);
            start_connect_wifi(info->single[WIFI_INFO_TYPE_SSID].str, password);
        }
        else if (strcmp(WIFI_MANAGER_CON_REMOVE, btn_text) == 0)
        {
			LV_LOG_USER("removing ssid: %s", info->single[WIFI_INFO_TYPE_SSID].str);
			aw_wifid_remove_networks(info->single[WIFI_INFO_TYPE_SSID].str, info->single[WIFI_INFO_TYPE_SSID].str_len);
            // 刷新WiFi列表显示
            LvSetWifiList();
            UpdateDisplayIp();
        }

        lv_obj_clean(bg);
        lv_obj_del(bg);
    }
}

/***********************************************
 *   Name：WifiEventEb
 * Depict：wifi按键事件响应
 ***********************************************/
static void  WifiConnect(struct Wifi_Info_t *list_info)
{
    if (list_info->WiFi_Usage != WIFI_USE_STATE_UNUSED)
    {
        lv_obj_t *bg = lv_obj_create(WifiManager);
        lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(bg, LV_PCT(100), LV_PCT(100));
        lv_obj_add_style(bg, &style_blur_background, 0);
        lv_obj_set_style_opa(bg, LV_OPA_90, 0);
        lv_obj_add_event_cb(bg, &cancel_event_cb, LV_EVENT_SHORT_CLICKED, bg);

        lv_obj_t *btn_list = lv_list_create(bg);
        lv_obj_align(btn_list, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(btn_list, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

        if (list_info->WiFi_Usage == WIFI_USE_STATE_USED)
        {
            int list_size = (sizeof(list_used)/sizeof(list_used[0]));
            for (int i = 0; i < list_size; i++)
            {
                lv_obj_t *btn = lv_list_add_btn(btn_list, NULL, list_used[i]);
                lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_obj_add_style(btn, &style_quick_operation, 0);
                lv_obj_add_event_cb(btn, &ListEventEb, LV_EVENT_SHORT_CLICKED, list_info);
            }
        }
        else if (list_info->WiFi_Usage == WIFI_USE_STATE_IS_USING)
        {
            int list_size = (sizeof(list_using)/sizeof(list_using[0]));
            for (int i = 0; i < list_size; i++)
            {
                lv_obj_t *btn = lv_list_add_btn(btn_list, NULL, list_using[i]);
                lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_obj_add_style(btn, &style_quick_operation, 0);
                lv_obj_add_event_cb(btn, &ListEventEb, LV_EVENT_SHORT_CLICKED, list_info);
            }
        }
        return;
    }

    // 创建连接输入密码弹窗
    Connect_cont = lv_obj_create(WifiManager);
    lv_obj_add_style(Connect_cont, &style_blur_background, 0);
    lv_obj_set_size(Connect_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_opa(Connect_cont, LV_OPA_90, 0);
    lv_obj_add_event_cb(Connect_cont, &bg_event_cb, LV_EVENT_SHORT_CLICKED, NULL);

    lv_obj_t *cont = lv_obj_create(Connect_cont);
    lv_obj_set_size(cont, LV_PCT(80), LV_SIZE_CONTENT);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);

    // 创建连接标题（待连接WiFi名称）
    char ssid_str[list_info->single[WIFI_INFO_TYPE_SSID].str_len + 1];
    memset(ssid_str, 0, list_info->single[WIFI_INFO_TYPE_SSID].str_len + 1);
    memcpy(ssid_str, list_info->single[WIFI_INFO_TYPE_SSID].str, list_info->single[WIFI_INFO_TYPE_SSID].str_len);
    lv_obj_t *title = lv_label_create(cont);
    lv_obj_add_style(title, &style_wifi_list, 0);
    lv_obj_set_width(title, LV_PCT(100));
    lv_label_set_text(title, "");

    lv_obj_t *title_str = lv_label_create(title);
    lv_obj_align(title_str, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(title_str, &style_wifi_list, 0);
    lv_label_set_text(title_str, ssid_str);

    // 创建密码输入
    lv_obj_t *pass_str = lv_label_create(cont);
    lv_obj_add_style(pass_str, &style_wifi_list, 0);
    lv_obj_align_to(pass_str, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_label_set_text(pass_str, WIFI_MANAGER_JOIN_PASS);

    lv_obj_t * pwd_ta = lv_textarea_create(cont);
    lv_obj_add_style(pwd_ta, &style_wifi_list, 0);
    lv_obj_align_to(pwd_ta, pass_str, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_textarea_set_text(pwd_ta, "");
    lv_textarea_set_password_mode(pwd_ta, true);
    lv_textarea_set_one_line(pwd_ta, true);
    lv_obj_set_width(pwd_ta, LV_PCT(100));
    lv_obj_add_event_cb(pwd_ta, &keyboard_event_cb, LV_EVENT_ALL, Connect_cont);

    lv_obj_t *btn_pwd_mode = lv_btn_create(pwd_ta);
    lv_obj_align(btn_pwd_mode, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_size(btn_pwd_mode, LV_PCT(10), LV_PCT(100));
    lv_obj_add_event_cb(btn_pwd_mode, &pwd_mode_event_cb, LV_EVENT_SHORT_CLICKED, pwd_ta);

    // 创建取消按键
    lv_obj_t *btn_cancel = lv_btn_create(cont);
    lv_obj_align_to(btn_cancel, pwd_ta, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_size(btn_cancel, LV_PCT(50), LV_PCT(30));
    lv_obj_add_event_cb(btn_cancel, &cancel_event_cb, LV_EVENT_SHORT_CLICKED, Connect_cont);

    lv_obj_t *label_cancel = lv_label_create(btn_cancel);
    lv_obj_add_style(label_cancel, &style_wifi_list, 0);
    lv_obj_align(label_cancel, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label_cancel, WIFI_MANAGER_JOIN_CANCEL);

    // 创建连接按键
    lv_obj_t *btn_connect = lv_btn_create(cont);
    lv_obj_align_to(btn_connect, btn_cancel, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
    lv_obj_set_size(btn_connect, LV_PCT(50), LV_PCT(30));
    lv_obj_add_event_cb(btn_connect, &connect_event_cb, LV_EVENT_SHORT_CLICKED, list_info);

    lv_obj_t *label_connect = lv_label_create(btn_connect);
    lv_obj_add_style(label_connect, &style_wifi_list, 0);
    lv_obj_align(label_connect, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label_connect, WIFI_MANAGER_JOIN_CONNECT);

    // 创建键盘
    if (keyboard == NULL)
    {
        keyboard = lv_keyboard_create(Connect_cont);
    }
    lv_obj_set_size(keyboard,  LV_PCT(100), LV_PCT(30));
    lv_keyboard_set_textarea(keyboard, pwd_ta);
}

/***********************************************
 *   Name：AddWifiEventEb
 * Depict：添加隐藏wifi按键事件响应
 ***********************************************/
static void AddWifiEventEb(lv_event_t * e)
{
    Add_wifi_cont = lv_obj_create(WifiManager);
    lv_obj_add_style(Add_wifi_cont, &style_blur_background, 0);
    lv_obj_set_size(Add_wifi_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_add_event_cb(Add_wifi_cont, &bg_event_cb, LV_EVENT_SHORT_CLICKED, NULL);

    lv_obj_t *cont = lv_obj_create(Add_wifi_cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 0);

    // 设置标题
    lv_obj_t *add_wifi_title = lv_label_create(cont);
    lv_obj_add_style(add_wifi_title, &style_wifi_list, 0);
    lv_obj_set_width(add_wifi_title, LV_PCT(100));
    lv_label_set_text(add_wifi_title, "");

    lv_obj_t *add_wifi_title_str = lv_label_create(add_wifi_title);
    lv_obj_align(add_wifi_title_str, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(add_wifi_title_str, &style_wifi_list, 0);
    lv_label_set_text(add_wifi_title_str, WIFI_MANAGER_ADD_TITLE);

    // 输入WiFi名称
    lv_obj_t *name_str = lv_label_create(cont);
    lv_obj_add_style(name_str, &style_wifi_list, 0);
    lv_obj_align_to(name_str, add_wifi_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_label_set_text(name_str, WIFI_MANAGER_ADD_NAME);

    lv_obj_t * name_ta = lv_textarea_create(cont);
    lv_obj_add_style(name_ta, &style_wifi_list, 0);
    lv_obj_align_to(name_ta, name_str, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_textarea_set_text(name_ta, "");
    lv_textarea_set_password_mode(name_ta, false);
    lv_textarea_set_one_line(name_ta, true);
    lv_obj_set_width(name_ta, LV_PCT(100));
    lv_obj_add_event_cb(name_ta, &keyboard_event_cb, LV_EVENT_ALL, Add_wifi_cont);

    // 输入安全性
    lv_obj_t *safety_str = lv_label_create(cont);
    lv_obj_add_style(safety_str, &style_wifi_list, 0);
    lv_obj_align_to(safety_str, name_ta, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_label_set_text(safety_str, WIFI_MANAGER_ADD_SAFETY);

    int count = 0;
    char dropdown_str[100];
    int list_size = (sizeof(list_safety)/sizeof(list_safety[0]));
    for (int i = 0; i < list_size; i++)
    {
        if ((i + 1) == list_size) sprintf(&dropdown_str[count], "%s", list_safety[i]);
        else sprintf(&dropdown_str[count], "%s\n", list_safety[i]);
        count += (strlen(list_safety[i]) + 1);
    }
    lv_obj_t * safety_dropdowm = lv_dropdown_create(cont);
    lv_obj_add_style(safety_dropdowm, &style_wifi_list, 0);
    lv_obj_align_to(safety_dropdowm, safety_str, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_width(safety_dropdowm, LV_PCT(100));
    lv_dropdown_set_options(safety_dropdowm, dropdown_str);
    lv_dropdown_set_selected(safety_dropdowm, 0);

    // 输入密码
    lv_obj_t *pass_str = lv_label_create(cont);
    lv_obj_add_style(pass_str, &style_wifi_list, 0);
    lv_obj_align_to(pass_str, safety_dropdowm, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_label_set_text(pass_str, WIFI_MANAGER_ADD_PASS);

    lv_obj_t * pwd_ta = lv_textarea_create(cont);
    lv_obj_add_style(pwd_ta, &style_wifi_list, 0);
    lv_obj_align_to(pwd_ta, pass_str, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_textarea_set_text(pwd_ta, "");
    lv_textarea_set_password_mode(pwd_ta, true);
    lv_textarea_set_one_line(pwd_ta, true);
    lv_obj_set_width(pwd_ta, LV_PCT(100));
    lv_obj_add_event_cb(pwd_ta, &keyboard_event_cb, LV_EVENT_ALL, Add_wifi_cont);

    lv_obj_t *btn_pwd_mode = lv_btn_create(pwd_ta);
    lv_obj_align(btn_pwd_mode, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_size(btn_pwd_mode, LV_PCT(10), LV_PCT(100));
    lv_obj_add_event_cb(btn_pwd_mode, &pwd_mode_event_cb, LV_EVENT_SHORT_CLICKED, pwd_ta);

    // 创建键盘
    if (keyboard == NULL)
    {
        keyboard = lv_keyboard_create(Add_wifi_cont);
    }
    lv_obj_set_size(keyboard,  LV_PCT(100), LV_PCT(30));
    lv_keyboard_set_textarea(keyboard, name_ta);

    // 创建退出按键
    lv_obj_t *btn_cancel = lv_btn_create(cont);
    lv_obj_align_to(btn_cancel, pwd_ta, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_size(btn_cancel, LV_PCT(50), LV_PCT(20));
    lv_obj_add_event_cb(btn_cancel, &cancel_event_cb, LV_EVENT_SHORT_CLICKED, Add_wifi_cont);

    lv_obj_t *label_cancel = lv_label_create(btn_cancel);
    lv_obj_add_style(label_cancel, &style_wifi_list, 0);
    lv_obj_align(label_cancel, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label_cancel, WIFI_MANAGER_ADD_CANCEL);

    // 创建保存事件
    lv_obj_t *btn_connect = lv_btn_create(cont);
    lv_obj_align_to(btn_connect, btn_cancel, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
    lv_obj_set_size(btn_connect, LV_PCT(50), LV_PCT(20));
    lv_obj_add_event_cb(btn_connect, &connect_event_cb, LV_EVENT_SHORT_CLICKED, Add_wifi_cont);

    lv_obj_t *label_connect = lv_label_create(btn_connect);
    lv_obj_add_style(label_connect, &style_wifi_list, 0);
    lv_obj_align(label_connect, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label_connect, WIFI_MANAGER_ADD_SAVE);
}

/***********************************************
 *   Name：WifiEventEb
 * Depict：wifi按键事件响应
 ***********************************************/
static void WifiEventEb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    if (code == LV_EVENT_SHORT_CLICKED) 
    {
        // 查找相应按键
        for (int i = 0; i < GetWifiManagerInfo.wifi_num; i++)
        {
            // 遍历按键列表
            if (btn != GetWifiManagerInfo.info[i]->btn_hand) continue;
            // 处理按键事件
            WifiConnect(GetWifiManagerInfo.info[i]);
            break;
        }
    }
}

/***********************************************
 *   Name：ExitEventCb
 * Depict：退出事件响应
 ***********************************************/
static void ExitEventCb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn_manager = lv_event_get_user_data(e);

    if (code == LV_EVENT_SHORT_CLICKED) 
    {
        WifiManageResetInit();
        lv_obj_clean(btn_manager);
        lv_obj_del(btn_manager);
    }

    lv_obj_t *scr = lv_scr_act();
    LvCreateWifiManager(scr);
}

/***********************************************
 *   Name：UpdateDisplayIp
 * Depict：更新显示IP
 ***********************************************/
static void UpdateDisplayIp(void)
{
    struct wifi_status status = {
		.state = STATE_UNKNOWN,
		.ssid = {'\0'},
	};

    int ret = aw_wifid_get_status(&status);
    if (ret >= 0)
    {
        if ((status.state != LastWifiStatus.state) || (strcmp(LastWifiStatus.ssid, status.ssid) != 0))
        {
            LV_LOG_USER("wifi state change, state: %d, ssid %s", status.state, status.ssid);
            if (status.state == NETWORK_CONNECTED)
            {
                LvSetWifiList();
                connection_status info;
                aw_wifid_get_connect_info(&info);
                LV_LOG_USER("get connect ip = %s", info.ip_address);
                lv_label_set_text(LabelIpAddr, info.ip_address);
            }
            else
            {
                LvSetWifiList();
                lv_label_set_text(LabelIpAddr, "");
            }
            memcpy(&LastWifiStatus, &status, sizeof(struct wifi_status));
        }  
    }
}

/***********************************************
 *   Name：WifiStateInquireTimer
 * Depict：WiFi状态查询定时器
 ***********************************************/
static void WifiStateInquireTimerCb(lv_timer_t * timer)
{
    UpdateDisplayIp();
}

/***********************************************
 *   Name：WifiGestureEventCb
 * Depict：WiFi手势响应事件
 ***********************************************/
static void WifiGestureEventCb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) 
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_BOTTOM)
        {
            LV_LOG_USER("Refresh the page and search for WiFi again");
            LvSetWifiList();
            UpdateDisplayIp();
        }
    }
}

/***********************************************
 *   Name：LvCreateWifiManager
 * Depict：创建WiFi连接管理器
 ***********************************************/
lv_obj_t *LvCreateWifiManager(lv_obj_t *parent)
{
    WifiManagerInit();
    // 创建文件管理器容器
    WifiManager = lv_obj_create(parent);
    lv_obj_set_size(WifiManager, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(WifiManager, &style_blur_background, 0);

    lv_obj_t *WifiInfoCont = lv_obj_create(WifiManager);
    lv_obj_set_flex_flow(WifiInfoCont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_size(WifiInfoCont, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(WifiInfoCont, &style_wifi_manager, 0);

    lv_obj_t *label_title = lv_label_create(WifiInfoCont);
    lv_obj_add_style(label_title, &style_wifi_list, 0);
    lv_obj_set_width(label_title, LV_PCT(100));
    lv_label_set_text(label_title, "");

    lv_obj_t *label_title_str = lv_label_create(label_title);
    lv_obj_align(label_title_str, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(label_title_str, &style_wifi_list, 0);
    lv_label_set_text(label_title_str, WIFI_MANAGER_TITLE);

    lv_obj_t *btn_exit = lv_btn_create(label_title);
    lv_obj_align(btn_exit, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_size(btn_exit, LV_PCT(10), LV_PCT(100));
    lv_obj_add_event_cb(btn_exit, &ExitEventCb, LV_EVENT_SHORT_CLICKED, WifiManager);

    lv_obj_t *btn_exit_str = lv_label_create(btn_exit);
    lv_obj_align(btn_exit_str, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(btn_exit_str, WIFI_MANAGER_EXIT);

    lv_obj_t *label_ip_str = lv_label_create(WifiInfoCont);
    lv_obj_add_style(label_ip_str, &style_wifi_list, 0);
    lv_label_set_text(label_ip_str, WIFI_MANAGER_IP_STR);

    LabelIpAddr = lv_label_create(WifiInfoCont);
    lv_obj_add_style(LabelIpAddr, &style_wifi_list, 0);
    lv_label_set_text(LabelIpAddr, "");
    // 创建WiFi状态查询定时器，
    WifiStateInquireTimer = lv_timer_create(WifiStateInquireTimerCb, WIFI_STATE_INQUIRE_TIME, NULL);
    
    // 创建文件标题容器
    WifiList = lv_obj_create(WifiInfoCont);
    lv_obj_set_flex_flow(WifiList, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_size(WifiList, LV_PCT(100), LV_PCT(90));
    lv_obj_add_style(WifiList, &style_wifi_manager_list, 0);
    lv_obj_set_flex_grow(WifiList, 1);
    lv_obj_add_flag(WifiList, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

    // 实现下拉刷新
    lv_obj_add_event_cb(WifiList, WifiGestureEventCb, LV_EVENT_GESTURE, NULL);
    lv_obj_clear_flag(WifiList, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(WifiList, LV_OBJ_FLAG_CLICKABLE);

    LV_LOG_USER("create wifi manager, show info list");
    LvSetWifiList();
    UpdateDisplayIp();
    return 0;
}

#endif
