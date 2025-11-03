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

void drawCell(uint8_t i, uint8_t j, uint16_t color) {
	lcd_Fill(SCREEN_X + i * CELL_SIZE, SCREEN_Y + j * CELL_SIZE, SCREEN_X + i * CELL_SIZE + CELL_SIZE, SCREEN_Y + j * CELL_SIZE + CELL_SIZE, color);
}

void renderScreen() {
    for (uint8_t row = 0; row < GRID_ROWS; row++) {
        for (uint8_t col = 0; col < GRID_COLS; col++) {
            uint16_t cellColor = BLACK;
            if (gameGrid[row][col] == 1) {
                cellColor = BLUE;
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
    lcd_Fill(SCREEN_X, SCREEN_Y, SCREEN_X + SCREEN_SIZE, SCREEN_Y + SCREEN_SIZE, WHITE);
    snake.color = GREEN;
    snake.headX = GRID_ROWS / 2;
    snake.headY = GRID_COLS / 2;
    snake.tailX = GRID_ROWS / 2;
    snake.tailY = GRID_COLS / 2 - 1;
    gameGrid[snake.headX][snake.headY] = 1;
    gameGrid[snake.tailX][snake.tailY] = 1;
    snake.color = GREEN;
    fruit.color = RED;
    generateFruit();
    snakeDirection = DOWN;
}
void advanceSnakeHead() {
    switch (snakeDirection) {
        case UP:    snake.headY--; break;
        case DOWN:  snake.headY++; break;
        case LEFT:  snake.headX--; break;
        case RIGHT: snake.headX++; break;
        default:    break;
    }
    gameGrid[snake.headX][snake.headY] = 1;
}
void removeSnakeTail() {
    uint16_t nextTailX = snake.tailX;
    uint16_t nextTailY = snake.tailY;
//    if (gameGrid[nextTailX - 1][nextTailY] == 1) {
//        nextTailX--;
//    } else if (gameGrid[nextTailX + 1][nextTailY] == 1) {
//        nextTailX++;
//    } else if (gameGrid[nextTailX][nextTailY - 1] == 1) {
//        nextTailY--;
//    } else if (gameGrid[nextTailX][nextTailY + 1] == 1) {
//        nextTailY++;
//    }
    // Check bounds and find next tail segment
    if (nextTailX > 0 && gameGrid[nextTailX - 1][nextTailY] == 1) {
        nextTailX--;
    } else if (nextTailX < GRID_ROWS - 1 && gameGrid[nextTailX + 1][nextTailY] == 1) {
        nextTailX++;
    } else if (nextTailY > 0 && gameGrid[nextTailX][nextTailY - 1] == 1) {
        nextTailY--;
    } else if (nextTailY < GRID_COLS - 1 && gameGrid[nextTailX][nextTailY + 1] == 1) {
        nextTailY++;
    }
    gameGrid[snake.tailX][snake.tailY] = 0;
    snake.tailX = nextTailX;
    snake.tailY = nextTailY;
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
    lcd_Fill(SCREEN_X, SCREEN_Y, SCREEN_X + SCREEN_SIZE, SCREEN_Y + SCREEN_SIZE, BLACK);
    lcd_ShowStr(SCREEN_X + 40, SCREEN_Y + 50, "START", GREEN, BLACK, 32, 1);
}
