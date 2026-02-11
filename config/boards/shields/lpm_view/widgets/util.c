/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>
#include "util.h"

LV_IMG_DECLARE(bolt);

void rotate_canvas(lv_obj_t *canvas, lv_color_t cbuf[]) {
    static lv_color_t cbuf_tmp[CANVAS_SIZE * CANVAS_SIZE];
    memcpy(cbuf_tmp, cbuf, sizeof(cbuf_tmp));
    lv_img_dsc_t img;
    img.data = (void *)cbuf_tmp;
    img.header.format = LV_IMAGE_FORMAT_TRUE_COLOR;  // LVGL 9.x 标准宏
    img.header.w = CANVAS_SIZE;
    img.header.h = CANVAS_SIZE;

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
    // LVGL 9.x 调整transform参数（角度单位为度，而非1/10度）
    lv_canvas_transform(canvas, &img, -90, LV_IMG_ZOOM_NONE, 0, 0, CANVAS_SIZE / 2,
                        CANVAS_SIZE / 2 - 1, false);
}

void draw_battery(lv_obj_t *canvas, const struct status_state *state) {
    // LVGL 9.x 使用lv_draw_rect_params_t替代lv_draw_rect_dsc_t
    lv_draw_rect_params_t rect_black_params;
    lv_draw_rect_params_init(&rect_black_params);
    rect_black_params.bg_color = LVGL_BACKGROUND;
    
    lv_draw_rect_params_t rect_white_params;
    lv_draw_rect_params_init(&rect_white_params);
    rect_white_params.bg_color = LVGL_FOREGROUND;

    // LVGL 9.x 使用lv_canvas_draw_rect_params替代lv_canvas_draw_rect
    lv_canvas_draw_rect_params(canvas, 0, 2, 29, 12, &rect_white_params);
    lv_canvas_draw_rect_params(canvas, 1, 3, 27, 10, &rect_black_params);
    lv_canvas_draw_rect_params(canvas, 2, 4, (state->battery + 2) / 4, 8, &rect_white_params);
    lv_canvas_draw_rect_params(canvas, 30, 5, 3, 6, &rect_white_params);
    lv_canvas_draw_rect_params(canvas, 31, 6, 1, 4, &rect_black_params);

    if (state->charging) {
        lv_draw_img_params_t img_params;
        lv_draw_img_params_init(&img_params);
        lv_canvas_draw_img(canvas, 9, -1, &bolt, &img_params);
    }
}

// LVGL 9.x 适配：参数结构体名从_dsc改为_params
void init_label_dsc(lv_draw_label_params_t *label_params, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align) {
    lv_draw_label_params_init(label_params);
    label_params->color = color;
    label_params->font = font;
    label_params->align = align;
}

void init_rect_dsc(lv_draw_rect_params_t *rect_params, lv_color_t bg_color) {
    lv_draw_rect_params_init(rect_params);
    rect_params->bg_color = bg_color;
}

void init_line_dsc(lv_draw_line_params_t *line_params, lv_color_t color, uint8_t width) {
    lv_draw_line_params_init(line_params);
    line_params->color = color;
    line_params->width = width;
}

void init_arc_dsc(lv_draw_arc_params_t *arc_params, lv_color_t color, uint8_t width) {
    lv_draw_arc_params_init(arc_params);
    arc_params->color = color;
    arc_params->width = width;
}
