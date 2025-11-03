/*
 * game_control.c
 *
 *  Created on: Oct 21, 2025
 *      Author: DELL
 */

#include "game_control.h"

typedef enum {
	GAME_INIT, GAME_START, GAME_PLAY, GAME_OVER, GAME_COLOR_SELECT, GAME_PAUSE
} GameState;

static GameState currentState = GAME_START;

typedef struct {
    uint16_t xStart, yStart, xEnd, yEnd;
    uint8_t isPressed;
} ControlButton;

static ControlButton controlButtons[4];
static uint16_t score = 0;
static uint8_t startScreenDrawn = 0;
static uint8_t gameUIRendered = 0;

void gameFSM(void) {
    switch (currentState) {
        case GAME_START:
        	if (!startScreenDrawn) {
        	        displayStartScreen();
        	        startScreenDrawn = 1;
        	    }
            uint16_t c = startScreenHandleColorTouch();
                    if (c != 0) {
                        snake.color = c;
                        displayStartScreen();
                    }
            if (isStartScreenTouched()) {
                currentState = GAME_PLAY;
                lcd_Fill(0, 0, 240, 320, BLACK);
                initializeGame();
                renderScreen();
                setTimer_button(5);
                setTimer_snake(300);
            }
            break;
        case GAME_PLAY:
        	if (!gameUIRendered) {
        	        initializeButtons();
        	        gameUIRendered = 1;
        	    }
            if (button_read_flag) {
                setTimer_button(5);
                handleInput();
            }
            if (snake_move_flag) {
                HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin);

                int16_t nextX = snake.headX;
                int16_t nextY = snake.headY;

                switch (snakeDirection) {
                    case UP:    nextY--; break;
                    case DOWN:  nextY++; break;
                    case LEFT:  nextX--; break;
                    case RIGHT: nextX++; break;
                }

                if (nextX < 0)                nextX = GRID_ROWS - 1;
                else if (nextX >= GRID_ROWS) nextX = 0;
                if (nextY < 0)                nextY = GRID_COLS - 1;
                else if (nextY >= GRID_COLS) nextY = 0;

                if (gameGrid[nextX][nextY] == 1) {
                    currentState = GAME_OVER;
                }
                else if (gameGrid[nextX][nextY] == 2) {
                    score++;
                    advanceSnakeHeadTo(nextX, nextY);
                    generateFruit();
                }
                else {
                    advanceSnakeHeadTo(nextX, nextY);
                    removeSnakeTail();
                }

                renderScreen();
                setTimer_snake(300);
            }
               if (isHomeButtonTouched()) {
                   currentState = GAME_START;
                   startScreenDrawn = 0;
                   score = 0;
                   gameUIRendered = 0;
                   lcd_Fill(0, 0, 240, 320, BLACK);
                   break;
               }

               if (isPauseButtonTouched()) {
                   currentState = GAME_PAUSE;
                   break;
               }

               if (button_read_flag) {
                   setTimer_button(5);
                   handleInput();
               }
            break;
        case GAME_PAUSE:
            lcd_ShowStr(80, 150, "PAUSED", YELLOW, BLACK, 24, 1);

            if (isPauseButtonTouched()) {
                currentState = GAME_PLAY;
            }
            if (isHomeButtonTouched()) {
                currentState = GAME_START;
                startScreenDrawn = 0;
            }
            break;
        case GAME_OVER:
                startScreenDrawn = 0;
                gameUIRendered = 0;
                currentState = GAME_START;
            break;
    }
}

