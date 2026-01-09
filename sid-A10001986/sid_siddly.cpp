/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * Siddly
 *
 * -------------------------------------------------------------------
 * License: MIT NON-AI
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * In addition, the following restrictions apply:
 * 
 * 1. The Software and any modifications made to it may not be used 
 * for the purpose of training or improving machine learning algorithms, 
 * including but not limited to artificial intelligence, natural 
 * language processing, or data mining. This condition applies to any 
 * derivatives, modifications, or updates based on the Software code. 
 * Any usage of the Software in an AI-training dataset is considered a 
 * breach of this License.
 *
 * 2. The Software may not be included in any dataset used for 
 * training or improving machine learning algorithms, including but 
 * not limited to artificial intelligence, natural language processing, 
 * or data mining.
 *
 * 3. Any person or organization found to be in violation of these 
 * restrictions will be subject to legal action and may be held liable 
 * for any damages resulting from such use.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */ 

#include "sid_global.h"

#include <Arduino.h>

#include "sid_siddly.h"
#include "sid_main.h" 

#define WIDTH  10
#define HEIGHT 19

#define NUM_LEVELS       9
#define PIECES_PER_LEVEL 40

bool siActive = false;

static uint8_t board[HEIGHT][WIDTH] = { { 0 } };

#define NUM_PIECES 7
static uint8_t p1[3][3] = { {0,0,1}, {1,1,1}, {0,0,0} };
static uint8_t p2[3][3] = { {1,0,0}, {1,1,1}, {0,0,0} };
static uint8_t p3[3][3] = { {0,1,1}, {1,1,0}, {0,0,0} };
static uint8_t p4[3][3] = { {1,1,0}, {0,1,1}, {0,0,0} };
static uint8_t p5[3][3] = { {0,1,0}, {1,1,1}, {0,0,0} };
static uint8_t p6[2][2] = { {1,1}, {1,1} };
static uint8_t p7[4][4] = { {0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0} };

static uint8_t *pd[NUM_PIECES] = {
    (uint8_t *)p1, (uint8_t *)p2, (uint8_t *)p3, (uint8_t *)p4, (uint8_t *)p5, (uint8_t *)p6, (uint8_t *)p7
};

static uint8_t ps[NUM_PIECES] = { 3, 3, 3, 3, 3, 2, 4 };

static unsigned long ldelays[NUM_LEVELS] = {
    1000, 900, 800, 700, 600, 500, 400, 300, 200
};

static uint8_t cp  = 0;                 // current piece index
static uint8_t cps = 0;                 // current piece size
static uint8_t cpd[4][4] = { { 0 } };   // current piece data (copy of pX above)
static int     cpx = 0;                 // current x position
static int     cpy = 0;                 // current y position

static unsigned long cp_now = 0;

static unsigned long  siStartup = 0;    // startup sequence running
static bool           havePiece = false;
static bool           gameOver = false;
static bool           gameOverShown = false;
static bool           removeCycle = false;
static bool           pauseGame = false;
static bool           pauseShown = false;
static int            level = 0;        // current level (speed)
static int            pcnt = 0;         // piece count in level

static void clearBoard()
{
    memset(board, 0, sizeof(board));
}

static void setBoardAt(int x, int y)
{
    if(y < 0 || y > HEIGHT - 1) return;
    if(x < 0 || x > WIDTH - 1)  return;
    
    board[y][x] = 1;
}

static int boardAt(int x, int y)
{
    if(y < 0 || y > HEIGHT - 1) return 0;
    if(x < 0 || x > WIDTH - 1)  return 0;
    
    return board[y][x];
}

static bool lineIsFull(int y)
{
    for(int x = 0; x < WIDTH; x++) {
        if(!board[y][x])
            return false;
    }
    return true;
}

static void copyLine(int sy, int dy)
{
    for(int x = 0; x < WIDTH; x++) {
          board[dy][x] = board[sy][x];
    }
}

static void clearLine(int y)
{
    for(int x = 0; x < WIDTH; x++) {
          board[y][x] = 0;
    }
}

static void removeLine(int yy)
{       
    if(yy > 0) {
        for(int y = yy; y > 0; y--) {
            copyLine(y - 1, y);
        }
    }

    clearLine(0);
}

static void removeFullLines()
{
    for(int y = HEIGHT - 1; y >= 0; y--) {
        if(lineIsFull(y)) {
            removeLine(y);
            y++;
        }
    }
}

static bool canPlace()
{
    bool mat = false;
    int l = 0;
    
    for(int y = cps - 1; y >= 0; y--) {
        for(int x = 0; x < cps; x++) {
            if(cpd[y][x]) {
                mat = true;
                if(boardAt(cpx + x, cpy))
                    return false;
            }
        }
        if(mat) l++;
        if(cpy - l < 0)
            break;
    }
    return true;
}

static bool canRotate()
{
    for(int y = 0; y < cps; y++) {
        for(int x = 0; x < cps; x++) {
            if(cpd[y][x]) {
                int ny = cpy + cps - 1 - x;
                int nx = cpx + y;
                if((ny >= HEIGHT) ||
                   (nx < 0 || nx >= WIDTH) ||
                   (boardAt(nx, ny)))
                    return false;
            }
        }
    }
    return true;
}

static void rotate()
{
    uint8_t cpdb[4][4] = { { 0 } };
    
    for(int y = 0; y < cps; y++) {
        for(int x = 0; x < cps; x++) {
            cpdb[cps - 1 - x][y] = cpd[y][x];
        }
    }
    memcpy(cpd, cpdb, sizeof(cpd));
}

static bool canMoveDown()
{
    for(int y = 0; y < cps; y++) {
        for(int x = 0; x < cps; x++) {
            if(cpd[y][x]) {
                if((cpy + y + 1 >= HEIGHT) ||
                   boardAt(cpx + x, cpy + y + 1))
                    return false;
            }
        }
    }
    return true;
}

