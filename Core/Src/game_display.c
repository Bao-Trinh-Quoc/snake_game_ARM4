/*
 * game_display.c
 *
 *  Created on: Oct 21, 2025
 *      Author: DELL
 */


#include "game_display.h"

struct Fruit {
    uint16_t x, y;
    uint16_t color;
} fruit;
struct Snake snake;
enum Direction snakeDirection = DOWN;

uint8_t gameGrid[GRID_ROWS][GRID_COLS];
static int16_t prevX[GRID_ROWS][GRID_COLS];
static int16_t prevY[GRID_ROWS][GRID_COLS];


void drawCell(uint8_t i, uint8_t j, uint16_t color) {
	lcd_Fill(SCREEN_X + i * CELL_SIZE, SCREEN_Y + j * CELL_SIZE, SCREEN_X + i * CELL_SIZE + CELL_SIZE, SCREEN_Y + j * CELL_SIZE + CELL_SIZE, color);
}

void renderScreen() {
    for (uint8_t row = 0; row < GRID_ROWS; row++) {
        for (uint8_t col = 0; col < GRID_COLS; col++) {
            uint16_t cellColor = BLACK;
            if (gameGrid[row][col] == 1) {
            	cellColor = (snake.color ? snake.color : BLUE);
            } else if (gameGrid[row][col] == 2) {
                cellColor = RED;
            }
            drawCell(row, col, cellColor);
        }
    }
}

void generateFruit() {
    do {
        fruit.x = rand() % GRID_ROWS;
        fruit.y = rand() % GRID_COLS;
    } while (gameGrid[fruit.x][fruit.y] != 0);
    gameGrid[fruit.x][fruit.y] = 2;
}
void initializeGame() {
    memset(gameGrid, 0, sizeof(gameGrid));
    for (uint8_t i = 0; i < GRID_ROWS; ++i)
        for (uint8_t j = 0; j < GRID_COLS; ++j)
            prevX[i][j] = prevY[i][j] = -1;

    lcd_Fill(SCREEN_X, SCREEN_Y, SCREEN_X + SCREEN_SIZE, SCREEN_Y + SCREEN_SIZE, WHITE);

    snake.headX = GRID_ROWS / 2;
    snake.headY = GRID_COLS / 2;
    snake.tailX = GRID_ROWS / 2;
    snake.tailY = GRID_COLS / 2 - 1;

    gameGrid[snake.tailX][snake.tailY] = 1;
    gameGrid[snake.headX][snake.headY] = 1;

    prevX[snake.headX][snake.headY] = snake.tailX;
    prevY[snake.headX][snake.headY] = snake.tailY;
    prevX[snake.tailX][snake.tailY] = -1;
    prevY[snake.tailX][snake.tailY] = -1;

    fruit.color = RED;
    generateFruit();
    snakeDirection = DOWN;
}

void advanceSnakeHead() {
    uint16_t oldHeadX = snake.headX;
    uint16_t oldHeadY = snake.headY;

    switch (snakeDirection) {
        case UP:    snake.headY--; break;
        case DOWN:  snake.headY++; break;
        case LEFT:  snake.headX--; break;
        case RIGHT: snake.headX++; break;
    }

    if (snake.headX < 0) snake.headX = GRID_ROWS - 1;
    if (snake.headX >= GRID_ROWS) snake.headX = 0;
    if (snake.headY < 0) snake.headY = GRID_COLS - 1;
    if (snake.headY >= GRID_COLS) snake.headY = 0;

    prevX[snake.headX][snake.headY] = oldHeadX;
    prevY[snake.headX][snake.headY] = oldHeadY;

    gameGrid[snake.headX][snake.headY] = 1;
}

void advanceSnakeHeadTo(int16_t nx, int16_t ny) {
    uint16_t oldHeadX = snake.headX;
    uint16_t oldHeadY = snake.headY;

    snake.headX = (uint16_t)nx;
    snake.headY = (uint16_t)ny;

    prevX[snake.headX][snake.headY] = oldHeadX;
    prevY[snake.headX][snake.headY] = oldHeadY;

    gameGrid[snake.headX][snake.headY] = 1;
}
void removeSnakeTail(void) {
    uint16_t curTailX = snake.tailX;
    uint16_t curTailY = snake.tailY;

    int16_t nextTailX = -1;
    int16_t nextTailY = -1;

    for (uint8_t i = 0; i < GRID_ROWS; i++) {
        for (uint8_t j = 0; j < GRID_COLS; j++) {
            if (prevX[i][j] == curTailX && prevY[i][j] == curTailY) {
                nextTailX = i;
                nextTailY = j;
                break;
            }
        }
        if (nextTailX != -1) break;
    }

    gameGrid[curTailX][curTailY] = 0;
    prevX[curTailX][curTailY] = -1;
    prevY[curTailX][curTailY] = -1;

    if (nextTailX != -1) {
        snake.tailX = (uint16_t)nextTailX;
        snake.tailY = (uint16_t)nextTailY;
    } else {
        snake.tailX = snake.headX;
        snake.tailY = snake.headY;
    }
}



void handleInput() {
    if (isButtonLeft() && (snakeDirection == UP || snakeDirection == DOWN)) {
        snakeDirection = LEFT;
    } else if (isButtonRight() && (snakeDirection == UP || snakeDirection == DOWN)) {
        snakeDirection = RIGHT;
    } else if (isButtonUp() && (snakeDirection == LEFT || snakeDirection == RIGHT)) {
        snakeDirection = UP;
    } else if (isButtonDown() && (snakeDirection == LEFT || snakeDirection == RIGHT)) {
        snakeDirection = DOWN;
    }
}


