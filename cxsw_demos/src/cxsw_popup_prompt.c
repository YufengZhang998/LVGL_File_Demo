/**
 * @file cxsw_popup_prompt.c
 * @author Feng
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "cxsw_popup_prompt.h"

/*********************
 *      DEFINES
 *********************/
#define LV_MSGBOX_FLAG_AUTO_PARENT  LV_OBJ_FLAG_WIDGET_1        /*Mark that the parent was automatically created*/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void QuickPopupInit(void);
static void QuickPopupResetInit(void);
static void msgbox_close_click_event_cb(lv_event_t * e);
static lv_obj_t * LvCreatePopup(lv_obj_t * parent, const char * title, const char * txt);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_style_t style_blur_background;
static lv_style_t style_Popup_title;
static lv_style_t style_Popup_txt;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
/***********************************************
 *   Name：QuickPopupInit
 * Depict：弹窗初始化
 ***********************************************/
static void QuickPopupInit(void)
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

    // 设置快捷操作样式
    lv_style_init(&style_Popup_title);
    lv_style_set_text_font(&style_Popup_title, &lv_font_montserrat_24);

    static lv_color_t title_background_color = {
        .ch.red = (POPUP_TITLE_BG_COLOR >> 16) & 0xFF,
        .ch.green = (POPUP_TITLE_BG_COLOR >> 8) & 0xFF,
        .ch.blue = (POPUP_TITLE_BG_COLOR) & 0xFF,
    };
    lv_style_set_bg_color(&style_Popup_title, title_background_color);
    lv_style_set_bg_grad_color(&style_Popup_title, title_background_color);
    lv_style_set_bg_opa(&style_Popup_title, LV_OPA_80);

    lv_style_init(&style_Popup_txt);
    lv_style_set_text_font(&style_Popup_txt, &lv_font_montserrat_20);
}

/***********************************************
 *   Name：QuickPopupResetInit
 * Depict：弹窗复位初始化
 ***********************************************/
static void QuickPopupResetInit(void)
{
    // 复位快捷操作样式
    lv_style_reset(&style_blur_background);
    lv_style_reset(&style_Popup_title);
    lv_style_reset(&style_Popup_txt);
}

/***********************************************
 *   Name：msgbox_close_click_event_cb
 * Depict：弹窗关闭
 ***********************************************/
static void msgbox_close_click_event_cb(lv_event_t * e)
{
    lv_obj_t *bg_cont = lv_event_get_user_data(e);
    QuickPopupResetInit();
    lv_msgbox_close(bg_cont);
}

/***********************************************
 *   Name：LvCreatePopup
 * Depict：创建弹窗提示
 ***********************************************/
static lv_obj_t * LvCreatePopup(lv_obj_t * parent, const char * title, const char * txt)
{
    LV_LOG_INFO("begin")
    bool auto_parent = false;
    if(parent == NULL) {
        auto_parent = true;
        parent = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(parent);
        lv_obj_set_style_bg_color(parent, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_set_style_bg_opa(parent, LV_OPA_50, 0);
        lv_obj_set_size(parent, LV_PCT(100), LV_PCT(100));
    }

    lv_obj_t *bg_cont = lv_obj_create(parent);
    lv_obj_add_style(bg_cont, &style_blur_background, 0);
    lv_obj_set_size(bg_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_opa(bg_cont, LV_OPA_90, 0);

    lv_obj_t * mbox = lv_obj_class_create_obj(&lv_msgbox_class, bg_cont);
    lv_obj_class_init_obj(mbox);
    LV_ASSERT_MALLOC(mbox);
    if(mbox == NULL) return NULL;

    if(auto_parent) lv_obj_add_flag(mbox, LV_MSGBOX_FLAG_AUTO_PARENT);

    lv_obj_set_size(mbox, LV_DPI_DEF * 2, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(mbox, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(mbox, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * label;
    label = lv_label_create(mbox);
    lv_obj_add_style(label, &style_Popup_title, 0);
    lv_label_set_text(label, title);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_flex_grow(label, 1);

    lv_obj_t * close_btn = lv_btn_create(mbox);
    lv_obj_set_ext_click_area(close_btn, LV_DPX(10));
    lv_obj_add_event_cb(close_btn, msgbox_close_click_event_cb, LV_EVENT_CLICKED, bg_cont);
    label = lv_label_create(close_btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE);
    const lv_font_t * font = lv_obj_get_style_text_font(close_btn, LV_PART_MAIN);
    lv_coord_t close_btn_size = lv_font_get_line_height(font) + LV_DPX(10);
    lv_obj_set_size(close_btn, close_btn_size, close_btn_size);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    label = lv_label_create(mbox);
    lv_label_set_text(label, txt);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(100));

    return mbox;
}

/***********************************************
 *   Name：LvCreateErrorPopup
 * Depict：创建错误弹窗提示
 ***********************************************/
lv_obj_t * LvCreateErrorPopup(const char * txt)
{
    QuickPopupInit();
    lv_obj_t *scr = lv_scr_act();
    return LvCreatePopup(scr, POPUP_TITLE_ERROE, txt);
}

/***********************************************
 *   Name：LvCreateWarnPopup
 * Depict：创建警告弹窗提示
 ***********************************************/
lv_obj_t * LvCreateWarnPopup(const char * txt)
{
    QuickPopupInit();
    lv_obj_t *scr = lv_scr_act();
    return LvCreatePopup(scr, POPUP_TITLE_WARN, txt);
}