void initializeControlButtons(void) {
    controlButtons[0] = (ControlButton){.xStart = DIRECTION_BTN_X + DIRECTION_BTN_SIZE + 10, .yStart = DIRECTION_BTN_Y + 10 + DIRECTION_BTN_SIZE, .xEnd = DIRECTION_BTN_X + 2 * DIRECTION_BTN_SIZE + 10, .yEnd = DIRECTION_BTN_Y + DIRECTION_BTN_SIZE*2 + 10 , .isPressed = 0};
    controlButtons[1] = (ControlButton){.xStart = DIRECTION_BTN_X + DIRECTION_BTN_SIZE + 10, .yStart = DIRECTION_BTN_Y + 2 * DIRECTION_BTN_SIZE + 20, .xEnd = DIRECTION_BTN_X + 2 * DIRECTION_BTN_SIZE + 10, .yEnd = DIRECTION_BTN_Y + 3 * DIRECTION_BTN_SIZE + 20, .isPressed = 0};
    controlButtons[2] = (ControlButton){.xStart = DIRECTION_BTN_X, .yStart = DIRECTION_BTN_Y + 2 * DIRECTION_BTN_SIZE + 20, .xEnd = DIRECTION_BTN_X + DIRECTION_BTN_SIZE,.yEnd = DIRECTION_BTN_Y + 3 * DIRECTION_BTN_SIZE + 20, .isPressed = 0};
    controlButtons[3] = (ControlButton){.xStart = DIRECTION_BTN_X + 2 * DIRECTION_BTN_SIZE + 20, .yStart = DIRECTION_BTN_Y + 2 * DIRECTION_BTN_SIZE + 20, .xEnd = DIRECTION_BTN_X + 3 * DIRECTION_BTN_SIZE + 20, .yEnd = DIRECTION_BTN_Y + 3 * DIRECTION_BTN_SIZE + 20, .isPressed = 0};
    for (int i = 0; i < 4; i++) {
    lcd_Fill(controlButtons[i].xStart, controlButtons[i].yStart,
             controlButtons[i].xEnd,   controlButtons[i].yEnd, WHITE);
    }
}

uint8_t isHomeButtonTouched(void) {
    return touch_IsTouched() &&
           touch_GetX() > 10 && touch_GetX() < 60 &&
           touch_GetY() > 5  && touch_GetY() < 30;
}

uint8_t isPauseButtonTouched(void) {
    return touch_IsTouched() &&
           touch_GetX() > 180 && touch_GetX() < 230 &&
           touch_GetY() > 5   && touch_GetY() < 30;
}

void initializeButtons(void) {
    initializeControlButtons();
    lcd_ShowStr(90, 10, "SCORE:", BLACK, WHITE, 16, 0);
    lcd_ShowIntNum(135, 10, score, 3, BLACK, WHITE, 16);

    lcd_Fill(10, 5, 60, 30, WHITE);
    lcd_DrawRectangle(10, 5, 60, 30, BLACK);
    lcd_ShowStr(15, 10, "HOME", BLACK, WHITE, 16, 0);

    lcd_Fill(180, 5, 230, 30, WHITE);
    lcd_DrawRectangle(180, 5, 230, 30, BLACK);
    lcd_ShowStr(185, 10, "PAUSE", BLACK, WHITE, 16, 0);
}

uint8_t isButtonUp(void) {
    if (touch_IsTouched() && touch_GetX() > controlButtons[0].xStart && touch_GetX() < controlButtons[0].xEnd && touch_GetY() > controlButtons[0].yStart && touch_GetY() < controlButtons[0].yEnd) {
        return 1;
    }
    return 0;
}

uint8_t isButtonDown(void) {
    if (touch_IsTouched() && touch_GetX() > controlButtons[1].xStart && touch_GetX() < controlButtons[1].xEnd && touch_GetY() > controlButtons[1].yStart && touch_GetY() < controlButtons[1].yEnd) {
        return 1;
    }
    return 0;
}

uint8_t isButtonLeft(void) {
    if (touch_IsTouched() && touch_GetX() > controlButtons[2].xStart && touch_GetX() < controlButtons[2].xEnd && touch_GetY() > controlButtons[2].yStart && touch_GetY() < controlButtons[2].yEnd) {
        return 1;
    }
    return 0;
}

uint8_t isButtonRight(void) {
    if (touch_IsTouched() && touch_GetX() > controlButtons[3].xStart && touch_GetX() < controlButtons[3].xEnd && touch_GetY() > controlButtons[3].yStart && touch_GetY() < controlButtons[3].yEnd) {
        return 1;
    }
    return 0;
}

uint8_t isStartScreenTouched(void) {
    if (!touch_IsTouched()) return 0;

    uint16_t tx = touch_GetX();
    uint16_t ty = touch_GetY();
    uint16_t btnX1 = SCREEN_X + 35;
    uint16_t btnX2 = SCREEN_X + SCREEN_SIZE - 35;
    uint16_t btnY1 = SCREEN_Y + 125;
    uint16_t btnY2 = SCREEN_Y + 170;

    if (tx > btnX1 && tx < btnX2 &&
        ty > btnY1 && ty < btnY2) {
        return 1;
    }
    return 0;
}
