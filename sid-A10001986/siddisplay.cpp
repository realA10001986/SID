/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * SIDDisplay Classes: Handles the SID LEDs
 *
 * -------------------------------------------------------------------
 * License: MIT
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
#include <Wire.h>

#include "siddisplay.h"

#include "sid_font.h"

static const uint16_t translator[10][20][2] =
{ 
    { 
        { 8+2, 1<<3 },    // bar 0, top most LED   { index in buffer [0-7 chip1, 8-15 chip2], bitmask }
        { 8+2, 1<<2 },
        { 8+2, 1<<1 },
        { 8+2, 1<<0 },
        {   0, 1<<15 },
        {   0, 1<<14 },
        {   0, 1<<13 },
        {   0, 1<<12 },
        {   0, 1<<11 },
        {   0, 1<<10 },
        {   0, 1<<9 },
        {   0, 1<<8 },
        {   0, 1<<7 },
        {   0, 1<<6 },
        {   0, 1<<5 },
        {   0, 1<<4 },
        {   0, 1<<3 },
        {   0, 1<<2 },
        {   0, 1<<1 },
        {   0, 1<<0 }     // bar 0, bottom LED
    },
    { 
        { 8+3, 1<<3 },    // bar 1, top most LED
        { 8+3, 1<<2 },
        { 8+3, 1<<1 },
        { 8+3, 1<<0 },
        {   1, 1<<15 },
        {   1, 1<<14 },
        {   1, 1<<13 },
        {   1, 1<<12 },
        {   1, 1<<11 },
        {   1, 1<<10 },
        {   1, 1<<9 },
        {   1, 1<<8 },
        {   1, 1<<7 },
        {   1, 1<<6 },
        {   1, 1<<5 },
        {   1, 1<<4 },
        {   1, 1<<3 },
        {   1, 1<<2 },
        {   1, 1<<1 },
        {   1, 1<<0 } 
    },
    { 
        { 8+4, 1<<3 },    // bar 2, top most LED
        { 8+4, 1<<2 },
        { 8+4, 1<<1 },
        { 8+4, 1<<0 },
        {   2, 1<<15 },
        {   2, 1<<14 },
        {   2, 1<<13 },
        {   2, 1<<12 },
        {   2, 1<<11 },
        {   2, 1<<10 },
        {   2, 1<<9 },
        {   2, 1<<8 },
        {   2, 1<<7 },
        {   2, 1<<6 },
        {   2, 1<<5 },
        {   2, 1<<4 },
        {   2, 1<<3 },
        {   2, 1<<2 },
        {   2, 1<<1 },
        {   2, 1<<0 } 
    },
    { 
        { 8+5, 1<<3 },    // bar 3, top most LED
        { 8+5, 1<<2 },
        { 8+5, 1<<1 },
        { 8+5, 1<<0 },
        {   3, 1<<15 },
        {   3, 1<<14 },
        {   3, 1<<13 },
        {   3, 1<<12 },
        {   3, 1<<11 },
        {   3, 1<<10 },
        {   3, 1<<9 },
        {   3, 1<<8 },
        {   3, 1<<7 },
        {   3, 1<<6 },
        {   3, 1<<5 },
        {   3, 1<<4 },
        {   3, 1<<3 },
        {   3, 1<<2 },
        {   3, 1<<1 },
        {   3, 1<<0 }
    },
    { 
        { 8+6, 1<<3 },    // bar 4, top most LED
        { 8+6, 1<<2 },
        { 8+6, 1<<1 },
        { 8+6, 1<<0 },
        {   4, 1<<15 },
        {   4, 1<<14 },
        {   4, 1<<13 },
        {   4, 1<<12 },
        {   4, 1<<11 },
        {   4, 1<<10 },
        {   4, 1<<9 },
        {   4, 1<<8 },
        {   4, 1<<7 },
        {   4, 1<<6 },
        {   4, 1<<5 },
        {   4, 1<<4 },
        {   4, 1<<3 },
        {   4, 1<<2 },
        {   4, 1<<1 },
        {   4, 1<<0 }
    },
    { 
        { 8+7, 1<<3 },    // bar 5, top most LED
        { 8+7, 1<<2 },
        { 8+7, 1<<1 },
        { 8+7, 1<<0 },
        {   5, 1<<15 },
        {   5, 1<<14 },
        {   5, 1<<13 },
        {   5, 1<<12 },
        {   5, 1<<11 },
        {   5, 1<<10 },
        {   5, 1<<9 },
        {   5, 1<<8 },
        {   5, 1<<7 },
        {   5, 1<<6 },
        {   5, 1<<5 },
        {   5, 1<<4 },
        {   5, 1<<3 },
        {   5, 1<<2 },
        {   5, 1<<1 },
        {   5, 1<<0 }
    },
    { 
        { 8+2, 1<<7 },    // bar 6, top most LED
        { 8+2, 1<<6 },
        { 8+2, 1<<5 },
        { 8+2, 1<<4 },
        {   6, 1<<15 },
        {   6, 1<<14 },
        {   6, 1<<13 },
        {   6, 1<<12 },
        {   6, 1<<11 },
        {   6, 1<<10 },
        {   6, 1<<9 },
        {   6, 1<<8 },
        {   6, 1<<7 },
        {   6, 1<<6 },
        {   6, 1<<5 },
        {   6, 1<<4 },
        {   6, 1<<3 },
        {   6, 1<<2 },
        {   6, 1<<1 },
        {   6, 1<<0 }
    },
    { 
        { 8+3, 1<<7 },    // bar 7, top most LED
        { 8+3, 1<<6 },
        { 8+3, 1<<5 },
        { 8+3, 1<<4 },
        {   7, 1<<15 },
        {   7, 1<<14 },
        {   7, 1<<13 },
        {   7, 1<<12 },
        {   7, 1<<11 },
        {   7, 1<<10 },
        {   7, 1<<9 },
        {   7, 1<<8 },
        {   7, 1<<7 },
        {   7, 1<<6 },
        {   7, 1<<5 },
        {   7, 1<<4 },
        {   7, 1<<3 },
        {   7, 1<<2 },
        {   7, 1<<1 },
        {   7, 1<<0 }
    },
    { 
        { 8+4, 1<<7 },    // bar 8, top most LED
        { 8+4, 1<<6 },
        { 8+4, 1<<5 },
        { 8+4, 1<<4 },
        { 8+0, 1<<15 },
        { 8+0, 1<<14 },
        { 8+0, 1<<13 },
        { 8+0, 1<<12 },
        { 8+0, 1<<11 },
        { 8+0, 1<<10 },
        { 8+0, 1<<9 },
        { 8+0, 1<<8 },
        { 8+0, 1<<7 },
        { 8+0, 1<<6 },
        { 8+0, 1<<5 },
        { 8+0, 1<<4 },
        { 8+0, 1<<3 },
        { 8+0, 1<<2 },
        { 8+0, 1<<1 },
        { 8+0, 1<<0 }
    },
    { 
        { 8+5, 1<<7 },    // bar 9, top most LED
        { 8+5, 1<<6 },
        { 8+5, 1<<5 },
        { 8+5, 1<<4 },
        { 8+1, 1<<15 },
        { 8+1, 1<<14 },
        { 8+1, 1<<13 },
        { 8+1, 1<<12 },
        { 8+1, 1<<11 },
        { 8+1, 1<<10 },
        { 8+1, 1<<9 },
        { 8+1, 1<<8 },
        { 8+1, 1<<7 },
        { 8+1, 1<<6 },
        { 8+1, 1<<5 },
        { 8+1, 1<<4 },
        { 8+1, 1<<3 },
        { 8+1, 1<<2 },
        { 8+1, 1<<1 },
        { 8+1, 1<<0 }
    }   
};

