/*
 * button.c
 *
 *  Created on: Sep 25, 2023
 *      Author: HaHuyen
 */
#include "button.h"

uint16_t button_count[16];
uint16_t spi_button = 0x0000;
#define DEBOUNCE_TICKS 3   // new

void button_init(){
	HAL_GPIO_WritePin(BTN_LOAD_GPIO_Port, BTN_LOAD_Pin, 1);
}

void button_Scan(){
	  HAL_GPIO_WritePin(BTN_LOAD_GPIO_Port, BTN_LOAD_Pin, 0);
	  HAL_GPIO_WritePin(BTN_LOAD_GPIO_Port, BTN_LOAD_Pin, 1);
	  HAL_SPI_Receive(&hspi1, (void*)&spi_button, 2, 10);
	  int button_index = 0;
	  uint16_t mask = 0x8000;
	  for(int i = 0; i < 16; i++){
		  if(i >= 0 && i <= 3){
			  button_index = i + 4; // do theo schematic thì spi gửi ko giống như button trên mạch
		  } else if (i >= 4 && i <= 7){  //-> cần convert lại cho nó đúng với thứ tự mình mún
			  button_index = 7 - i;
		  } else if (i >= 8 && i <= 11){
			  button_index = i + 4;
		  } else {
			  button_index = 23 - i;
		  }
		  if(spi_button & mask) button_count[button_index] = 0;
		  else button_count[button_index]++;
//		  if(spi_button & mask) button_count[i] = 0;
//		  else button_count[i]++;
		  mask = mask >> 1;
	  }
}

uint8_t button_pressed_edge(uint8_t idx){ //new
    static uint8_t latched[16] = {0};
    if (button_count[idx] >= DEBOUNCE_TICKS){
        if (!latched[idx]) { latched[idx] = 1; return 1; }
    } else {
        latched[idx] = 0;
    }
    return 0;
}