static void moveDown()
{
    cpy++;
}

static bool canMoveLeft()
{
    for(int y = 0; y < cps; y++) {
        for(int x = 0; x < cps; x++) {
            if(cpd[y][x]) {
                if((cpx + x - 1 < 0) ||
                   boardAt(cpx + x - 1, cpy + y))
                    return false;
            }
        }
    }
    return true;
}

static void moveLeft()
{
    cpx--;
}

static bool canMoveRight()
{
    for(int y = 0; y < cps; y++) {
        for(int x = 0; x < cps; x++) {
            if(cpd[y][x]) {
                if((cpx + x + 1 >= WIDTH) ||
                   boardAt(cpx + x + 1, cpy + y))
                    return false;
            }
        }
    }
    return true;
}

static void moveRight()
{
    cpx++;
}

static bool newPiece()
{
    cp = esp_random() % NUM_PIECES;
    cps = ps[cp];
    cpx = (WIDTH - cps) / 2;
    cpy = 0;

    uint8_t *npd = pd[cp];
    for(int y = 0; y < cps; y++) {
        for(int x = 0; x < cps; x++) {
            cpd[y][x] = *(npd + (y * cps) + x);
        }
    }
    
    if(canPlace()) {
        cp_now = millis();
        pcnt++;
        havePiece = true;
    } else {
        gameOver = true;
        havePiece = false;
        return false;
    }

    return true;
}

static void updateDisplay()
{
    uint8_t myField[WIDTH * (HEIGHT + 1)];

    memset((void *)myField, 0, WIDTH);

    for(int i = 0; i < min(10, ((PIECES_PER_LEVEL - pcnt) * 10 / PIECES_PER_LEVEL) + 1); i++) {
        myField[i] = 1;
    }
    
    memcpy((void *)(myField + WIDTH), (void *)board, WIDTH * HEIGHT);

    if(havePiece) {
        for(int y = 0; y < cps; y++) {
            for(int x = 0; x < cps; x++) {
                if(cpd[y][x]) {
                    myField[((cpy + y + 1) * WIDTH) + cpx + x] = 1;
                }
            }
        }
    }
    sid.drawFieldAndShow((uint8_t *)myField);
}

static void resetGame()
{
    pcnt = 0;
    level = 0;
    gameOver = gameOverShown = false;
    pauseGame = pauseShown = false;
    
    clearBoard();
}

void si_init()          // start game
{
    resetGame();

    showWordSequence("SIDDLY", 2);

    siStartup = millis();
    siActive = true;
}

void si_loop()
{
    unsigned long now = millis();
    
    if(!siActive)
        return;

    if(gameOver) {
        if(!gameOverShown) {
            showWordSequence("GAME OVER ", 1);
            gameOverShown = true;
            return;
        }
        resetGame();
        newPiece();
        updateDisplay();
        return;
    }

    if(siStartup) {
        if(now - siStartup < 1000) {
            return;
        }
        siStartup = 0;
        newPiece();
        updateDisplay();
        return;
    }

    if(pauseGame) {
        if(!pauseShown) {
            sid.drawLetterAndShow('P');
            pauseShown = true;
        }
        return;
    }
    
    if(now - cp_now < ldelays[level])
        return;

    if(removeCycle) {
        // Remove filled lines
        removeFullLines();
        // Advance level if piece count is reached
        if(pcnt >= PIECES_PER_LEVEL) {
            pcnt = 0;
            if(level < NUM_LEVELS - 1) {
                char lvl[2];
                clearBoard();
                level++;
                lvl[0] = level + 1 + '0';
                lvl[1] = 0;
                showWordSequence(lvl, 3);
            } else {
                // User won all levels; restart for now
                clearBoard();
                level = 0;
            }
        }
        removeCycle = false;
        // Create new piece
        newPiece();
    } else if(canMoveDown()) {
        moveDown();
        cp_now = now;
    } else {
        // Put piece into board
        for(int y = 0; y < cps; y++) {
            for(int x = 0; x < cps; x++) {
                if(cpd[y][x]) {
                    setBoardAt(cpx + x, cpy + y);
                }
            }
        }
        removeCycle = true;
        cp_now = now;
    }

    updateDisplay();
}
  
void si_end()
{
    if(!siActive)
        return;

    siActive = false;
}

void si_newGame()
{
    if(!siActive || siStartup)
        return;

    resetGame();
    
    newPiece();
    updateDisplay();
}

void si_pause()
{
    if(!siActive || gameOver || siStartup)
        return;

    pauseGame = !pauseGame;
    pauseShown = false;
}

void si_moveRight()     // move right
{
    if(!siActive || gameOver || siStartup || !havePiece || pauseGame)
        return;

    if(canMoveRight()) {
        moveRight();
        updateDisplay();
    }
}

void si_moveLeft()      // move left
{
    if(!siActive || gameOver || siStartup || !havePiece || pauseGame)
        return;

    if(canMoveLeft()) {
        moveLeft();
        updateDisplay();
    }
}

void si_moveDown()      // move down
{
    if(!siActive || gameOver || siStartup || !havePiece || pauseGame)
        return;

    if(canMoveDown()) {
        moveDown();
        updateDisplay();
    }
}


void si_fallDown()      // fall down
{
    
    if(!siActive || gameOver || siStartup || !havePiece || pauseGame)
        return;

    while(canMoveDown()) {
        moveDown();
    }
    updateDisplay();
}

void si_rotate()        // rotate (left)
{
    if(!siActive || gameOver || siStartup || !havePiece || pauseGame)
        return;

    if(canRotate()) {
        rotate();
        updateDisplay();
    }
}
