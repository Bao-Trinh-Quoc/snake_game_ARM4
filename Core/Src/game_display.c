/*
 * game_display.c
 *  Nền đen, viền trắng 1px. Vẽ incremental, không lưới, không giật.
 */

#include "game_display.h"
#include <stdlib.h>
#include <string.h>

struct Fruit {
    uint16_t x, y;
    uint16_t color;
} fruit;

struct Snake snake;
enum Direction snakeDirection = DOWN;

uint8_t  gameGrid[GRID_ROWS][GRID_COLS];
static int16_t prevX[GRID_ROWS][GRID_COLS];
static int16_t prevY[GRID_ROWS][GRID_COLS];

/* ========= KHUNG & VÙNG VẼ =========
   - Khung (frame) trắng 1px nằm ở biên SCREEN_X..SCREEN_X+SCREEN_SIZE
   - Vùng vẽ PLAY_* là phần lõm vào 1px (để ô không chạm viền)        */
#define FRAME_COLOR   WHITE
#define PLAY_X        (SCREEN_X + 1)
#define PLAY_Y        (SCREEN_Y + 1)
#define PLAY_SIZE     (SCREEN_SIZE - 2)
/* Ô cuối cùng sẽ kết thúc tại: PLAY_X + PLAY_SIZE - 1 (lọt trong khung) */

static inline void drawPlayfieldFrame(void) {
    lcd_DrawRectangle(SCREEN_X, SCREEN_Y,
                      SCREEN_X + SCREEN_SIZE, SCREEN_Y + SCREEN_SIZE,
                      FRAME_COLOR);
}

/* Vẽ 1 ô GRID (i,j) trong vùng PLAY, clamp biên an toàn */
static inline void drawCell(uint8_t i, uint8_t j, uint16_t color) {
    // Pixel bắt đầu của ô
    uint16_t x1 = PLAY_X + i * CELL_SIZE;
    uint16_t y1 = PLAY_Y + j * CELL_SIZE;

    // Pixel kết thúc của ô (bao gồm) nhưng không vượt quá vùng PLAY
    uint16_t x2 = x1 + CELL_SIZE - 1;
    uint16_t y2 = y1 + CELL_SIZE - 1;

    // Clamp để đảm bảo KHÔNG chạm viền ngoài (tránh đè viền)
    uint16_t maxX = PLAY_X + PLAY_SIZE - 1;
    uint16_t maxY = PLAY_Y + PLAY_SIZE - 1;
    if (x2 > maxX) x2 = maxX;
    if (y2 > maxY) y2 = maxY;

    // Nếu vì cấu hình CELL_SIZE không chia hết, ô mép có thể co lại 1px — chấp nhận.
    lcd_Fill(x1, y1, x2, y2, color);
}

/* Không redraw toàn màn để tránh giật */
void renderScreen(void) {
    // intentionally empty — vẽ incremental ở advance/remove/generate
}

void generateFruit(void) {
    do {
        fruit.x = rand() % GRID_ROWS;
        fruit.y = rand() % GRID_COLS;
    } while (gameGrid[fruit.x][fruit.y] != 0);

    gameGrid[fruit.x][fruit.y] = 2;
    drawCell(fruit.x, fruit.y, RED);  // vẽ ngay mồi
}

void initializeGame(void) {
    memset(gameGrid, 0, sizeof(gameGrid));
    for (uint8_t i = 0; i < GRID_ROWS; ++i)
        for (uint8_t j = 0; j < GRID_COLS; ++j)
            prevX[i][j] = prevY[i][j] = -1;

    // NỀN vùng chơi: đen (chỉ tô vùng PLAY, không đụng viền trắng)
    lcd_Fill(PLAY_X, PLAY_Y, PLAY_X + PLAY_SIZE, PLAY_Y + PLAY_SIZE, BLACK);

    // Viền trắng 1px — vẽ MỘT LẦN
    drawPlayfieldFrame();

    // Khởi tạo rắn (2 ô)
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

    // Vẽ 2 ô rắn ban đầu (lọt trong vùng PLAY, không đè viền)
    {
        uint16_t snakeColor = (snake.color ? snake.color : BLUE);
        drawCell(snake.tailX, snake.tailY, snakeColor);
        drawCell(snake.headX, snake.headY, snakeColor);
    }

    // Mồi
    fruit.color = RED;
    generateFruit();

    // Hướng
    snakeDirection = DOWN;
}

