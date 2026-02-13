/*
 * Copyright (c) 2025 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include "status.h"
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/wpm.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool profiles_connected[NICEVIEW_PROFILE_COUNT];
    bool profiles_bonded[NICEVIEW_PROFILE_COUNT];
};

struct layer_status_state {
    zmk_keymap_layer_index_t index;
    const char *label;
};

struct wpm_status_state {
    uint8_t wpm;
};

static void draw_top(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);
    lv_draw_label_dsc_t label_dsc_wpm;
    init_label_dsc(&label_dsc_wpm, LVGL_FOREGROUND, &lv_font_unscii_8, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_rect_dsc_t rect_white_dsc;
    init_rect_dsc(&rect_white_dsc, LVGL_FOREGROUND);
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 1);

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
    draw_battery(canvas, state);

    canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_dsc,
                     state->connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);

    // Draw WPM
    canvas_draw_rect(canvas, 0, 21, 70, 32, &rect_white_dsc);
    canvas_draw_rect(canvas, 1, 22, 66, 30, &rect_black_dsc);

    uint8_t current_wpm = 0;
#if IS_ENABLED(CONFIG_ZMK_WPM)
    current_wpm = zmk_wpm_get_state();
#endif

    char wpm_text[6] = {};
    snprintf(wpm_text, sizeof(wpm_text), "%d", current_wpm);
    canvas_draw_text(canvas, 42, 42, 24, &label_dsc_wpm, wpm_text);

    lv_point_t points[10];
    for (int i = 0; i < 10; i++) {
        points[i].x = 2 + i * 7;
        points[i].y = 50; 
    }
    canvas_draw_line(canvas, points, 10, &line_dsc);

    rotate_canvas(canvas);
}

static void draw_middle(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_rect_dsc_t rect_white_dsc;
    init_rect_dsc(&rect_white_dsc, LVGL_FOREGROUND);
    lv_draw_arc_dsc_t arc_dsc;
    init_arc_dsc(&arc_dsc, LVGL_FOREGROUND, 2);
    lv_draw_arc_dsc_t arc_dsc_filled;
    init_arc_dsc(&arc_dsc_filled, LVGL_FOREGROUND, 9);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_CENTER);
    lv_draw_label_dsc_t label_dsc_black;
    init_label_dsc(&label_dsc_black, LVGL_BACKGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_CENTER);

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);

    int circle_offsets[NICEVIEW_PROFILE_COUNT][2] = {
        {13, 13}, {55, 13}, {34, 34}, {13, 55}, {55, 55},
    };

    for (int i = 0; i < NICEVIEW_PROFILE_COUNT; i++) {
        bool selected = false;
        bool connected = false;
        bool open = true;

#if IS_ENABLED(CONFIG_ZMK_BLE)
        selected = (i == zmk_ble_active_profile_index());
        connected = zmk_ble_profile_is_connected(i);
        open = zmk_ble_profile_is_open(i);
#endif

        if (connected) {
            canvas_draw_arc(canvas, circle_offsets[i][0], circle_offsets[i][1], 13, 0, 360, &arc_dsc);
        } else if (!open) {
            const int segments = 8;
            for (int j = 0; j < segments; ++j)
                canvas_draw_arc(canvas, circle_offsets[i][0], circle_offsets[i][1], 13,
                                360. / segments * j + 10, 360. / segments * (j + 1) - 10, &arc_dsc);
        }

        if (selected) {
            canvas_draw_arc(canvas, circle_offsets[i][0], circle_offsets[i][1], 9, 0, 359, &arc_dsc_filled);
        }

        char label[2];
        snprintf(label, sizeof(label), "%d", i + 1);
        canvas_draw_text(canvas, circle_offsets[i][0] - 8, circle_offsets[i][1] - 10, 16,
                         (selected ? &label_dsc_black : &label_dsc), label);
    }

    rotate_canvas(canvas);
}

static void draw_bottom(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 2);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_CENTER);

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);

    uint8_t active_layer_index = 0;
    const char *layer_name = NULL;

#if IS_ENABLED(CONFIG_ZMK_KEYMAP)
    active_layer_index = zmk_keymap_highest_layer_active();
    layer_name = zmk_keymap_layer_name(active_layer_index);
#endif

    if (layer_name == NULL || strlen(layer_name) == 0) {
        char text[12] = {};
        snprintf(text, sizeof(text), "LAYER %i", active_layer_index);
        canvas_draw_text(canvas, 0, 0, 72, &label_dsc, text);
    } else {
        canvas_draw_text(canvas, 0, 0, 72, &label_dsc, layer_name);
    }

    rotate_canvas(canvas);
}

static void set_battery_status(struct zmk_widget_status *widget, struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif
    widget->state.battery = state.level;
    draw_top(widget->obj, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_status_state){
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif
    };
}

// 修改监听器名称以避免重复定义
ZMK_DISPLAY_WIDGET_LISTENER(lpm_widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)
ZMK_SUBSCRIPTION(lpm_widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(lpm_widget_battery_status, zmk_usb_conn_state_changed);
#endif

static void set_output_status(struct zmk_widget_status *widget, const struct output_status_state *state) {
    draw_top(widget->obj, &widget->state);
    draw_middle(widget->obj, &widget->state);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}

static struct output_status_state output_status_get_state(const zmk_event_t *_eh) {
    struct output_status_state state = {0};
#if IS_ENABLED(CONFIG_ZMK_BLE)
    state.selected_endpoint = zmk_endpoint_get_selected();
    state.active_profile_index = zmk_ble_active_profile_index();
    state.active_profile_connected = zmk_ble_active_profile_is_connected();
    state.active_profile_bonded = !zmk_ble_profile_is_open(zmk_ble_active_profile_index());
    for (int i = 0; i < MIN(NICEVIEW_PROFILE_COUNT, ZMK_BLE_PROFILE_COUNT); ++i) {
        state.profiles_connected[i] = zmk_ble_profile_is_connected(i);
        state.profiles_bonded[i] = !zmk_ble_profile_is_open(i);
    }
#endif
    return state;
}

ZMK_DISPLAY_WIDGET_LISTENER(lpm_widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(lpm_widget_output_status, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(lpm_widget_output_status, zmk_usb_conn_state_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(lpm_widget_output_status, zmk_ble_active_profile_changed);
#endif

static void set_layer_status(struct zmk_widget_status *widget, struct layer_status_state state) {
    draw_bottom(widget->obj, &widget->state);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    zmk_keymap_layer_index_t index = 0;
#if IS_ENABLED(CONFIG_ZMK_KEYMAP)
    index = zmk_keymap_highest_layer_active();
#endif
    return (struct layer_status_state){
        .index = index, 
        .label = zmk_keymap_layer_name(index)
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(lpm_widget_layer_status, struct layer_status_state, 
                            layer_status_update_cb, layer_status_get_state)
#if IS_ENABLED(CONFIG_ZMK_KEYMAP)
ZMK_SUBSCRIPTION(lpm_widget_layer_status, zmk_layer_state_changed);
#endif

static void set_wpm_status(struct zmk_widget_status *widget, struct wpm_status_state state) {
    draw_top(widget->obj, &widget->state);
}

static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_wpm_status(widget, state); }
}

struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    uint8_t wpm = 0;
#if IS_ENABLED(CONFIG_ZMK_WPM)
    wpm = zmk_wpm_get_state();
#endif
    return (struct wpm_status_state){.wpm = wpm};
}

ZMK_DISPLAY_WIDGET_LISTENER(lpm_widget_wpm_status, struct wpm_status_state, 
                            wpm_status_update_cb, wpm_status_get_state)
#if IS_ENABLED(CONFIG_ZMK_WPM)
ZMK_SUBSCRIPTION(lpm_widget_wpm_status, zmk_wpm_state_changed);
#endif

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 144, 72);
    
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);
    
    lv_obj_t *middle = lv_canvas_create(widget->obj);
    lv_obj_align(middle, LV_ALIGN_TOP_LEFT, 58, 0);
    lv_canvas_set_buffer(middle, widget->cbuf2, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);
    
    lv_obj_t *bottom = lv_canvas_create(widget->obj);
    lv_obj_align(bottom, LV_ALIGN_TOP_LEFT, 130, 0);
    lv_canvas_set_buffer(bottom, widget->cbuf3, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);

    sys_slist_append(&widgets, &widget->node);
    
    lpm_widget_battery_status_init();
    lpm_widget_output_status_init();
    lpm_widget_layer_status_init();
    lpm_widget_wpm_status_init();

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
