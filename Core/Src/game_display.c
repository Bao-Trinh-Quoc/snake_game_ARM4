/*
 * game_display.c
 *  Nền đen, viền trắng 1px. Vẽ incremental, không lưới, không giật.
 */

#include "game_display.h"
#include "game_control.h"

#include <stdlib.h>
#include <string.h>
#include "lcd.h"
#include "at24c.h"

#define BOMB_LIFETIME_TICKS 27
struct Snake snake;
enum Direction snakeDirection = DOWN;
#define GRID_BG_COLOR BLACK
uint8_t  gameGrid[GRID_ROWS][GRID_COLS];
static int16_t prevX[GRID_ROWS][GRID_COLS];
static int16_t prevY[GRID_ROWS][GRID_COLS];
extern uint8_t selectedMap;
/* ========= KHUNG & VÙNG VẼ =========
   - Khung (frame) trắng 1px nằm ở biên SCREEN_X..SCREEN_X+SCREEN_SIZE
   - Vùng vẽ PLAY_* là phần lõm vào 1px (để ô không chạm viền)        */
#define FRAME_COLOR   WHITE
#define PLAY_X (SCREEN_X + 1)
#define PLAY_Y (SCREEN_Y + 1)
#define PLAY_W (SCREEN_W - 2)
#define PLAY_H (SCREEN_H - 2)

void placeObstaclePlus(void);
void placeMazeObstacles(void) ;
void placeBorderWalls(void) ;

// Thêm vào gần SnakeShape:
typedef enum {
    SHAPE_SQUARE,
    SHAPE_CIRCLE
} SnakeShape;


SnakeShape snakeShape = SHAPE_SQUARE;  // mặc định vuông

/* Ô cuối cùng sẽ kết thúc tại: PLAY_X + PLAY_SIZE - 1 (lọt trong khung) */

inline void drawPlayfieldFrame(void) {
	lcd_DrawRectangle(
	    SCREEN_X,
	    SCREEN_Y,
	    SCREEN_X + SCREEN_W,
	    SCREEN_Y + SCREEN_H,
	    FRAME_COLOR
	);
}


