/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * SIDDisplay Class: Handles the SID LEDs
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

#ifndef _SIDDISPLAY_H
#define _SIDDISPLAY_H

// Special sequences
#define SID_SS_REMSTART    1
#define SID_SS_REMEND      2
#define SID_SS_IRBADINP    3
#define SID_SS_MAX         SID_SS_IRBADINP

#define SD_BUF_SIZE   16  // Buffer size in words (16bit)

class sidDisplay {

    public:

        sidDisplay(uint8_t address1, uint8_t address2);
        void begin();
        void on();
        void off();

        void lampTest();

        void clearBuf();

        uint8_t setBrightness(uint8_t level, bool setInitial = false);
        void    resetBrightness();
        uint8_t setBrightnessDirect(uint8_t level);
        uint8_t getBrightness();
        
        void show();

        void clearDisplayDirect();

        void drawBar(uint8_t bar, uint8_t bottom, uint8_t top);
        void drawBarWithHeight(uint8_t bar, uint8_t height);
        void clearBar(uint8_t bar);
        void drawDot(uint8_t bar, uint8_t dot_y);

        void drawFieldAndShow(uint8_t *fieldData);

        void drawLetterAndShow(char alpha, int x = 0, int y = 8);
        void drawLetterMask(char alpha, int x, int y);
        void drawClockAndShow(uint8_t *dateBuf, int dx, int dy);

        void specialSig(uint8_t sig);
        bool specialTrigger();

    private:
        void superImposeSpecSig();
        void directCmd(uint8_t val);
        
        uint8_t _address[2] = { 0, 0 };

        uint8_t _brightness = 15;     // current display brightness
        uint8_t _origBrightness = 15; // value from settings

        uint8_t       _specialSig = 0;
        unsigned long _specialSigNow = 0;
        bool          _specialTrigger = false;
        
        uint16_t _displayBuffer[SD_BUF_SIZE];

};

#endif