// Store i2c address and display ID
sidDisplay::sidDisplay(uint8_t address1, uint8_t address2)
{
    _address[0] = address1;
    _address[1] = address2;
}

// Start the display
void sidDisplay::begin()
{
    directCmd(0x20 | 1);    // turn on oscillator

    clearBuf();             // clear buffer
    setBrightness(15);      // setup initial brightness
    clearDisplayDirect();   // clear display RAM
    on();                   // turn it on
}

// Turn on the display
void sidDisplay::on()
{
    directCmd(0x80 | 1);
}

// Turn off the display
void sidDisplay::off()
{
    directCmd(0x80);
}

void sidDisplay::lampTest()
{ 
    for(int j = 0; j < 2; j++) {
        Wire.beginTransmission(_address[j]);  
        Wire.write(0x00);  // start address
        for(int i = 0; i < SD_BUF_SIZE / 2; i++) {
            Wire.write(0xff);
            Wire.write(0xff);
        }
        Wire.endTransmission();
    }
}


// Clear the buffer
void sidDisplay::clearBuf()
{
    for(int i = 0; i < SD_BUF_SIZE; i++) {
        _displayBuffer[i] = 0;
    }
}

// Set display brightness
// Valid brightness levels are 0 to 15.
// 255 sets it to previous level
uint8_t sidDisplay::setBrightness(uint8_t level, bool setInitial)
{
    if(level == 255)
        level = _brightness;    // restore to old val

    _brightness = setBrightnessDirect(level);

    if(setInitial) _origBrightness = _brightness;

    return _brightness;
}

void sidDisplay::resetBrightness()
{
    _brightness = setBrightnessDirect(_origBrightness);
}