/* Không dùng trong FSM hiện tại, giữ lại tham khảo */
void advanceSnakeHead(void) {
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

    drawCell(snake.headX, snake.headY, (snake.color ? snake.color : BLUE));
}

void advanceSnakeHeadTo(int16_t nx, int16_t ny) {
    uint16_t oldHeadX = snake.headX;
    uint16_t oldHeadY = snake.headY;

    snake.headX = (uint16_t)nx;
    snake.headY = (uint16_t)ny;

    prevX[snake.headX][snake.headY] = oldHeadX;
    prevY[snake.headX][snake.headY] = oldHeadY;

    gameGrid[snake.headX][snake.headY] = 1;

    drawCell(snake.headX, snake.headY, (snake.color ? snake.color : BLUE));
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
    drawCell(curTailX, curTailY, BLACK);  // trả lại nền đen (không đụng viền)

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

/* Input giữ nguyên */
void handleInput(void) {
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

/* Start screen giữ nguyên như trước */
void displayStartScreen(void) {
    lcd_Fill(0, 0, 240, 320, BLACK);
    lcd_ShowStr(SCREEN_X + 25, SCREEN_Y + 10, "SNAKE GAME", WHITE, BLACK, 24, 0);

    lcd_ShowStr(SCREEN_X + 10, SCREEN_Y + 45, "Choose color", WHITE, BLACK, 16, 0);

    uint16_t top  = SCREEN_Y + 70;
    uint16_t left = SCREEN_X + 10;
    uint16_t w    = 26;
    uint16_t h    = 25;
    uint16_t gap  = 10;

    lcd_Fill(left, top, left + w, top + h, GREEN);
    lcd_Fill(left-1, top-1, left + w+1, top+1, WHITE);
    lcd_Fill(left-1, top+h-1, left + w+1, top+h+1, WHITE);
    lcd_Fill(left-1, top-1, left+1, top+h+1, WHITE);
    lcd_Fill(left+w-1, top-1, left+w+1, top+h+1, WHITE);

    uint16_t x2 = left + (w + gap);
    lcd_Fill(x2, top, x2 + w, top + h, BLUE);
    lcd_Fill(x2-1, top-1, x2 + w+1, top+1, WHITE);
    lcd_Fill(x2-1, top+h-1, x2 + w+1, top+h+1, WHITE);
    lcd_Fill(x2-1, top-1, x2+1, top+h+1, WHITE);
    lcd_Fill(x2+w-1, top-1, x2+w+1, top+h+1, WHITE);

    uint16_t x3 = left + (w + gap)*2;
    lcd_Fill(x3, top, x3 + w, top + h, MAGENTA);
    lcd_Fill(x3-1, top-1, x3 + w+1, top+1, WHITE);
    lcd_Fill(x3-1, top+h-1, x3 + w+1, top+h+1, WHITE);
    lcd_Fill(x3-1, top-1, x3+1, top+h+1, WHITE);
    lcd_Fill(x3+w-1, top-1, x3+w+1, top+h+1, WHITE);

    uint16_t x4 = left + (w + gap)*3;
    lcd_Fill(x4, top, x4 + w, top + h, YELLOW);
    lcd_Fill(x4-1, top-1, x4 + w+1, top+1, WHITE);
    lcd_Fill(x4-1, top+h-1, x4 + w+1, top+h+1, WHITE);
    lcd_Fill(x4-1, top-1, x4+1, top+h+1, WHITE);
    lcd_Fill(x4+w-1, top-1, x4+w+1, top+h+1, WHITE);

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
