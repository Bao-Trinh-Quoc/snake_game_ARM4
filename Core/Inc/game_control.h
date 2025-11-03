#ifndef INC_GAME_CONTROL_H_
#define INC_GAME_CONTROL_H_

#include "software_timer.h"
#include "lcd.h"
#include "touch.h"
#include "game_display.h"


#define DIRECTION_BTN_X 50
#define DIRECTION_BTN_Y 160
#define DIRECTION_BTN_SIZE 40

void gameFSM(void);

uint8_t isStartScreenTouched(void);

uint8_t isRetryButtonTouched(void);

void updateSnakeDirection(void);

uint8_t isButtonUp(void);

uint8_t isButtonDown(void);

uint8_t isButtonLeft(void);

uint8_t isButtonRight(void);

uint8_t isHomeButtonTouched(void);
uint8_t isPauseButtonTouched(void);

#endif /* INC_GAME_CONTROL_H_ */