uint8_t sidDisplay::setBrightnessDirect(uint8_t level)
{
    if(level > 15)
        level = 15;

    directCmd(0xe0 | level);

    return level;
}

uint8_t sidDisplay::getBrightness()
{
    return _brightness;
}

// Draw bar into buffer, do NOT call show
void sidDisplay::drawBarWithHeight(uint8_t bar, uint8_t height)
{
    // Clear bar
    // Draw bar with given height

    if(height > 127) height = 0;
    if(height > 20) height = 20;

    if(height < 20) {
        for(int i = 0; i < 20 - height; i++) {
            _displayBuffer[translator[bar][i][0]] &= ~(translator[bar][i][1]);
        }
    }
    if(height > 0) {
        for(int i = 20 - height; i < 20; i++) {
            _displayBuffer[translator[bar][i][0]] |= translator[bar][i][1];
        }
    }
}

// Draw bar into buffer, do NOT call show
void sidDisplay::drawBar(uint8_t bar, uint8_t bottom, uint8_t top)
{
    // Clear bar
    // Draw bar from top to bottom (0-19, 0=bottom)

    if(top > 19) top = 19;
    if(bottom > 19) bottom = 19;
    if(bottom > top) bottom = top;

    if(top < 19) {
        for(int i = 0; i <= 19-top; i++) {
            _displayBuffer[translator[bar][i][0]] &= ~(translator[bar][i][1]);
        }
    }
    if(bottom > 0) {
        for(int i = 19; i <= 19-bottom; i--) {
            _displayBuffer[translator[bar][i][0]] &= ~(translator[bar][i][1]);
        }
    }
    for(int i = 19-top; i <= 19-bottom; i++) {
        _displayBuffer[translator[bar][i][0]] |= translator[bar][i][1];
    }
}

void sidDisplay::clearBar(uint8_t bar)
{
    for(int i = 0; i <= 19; i++) {
        _displayBuffer[translator[bar][i][0]] &= ~(translator[bar][i][1]);
    }
}

// Draw dot into buffer, do NOT call show
void sidDisplay::drawDot(uint8_t bar, uint8_t dot_y)
{
    // Do not clear bar
    // Draw dot at dot_y (0 = bottom)
    if(dot_y > 19) dot_y = 19;

    _displayBuffer[translator[bar][19-dot_y][0]] |= translator[bar][19-dot_y][1];
}

void sidDisplay::drawFieldAndShow(uint8_t *fieldData)
{
    // Draw entire field. Data is 0 or 1, organized in lines
    for(int i = 0, k = 0; i < 20; i++, k += 10) {
        for(int j = 0; j < 10; j++) {
            if(fieldData[k+j]) {
                _displayBuffer[translator[j][i][0]] |= translator[j][i][1];
            } else {
                _displayBuffer[translator[j][i][0]] &= ~(translator[j][i][1]);
            }
        }
    }
    show();
}

void sidDisplay::drawLetterAndShow(char alpha, int x, int y)
{
    uint8_t field[20*10] = { 0 };
    int w = 10, h = 10, fx = 0, fy = 0, a = 0x200, s;

    if(x < -9 || x > 9 || y < -9 || y > 19) {
        clearDisplayDirect();
        return;
    }
    
    if(alpha >= '0' && alpha <= '9') {
        alpha -= '0';
    } else if(alpha >= 'A' && alpha <= 'Z') {
        alpha -= ('A' - 10);
    } else if(alpha >= 'a' && alpha <= 'z') {
        alpha -= ('a' - 10);
    } else if(alpha == '.') {
        alpha = 36;
    } else if(alpha == '&') {
        alpha = 37;
    } else if(alpha == '*') {
        alpha = 38;
    } else if(alpha == '#') {
        alpha = 39;
    } else if(alpha == '^') {
        alpha = 40;
    } else if(alpha == '$') {
        alpha = 41;
    } else if(alpha == '<') {
        alpha = 42;
    } else if(alpha == '>') {
        alpha = 43;
    } else if(alpha == '~') {
        alpha = 44;
    } else {
        clearDisplayDirect();   
        return;
    }

    if(x < 0) {
        fx = -x;
        a >>= fx;
        w = 10 - fx;
        x = 0;
    } else if(x > 0) {
        w = 10 - x;
    }
    if(y < 0) {
        fy = -y;
        h = 10 - fy;
        y = 0;
    } else if(y > (20-10)) {       
        h = 20 - y;           
    }

    for(int yy = fy; yy < h; yy++, y++) {
        uint16_t font = alphaChars[alpha][yy];
        int xxx = x;
        for(int xx = fx, s = a; xx < w; xx++, s >>= 1, xxx++) {
            if(font & s) {
                field[(y*10) + xxx] = 1;
            }
        }
    }
    drawFieldAndShow(field);
}

