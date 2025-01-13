/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * Snake
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

#include "sid_snake.h"
#include "sid_main.h" 

#define WIDTH  10
#define HEIGHT 20

#define MAXLENGTH 100

#define NUM_LEVELS       9
#define APPLES_PER_LEVEL 15

bool snActive = false;

static int spx = 0;
static int spy = 0;
static int sdx = 0;
static int sdy = 0;
static int scl = 0;
static int sml = 0;
static int snake[MAXLENGTH][2] = { { 0, 0 } };

static int apx = 0;
static int apy = 0;

static const unsigned long ldelays[NUM_LEVELS] = {
    700, 600, 550, 500, 450, 400, 300, 200, 100
};

static unsigned long  cp_now = 0;

static unsigned long  snStartup = 0;    // startup sequence running
static bool           gameOver = false;
static bool           gameOverShown = false;
static bool           pauseGame = false;
static bool           pauseShown = false;
static int            level = 0;        // current level (speed)
static int            acnt = 0;         // apple count in level

static void updateDisplay()
{
    uint8_t myField[WIDTH * HEIGHT] = { 0 };

    // Snake
    for(int i = 0; i < scl - 1; i++) {
        myField[(snake[i][1] * WIDTH) + snake[i][0]] = 1;
    }

    // Apple
    if(apx >= 0) {
        myField[(apy * WIDTH) + apx] = 1;
    }
    
    sid.drawFieldAndShow((uint8_t *)myField);
}

static void shiftSnake()
{
    for(int i = MAXLENGTH - 2; i >= 0; i--) {
        snake[i+1][0] = snake[i][0];
        snake[i+1][1] = snake[i][1];
    }
}

static bool appleHitsSnake()
{
    for(int i = 0; i < scl; i++) {
        if(apx == snake[i][0] && apy == snake[i][1])
            return true;
    }

    return false;
}

static void resetGame()
{
    spx = WIDTH / 2;    // pos of head
    spy = HEIGHT / 2;
    sdx = 1;            // movement deltas
    sdy = 0;
    scl = 4;            // current length incl head

    for(int i = 0; i < scl; i++) {
      snake[i][0] = spx - i;
      snake[i][1] = spy;
    }

    apx = WIDTH / 4;    // apple position
    apy = HEIGHT / 4;

    acnt = 1;

    gameOver = gameOverShown = false;
    pauseGame = pauseShown = false;
}

void sn_init()
{
    resetGame();
    level = 0;
    
    showWordSequence("SNAKE", 2);

    snStartup = millis();
    snActive = true;
}

void sn_loop()
{
    unsigned long now = millis();
    bool skipCheck = false;
    bool newApple = false;
    
    if(!snActive)
        return;

    if(gameOver) {
        if(!gameOverShown) {
            showWordSequence("GAME OVER ", 1);
            gameOverShown = true;
            return; 
        }
        resetGame();
        level = 0;
        updateDisplay();
        cp_now = now;
        return;
    }

    if(snStartup) {
        if(now - snStartup < 1000) {
            return;
        }
        snStartup = 0;
        updateDisplay();
        cp_now = now;
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

    // Move snake
    spx += sdx;
    spy += sdy;

    // Wrap snake
    if(spx < 0) spx = WIDTH - 1;
    if(spx >= WIDTH) spx = 0;
    if(spy < 0) spy = HEIGHT - 1;
    if(spy >= HEIGHT) spy = 0;

    // Update snake[]
    shiftSnake();
    snake[0][0] = spx;
    snake[0][1] = spy;

    // Check if head hits apple
    if(spx == apx && spy == apy) {
        scl++;
        acnt++;
        if(scl >= MAXLENGTH || acnt > APPLES_PER_LEVEL) {
            char lvl[2];
            resetGame();
            if(level < NUM_LEVELS - 1) {
                char lvl[2];
                level++;
                lvl[0] = level + 1 + '0';
                lvl[1] = 0;
                showWordSequence(lvl, 3);
            } else {
                // User won all levels; restart for now
                level = 0;
            }
            skipCheck = true;
        } else
            newApple = true;
    }

    // Make new apply
    if(newApple) {
        do {
          apx = esp_random() % WIDTH;
          apy = esp_random() % HEIGHT;
        } while(appleHitsSnake());
    }

    // Check if any body parts of snake collide -> game over
    if(!skipCheck) {
        for(int i = 0; i < scl; i++) {
            int cx = snake[i][0], cy = snake[i][1];
            for(int j = i + 1; j < scl; j++) {
                if(snake[j][0] == cx && snake[j][1] == cy) {
                    gameOver = true;
                }
            }
        }
    }

    cp_now = now;
    updateDisplay();
}

void sn_end()
{
    if(!snActive)
        return;

    snActive = false;
}

void sn_newGame()
{
    if(!snActive || snStartup)
        return;

    resetGame();
    level = 0;
    updateDisplay();
}

void sn_pause()
{
    if(!snActive || gameOver || snStartup)
        return;

    pauseGame = !pauseGame;
    pauseShown = false;
}

void sn_moveRight()     // move right
{
    if(!snActive || gameOver || snStartup || pauseGame)
        return;

    sdx = 1;
    sdy = 0;
}

void sn_moveLeft()      // move left
{
    if(!snActive || gameOver || snStartup || pauseGame)
        return;

    sdx = -1;
    sdy = 0;
}

void sn_moveUp()      // move down
{
    if(!snActive || gameOver || snStartup || pauseGame)
        return;

    sdx = 0;
    sdy = -1;
}

void sn_moveDown()      // move down
{
    if(!snActive || gameOver || snStartup || pauseGame)
        return;

    sdx = 0;
    sdy = 1;
}
