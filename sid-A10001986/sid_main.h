/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * Main controller
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
#ifndef _SID_MAIN_H
#define _SID_MAIN_H

#include "siddisplay.h"

extern unsigned long powerupMillis;

extern sidDisplay sid;

#define SID_MAX_IDLE_MODE 5
extern uint16_t idleMode;
extern bool     strictMode;

// Number of IR keys
#define NUM_IR_KEYS 17

extern bool irLocked;

extern bool TCDconnected;

extern bool FPBUnitIsOn;
extern bool sidNM;
extern bool blockScan;

extern bool TTrunning;
extern bool IRLearning;

extern bool networkTimeTravel;
extern bool networkTCDTT;
extern bool networkReentry;
extern bool networkAbort;
extern bool networkAlarm;
extern uint16_t networkLead;
extern uint16_t networkP1;

extern uint32_t myRemID;

extern bool doPrepareTT;
extern bool doWakeup;

extern bool sidBusy;

void main_boot();
void main_setup();
void main_loop();

void flushDelayedSave();

void showWaitSequence(bool force = false);
void endWaitSequence();

void allOff();
void prepareReboot();

void populateIRarray(uint32_t *irkeys, int index);
void copyIRarray(uint32_t *irkeys, int index);

void showWordSequence(const char *text, int speed = 3);

void mydelay(unsigned long mydel, bool withIR);

void prepareTT();
void wakeup();

void setIdleMode(int idleNo);

void switch_to_idle();
void switch_to_sa();

void addCmdQueue(uint32_t command);
void bttfn_loop();

#endif
