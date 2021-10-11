/**
 * @file cxsw_waiting_panel.c
 * @author Feng
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "cxsw_waiting_panel.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_style_t style_blur_background;
static lv_style_t style_prompt_text;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/***********************************************
 *   Name：LvCreateWaitingPanel
 * Depict：创建等待面板
 ***********************************************/
lv_obj_t *LvCreateWaitingPanel(lv_obj_t *parent)
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
    lv_style_set_opa(&style_blur_background, LV_OPA_90);

    // 设置列表字体样式
    lv_style_init(&style_prompt_text);
    lv_style_set_text_font(&style_prompt_text, &lv_font_montserrat_30);

    lv_obj_t *WaitingPanel = lv_obj_create(parent);
    lv_obj_align(WaitingPanel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(WaitingPanel, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(WaitingPanel, &style_blur_background, 0);
    lv_obj_set_style_opa(WaitingPanel, LV_OPA_90, 0);

    lv_obj_t *WaitingPanelText = lv_label_create(WaitingPanel);
    lv_obj_align(WaitingPanelText, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(WaitingPanelText, &style_prompt_text, 0);
    lv_label_set_text(WaitingPanelText, WAITING_PANEL_TEXT);

    // 重绘屏幕，防止某些操作会阻碍UI刷新，UI以 “LV_DISP_DEF_REFR_PERIOD” 定义事件周期刷新
    lv_refr_now(NULL);
    LV_LOG_USER("Please wait for the operation to complete");
    return WaitingPanel;
}

/***********************************************
 *   Name：LvDeleteWaitingPanel
 * Depict：删除等待面板
 ***********************************************/
lv_obj_t *LvDeleteWaitingPanel(lv_obj_t *parent)
{
    // 复位样式
    lv_style_reset(&style_blur_background);
    lv_style_reset(&style_prompt_text);

    LV_LOG_USER("The operation is complete, delete the waiting panel");
    lv_obj_clean(parent);
    lv_obj_del(parent);
    return 0;
}
