/*
 * game_control.c
 *
 *  Created on: Oct 21, 2025
 *      Author: DELL
 */

#include "game_control.h"

#define BTN_IDX_UP      1 //new
#define BTN_IDX_DOWN    9
#define BTN_IDX_LEFT    4
#define BTN_IDX_RIGHT   6
#define BTN_IDX_START   5
#define BTN_PAUSE  		15


typedef enum {
    GAME_START, GAME_PLAY, GAME_OVER, GAME_PAUSE
} GameState;

static GameState currentState = GAME_START;

typedef struct {
    uint16_t xStart, yStart, xEnd, yEnd;
    uint8_t isPressed;
} ControlButton;

static ControlButton controlButtons[4];
static uint16_t score = 0;

void gameFSM(void) {
	initializeButtons();
    switch (currentState) {
        case GAME_START:
            displayStartScreen();
            if (isStartScreenTouched()) {
                currentState = GAME_PLAY;
                initializeGame();
                renderScreen();
                setTimer_button(5);
                setTimer_snake(500);
            }
            break;
        case GAME_PLAY:
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
                    default:    break;
                }

                if (nextX < 0 || nextY < 0 || nextX >= GRID_ROWS || nextY >= GRID_COLS || gameGrid[nextX][nextY] == 1) {
                	currentState = GAME_OVER;
                }
                else if (gameGrid[nextX][nextY] == 2) {
                    score++;
                    advanceSnakeHead();
                    generateFruit();
                }
                else{
                	advanceSnakeHead();
                	removeSnakeTail();
                }
                renderScreen();
                setTimer_snake(500);
            }
            if(isPhyStartEdge()){ //new
                currentState = GAME_START;
                initializeGame();
                renderScreen();
                score = 0;
            }
            if (isPhyPauseEdge()) { //new
                currentState = GAME_PAUSE;
                displayPauseScreen();
            break;
        case GAME_OVER:
        	displayStartScreen();
            if (isStartScreenTouched()) {
                currentState = GAME_PLAY;
                initializeGame();
                renderScreen();
                score = 0;
            }
            break;
        case GAME_PAUSE: //new
            if (button_read_flag) {
                setTimer_button(5);
                if (isPhyPauseEdge() && currentState == GAME_PAUSE) {
                    currentState = GAME_PLAY;
                    renderScreen();  
                    return;
                }
            }
            break;
        }
    }
}

void initializeControlButtons(void) {
    controlButtons[0] = (ControlButton){.xStart = DIRECTION_BTN_X + DIRECTION_BTN_SIZE + 10, .yStart = DIRECTION_BTN_Y + 10 + DIRECTION_BTN_SIZE, .xEnd = DIRECTION_BTN_X + 2 * DIRECTION_BTN_SIZE + 10, .yEnd = DIRECTION_BTN_Y + DIRECTION_BTN_SIZE*2 + 10 , .isPressed = 0};
    controlButtons[1] = (ControlButton){.xStart = DIRECTION_BTN_X + DIRECTION_BTN_SIZE + 10, .yStart = DIRECTION_BTN_Y + 2 * DIRECTION_BTN_SIZE + 20, .xEnd = DIRECTION_BTN_X + 2 * DIRECTION_BTN_SIZE + 10, .yEnd = DIRECTION_BTN_Y + 3 * DIRECTION_BTN_SIZE + 20, .isPressed = 0};
    controlButtons[2] = (ControlButton){.xStart = DIRECTION_BTN_X, .yStart = DIRECTION_BTN_Y + 2 * DIRECTION_BTN_SIZE + 20, .xEnd = DIRECTION_BTN_X + DIRECTION_BTN_SIZE,.yEnd = DIRECTION_BTN_Y + 3 * DIRECTION_BTN_SIZE + 20, .isPressed = 0};
    controlButtons[3] = (ControlButton){.xStart = DIRECTION_BTN_X + 2 * DIRECTION_BTN_SIZE + 20, .yStart = DIRECTION_BTN_Y + 2 * DIRECTION_BTN_SIZE + 20, .xEnd = DIRECTION_BTN_X + 3 * DIRECTION_BTN_SIZE + 20, .yEnd = DIRECTION_BTN_Y + 3 * DIRECTION_BTN_SIZE + 20, .isPressed = 0};

    for (int i = 0; i < 4; i++) {
        lcd_Fill(controlButtons[i].xStart, controlButtons[i].yStart, controlButtons[i].xEnd, controlButtons[i].yEnd, BLACK);
    }
}

void initializeButtons(void) {
    initializeControlButtons();
    lcd_ShowStr(90, 10, "SCORE:", BLACK, WHITE, 16, 0);
    lcd_ShowIntNum(135, 10, score, 3, BLACK, WHITE, 16);
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

uint8_t isPhyButtonUpEdge(void)    { return button_pressed_edge(BTN_IDX_UP);    }//new
uint8_t isPhyButtonDownEdge(void)  { return button_pressed_edge(BTN_IDX_DOWN);  }
uint8_t isPhyButtonLeftEdge(void)  { return button_pressed_edge(BTN_IDX_LEFT);  }
uint8_t isPhyButtonRightEdge(void) { return button_pressed_edge(BTN_IDX_RIGHT); }
uint8_t isPhyStartEdge(void)       { return button_pressed_edge(BTN_IDX_START);}
uint8_t isPhyPauseEdge(void)       { return button_pressed_edge(BTN_PAUSE);}

// uint8_t isStartScreenTouched(void) {
//     if (!touch_IsTouched()) return 0;
//     return touch_GetX() > SCREEN_X && touch_GetX() < SCREEN_X + SCREEN_SIZE && touch_GetY() > SCREEN_Y && touch_GetY() < SCREEN_Y + SCREEN_SIZE;
// }
uint8_t isStartScreenTouched(void){ //new
    if (touch_IsTouched() &&
        touch_GetX() > SCREEN_X && touch_GetX() < SCREEN_X + SCREEN_SIZE &&
        touch_GetY() > SCREEN_Y && touch_GetY() < SCREEN_Y + SCREEN_SIZE) {
        return 1;
    }
    if (isPhyStartEdge()) return 1;
    return 0;
}

