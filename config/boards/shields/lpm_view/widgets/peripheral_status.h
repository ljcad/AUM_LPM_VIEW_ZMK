/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */
#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/endpoints.h> // 必须包含，因为用到了 zmk_endpoint_instance 结构体
#include "util.h"

// 补全缺失的结构体定义
struct status_state {
    uint8_t battery;
    bool charging;
    struct zmk_endpoint_instance selected_endpoint;
    uint8_t active_profile_index;
    uint8_t layer_index;
    const char *layer_label;
};

struct zmk_widget_status {
    sys_snode_t node;
    lv_obj_t *obj;
    uint8_t cbuf[CANVAS_BUF_SIZE];
    uint8_t cbuf2[CANVAS_BUF_SIZE];
    uint8_t cbuf3[CANVAS_BUF_SIZE];
    struct status_state state;
};

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget);
