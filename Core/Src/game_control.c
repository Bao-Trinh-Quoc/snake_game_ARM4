/*
 * game_control.c
 *
 *  Created on: Oct 21, 2025
 *      Author: DELL
 */

#include "game_control.h"
#include "led7seg_app.h"
#include "game_display.h"

#define BTN_IDX_UP      1 //new
#define BTN_IDX_DOWN    9
#define BTN_IDX_LEFT    4
#define BTN_IDX_RIGHT   6

#define DARKGRAY 0xA9A9A9

#define PLAY_X (SCREEN_X + 1)
#define PLAY_Y (SCREEN_Y + 1)
#define PLAY_W (SCREEN_W - 1)
#define PLAY_H (SCREEN_H - 1)

uint8_t selectedMap = 0;


GameState currentState = GAME_START;

typedef struct {
    uint16_t xStart, yStart, xEnd, yEnd;
    uint8_t isPressed;
} ControlButton;
Fruit fruit;
Bomb bomb;

static ControlButton controlButtons[4];
static uint16_t score = 0;
static uint8_t startScreenDrawn = 0;
static uint8_t gameUIRendered = 0;
uint16_t highscore = 0;

void initializeButtons(void);

/* ====== Score UI (giữ nguyên vị trí tương thích layout hiện có) ====== */
#define SCORE_LABEL_X  70
#define SCORE_LABEL_Y  10
#define SCORE_NUM_X    120
#define SCORE_NUM_Y    10
#define SCORE_NUM_W    48
#define SCORE_NUM_H    16
static int lastScore = -1;
// [NEW] Khóa chống dính khi vừa vào GAME_START
static uint8_t startInputLock = 0;


static void updateScoreUI(void) {
    if (!gameUIRendered) return;
    if (lastScore == score) return;

    lcd_Fill(SCORE_NUM_X, SCORE_NUM_Y,
             SCORE_NUM_X + SCORE_NUM_W,
             SCORE_NUM_Y + SCORE_NUM_H,
             DARKBLUE);
    lcd_ShowIntNum(SCORE_NUM_X, SCORE_NUM_Y, score, 3, BLACK, WHITE, 16);
    lastScore = score;
}
/* ================================================================ */

/* ====== Game Over overlay (THÊM MỚI) ====== */
static uint8_t gameOverScreenDrawn = 0;

/* Khung và nút RESTART */
#define GO_BOX_X1    20
#define GO_BOX_Y1    70
#define GO_BOX_X2    220
#define GO_BOX_Y2    200

#define GO_BTN_W     100
#define GO_BTN_H     30
#define GO_BTN_X1    ((240 - GO_BTN_W)/2)
#define GO_BTN_Y1    (GO_BOX_Y2 - 40)
#define GO_BTN_X2    (GO_BTN_X1 + GO_BTN_W)
#define GO_BTN_Y2    (GO_BTN_Y1 + GO_BTN_H)

void displayGameOverScreen(void) {
    /* [NEW] clear full screen to avoid overlay */
    lcd_Fill(0, 0, 240, 320, BLACK);

    /* hộp */
    lcd_DrawRectangle(GO_BOX_X1, GO_BOX_Y1, GO_BOX_X2, GO_BOX_Y2, WHITE);
    lcd_Fill(GO_BOX_X1+1, GO_BOX_Y1+1, GO_BOX_X2-1, GO_BOX_Y2-1, BLACK);

    /* tiêu đề + điểm */
    lcd_ShowStr(65, GO_BOX_Y1 + 15, "GAME OVER", RED, BLACK, 24, 1);
    lcd_ShowStr(70, GO_BOX_Y1 + 50, "Score:", WHITE, BLACK, 16, 0);
    lcd_ShowIntNum(120, GO_BOX_Y1 + 50, score, 3, WHITE, BLACK, 16);

    /* nút RESTART */
    lcd_Fill(GO_BTN_X1, GO_BTN_Y1, GO_BTN_X2, GO_BTN_Y2, WHITE);
    lcd_DrawRectangle(GO_BTN_X1, GO_BTN_Y1, GO_BTN_X2, GO_BTN_Y2, BLACK);
    lcd_ShowStr(GO_BTN_X1 + 15, GO_BTN_Y1 + 7, "RESTART", BLACK, WHITE, 16, 0);
}


