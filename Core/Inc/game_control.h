#ifndef INC_GAME_CONTROL_H_
#define INC_GAME_CONTROL_H_

#include "software_timer.h"
#include "lcd.h"
#include "touch.h"
#include "game_display.h"
#include "button.h"


#define DIRECTION_BTN_X 50
#define DIRECTION_BTN_Y 160
#define DIRECTION_BTN_SIZE 30
extern uint8_t selectedMap;
typedef enum {
    GAME_INIT,
    GAME_START,
    GAME_PLAY,
    GAME_OVER,
    GAME_COLOR_SELECT,
    GAME_PAUSE,
    GAME_MAP_SELECT
} GameState;
typedef struct {
    uint16_t x, y;
    uint16_t color;
    uint8_t  type;
} Fruit;

typedef struct {
    uint16_t x, y;
    uint8_t  active;
    int16_t  ticks;
} Bomb;

extern Fruit fruit;
extern Bomb bomb;
extern GameState currentState;
void gameFSM(void);

uint8_t isStartScreenTouched(void);
void drawPlayfieldFrame(void);
uint8_t isRetryButtonTouched(void);

void updateSnakeDirection(void);

uint8_t isButtonUp(void);

uint8_t isButtonDown(void);
void redrawGameFromState(void);
void refreshUIAfterLoad(void);
uint8_t isButtonLeft(void);

uint8_t isButtonRight(void);
uint8_t isPhyButtonUpEdge(void);  //new
uint8_t isPhyButtonDownEdge(void);
uint8_t isPhyButtonLeftEdge(void);
uint8_t isPhyButtonRightEdge(void);
uint8_t isPhyStartEdge(void);
uint8_t isPhyPauseEdge(void);
void initializeButtons(void);
void displayGameOverScreen(void);
uint8_t isHomeButtonTouched(void);
uint8_t isPauseButtonTouched(void);

#endif /* INC_GAME_CONTROL_H_ */
