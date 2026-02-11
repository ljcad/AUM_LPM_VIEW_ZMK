#ifndef UTIL_H
#define UTIL_H

#include <lvgl.h>
#include "status.h"

#define CANVAS_SIZE 72

void rotate_canvas(lv_obj_t *canvas, lv_color_t cbuf[]);
void draw_battery(lv_obj_t *canvas, const struct status_state *state);

// LVGL 8.x 原生声明（_dsc后缀）
void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align);
void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color);
void init_line_dsc(lv_draw_line_dsc_t *line_dsc, lv_color_t color, uint8_t width);
void init_arc_dsc(lv_draw_arc_dsc_t *arc_dsc, lv_color_t color, uint8_t width);

#endif // UTIL_H