static inline uint8_t isRestartTouched(void) {
    return touch_IsTouched() &&
           touch_GetX() > GO_BTN_X1 && touch_GetX() < GO_BTN_X2 &&
           touch_GetY() > GO_BTN_Y1 && touch_GetY() < GO_BTN_Y2;
}
/* ========================================== */

void gameFSM(void) {
    switch (currentState) {
        case GAME_INIT:
            score = 0;
            //highscore = 0;
            currentState = GAME_START;
            startInputLock = 1;   // [NEW] chờ nhả tay sau khi vào màn Start

            break;

        case GAME_START:
        	// [NEW] Nếu đang khóa: chỉ chờ nhả tay rồi mới cho bấm START/đổi màu
        	if (startInputLock) {
        	    if (!touch_IsTouched()) {
        	        startInputLock = 0;   // đã nhả → mở khóa
        	    }
        	    break;                    // thoát case, KHÔNG xử lý startScreenHandleColorTouch/isStartScreenTouched
        	}
            if (!startScreenDrawn) {
                displayStartScreen();
                startScreenDrawn = 1;
            }
            {
                uint16_t c = startScreenHandleColorTouch();
                if (c != 0) {
                    snake.color = c;
                    displayStartScreen();
                }
            }
            // Chọn shape

             {
            	 uint8_t changed = startScreenHandleShapeTouch();
            	 if (changed) {
            		 displayStartScreen();   // vẽ lại để highlight + Preview
            	 }
            }
            if (isStartScreenTouched()) {
            	buzzer_SetVolume(90);
            	    HAL_Delay(20);
            	    buzzer_SetVolume(0);
                currentState = GAME_MAP_SELECT;
                at24c_WriteOneByte(0x0000, currentState);
                lcd_Fill(0,0,240,320,BLACK);
                displayMapSelectScreen();
            }

            break;

        case GAME_PLAY:
            if (!gameUIRendered) {
                initializeButtons();
                gameUIRendered = 1;
                updateScoreUI();
            }

            if (button_read_flag) {
                setTimer_button(5);
                handleInput();
            }

            if (snake_move_flag) {
                HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin);

                int16_t nextX = (int16_t)snake.headX;
                int16_t nextY = (int16_t)snake.headY;

                switch (snakeDirection) {
                    case UP:    nextY--; break;
                    case DOWN:  nextY++; break;
                    case LEFT:  nextX--; break;
                    case RIGHT: nextX++; break;
                }

                /* wrap biên */
                if (nextX < 0)                nextX = GRID_ROWS - 1;
                else if (nextX >= GRID_ROWS)  nextX = 0;
                if (nextY < 0)                nextY = GRID_COLS - 1;
                else if (nextY >= GRID_COLS)  nextY = 0;
                uint8_t cell = gameGrid[nextX][nextY];

                if (cell == 1 || cell == 3 || cell == 4) {
                    /* 1 = thân rắn, 3 = tường, 4 = bomb → Game Over */
                	buzzer_SetVolume(90);   // âm lớn khi chết
                	    HAL_Delay(300);
                	    buzzer_SetVolume(0);
                    currentState         = GAME_OVER;
                    at24c_WriteOneByte(0x0000, currentState);
                    gameOverScreenDrawn  = 0;
                    gameUIRendered       = 0;
                    break;
                }
                else if (cell == 2) {
                    /* ăn mồi */
                    buzzer_SetVolume(90);   // kêu 1 tí khi ăn trái
                    HAL_Delay(20);
                    buzzer_SetVolume(0);
                    score++;
                    if (score > highscore) highscore = score;
                    led7_show_score_dual(score, highscore);
                    advanceSnakeHeadTo(nextX, nextY);
                    generateFruit();     // generateFruit giờ sẽ sinh thêm bomb
                    updateScoreUI();
                }
                else {
                    /* ô trống */
                    advanceSnakeHeadTo(nextX, nextY);
                    removeSnakeTail();
                }
                updateBombLifetime();

                setTimer_snake(300);
            }

            if (isHomeButtonTouched()) {
                currentState = GAME_INIT;
                at24c_WriteOneByte(0x0000, currentState);
                startScreenDrawn = 0;
                score = 0;
                lastScore = -1;
                gameUIRendered = 0;
                lcd_Fill(0, 0, 240, 320, BLACK);
                break;
            }

            if (button_read_flag) {
                setTimer_button(5);
                handleInput();
            }

            if (isPauseButtonTouched()) {
                            currentState = GAME_PAUSE;
                        }
            break;

        case GAME_MAP_SELECT:
        {
            int sel = mapSelectHandleTouch();

            if (sel >= 0 && sel <= 3) {
            	buzzer_SetVolume(90);
            	    HAL_Delay(20);
            	    buzzer_SetVolume(0);

                selectedMap = sel;
                displayMapSelectScreen();
            }

            if (sel == 100) {  // START
            	buzzer_SetVolume(90);
            	    HAL_Delay(20);
            	    buzzer_SetVolume(0);
                lcd_Fill(0,0,240,320,BLACK);
                currentState = GAME_PLAY;
                at24c_WriteOneByte(0x0000, currentState);
                initializeGame();   // KHÔNG vẽ map bên trong hàm này nữa

                // vẽ map theo selectedMap
                switch (selectedMap) {
                    case 0: /* Classic: không vẽ tường */ break;
                    case 1: placeBorderWalls();   break;
                    case 2: placeObstaclePlus();  break;
                    case 3: placeMazeObstacles(); break;
                }


                setTimer_button(5);
                setTimer_snake(300);
            }
        }
        break;

        case GAME_PAUSE:
        	lcd_ShowStr(100, 110, "PAUSED", YELLOW, BLACK, 16, 1);
            // Nhấn PAUSE lần nữa -> tiếp tục chơi
            if (isPauseButtonTouched()) {
            	lcd_ShowStr(100, 110, "PAUSED", BLACK, BLACK, 16, 1);
                currentState = GAME_PLAY;
                // Bật lại timer để rắn và button hoạt động
                setTimer_button(5);
                setTimer_snake(300);
            }

            // Nhấn HOME -> về INIT, xoá màn, reset
            if (isHomeButtonTouched()) {
                currentState     = GAME_INIT;
                startScreenDrawn = 0;
                gameUIRendered   = 0;
                score            = 0;
                lastScore        = -1;
                lcd_Fill(0, 0, 240, 320, BLACK);
            }
            break;


        case GAME_OVER:
            /* vẽ overlay 1 lần, đợi người chơi bấm RESTART */
            if (!gameOverScreenDrawn) {
            	buzzer_SetVolume(90);
            	    HAL_Delay(300);
            	    buzzer_SetVolume(0);
                displayGameOverScreen();
                gameOverScreenDrawn = 1;
            }
            if (score > highscore) {
                    highscore = score;
                    at24c_WriteOneByte(0x0013, highscore >> 8);
                    at24c_WriteOneByte(0x0014, highscore & 0xFF);
                }

            if (isRestartTouched()) {
                currentState     = GAME_INIT;  /* theo yêu cầu: RESTART → về INIT */
                startInputLock = 1;   // [NEW] chống dính START khi vừa quay lại GameStart
                startScreenDrawn = 0; // (giữ nguyên dòng này nếu đã có)
                startScreenDrawn = 0;
                gameUIRendered   = 0;
                lastScore        = -1;
                lcd_Fill(0, 0, 240, 320, BLACK);
            }
            break;
    }
    led7_show_score_dual(score, highscore);

}

