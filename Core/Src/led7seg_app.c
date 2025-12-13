/*
 * led7seg_app.c
 *
 *  Created on: Nov 8, 2025
 *      Author: Admin
 */


#include "led7seg_app.h"
#include "led_7seg.h"
#include "spi.h"

static const uint8_t POS_MAP[4] = { 3, 2, 1, 0 };


static inline void set_digit_or_off(int pos, int value, uint8_t show_dot) {
    if (value < 0) {
    	led7_SetDigit(0x00, pos, 0);
        led_Off((uint8_t)pos);
    } else {
        led_On((uint8_t)pos);
        led7_SetDigit(value, pos, show_dot);
    }
}

void led7_setup(void) {
    led7_init();
    led7_SetColon(0);
    led7_show_score(0);
}

void led7_show_score(uint16_t score) {
    if (score > 9999) score %= 10000;

    int d0 = (int)(score % 10);            // units
    int d1 = (int)((score / 10)  % 10);    // tens
    int d2 = (int)((score / 100) % 10);    // hundreds
    int d3 = (int)((score / 1000) % 10);   // thousands

    if (score >= 1000) {
        set_digit_or_off(POS_MAP[3], d3, 0);   // thousands
        set_digit_or_off(POS_MAP[2], d2, 0);   // hundreds
        set_digit_or_off(POS_MAP[1], d1, 0);   // tens
        set_digit_or_off(POS_MAP[0], d0, 0);   // units
    } else if (score >= 100) {
        set_digit_or_off(POS_MAP[3], -1, 0);   // thousands OFF
        set_digit_or_off(POS_MAP[2], d2, 0);   // hundreds
        set_digit_or_off(POS_MAP[1], d1, 0);   // tens
        set_digit_or_off(POS_MAP[0], d0, 0);   // units
    } else if (score >= 10) {
        set_digit_or_off(POS_MAP[3], -1, 0);   // thousands OFF
        set_digit_or_off(POS_MAP[2], -1, 0);   // hundreds OFF
        set_digit_or_off(POS_MAP[1], d1, 0);   // tens
        set_digit_or_off(POS_MAP[0], d0, 0);   // units
    } else {
        // 1 chữ số: chỉ hiện hàng đơn vị, các hàng khác tắt
        set_digit_or_off(POS_MAP[3], -1, 0);   // thousands OFF
        set_digit_or_off(POS_MAP[2], -1, 0);   // hundreds OFF
        set_digit_or_off(POS_MAP[1], -1, 0);   // tens OFF
        set_digit_or_off(POS_MAP[0], d0, 0);   // units
    }
}