void lcd_FillCircle(int x0, int y0, int radius, uint16_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (x <= y) {
        // Vẽ các hàng ngang để tô kín hình tròn
        for (int i = x0 - x; i <= x0 + x; i++) {
        	lcd_DrawPoint(i, y0 + y, color);
        	lcd_DrawPoint(i, y0 - y, color);
        }
        for (int i = x0 - y; i <= x0 + y; i++) {
        	lcd_DrawPoint(i, y0 + x, color);
        	lcd_DrawPoint(i, y0 - x, color);
        }

        if (d < 0)
            d = d + 4 * x + 6;
        else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}
// ============================
// Vẽ ô bình thường
// ============================
static inline void drawCell(uint8_t i, uint8_t j, uint16_t color) {
    uint16_t x1 = PLAY_X + i * CELL_SIZE;
    uint16_t y1 = PLAY_Y + j * CELL_SIZE;

    uint16_t x2 = x1 + CELL_SIZE;
    uint16_t y2 = y1 + CELL_SIZE;

    // đổi PLAY_SIZE → PLAY_W và PLAY_H
    uint16_t maxX = PLAY_X + PLAY_W - 1;
    uint16_t maxY = PLAY_Y + PLAY_H - 1;

    if (x2 > maxX) x2 = maxX;
    if (y2 > maxY) y2 = maxY;

    lcd_Fill(x1, y1, x2, y2, color);
}


// ============================
// Vẽ thân rắn hoặc xóa ô
// ============================
void drawCell_Shape(uint8_t i, uint8_t j, uint16_t color) {

    // Nếu là xóa (BLACK) → chỉ xóa ô trống
    if (color == BLACK) {
        if (gameGrid[i][j] == 0) {
            drawCell(i, j, GRID_BG_COLOR);
        }
        return;
    }

    uint16_t x1 = PLAY_X + i * CELL_SIZE;
    uint16_t y1 = PLAY_Y + j * CELL_SIZE;
    uint16_t x2 = x1 + CELL_SIZE - 1;
    uint16_t y2 = y1 + CELL_SIZE - 1;

    int x_center = x1 + CELL_SIZE / 2;
    int y_center = y1 + CELL_SIZE / 2;
    int radius   = CELL_SIZE / 2-2 ;  // giảm để không tràn ra ô khác

    if (snakeShape == SHAPE_CIRCLE) {
        drawCell(i, j, BLACK); // xóa nền
        lcd_FillCircle(x_center, y_center, radius, color);
    } else {
        drawCell(i, j, color); // vuông kín ô
    }
}


// ============================
// Vẽ đầu rắn
// ============================
void drawSnakeHeadCell(uint8_t i, uint8_t j, uint16_t color, enum Direction dir)
{
    uint16_t x1 = PLAY_X + i * CELL_SIZE ;
    uint16_t y1 = PLAY_Y + j * CELL_SIZE ;
    uint16_t x2 = x1 + CELL_SIZE -1;
    uint16_t y2 = y1 + CELL_SIZE -1;

    // FULL ô như thân (vuông hoặc tròn)
    int cx = x1 + CELL_SIZE / 2;
    int cy = y1 + CELL_SIZE / 2;
    int r  = CELL_SIZE / 2 -1;

    drawCell(i, j, BLACK);   // clear nền trước

    // ===== Vẽ đầu KHÔNG viền =====
    if (snakeShape == SHAPE_CIRCLE) {
        // Hình tròn đầy
        lcd_FillCircle(cx, cy, r, color);
    } else {
        // Hình vuông đầy
        lcd_Fill(x1, y1, x2, y2, color);
    }

    // ===== MẮT RẮN =====
    int eyeR   = r / 3;
    if (eyeR < 2) eyeR = 2;

    int pupilR = eyeR / 2;

    int ex1, ey1, ex2, ey2;

    switch(dir) {
        case UP:
            ex1 = cx - eyeR; ey1 = y1 + 3;
            ex2 = cx + eyeR; ey2 = y1 + 3;
            break;

        case DOWN:
            ex1 = cx - eyeR; ey1 = y2 - 3;
            ex2 = cx + eyeR; ey2 = y2 - 3;
            break;

        case LEFT:
            ex1 = x1 + 3; ey1 = cy - eyeR;
            ex2 = x1 + 3; ey2 = cy + eyeR;
            break;

        default: // RIGHT
            ex1 = x2 - 3; ey1 = cy - eyeR;
            ex2 = x2 - 3; ey2 = cy + eyeR;
            break;
    }

    // Mắt trắng
    lcd_FillCircle(ex1, ey1, eyeR, WHITE);
    lcd_FillCircle(ex2, ey2, eyeR, WHITE);

    // Con ngươi
    lcd_FillCircle(ex1, ey1, pupilR, BLACK);
    lcd_FillCircle(ex2, ey2, pupilR, BLACK);
}




/* Không redraw toàn màn để tránh giật */
void renderScreen(void) {

}

void generateFruit(void) {
    static const uint16_t fruitColors[] = {
        RED, YELLOW, MAGENTA, CYAN, ORANGE, GBLUE
    };
    uint8_t normalColorId = rand() % 6;

    uint8_t isSuperFruit = (rand() % 20 == 0);  // 5% super fruit

    uint16_t x, y;

    // ======= RANDOM VỊ TRÍ AN TOÀN CHO FRUIT ========
    while (1) {
        x = 1 + rand() % (GRID_ROWS - 2);
        y = 1 + rand() % (GRID_COLS - 2);

        int dist = abs((int)x - (int)snake.headX) + abs((int)y - (int)snake.headY);

        // spawn xa đầu rắn ≥ 3 ô, và chỉ vào ô trống
        if (gameGrid[x][y] == 0 && dist >= 3 && !(bomb.active && bomb.x == x && bomb.y == y))
            break;
    }

    fruit.x     = x;
    fruit.y     = y;
    fruit.type  = isSuperFruit ? 2 : 1;                  // 1: thường, 2: super
    fruit.color = isSuperFruit ? YELLOW : fruitColors[normalColorId];

    gameGrid[x][y] = 2;

    // ======= VẼ TRÁI MỒI GỌN TRONG 1 Ô ========
    uint16_t x1 = PLAY_X + x * CELL_SIZE;
    uint16_t y1 = PLAY_Y + y * CELL_SIZE;
    uint16_t x2 = x1 + CELL_SIZE - 1;
    uint16_t y2 = y1 + CELL_SIZE - 1;

    // Clear hoàn toàn cell trước khi vẽ fruit
    lcd_Fill(x1, y1, x2, y2, GRID_BG_COLOR);

    int cx = (x1 + x2) / 2;
    int cy = (y1 + y2) / 2;

    int rOuter = CELL_SIZE / 2 - 2;   // luôn nằm TRONG cell
    if (rOuter < 2) rOuter = 2;

    if (isSuperFruit) {
        // SUPER: vòng tròn vàng dày
        lcd_FillCircle(cx, cy, rOuter, fruit.color);
        lcd_FillCircle(cx, cy, rOuter - 3, BLACK);   // hiệu ứng viền
    } else {
        // thường: full tròn trong 1 ô
        lcd_FillCircle(cx, cy, rOuter, fruit.color);
    }

    // =================================================
    //              SINH THÊM BOMB NGẪU NHIÊN
    // =================================================
    // Chỉ tạo bomb mới nếu hiện tại chưa có bomb
    if (!bomb.active) {
        // ~25% cơ hội tạo bomb mỗi lần sinh fruit
        if (rand() % 4 == 0) {
            uint16_t bx = 0, by = 0;
            uint8_t  tryCount = 0;

            while (tryCount < 25) {
                bx = 1 + rand() % (GRID_ROWS - 2);
                by = 1 + rand() % (GRID_COLS - 2);

                // chỉ spawn bomb vào ô TRỐNG (0)
                if (gameGrid[bx][by] == 0)
                    break;

                tryCount++;
            }

            if (gameGrid[bx][by] == 0) {
                bomb.x      = bx;
                bomb.y      = by;
                bomb.active = 1;
                bomb.ticks  = BOMB_LIFETIME_TICKS;

                gameGrid[bx][by] = 4;   // 4 = bomb

                uint16_t bx1 = PLAY_X + bx * CELL_SIZE;
                uint16_t by1 = PLAY_Y + by * CELL_SIZE;
                uint16_t bx2 = bx1 + CELL_SIZE - 1;
                uint16_t by2 = by1 + CELL_SIZE - 1;

                // clear cell
                lcd_Fill(bx1, by1, bx2, by2, BLACK);

                // vẽ bomb: ô đỏ sẫm + dấu X trắng
                uint16_t inset = 2;
                uint16_t ix1 = bx1 + inset;
                uint16_t iy1 = by1 + inset;
                uint16_t ix2 = bx2 - inset;
                uint16_t iy2 = by2 - inset;

                lcd_Fill(ix1, iy1, ix2, iy2, BRRED);

                uint16_t size = ix2 - ix1;
                for (uint16_t k = 0; k <= size; ++k) {
                    lcd_DrawPoint(ix1 + k, iy1 + k, WHITE);
                    lcd_DrawPoint(ix1 + k, iy2 - k, WHITE);
                }
            }
        }
    }
}
void generateBomb(void) {
    if (bomb.active) return;   // đang có rồi, không tạo thêm

    while (1) {
        uint16_t bx = 1 + rand() % (GRID_ROWS - 2);
        uint16_t by = 1 + rand() % (GRID_COLS - 2);

        if (gameGrid[bx][by] == 0 && !(fruit.x == bx && fruit.y == by)) {
            bomb.x = bx;
            bomb.y = by;
            bomb.active = 1;
            bomb.ticks  = BOMB_LIFETIME_TICKS;
            gameGrid[bx][by] = 4;

            drawCell_Shape(bx, by, BRRED); // Bomb màu đỏ sẫm
            break;
        }
    }
}
void updateBombLifetime(void) {
    if (!bomb.active) return;

    bomb.ticks--;

    if (bomb.ticks <= 0) {
        // xóa bomb
        bomb.active = 0;
        gameGrid[bomb.x][bomb.y] = 0;
        drawCell_Shape(bomb.x, bomb.y, BLACK);
    }
    if (!bomb.active && (rand() % 25 == 0)) {
                        generateBomb();
                    }
}



void generateSafeSnakeStart(uint16_t *hx, uint16_t *hy,
                            uint16_t *tx, uint16_t *ty,
                            enum Direction *dir)
{
    while (1) {
        // tránh viền: 1..GRID-2
        uint16_t x = 1 + rand() % (GRID_ROWS - 2);
        uint16_t y = 1 + rand() % (GRID_COLS - 2);

        // ô đầu phải trống
        if (gameGrid[x][y] != 0) continue;

        // random hướng 0..3
        int d = rand() % 4;
        int dx = 0, dy = 0;

        switch (d) {
            case 0: dx = -1; dy = 0;  *dir = LEFT;  break;
            case 1: dx =  1; dy = 0;  *dir = RIGHT; break;
            case 2: dx = 0;  dy = -1; *dir = UP;    break;
            case 3: dx = 0;  dy =  1; *dir = DOWN;  break;
        }

        // tail = ngược hướng head
        int txPos = x - dx;
        int tyPos = y - dy;

        // tail phải nằm trong map
        if (txPos < 1 || txPos > GRID_ROWS - 2) continue;
        if (tyPos < 1 || tyPos > GRID_COLS - 2) continue;

        // tail không trùng obstacle hoặc fruit
        if (gameGrid[txPos][tyPos] != 0) continue;

        // hợp lệ -> trả dữ liệu
        *hx = x;
        *hy = y;
        *tx = txPos;
        *ty = tyPos;
        return;
    }
}

void initializeGame(void) {
    memset(gameGrid, 0, sizeof(gameGrid));
    for (uint8_t i = 0; i < GRID_ROWS; ++i)
        for (uint8_t j = 0; j < GRID_COLS; ++j)
            prevX[i][j] = prevY[i][j] = -1;

    // nền vùng chơi
    lcd_Fill(PLAY_X, PLAY_Y, PLAY_X + PLAY_W, PLAY_Y + PLAY_H, BLACK);

    // Viền LCD (frame trắng)
    drawPlayfieldFrame();
    switch (selectedMap) {
        case 1:
            placeBorderWalls();
            break;
        case 2:
            placeObstaclePlus();
            break;
        case 3:
            placeMazeObstacles();
            break;
        default:
            // 0 = classic -> không vẽ gì
            break;
    }
    // ==== Viền map hoặc Maze trước ====
//    placeBorderWalls();
//    placeMazeObstacles();   // nếu có maze bên trong
    // ==== Random vị trí + random hướng ====
    uint16_t hx, hy, tx, ty;
    enum Direction dir;

    generateSafeSnakeStart(&hx, &hy, &tx, &ty, &dir);

    snake.headX = hx;
    snake.headY = hy;
    snake.tailX = tx;
    snake.tailY = ty;
    snakeDirection = dir;   // <<<< RANDOM HƯỚNG

    gameGrid[tx][ty] = 1;
    gameGrid[hx][hy] = 1;

    prevX[hx][hy] = tx;
    prevY[hx][hy] = ty;
    prevX[tx][ty] = -1;
    prevY[tx][ty] = -1;

    // Vẽ rắn: đuôi = thân, head = có mặt
    {
        uint16_t snakeColor = (snake.color ? snake.color : GREEN);
        drawCell_Shape(tx, ty, snakeColor);                              // đuôi/thân
        drawSnakeHeadCell(hx, hy, snakeColor, snakeDirection);     // đầu có mắt
    }

    // ==== Spawn mồi tránh viền + tránh tường ====
    fruit.color = RED;
    generateFruit();
}


void placeObstaclePlus(void) {
    uint8_t cx = GRID_ROWS / 2;
    uint8_t cy = GRID_COLS / 2;

    // Ngang mỏng (2 px)
    for (int x = cx - 4; x <= cx + 4; x++) {
        gameGrid[x][cy] = 3;
        drawCell(x, cy, GBLUE);
    }

    // Dọc mỏng (2 px)
    for (int y = cy - 4; y <= cy + 4; y++) {
        gameGrid[cx][y] = 3;
        drawCell(cx, y, GBLUE);
    }
}


void placeMazeObstacles(void) {

    // ----- Vertical left -----
    for (int y = 3; y <= 10; y++) {
        gameGrid[4][y] = 3;
        drawCell(4, y, BRRED);
    }

    // ----- Vertical right -----
    for (int y = 4; y <= 14; y++) {
    	gameGrid[15][y] = 3;
    	drawCell(15, y, BRRED);
    }
    	// Horizontal mid
    	for (int x = 8; x <= 14; x++) {
    	    gameGrid[x][8] = 3;
    	    drawCell(x, 8, BRRED);
    	}

    // ----- Short vertical bottom -----
    for (int y = 12; y <= 14; y++) {
        gameGrid[8][y] = 3;
        drawCell(8, y, BRRED);
    }

    // ----- Horizontal bottom -----
    for (int x = 6; x <= 11; x++) {
        gameGrid[x][15] = 3;
        drawCell(x, 15, BRRED);
    }
}



void placeBorderWalls(void) {
    // === Top border (viền trên) ===
    for (int x = 0; x < GRID_ROWS; x++) {
        gameGrid[x][0] = 3;
        drawCell(x, 0, MAGENTA );
    }

    // === Bottom border (viền dưới) ===
    for (int x = 0; x < GRID_ROWS; x++) {
        gameGrid[x][GRID_COLS] = 3;
        drawCell(x, GRID_COLS - 1, MAGENTA );
    }

    // === Left border (viền trái) ===
    for (int y = 0; y < GRID_COLS; y++) {
        gameGrid[0][y] = 3;
        drawCell(0, y, MAGENTA );
    }

    // === Right border (viền phải) ===
    for (int y = 0; y < GRID_COLS; y++) {
        gameGrid[GRID_ROWS][y] = 3;
        drawCell(GRID_ROWS - 1, y, MAGENTA);
    }
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

    drawCell_Shape(snake.headX, snake.headY, (snake.color ? snake.color : GREEN));
}

void advanceSnakeHeadTo(int16_t nx, int16_t ny) {
    uint16_t oldHeadX = snake.headX;
    uint16_t oldHeadY = snake.headY;

    uint16_t snakeColor = (snake.color ? snake.color : GREEN);

    drawCell_Shape(oldHeadX, oldHeadY, snakeColor);

    // Cập nhật head mới
    snake.headX = (uint16_t)nx;
    snake.headY = (uint16_t)ny;

    prevX[snake.headX][snake.headY] = oldHeadX;
    prevY[snake.headX][snake.headY] = oldHeadY;

    gameGrid[snake.headX][snake.headY] = 1;

    drawSnakeHeadCell(snake.headX, snake.headY, snakeColor, snakeDirection);
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
    drawCell_Shape(curTailX, curTailY, BLACK);  // trả lại nền đen (không đụng viền)

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
    if ((isButtonLeft() || isPhyButtonLeftEdge()) && (snakeDirection == UP || snakeDirection == DOWN)) {
        snakeDirection = LEFT;
    } else if ((isButtonRight() || isPhyButtonRightEdge()) && (snakeDirection == UP || snakeDirection == DOWN)) {
        snakeDirection = RIGHT;
    } else if ((isButtonUp() || isPhyButtonUpEdge()) && (snakeDirection == LEFT || snakeDirection == RIGHT)) {
        snakeDirection = UP;
    } else if ((isButtonDown() || isPhyButtonDownEdge()) && (snakeDirection == LEFT || snakeDirection == RIGHT)) {
        snakeDirection = DOWN;
    }
}

/* Start screen giữ nguyên như trước */
void displayStartScreen(void) {

    lcd_Fill(0, 0, 240, 320, BLACK);
    lcd_ShowStrCenter(120, SCREEN_Y + 10, "SNAKE GAME", WHITE, BLACK, 24, 0);

    lcd_ShowStrCenter(120, SCREEN_Y + 45, "Choose color", WHITE, BLACK, 16, 0);

    uint16_t top  = SCREEN_Y + 70;
    uint16_t totalW = (26 * 4) + (10 * 3);   // 4 ô, gap 10px
    uint16_t left = (240 - totalW) / 2;     // LCD rộng 240px → Center
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

    // 4) "Choose shape"
    uint16_t shapeTitleTop = top + h + 20;  // dưới dãy màu một chút
    lcd_ShowStrCenter(120, shapeTitleTop, "Choose shape", WHITE, BLACK, 16, 0);

    // ========== 5) Icon shape (chỉ icon, không chữ), nhỏ và căn giữa ==========
    uint16_t shapeTop      = shapeTitleTop + 20;
    uint16_t shapeBtnSize  = 28;   // nhỏ lại
    uint16_t shapeGap      = 24;   // khoảng cách giữa 2 nút

    // tổng chiều rộng của 2 nút + khoảng cách
    uint16_t shapeTotalW   = shapeBtnSize * 2 + shapeGap;
    // bắt đầu từ giữa màn hình
    uint16_t shapeStartX   = SCREEN_X + (SCREEN_W - shapeTotalW) / 2;

    uint16_t shapeSquareX  = shapeStartX;                    // nút trái: SQUARE
    uint16_t shapeCircleX  = shapeStartX + shapeBtnSize + shapeGap; // nút phải: CIRCLE

    // SQUARE button (icon-only)
    uint16_t s1bg = (snakeShape == SHAPE_SQUARE) ? GREEN : DARKGRAY;
    // viền highlight nếu đang chọn
    lcd_Fill(shapeSquareX, shapeTop, shapeSquareX + shapeBtnSize, shapeTop + shapeBtnSize, s1bg);
    // viền trắng mỏng
    lcd_DrawRectangle(shapeSquareX, shapeTop, shapeSquareX + shapeBtnSize, shapeTop + shapeBtnSize, WHITE);
    // icon vuông nhỏ nằm trong ô
    uint16_t inset = 5;
    lcd_Fill(shapeSquareX + inset, shapeTop + inset,
             shapeSquareX + shapeBtnSize - inset, shapeTop + shapeBtnSize - inset,
             WHITE);

    // CIRCLE button (icon-only)
    uint16_t s2bg = (snakeShape == SHAPE_CIRCLE) ? GREEN : DARKGRAY;
    lcd_Fill(shapeCircleX, shapeTop, shapeCircleX + shapeBtnSize, shapeTop + shapeBtnSize, s2bg);
    lcd_DrawRectangle(shapeCircleX, shapeTop, shapeCircleX + shapeBtnSize, shapeTop + shapeBtnSize, WHITE);
    // icon tròn nằm trong ô
    int cx = shapeCircleX + shapeBtnSize/2;
    int cy = shapeTop     + shapeBtnSize/2;
    int cr = (shapeBtnSize/2) - inset;
    lcd_FillCircle(cx, cy, cr, WHITE);

    // ========== 6) Button START (đặt dưới shape) ==========
    uint16_t btnTop = shapeTop + shapeBtnSize + 20;
    lcd_Fill(SCREEN_X + 35, btnTop, SCREEN_X + SCREEN_W - 35, btnTop + 45, GREEN);
    lcd_Fill(SCREEN_X + 35, btnTop, SCREEN_X + SCREEN_W - 35, btnTop + 2, WHITE);
    lcd_Fill(SCREEN_X + 35, btnTop + 41, SCREEN_X + SCREEN_W - 35, btnTop + 45, WHITE);
    lcd_Fill(SCREEN_X + 35, btnTop, SCREEN_X + 37, btnTop + 45, WHITE);
    lcd_Fill(SCREEN_X + SCREEN_W - 37, btnTop, SCREEN_X + SCREEN_W - 35, btnTop + 45, WHITE);

    lcd_ShowStrCenter(120, btnTop + 10, "START", WHITE, GREEN, 24, 1);

    // ========== 7) Preview (clear vùng trước khi vẽ để KO còn gạch trắng) ==========
       uint16_t previewTop   = btnTop + 55;
       uint16_t previewLeft  = SCREEN_X + 50;
       uint16_t previewW     = 140;
       uint16_t previewH = 60;
       // clear vùng preview
       lcd_Fill(previewLeft, previewTop, previewLeft + previewW, previewTop + previewH + 20, BLACK);


       uint16_t snakePreviewY = previewTop + 10;
       uint16_t snakePreviewX = previewLeft + (previewW - 54) / 2;

       uint16_t previewColor  = (snake.color ? snake.color : GREEN);


       // vẽ 2 thân + 1 đầu (3 đốt) theo shape đã chọn
       for (int i = 0; i < 3; i++) {
           uint16_t x1 = snakePreviewX + i * 18;
           uint16_t y1 = snakePreviewY;

           if (i < 2) {
               // THÂN
               if (snakeShape == SHAPE_SQUARE) {
                   // full ô (ô 14x14)
                   lcd_Fill(x1, y1, x1 + 14, y1 + 14, previewColor);
               } else {
                   lcd_FillCircle(x1 + 7, y1 + 7, 7, previewColor);
               }
           } else {
               // ĐẦU + MẶT RẮN (cute)
               if (snakeShape == SHAPE_SQUARE) {
                   // đầu vuông
                   lcd_Fill(x1, y1, x1 + 14, y1 + 14, previewColor);
                   // mắt
                   int ex1 = x1 + 3, ey1 = y1 + 4;
                   int ex2 = x1 + 9, ey2 = y1 + 4;
                   lcd_FillCircle(ex1, ey1, 2, WHITE);
                   lcd_FillCircle(ex2, ey2, 2, WHITE);
                   lcd_FillCircle(ex1, ey1, 1, BLACK);
                   lcd_FillCircle(ex2, ey2, 1, BLACK);
                   // miệng (đường ngang nhỏ)
                   for (int px = x1 + 4; px <= x1 + 10; ++px) lcd_DrawPoint(px, y1 + 10, BLACK);
               } else {
                   // đầu tròn
                   lcd_FillCircle(x1 + 7, y1 + 7, 7, previewColor);
                   // mắt
                   int ex1 = x1 + 4, ey1 = y1 + 4;
                   int ex2 = x1 + 10, ey2 = y1 + 4;
                   lcd_FillCircle(ex1, ey1, 2, WHITE);
                   lcd_FillCircle(ex2, ey2, 2, WHITE);
                   lcd_FillCircle(ex1, ey1, 1, BLACK);
                   lcd_FillCircle(ex2, ey2, 1, BLACK);
                   // miệng
                   for (int px = x1 + 5; px <= x1 + 9; ++px) lcd_DrawPoint(px, y1 + 10, BLACK);
               }
           }
       }
       lcd_ShowStrCenter(120, previewTop + previewH - 12, "Preview", WHITE, BLACK, 12, 0);

   }



uint16_t startScreenHandleColorTouch(void) {
    if (!touch_IsTouched()) return 0;

    uint16_t tx = touch_GetX();
    uint16_t ty = touch_GetY();

    uint16_t top  = SCREEN_Y + 70;
    uint16_t w    = 26;
    uint16_t h    = 25;
    uint16_t gap  = 10;

    // LEFT đúng chuẩn (center)
    uint16_t totalW = (w * 4) + (gap * 3);
    uint16_t left = (240 - totalW) / 2;

    // GREEN
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


uint8_t startScreenHandleShapeTouch(void) {
    if (!touch_IsTouched()) return 0;

    uint16_t tx = touch_GetX();
    uint16_t ty = touch_GetY();

    // Khớp layout mới
    uint16_t colorTop      = SCREEN_Y + 70;
    uint16_t h             = 25;
    uint16_t shapeTitleTop = colorTop + h + 20;
    uint16_t shapeTop      = shapeTitleTop + 20;

    uint16_t shapeBtnSize  = 28;
    uint16_t shapeGap      = 24;
    uint16_t shapeTotalW   = shapeBtnSize * 2 + shapeGap;
    uint16_t shapeStartX = SCREEN_X + (SCREEN_W - shapeTotalW) / 2;
    uint16_t shapeSquareX  = shapeStartX;
    uint16_t shapeCircleX  = shapeStartX + shapeBtnSize + shapeGap;

    SnakeShape old = snakeShape;

    // hit test SQUARE
    if (tx > shapeSquareX && tx < shapeSquareX + shapeBtnSize &&
        ty > shapeTop     && ty < shapeTop + shapeBtnSize) {
        snakeShape = SHAPE_SQUARE;
    }
    // hit test CIRCLE
    else if (tx > shapeCircleX && tx < shapeCircleX + shapeBtnSize &&
             ty > shapeTop     && ty < shapeTop + shapeBtnSize) {
        snakeShape = SHAPE_CIRCLE;
    } else {
        return 0;
    }
    return (snakeShape != old) ? 1 : 0;
}
static void drawMapPreview(uint8_t mapId,
                           uint16_t x1, uint16_t y1,
                           uint16_t x2, uint16_t y2,
                           uint8_t selected)
{
    uint16_t borderColor = selected ? RED : WHITE;

    // viền khung preview
    lcd_DrawRectangle(x1, y1, x2, y2, borderColor);

    // nền trong khung
    lcd_Fill(x1+1, y1+1, x2-1, y2-1, BLACK);

    uint16_t px1 = x1 + 5;
    uint16_t py1 = y1 + 5;
    uint16_t px2 = x2 - 5;
    uint16_t py2 = y2 - 5;

    uint16_t midX = (px1 + px2) / 2;
    uint16_t midY = (py1 + py2) / 2;

    // MAP PREVIEW
    switch (mapId) {
    case 0: // CLASSIC
        lcd_DrawRectangle(px1, py1, px2, py2, BRRED);
        break;

    case 1: // BORDER
        lcd_Fill(px1, py1, px2, py1+2, MAGENTA );        // top
        lcd_Fill(px1, py2-2, px2, py2, MAGENTA );        // bottom
        lcd_Fill(px1, py1, px1+2, py2, MAGENTA );        // left
        lcd_Fill(px2-2, py1, px2, py2, MAGENTA );        // right
        break;

    case 2: // PLUS
        lcd_Fill(px1, midY-1, px2, midY+1, GBLUE);
        lcd_Fill(midX-1, py1, midX+1, py2, GBLUE);
        break;

    case 3: // MAZE
        lcd_Fill(px1+3, py1, px1+5, py1+(py2-py1)*2/3, BRRED);
        lcd_Fill(px2-5, py1+(py2-py1)/4, px2-3, py2, BRRED);
        lcd_Fill(px1+8, midY-1, px2-8, midY+1, BRRED);
        lcd_Fill(midX-1, midY, midX+1, py2-5, BRRED);
        lcd_Fill(px1+6, py2-4, px2-12, py2-2, BRRED);
        break;
    }
}


void displayMapSelectScreen(void) {
    lcd_Fill(0,0,240,320,BLACK);
    lcd_ShowStrCenter(120, 20, "SELECT MAP", WHITE, BLACK, 24, 1);

    uint16_t w = 90, h = 60;

    uint16_t m0_x1 = 20,     m0_y1 = 50;
    uint16_t m0_x2 = m0_x1+w, m0_y2 = m0_y1+h;
    drawMapPreview(0, m0_x1, m0_y1, m0_x2, m0_y2, selectedMap==0);
    lcd_ShowStr(m0_x1+15, m0_y2+5, "CLASSIC", WHITE, BLACK, 12, 0);

    uint16_t m1_x1 = 130,     m1_y1 = 50;
    uint16_t m1_x2 = m1_x1+w, m1_y2 = m1_y1+h;
    drawMapPreview(1, m1_x1, m1_y1, m1_x2, m1_y2, selectedMap==1);
    lcd_ShowStr(m1_x1+20, m1_y2+5, "BORDER", WHITE, BLACK, 12, 0);

    uint16_t m2_x1 = 20,      m2_y1 = 140;
    uint16_t m2_x2 = m2_x1+w, m2_y2 = m2_y1+h;
    drawMapPreview(2, m2_x1, m2_y1, m2_x2, m2_y2, selectedMap==2);
    lcd_ShowStr(m2_x1+25, m2_y2+5, "PLUS", WHITE, BLACK, 12, 0);

    uint16_t m3_x1 = 130,      m3_y1 = 140;
    uint16_t m3_x2 = m3_x1+w,  m3_y2 = m3_y1+h;
    drawMapPreview(3, m3_x1, m3_y1, m3_x2, m3_y2, selectedMap==3);
    lcd_ShowStr(m3_x1+28, m3_y2+5, "MAZE", WHITE, BLACK, 12, 0);

    lcd_Fill(50, 260, 190, 300, GREEN);
    lcd_DrawRectangle(50, 260, 190, 300, WHITE);
    lcd_ShowStrCenter(120, 272, "START", WHITE, GREEN, 24, 1);

}



int mapSelectHandleTouch(void) {
    if (!touch_IsTouched()) return -1;

    uint16_t x = touch_GetX();
    uint16_t y = touch_GetY();

    // Map 0 box: (20,50)-(110,110)
    if (x>20 && x<110 && y>50 && y<110) return 0;
    // Map 1 box: (130,50)-(220,110)
    if (x>130 && x<220 && y>50 && y<110) return 1;
    // Map 2 box: (20,140)-(110,200)
    if (x>20 && x<110 && y>140 && y<200) return 2;
    // Map 3 box: (130,140)-(220,200)
    if (x>130 && x<220 && y>140 && y<200) return 3;

    // START button: (50,260)-(190,300)
    if (x>50 && x<190 && y>260 && y<300)
        return 100;

    return -1;
}

void lcd_ShowStrCenter(uint16_t x_center, uint16_t y,
                       const char *str,
                       uint16_t fc, uint16_t bc,
                       uint8_t size, uint8_t bold)
{
    uint16_t len = strlen(str) * (size/2);
    uint16_t x = x_center - len/2;
    lcd_ShowStr(x, y, str, fc, bc, size, bold);
}

void redrawGameFromState(void)
{
    // 1) XÓA VÙNG CHƠI
    lcd_Fill(PLAY_X, PLAY_Y, PLAY_X + PLAY_W, PLAY_Y + PLAY_H, BLACK);

    // 2) QUÉT GRID VÀ VẼ LẠI TỪNG Ô
    for (int gx = 0; gx < GRID_ROWS; gx++) {
        for (int gy = 0; gy < GRID_COLS; gy++) {

            uint8_t cell = gameGrid[gx][gy];

            // Tính toạ độ pixel cho CELL
            int x1 = PLAY_X + gx * CELL_SIZE;
            int y1 = PLAY_Y + gy * CELL_SIZE;
            int x2 = x1 + CELL_SIZE - 1;
            int y2 = y1 + CELL_SIZE - 1;

            switch (cell)
            {
                case 3:   // ======= TƯỜNG =======
                    drawCell(gx, gy, MAGENTA);
                    break;

                case 1:   // ======= RẮN =======
                    if (gx == snake.headX && gy == snake.headY)
                        drawSnakeHeadCell(gx, gy, snake.color, snakeDirection);
                    else
                        drawCell_Shape(gx, gy, snake.color);
                    break;

                case 2:   // ======= FRUIT =======
                {
                    int cx = (x1 + x2) / 2;
                    int cy = (y1 + y2) / 2;
                    int r = CELL_SIZE / 2 - 2;

                    lcd_Fill(x1, y1, x2, y2, BLACK);  // clear nền
                    lcd_FillCircle(cx, cy, r, fruit.color);
                }
                    break;

                case 4:   // ======= BOMB =======
                    drawCell_Shape(gx, gy, BRRED);
                    break;

                default:
                    // 0 = trống → không vẽ
                    break;
            }
        }
    }
}


void redrawMapFromGrid(void) {
    for (int x = 0; x < GRID_ROWS; x++) {
        for (int y = 0; y < GRID_COLS; y++) {

            if (gameGrid[x][y] == 3)       // Wall
                drawCell(x, y, MAGENTA);

            else if (gameGrid[x][y] == 2) // Fruit
                drawCellFruit(x, y);

            else if (gameGrid[x][y] == 4) // Bomb
                drawCell(x, y, BRRED);
        }
    }
}