static void drawLineH(int x1, int x2, int y, uint16_t color)
{
    if (x2 < x1) { int t=x1; x1=x2; x2=t; }
    lcd_Fill(x1, y, x2, y, color);   // fill đúng 1 dòng
}


void initializeControlButtons(void) {

    int baseX = DIRECTION_BTN_X + 15;
    int baseY = DIRECTION_BTN_Y + 70;

    int S = DIRECTION_BTN_SIZE;       // SIZE thực tế của nút
    int GAP = 10;

    // UP
    controlButtons[0] = (ControlButton){
        .xStart = baseX + S + GAP,
        .yStart = baseY,
        .xEnd   = baseX + 2*S + GAP,
        .yEnd   = baseY + S
    };

    // DOWN
    controlButtons[1] = (ControlButton){
        .xStart = baseX + S + GAP,
        .yStart = baseY + S + GAP,
        .xEnd   = baseX + 2*S + GAP,
        .yEnd   = baseY + 2*S + GAP
    };

    // LEFT
    controlButtons[2] = (ControlButton){
        .xStart = baseX,
        .yStart = baseY + S + GAP,
        .xEnd   = baseX + S,
        .yEnd   = baseY + 2*S + GAP
    };

    // RIGHT
    controlButtons[3] = (ControlButton){
        .xStart = baseX + 2*S + 2*GAP,
        .yStart = baseY + S + GAP,
        .xEnd   = baseX + 3*S + 2*GAP,
        .yEnd   = baseY + 2*S + GAP
    };

    // ===== VẼ NÚT + TAM GIÁC ĐẶC =====
    // ===== VẼ 4 NÚT + TAM GIÁC ĐẶC =====
    for (int i = 0; i < 4; i++) {

        int cx = controlButtons[i].xStart + (S/2);
        int cy = controlButtons[i].yStart + (S/2);
        int r  = S/2;

        // 1) Vẽ nút tròn 2 lớp (NỀN)
        lcd_FillCircle(cx, cy, r, WHITE);
        lcd_FillCircle(cx, cy, r-3, LIGHTGRAY);

        // 2) Tính tam giác
        int tri = r - 6;   // giảm từ r-6 hoặc r-8 → r-12 để nhỏ gọn

        int x1, y1, x2, y2, x3, y3;

        if (i == 0) {          // UP
            x1 = cx;           y1 = cy - tri;
            x2 = cx - tri;     y2 = cy + tri/2;
            x3 = cx + tri;     y3 = cy + tri/2;
        }
        else if (i == 1) {     // DOWN
            x1 = cx;           y1 = cy + tri;
            x2 = cx - tri;     y2 = cy - tri/2;
            x3 = cx + tri;     y3 = cy - tri/2;
        }
        else if (i == 2) {     // LEFT
            x1 = cx - tri;     y1 = cy;
            x2 = cx + tri/2;   y2 = cy - tri;
            x3 = cx + tri/2;   y3 = cy + tri;
        }
        else {                 // RIGHT
            x1 = cx + tri;     y1 = cy;
            x2 = cx - tri/2;   y2 = cy - tri;
            x3 = cx - tri/2;   y3 = cy + tri;
        }

        // Vẽ tam giác rỗng – luôn hiển thị
        lcd_DrawLine(x1, y1, x2, y2, BLACK);
        lcd_DrawLine(x2, y2, x3, y3, BLACK);
        lcd_DrawLine(x3, y3, x1, y1, BLACK);
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
	lcd_Fill(0, 0, 240, PLAY_Y, CYAN);

	// Vùng UI dưới
	lcd_Fill(0, PLAY_Y + PLAY_H, 240, 320, CYAN);

	// Vùng UI trái
	lcd_Fill(0, PLAY_Y, PLAY_X, PLAY_Y + PLAY_H, CYAN);

	// Vùng UI phải
	lcd_Fill(PLAY_X + PLAY_W, PLAY_Y, 240, PLAY_Y + PLAY_H, CYAN);
    initializeControlButtons();
    lcd_Fill(0, 0, 240, 30, DARKBLUE); // Thanh màu xanh nhạt ở đầu

    // --- Nút HOME ---
    lcd_Fill(5, 2, 60, 25, WHITE);                      // Nền trắng cho nút
    lcd_DrawRectangle(5, 2, 60, 25, BLACK);             // Viền đen
    lcd_ShowStr(15, 7, "HOME", BLACK, WHITE, 16, 1);   // Chữ giữa nút, font đậm

    // --- SCORE hiển thị ở giữa ---
    lcd_ShowStr(70, 7, "SCORE:", WHITE, DARKBLUE, 16, 1);
    lcd_ShowIntNum(120, 7, score, 3, YELLOW, DARKBLUE, 16);


    // --- Nút PAUSE ---
    lcd_Fill(175, 2, 235,25, WHITE);                   // Nền trắng
    lcd_DrawRectangle(175, 2, 235, 25, BLACK);          // Viền đen
    lcd_ShowStr(185,7, "PAUSE", BLACK, WHITE, 16, 1); // Chữ giữa nút
}

uint8_t isButtonUp(void) {
    if (touch_IsTouched() &&
        touch_GetX() > controlButtons[0].xStart && touch_GetX() < controlButtons[0].xEnd &&
        touch_GetY() > controlButtons[0].yStart && touch_GetY() < controlButtons[0].yEnd) {
        return 1;
    }
    return 0;
}

uint8_t isButtonDown(void) {
    if (touch_IsTouched() &&
        touch_GetX() > controlButtons[1].xStart && touch_GetX() < controlButtons[1].xEnd &&
        touch_GetY() > controlButtons[1].yStart && touch_GetY() < controlButtons[1].yEnd) {
        return 1;
    }
    return 0;
}

uint8_t isButtonLeft(void) {
    if (touch_IsTouched() &&
        touch_GetX() > controlButtons[2].xStart && touch_GetX() < controlButtons[2].xEnd &&
        touch_GetY() > controlButtons[2].yStart && touch_GetY() < controlButtons[2].yEnd) {
        return 1;
    }
    return 0;
}

uint8_t isButtonRight(void) {
    if (touch_IsTouched() &&
        touch_GetX() > controlButtons[3].xStart && touch_GetX() < controlButtons[3].xEnd &&
        touch_GetY() > controlButtons[3].yStart && touch_GetY() < controlButtons[3].yEnd) {
        return 1;
    }
    return 0;
}

uint8_t isStartScreenTouched(void) {
    if (!touch_IsTouched()) return 0;

    uint16_t tx = touch_GetX();
    uint16_t ty = touch_GetY();

    // 4) "Choose shape"
    uint16_t colorTop = SCREEN_Y + 70;
    uint16_t h        = 25;
    uint16_t shapeTitleTop = colorTop + h + 20;

    // ====== 5) Shape icons ======
    uint16_t shapeTop      = shapeTitleTop + 20;
    uint16_t shapeBtnSize  = 28;

    // ====== 6) Button START ======
    uint16_t btnTop = shapeTop + shapeBtnSize + 20;

    uint16_t btnX1 = SCREEN_X + 35;
    uint16_t btnX2 = SCREEN_X + SCREEN_W - 35;
    uint16_t btnY1 = btnTop;
    uint16_t btnY2 = btnTop + 45;

    // ====== Kiểm tra chạm ======
    if (tx > btnX1 && tx < btnX2 &&
        ty > btnY1 && ty < btnY2)
    {
        return 1;
    }

    return 0;
}

uint8_t isPhyButtonUpEdge(void)    { return button_pressed_edge(BTN_IDX_UP);    }
uint8_t isPhyButtonDownEdge(void)  { return button_pressed_edge(BTN_IDX_DOWN);  }
uint8_t isPhyButtonLeftEdge(void)  { return button_pressed_edge(BTN_IDX_LEFT);  }
uint8_t isPhyButtonRightEdge(void) { return button_pressed_edge(BTN_IDX_RIGHT); }
void saveGameState() {
    at24c_WriteOneByte(0x0000, 0xA5);

    at24c_WriteOneByte(0x0001, selectedMap);

    at24c_WriteOneByte(0x0002, (score >> 8));
    at24c_WriteOneByte(0x0003, (score & 0xFF));

    at24c_WriteOneByte(0x0004, (snake.color >> 8));
    at24c_WriteOneByte(0x0005, (snake.color & 0xFF));

    at24c_WriteOneByte(0x0006, snake.headX);
    at24c_WriteOneByte(0x0007, snake.headY);
    at24c_WriteOneByte(0x0008, snake.tailX);
    at24c_WriteOneByte(0x0009, snake.tailY);
    at24c_WriteOneByte(0x000A, snakeDirection);

    // Fruit
    at24c_WriteOneByte(0x000B, fruit.x);
    at24c_WriteOneByte(0x000C, fruit.y);
    at24c_WriteOneByte(0x000D, fruit.type);
    at24c_WriteOneByte(0x0015, fruit.color >> 8);
    at24c_WriteOneByte(0x0016, fruit.color & 0xFF);

    // Bomb
    at24c_WriteOneByte(0x000E, bomb.active);
    at24c_WriteOneByte(0x000F, bomb.x);
    at24c_WriteOneByte(0x0010, bomb.y);
    at24c_WriteOneByte(0x0011, (bomb.ticks >> 8));
    at24c_WriteOneByte(0x0012, (bomb.ticks & 0xFF));

    at24c_WriteOneByte(0x0013, (highscore >> 8));
    at24c_WriteOneByte(0x0014, (highscore & 0xFF));

    for (int x = 0; x < GRID_ROWS; x++)
        for (int y = 0; y < GRID_COLS; y++)
            at24c_WriteOneByte(0x20 + x*GRID_COLS + y, gameGrid[x][y]);
}

void loadGameState() {
    if (at24c_ReadOneByte(0x0000) != 0xA5) return;

    selectedMap = at24c_ReadOneByte(0x0001);

    score = (at24c_ReadOneByte(0x0002) << 8) |
             at24c_ReadOneByte(0x0003);

    snake.color = (at24c_ReadOneByte(0x0004) << 8) |
                   at24c_ReadOneByte(0x0005);

    snake.headX = at24c_ReadOneByte(0x0006);
    snake.headY = at24c_ReadOneByte(0x0007);
    snake.tailX = at24c_ReadOneByte(0x0008);
    snake.tailY = at24c_ReadOneByte(0x0009);
    snakeDirection = at24c_ReadOneByte(0x000A);

    // Fruit
    fruit.x    = at24c_ReadOneByte(0x000B);
    fruit.y    = at24c_ReadOneByte(0x000C);
    fruit.type = at24c_ReadOneByte(0x000D);
    fruit.color = ((at24c_ReadOneByte(0x0015) << 8) |
                    at24c_ReadOneByte(0x0016));

    // Bomb
    bomb.active = at24c_ReadOneByte(0x000E);
    bomb.x      = at24c_ReadOneByte(0x000F);
    bomb.y      = at24c_ReadOneByte(0x0010);
    bomb.ticks  = (at24c_ReadOneByte(0x0011) << 8) |
                   at24c_ReadOneByte(0x0012);

    highscore = (at24c_ReadOneByte(0x0013) << 8) |
                 at24c_ReadOneByte(0x0014);

    for (int x = 0; x < GRID_ROWS; x++)
        for (int y = 0; y < GRID_COLS; y++)
            gameGrid[x][y] = at24c_ReadOneByte(0x20 + x*GRID_COLS + y);
}




void refreshUIAfterLoad(void) {
    lastScore = -1;  // ép updateScoreUI chạy lại
    gameUIRendered = 1;
    updateScoreUI();
    led7_show_score_dual(score, highscore);
}
// =====================================================================
