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

void led7_show_score_dual(uint16_t score, uint16_t high) {
    // Mỗi bên tối đa 2 chữ số để vừa 4 LED
    int s0 = (int)(score % 10);           // đơn vị (→ led 0)
    int s1 = (int)((score / 10) % 10);    // chục   (→ led 1)

    int h0 = (int)(high % 10);            // đơn vị (→ led 2)
    int h1 = (int)((high / 10) % 10);     // chục   (→ led 3)

    // HIGH (trái) – “ngược lại”: đơn vị ở led 2, chục ở led 3
    // POS_MAP[2] là led trái-giữa, POS_MAP[3] là led trái nhất
    if (high >= 10) {
        set_digit_or_off(POS_MAP[3], h1, 0); // chục high → led 3
        set_digit_or_off(POS_MAP[2], h0, 0); // đơn vị high → led 2
    } else if (high >= 1) {
        set_digit_or_off(POS_MAP[3], -1, 0); // tắt led 3
        set_digit_or_off(POS_MAP[2], h0, 0); // đơn vị high → led 2
    } else {
        // high = 0: chỉ bật led 2 = 0 nếu muốn thấy “0”; nếu muốn tắt hẳn thì set -1
        set_digit_or_off(POS_MAP[3], -1, 0);
        set_digit_or_off(POS_MAP[2], 0,  0);
    }

    // SCORE (phải): đơn vị → led 0, chục → led 1
    if (score >= 10) {
        set_digit_or_off(POS_MAP[1], s1, 0); // chục score → led 1
        set_digit_or_off(POS_MAP[0], s0, 0); // đơn vị score → led 0
    } else {
        set_digit_or_off(POS_MAP[1], -1, 0); // tắt led 1
        set_digit_or_off(POS_MAP[0], s0, 0); // đơn vị score → led 0
    }
    led7_SetColon(1);
}