void displayStartScreen(void) {
	lcd_Fill(0, 0, 240, 320, BLACK);
    lcd_ShowStr(SCREEN_X + 25, SCREEN_Y + 10, "SNAKE GAME", WHITE, BLACK, 24, 0);

    lcd_ShowStr(SCREEN_X + 10, SCREEN_Y + 45, "Choose color", WHITE, BLACK, 16, 0);

    uint16_t top  = SCREEN_Y + 70;
    uint16_t left = SCREEN_X + 10;
    uint16_t w    = 26;
    uint16_t h    = 25;
    uint16_t gap  = 10;

    lcd_Fill(left,               top, left + w,               top + h, GREEN);
    lcd_Fill(left-1,             top-1, left + w+1,           top+1,   WHITE);
    lcd_Fill(left-1,             top+h-1, left + w+1,         top+h+1, WHITE);
    lcd_Fill(left-1,             top-1, left+1,               top+h+1, WHITE);
    lcd_Fill(left+w-1,           top-1, left+w+1,             top+h+1, WHITE);

    uint16_t x2 = left + (w + gap);
    lcd_Fill(x2,                 top, x2 + w,                 top + h, BLUE);
    lcd_Fill(x2-1,               top-1, x2 + w+1,             top+1,   WHITE);
    lcd_Fill(x2-1,               top+h-1, x2 + w+1,           top+h+1, WHITE);
    lcd_Fill(x2-1,               top-1, x2+1,                 top+h+1, WHITE);
    lcd_Fill(x2+w-1,             top-1, x2+w+1,               top+h+1, WHITE);

    uint16_t x3 = left + (w + gap)*2;
    lcd_Fill(x3,                 top, x3 + w,                 top + h, MAGENTA);
    lcd_Fill(x3-1,               top-1, x3 + w+1,             top+1,   WHITE);
    lcd_Fill(x3-1,               top+h-1, x3 + w+1,           top+h+1, WHITE);
    lcd_Fill(x3-1,               top-1, x3+1,                 top+h+1, WHITE);
    lcd_Fill(x3+w-1,             top-1, x3+w+1,               top+h+1, WHITE);

    uint16_t x4 = left + (w + gap)*3;
    lcd_Fill(x4,                 top, x4 + w,                 top + h, YELLOW);
    lcd_Fill(x4-1,               top-1, x4 + w+1,             top+1,   WHITE);
    lcd_Fill(x4-1,               top+h-1, x4 + w+1,           top+h+1, WHITE);
    lcd_Fill(x4-1,               top-1, x4+1,                 top+h+1, WHITE);
    lcd_Fill(x4+w-1,             top-1, x4+w+1,               top+h+1, WHITE);

    uint16_t btnTop = SCREEN_Y + 125;
    lcd_Fill(SCREEN_X + 35, btnTop, SCREEN_X + SCREEN_SIZE - 35, btnTop + 45, GREEN);
    lcd_Fill(SCREEN_X + 35, btnTop, SCREEN_X + SCREEN_SIZE - 35, btnTop + 2, WHITE);
    lcd_Fill(SCREEN_X + 35, btnTop + 43, SCREEN_X + SCREEN_SIZE - 35, btnTop + 45, WHITE);
    lcd_Fill(SCREEN_X + 35, btnTop, SCREEN_X + 37, btnTop + 45, WHITE);
    lcd_Fill(SCREEN_X + SCREEN_SIZE - 37, btnTop, SCREEN_X + SCREEN_SIZE - 35, btnTop + 45, WHITE);
    lcd_ShowStr(SCREEN_X + 50, btnTop + 10, "START", WHITE, GREEN, 24, 1);

    uint16_t snakePreviewY = btnTop + 60;
    uint16_t snakePreviewX = SCREEN_X + 60;
    uint16_t previewColor = (snake.color ? snake.color : GREEN);

    for (int i = 0; i < 3; i++) {
        uint16_t x1 = snakePreviewX + i * 15;
        uint16_t y1 = snakePreviewY;
        lcd_Fill(x1, y1, x1 + 13, y1 + 13, previewColor);
        lcd_DrawRectangle(x1, y1, x1 + 13, y1 + 13, WHITE);
    }

    lcd_ShowStr(snakePreviewX - 10, snakePreviewY + 18, "Preview", WHITE, BLACK, 12, 0);
}

uint16_t startScreenHandleColorTouch(void) {
    if (!touch_IsTouched()) return 0;

    uint16_t tx = touch_GetX();
    uint16_t ty = touch_GetY();

    uint16_t top  = SCREEN_Y + 70;
    uint16_t left = SCREEN_X + 10;
    uint16_t w    = 26;
    uint16_t h    = 25;
    uint16_t gap  = 10;

    if (tx > left && tx < left + w &&
        ty > top  && ty < top + h) {
        return GREEN;
    }

    uint16_t x2 = left + (w + gap);
    if (tx > x2 && tx < x2 + w &&
        ty > top && ty < top + h) {
        return BLUE;
    }

    uint16_t x3 = left + (w + gap) * 2;
    if (tx > x3 && tx < x3 + w &&
        ty > top && ty < top + h) {
        return MAGENTA;
    }

    uint16_t x4 = left + (w + gap) * 3;
    if (tx > x4 && tx < x4 + w &&
        ty > top && ty < top + h) {
        return YELLOW;
    }
    return 0;
}