void sidDisplay::drawLetterMask(char alpha, int x, int y)
{
    int w = 8, h = 8, fx = 0, fy = 0, a = 0x80, s;

    if(x < -7 || x > 9 || y < -7 || y > 19) {
        return;
    }
    
    if(alpha >= '0' && alpha <= '9') {
        alpha -= '0';
    } else if(alpha >= 'A' && alpha <= 'Z') {
        alpha -= ('A' - 10);
    } else if(alpha >= 'a' && alpha <= 'z') {
        alpha -= ('a' - 10);
    } else if(alpha == '.') {
        alpha = 36;
    } else if(alpha == '#') {
        alpha = 37;
    } else if(alpha >= '$' && alpha <= '\'') {
        alpha -= '$';
        alpha += 38;
    } else { 
        return;
    }

    if(x < 0) {
        fx = -x;
        a >>= fx;
        w = 8 - fx;
        x = 0;
    } else if(x > 2) {
        w = 8 - x;
    }
    if(y < 0) {
        fy = -y;
        h = 8 - fy;
        y = 0;
    } else if(y > (20-8)) {       
        h = 20 - y;           
    }

    for(int yy = fy; yy < h; yy++, y++) {
        uint8_t font = alphaChars8[alpha][yy];
        int xxx = x;
        for(int xx = fx, s = a; xx < w; xx++, s >>= 1, xxx++) {
            if(font & s) {
                _displayBuffer[translator[xxx][y][0]] &= ~(translator[xxx][y][1]);
            }
        }
    }
}

void sidDisplay::drawClockAndShow(uint8_t *dateBuf, int dx, int dy)
{
    uint8_t field[20*10] = { 0 };
    uint8_t fields[11*9] = { 0 };
    int x[4], y[4], nums[4];
    int ampm = -1;
    uint8_t t = dateBuf[4];
    int s, c, cx, h, w, ox, oy, yyy;

    if(dx < -9 || dy < -11 || dx > 9 || dy > 19) {
        clearDisplayDirect();   
        return;
    }

    x[0] = x[2] = 0; x[1] = x[3] = 5;
    y[0] = y[1] = 0; y[2] = y[3] = 6;
    if(!(dateBuf[7] & 0x80)) {
        ampm = (t > 11) ? 0 : 1;
        if(!t)          t = 12;
        else if(t > 12) t -= 12;
    }
    nums[0] = t / 10;
    nums[1] = t % 10;
    nums[2] = dateBuf[5] / 10;
    nums[3] = dateBuf[5] % 10;
    
    for(c = 0; c < 4; c++) {
        for(int yy = y[c], yyy = 0; yy < y[c] + 5; yy++, yyy++) {
            uint8_t font = numChars4[nums[c]][yyy];
            for(int xx = x[c], s = 0x08; xx < x[c] + 4; xx++, s >>= 1) {
                if(font & s) {
                    fields[(yy*9) + xx] = 1;
                }
            }
        }
    }
    
    if(dx < 0) {
        ox = -dx;
        w = 9 - ox;
        dx = 0;
    } else {
        ox = 0;
        w = min((10 - dx), 9);
    }
    if(dy < 0) {
        oy = -dy;
        h = 11 - oy;
        dy = 0;
    } else {
        oy = 0;
        h = min((20-dy), 11);
    }

    for(int yy = dy, c = oy; c < h; yy++, c++) {
        for(int xx = dx, cx = ox; cx < w; xx++, cx++) {
            field[(yy*10)+xx] = fields[(c*9)+cx];
        }
    }
    
    drawFieldAndShow(field);
}


// Show the buffer
void sidDisplay::show()
{
    uint16_t *tp = &_displayBuffer[0];
    
    for(int j = 0; j < 2; j++) {
        Wire.beginTransmission(_address[j]);
        Wire.write(0x00);
        for(int i = 0; i < SD_BUF_SIZE / 2; i++) {
            uint16_t t = *tp++;
            Wire.write(t & 0xff);
            Wire.write(t >> 8);
        }
        Wire.endTransmission();
    }
}

void sidDisplay::clearDisplayDirect()
{
    for(int j = 0; j < 2; j++) {
        Wire.beginTransmission(_address[j]);
        Wire.write(0x00);
        for(int i = 0; i < SD_BUF_SIZE / 2; i++) {
            Wire.write(0x00);
            Wire.write(0x00);
        }
        Wire.endTransmission();
    }
}

void sidDisplay::directCmd(uint8_t val)
{
    for(int j = 0; j < 2; j++) {
        Wire.beginTransmission(_address[j]);
        Wire.write(val);
        Wire.endTransmission();
    }
}
