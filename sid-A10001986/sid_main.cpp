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

#include "sid_global.h"

#include <Arduino.h>
#include <WiFi.h>
#include "input.h"

#include "sid_main.h"
#include "sid_settings.h"
#include "sid_wifi.h"
#include "sid_sa.h"
#include "sid_siddly.h"
#include "sid_snake.h"

unsigned long powerupMillis = 0;

// The SID display object
sidDisplay sid(0x74, 0x72);

// The IR-remote object
static IRRemote ir_remote(0, IRREMOTE_PIN);
static uint8_t IRFeedBackPin = IR_FB_PIN;

// The tt button / TCD tt trigger
static SIDButton TTKey = SIDButton(TT_IN_PIN,
    false,    // Button/Trigger is active HIGH
    false,    // Disable internal pull-up resistor
    true      // Enable internal pull-down resistor
);

#define TT_DEBOUNCE    50    // tt button debounce time in ms
#define TT_PRESS_TIME 200    // tt button will register a short press
#define TT_HOLD_TIME 5000    // time in ms holding the tt button will count as a long press
static bool isTTKeyPressed = false;
static bool isTTKeyHeld = false;

bool networkTimeTravel = false;
bool networkTCDTT      = false;
bool networkReentry    = false;
bool networkAbort      = false;
bool networkAlarm      = false;
uint16_t networkLead   = ETTO_LEAD;

#define SID_IDLE_0    0
#define SID_IDLE_1    1
#define SID_IDLE_2    2
#define SID_IDLE_3    3
#define SID_IDLE_BL   4   // "backlot mode"
#define SID_IDLE_IDC  5   // text / "identity crisis"

#define SBLF_REPEAT   1
#define SBLF_ISTT     2
#define SBLF_LM       4
#define SBLF_SKIPSHOW 8
#define SBLF_LMTT     16
#define SBLF_NOBL     32
#define SBLF_ANIM     64
#define SBLF_STRICT   128
uint16_t              idleMode = 0;
bool                  strictMode = true;
static int            sidBaseLine = 0;
static int            strictBaseLine = 0;
static bool           blWayup = true;
static unsigned long  lastChange = 0;
static unsigned long  idleDelay = 800;
static uint8_t        oldIdleHeight[10] = { 
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19
};

static int            LMIdx, LMY, LMState;
static unsigned long  LMAdvNow, LMDelay;
static unsigned long  lastChange2 = 0;
static unsigned long  idleDelay2 = 800;
static char           LM[] = { 0xa8,0xa8,0xca,0xc9,0xcb,0xc3,0xa8,0xdc,0xc7,0xa8,0xdc,0xc0,0xcd,0xa8,0xce,0xdd,0xdc,0xdd,0xda,0xcd,0xa8,0 }; // Space at beginning for letting pattern grow first
static const char     LMTT[] = { 36, 37, 38, 39, 0 };

#define ID5_STEPS 14
static int id5idx = 0;
static const uint8_t idle5[ID5_STEPS][10] = {
    {  6,  8,  6,  5,  8, 11, 11, 11, 12, 12 }, // 1
    { 10, 10, 10, 11, 11, 11, 11, 11, 12, 12 }, // 2
    { 14, 14, 15, 13, 13, 11, 11, 11, 12, 12 }, // 3
    { 14, 14, 15, 13, 13, 13, 13, 15, 14, 14 }, // 4
    { 14, 14, 15, 13, 13, 15, 16, 19, 16, 17 }, // 5
    { 16, 18, 17, 15, 17, 15, 16, 19, 16, 17 }, // 6
    { 16, 18, 17, 15, 17, 20, 18, 20, 20, 20 }, // 7
    { 19, 20, 20, 17, 19, 20, 18, 20, 20, 20 }, // 8
    { 16, 18, 17, 15, 17, 20, 18, 20, 20, 20 }, // 9
    { 16, 18, 17, 15, 17, 15, 16, 19, 16, 17 }, // 10
    { 14, 14, 15, 13, 13, 15, 16, 19, 16, 17 }, // 11
    { 14, 14, 15, 13, 13, 13, 13, 15, 14, 14 }, // 12
    { 14, 14, 15, 13, 13, 11, 11, 11, 12, 12 }, // 13
    { 10, 10, 10, 11, 11, 11, 11, 11, 12, 12 }  // 14
};

static bool useGPSS     = false;
static bool usingGPSS   = false;
static int16_t gpsSpeed = -1;
static int16_t prevGPSSpeed = -2;
static int16_t oldGpsSpeed = -2;
static bool spdIsRotEnc = false;

static bool useNM = false;
static bool tcdNM = false;
bool        sidNM = false;
static bool useFPO = false;
static bool tcdFPO = false;
static int  FPOSAMode = -1;
static bool bttfnTT = true;
static bool oldSidNM = false;

static bool skipTTAnim = false;

// Time travel status flags etc.
bool                 TTrunning = false;  // TT sequence is running
static bool          extTT = false;      // TT was triggered by TCD
static unsigned long TTstart = 0;
static unsigned long P0duration = ETTO_LEAD;
static bool          TTP0 = false;
static bool          TTP1 = false;
static bool          TTP2 = false;
static unsigned long TTFInt = 0;
static unsigned long TTFDelay = 0;
static unsigned long TTfUpdNow = 0;
static int           TTcnt = 0;
static int           TTClrBar = 0;
static int           TTClrBarInc = 1;
static int           TTBarCnt = 0;
static bool          TTBri = false;
static bool          TTSAStopped = false;
static uint16_t      TTsbFlags = 0;
static int           TTLMIdx = 0;
static bool          TTLMTrigger = false;

#define TT_SQF_LN 51
static const uint8_t ttledseqfull[TT_SQF_LN][10] = {
    {  1,  0,  0,  4,  0,  0,  0,  0,  0,  0 },
    {  2,  1,  0,  4,  0,  0,  0,  0,  0,  0 },
    {  3,  2,  0,  5,  0,  0,  0,  0,  0,  0 },
    {  4,  2,  0,  6,  0,  0,  0,  0,  0,  0 },
    {  4,  3,  0,  6,  0,  0,  0,  0,  0,  0 },
    {  4,  3,  0,  7,  0,  0,  0,  0,  0,  0 },
    {  4,  4,  0,  7,  0,  0,  0,  0,  0,  0 },
    {  4,  4,  0,  8,  0,  0,  0,  0,  0,  0 },
    {  4,  5,  0,  9,  0,  0,  0,  0,  0,  0 },
    {  4,  5,  0,  9,  0,  1,  0,  0,  0,  0 }, // 10
    {  5,  5,  0,  9,  0,  1,  0,  0,  0,  0 },
    {  5,  6,  0, 10,  0,  1,  0,  0,  0,  0 },
    {  5,  7,  0, 10,  0,  1,  0,  0,  0,  1 },
    {  5,  8,  0, 10,  0,  1,  0,  0,  0,  2 },
    {  6,  9,  0, 10,  0,  2,  0,  0,  0,  3 },
    {  6,  9,  0, 10,  0,  2,  1,  0,  0,  4 },
    {  6,  9,  0, 10,  0,  3,  1,  0,  0,  5 },
    {  6,  9,  0, 10,  0,  3,  2,  0,  0,  6 },
    {  6,  9,  0, 10,  0,  3,  3,  0,  0,  7 },
    {  6,  9,  0, 10,  0,  3,  4,  0,  0,  8 }, // 20
    {  6,  9,  0, 10,  0,  3,  4,  0,  0,  9 },
    {  7, 10,  0, 10,  0,  3,  4,  0,  0,  9 },
    {  7, 10,  0, 10,  0,  4,  5,  0,  1,  9 },
    {  8, 10,  0, 10,  0,  4,  6,  0,  1,  9 },
    {  8, 10,  0, 10,  0,  4,  7,  0,  2,  9 },
    {  8, 10,  0, 10,  0,  4,  8,  0,  2, 10 },
    {  8, 10,  0, 10,  0,  4,  8,  0,  3, 10 },
    {  8, 10,  0, 10,  0,  4,  9,  0,  3, 10 },
    {  8, 10,  0, 10,  0,  4, 10,  0,  3, 10 },
    {  9, 10,  0, 10,  0,  4, 10,  0,  4, 10 }, // 30
    {  9, 10,  0, 10,  0,  5, 10,  0,  5, 10 },
    {  9, 10,  0, 10,  0,  6, 10,  0,  6, 10 },
    {  9, 10,  0, 10,  0,  7, 10,  0,  7, 10 },
    { 10, 10,  0, 10,  0,  8, 10,  0,  8, 10 },
    { 10, 10,  0, 10,  0,  9, 10,  0,  9, 10 },
    { 10, 10,  1, 10,  0, 10, 10,  0, 10, 10 },
    { 10, 10,  1, 10,  0, 11, 10,  0, 11, 10 },
    { 10, 10,  2, 10,  0, 12, 10,  0, 12, 10 },
    { 11, 10,  2, 10,  0, 12, 10,  0, 13, 10 },
    { 12, 10,  3, 10,  0, 12, 10,  0, 14, 10 }, // 40
    { 13, 10,  3, 10,  0, 12, 10,  0, 15, 10 },
    { 14, 10,  3, 10,  0, 12, 10,  0, 16, 10 },
    { 15, 10,  4, 10,  0, 12, 10,  0, 17, 10 },
    { 16, 10,  4, 10,  0, 12, 10,  0, 18, 10 },
    { 17, 10,  4, 10,  0, 12, 10,  0, 19, 10 },
    { 18, 10,  6, 10,  0, 12, 10,  0, 20, 10 },
    { 19, 10,  6, 10,  0, 12, 10,  0, 20, 10 },
    { 19, 11,  6, 10,  0, 12, 11,  0, 20, 10 },   // 48
    { 20, 15,  7, 10,  0, 12, 15,  0, 20, 10 },   // 51
    { 20, 20, 10, 10,  5, 12, 20,  6, 20, 10 },   // 55-ish
    { 20, 20, 13, 20, 20, 19, 20, 10, 20, 17 }    // 60 - tt
};

#define TT_SQ_LN 29
static const uint8_t ttledseq[TT_SQ_LN][10] = {
//     1   2   3   4   5   6   7   8   9  10
    {  0,  1,  0,  0,  0,  0,  0,  0,  0,  0 },   // 0
    {  1,  2,  0,  2,  0,  0,  1,  0,  1,  1 },   // 1
    {  2,  3,  0,  2,  0,  1,  2,  0,  1,  2 },   // 2
    {  3,  4,  0,  3,  0,  1,  3,  0,  2,  3 },   // 3
    {  4,  5,  0,  3,  0,  1,  4,  0,  2,  4 },   // 4
    {  5,  6,  0,  5,  0,  2,  6,  0,  2,  5 },   // 5
    {  6,  7,  0,  7,  0,  2,  7,  0,  2,  7 },   // 6    bl 6
    {  7,  9,  0,  9,  0,  3,  8,  0,  3,  9 },   // 7    bl 8
    {  8, 10,  0, 10,  0,  4,  9,  0,  3, 10 },   // 8  m bl 10 (26)
    {  8, 10,  0, 10,  0,  5,  9,  0,  4, 10 },   // 9
    {  8, 10,  0, 10,  0,  5,  9,  0,  5, 10 },   // 10
    {  8, 10,  0, 10,  0,  6,  9,  0,  6, 10 },   // 11
    {  8, 10,  0, 10,  0,  8,  9,  0,  7, 10 },   // 12
    {  8, 10,  0, 10,  0, 10,  9,  0,  8, 10 },   // 13
    {  8, 10,  0, 10,  0, 10,  9,  0,  9, 10 },   // 14
    { 10, 10,  1, 10,  0, 10, 10,  0, 10, 10 },   // 15 m bl 10 (36)
    { 10, 10,  1, 10,  0, 10, 10,  0, 10, 10 },   // 16 m bl 10
    { 10, 10,  1, 10,  0, 11, 10,  0, 11, 10 },   // 17 m       (37)
    { 10, 10,  2, 10,  0, 11, 10,  0, 12, 10 },   // 18 
    { 11, 10,  3, 10,  0, 11, 10,  0, 13, 10 },   // 19 
    { 12, 10,  3, 10,  0, 12, 10,  0, 14, 10 },   // 20 
    { 13, 10,  3, 10,  0, 12, 10,  0, 15, 10 },   // 21 m       (41)
    { 14, 10,  4, 10,  0, 12, 10,  0, 16, 10 },   // 22 
    { 15, 10,  4, 10,  0, 12, 10,  0, 17, 10 },   // 23 
    { 16, 10,  4, 10,  0, 12, 10,  0, 18, 10 },   // 24 m       (44)
    { 19, 10,  6, 10,  0, 12, 10,  0, 20, 10 },   // 25 m       (47)
    { 20, 15,  7, 10,  0, 12, 15,  7, 20, 10 },   // 26 m       (n/a)
    { 20, 20, 10, 10,  5, 12, 20, 10, 20, 10 },   // 27 m       (n/a)
    { 20, 20, 13, 20, 20, 19, 20, 10, 20, 17 },   // 28 m       (60)
};
static const uint8_t seqEntry[21] = {
  //  0  1  2  3  4  5  6  7  8  9 10  11  12  13  14  15  16  17  18  19  20      baseline
      0, 1, 2, 3, 4, 5, 6, 6, 7, 7, 8, 15, 18, 19, 20, 21, 21, 22, 22, 23, 24   // index in sequ
};

#define TT_AMP_STEPS 16
static const int TTampFacts[TT_AMP_STEPS] = {
    100, 110, 120, 130, 150,
    170, 200, 250, 300, 400, 
    500, 800, 1000, 1500, 2000, 2000
};

// Durations of tt phases for internal tt
#define P0_DUR          5000    // acceleration phase
#define P1_DUR          5000    // time tunnel phase
#define P2_DUR          3000    // re-entry phase

bool         TCDconnected = false;
static bool  noETTOLead = false;

static bool          brichanged = false;
static unsigned long brichgnow = 0;
static bool          ipachanged = false;
static unsigned long ipachgnow = 0;
static bool          irlchanged = false;
static unsigned long irlchgnow = 0;

static unsigned long ssLastActivity = 0;
static unsigned long ssDelay = 0;
static unsigned long ssOrigDelay = 0;
static bool          ssActive = false;
static bool          ssClock = false;
static bool          ssIsClock = false;
static int           ssClkPos = 0;
static uint8_t       ssDateBuf[8];
static bool          ssNMbuf = false;
static bool          ssClockOffinNM = false;

static bool          nmOld = false;
static bool          fpoOld = false;
bool                 FPBUnitIsOn = true;

/*
 * Leave first two columns at 0 here, those will be filled
 * by a user-provided ir_keys.txt file on the SD card, and 
 * learned keys respectively.
 */
#define NUM_REM_TYPES 3
static uint32_t remote_codes[NUM_IR_KEYS][NUM_REM_TYPES] = {
//    U  L  Default     (U = user provided via ir_keys.txt; L = Learned)
    { 0, 0, 0x97483bfb },    // 0:  0
    { 0, 0, 0xe318261b },    // 1:  1
    { 0, 0, 0x00511dbb },    // 2:  2
    { 0, 0, 0xee886d7f },    // 3:  3
    { 0, 0, 0x52a3d41f },    // 4:  4
    { 0, 0, 0xd7e84b1b },    // 5:  5
    { 0, 0, 0x20fe4dbb },    // 6:  6
    { 0, 0, 0xf076c13b },    // 7:  7
    { 0, 0, 0xa3c8eddb },    // 8:  8
    { 0, 0, 0xe5cfbd7f },    // 9:  9
    { 0, 0, 0xc101e57b },    // 10: *
    { 0, 0, 0xf0c41643 },    // 11: #
    { 0, 0, 0x3d9ae3f7 },    // 12: arrow up
    { 0, 0, 0x1bc0157b },    // 13: arrow down
    { 0, 0, 0x8c22657b },    // 14: arrow left
    { 0, 0, 0x0449e79f },    // 15: arrow right
    { 0, 0, 0x488f3cbb }     // 16: OK/Enter
};

#define INPUTLEN_MAX 6
static char          inputBuffer[INPUTLEN_MAX + 2];
static char          inputBackup[INPUTLEN_MAX + 2];
static int           inputIndex = 0;
static bool          inputRecord = false;
static unsigned long lastKeyPressed = 0;
static int           maxIRctrls = NUM_REM_TYPES;

#define IR_FEEDBACK_DUR 300
static bool          irFeedBack = false;
static unsigned long irFeedBackNow = 0;
static unsigned long irFeedBackDur = IR_FEEDBACK_DUR;
static bool          irErrFeedBack = false;
static unsigned long irErrFeedBackNow = 0;
static int           irErrFBState = 0;

bool                 irLocked = false;

bool                 IRLearning = false;
static uint32_t      backupIRcodes[NUM_IR_KEYS];
static int           IRLearnIndex = 0;
static unsigned long IRLearnNow;
static unsigned long IRFBLearnNow;
static bool          IRLearnBlink = false;
static const char    IRLearnKeys[] = "0123456789*#^$<>~";

uint32_t             myRemID = 0x12345678;
static bool          remoteAllowed = false;
static bool          remMode = false;
static bool          remHoldKey = false;

#define BTTFN_VERSION              1
#define BTTFN_SUP_MC            0x80
#define BTTF_PACKET_SIZE          48
#define BTTF_DEFAULT_LOCAL_PORT 1338    // 1339, 1340 for multicast
#define BTTFN_POLL_INT          1000
#define BTTFN_POLL_INT_FAST      800
#define BTTFN_NOT_PREPARE  1
#define BTTFN_NOT_TT       2
#define BTTFN_NOT_REENTRY  3
#define BTTFN_NOT_ABORT_TT 4
#define BTTFN_NOT_ALARM    5
#define BTTFN_NOT_REFILL   6
#define BTTFN_NOT_FLUX_CMD 7
#define BTTFN_NOT_SID_CMD  8
#define BTTFN_NOT_PCG_CMD  9
#define BTTFN_NOT_WAKEUP   10
#define BTTFN_NOT_AUX_CMD  11
#define BTTFN_NOT_VSR_CMD  12
#define BTTFN_NOT_SPD      15
#define BTTFN_TYPE_ANY     0    // Any, unknown or no device
#define BTTFN_TYPE_FLUX    1    // Flux Capacitor
#define BTTFN_TYPE_SID     2    // SID
#define BTTFN_TYPE_PCG     3    // Plutonium gauge panel
#define BTTFN_TYPE_VSR     4    // VSR
#define BTTFN_TYPE_AUX     5    // Aux (user custom device)
#define BTTFN_TYPE_REMOTE  6    // Futaba remote control
#define BTTFN_REMCMD_KP_PING    4
#define BTTFN_REMCMD_KP_KEY     5
#define BTTFN_REMCMD_KP_BYE     6
#define BTTFN_REM_MAX_COMMAND   BTTFN_REMCMD_KP_BYE
#define BTTFN_SSRC_NONE         0
#define BTTFN_SSRC_GPS          1
#define BTTFN_SSRC_ROTENC       2
#define BTTFN_SSRC_REM          3
#define BTTFN_SSRC_P0           4
#define BTTFN_SSRC_P1           5
#define BTTFN_SSRC_P2           6
static const uint8_t BTTFUDPHD[4] = { 'B', 'T', 'T', 'F' };
static bool          useBTTFN = false;
static WiFiUDP       bttfUDP;
static UDP*          sidUDP;
#ifdef BTTFN_MC
static WiFiUDP       bttfMcUDP;
static UDP*          sidMcUDP;
#endif
static byte          BTTFUDPBuf[BTTF_PACKET_SIZE];
static unsigned long BTTFNUpdateNow = 0;
static unsigned long bttfnSIDPollInt = BTTFN_POLL_INT;
static unsigned long BTFNTSAge = 0;
static unsigned long BTTFNTSRQAge = 0;
static bool          BTTFNPacketDue = false;
static bool          BTTFNWiFiUp = false;
static uint8_t       BTTFNfailCount = 0;
static uint32_t      BTTFUDPID    = 0;
static unsigned long lastBTTFNpacket = 0;
static bool          BTTFNBootTO = false;
static bool          haveTCDIP = false;
static IPAddress     bttfnTcdIP;
static uint32_t      bttfnTCDSeqCnt = 0;
static uint8_t       bttfnReqStatus = 0x53; // Request capabilities, status, speed, date/time
static bool          TCDSupportsRemKP = false;
#ifdef BTTFN_MC
static uint32_t      tcdHostNameHash = 0;
static byte          BTTFMCBuf[BTTF_PACKET_SIZE];
static uint8_t       bttfnMcMarker = 0;
static IPAddress     bttfnMcIP(224, 0, 0, 224);
#endif
static uint32_t      bttfnSeqCnt[BTTFN_REM_MAX_COMMAND+1] = { 1 };

enum {
    BTTFN_KP_KS_PRESSED,
    BTTFN_KP_KS_HOLD,
};

static int      iCmdIdx = 0;
static int      oCmdIdx = 0;
static uint32_t commandQueue[16] = { 0 };

static uint8_t  bttfnDateBuf[8] = { 0xff };
static unsigned long bttfnDateNow = 0;

#define GET32(a,b)          \
    (((a)[b])            |  \
    (((a)[(b)+1]) << 8)  |  \
    (((a)[(b)+2]) << 16) |  \
    (((a)[(b)+3]) << 24))
#define SET32(a,b,c)                        \
    (a)[b]       = ((uint32_t)(c)) & 0xff;  \
    ((a)[(b)+1]) = ((uint32_t)(c)) >> 8;    \
    ((a)[(b)+2]) = ((uint32_t)(c)) >> 16;   \
    ((a)[(b)+3]) = ((uint32_t)(c)) >> 24; 

// Forward declarations ------

static void startIRLearn();
static void endIRLearn(bool restore);
static void handleIRinput();
static void handleIRKey(int command);
static void handleRemoteCommand();
static void clearInpBuf();
static int  execute(bool isIR);
static void startIRfeedback();
static void endIRfeedback();

static void showBaseLine(int variation = 20, uint16_t flags = 0);
static void showIdle(bool freezeBaseLine = false);
static void play_startup();
static void timeTravel(bool TCDtriggered, uint16_t P0Dur);

static void showChar(const char text);
static void fadeOutChar();

static void span_start();
static void span_stop(bool skipClearDisplay = false);
static void siddly_start();
static void siddly_stop();
static void snake_start();
static void snake_stop();

static void inc_bri();
static void dec_bri();

static void ttkeyScan();
static void TTKeyPressed();
static void TTKeyHeld();

static void ssStart();
static void ssEnd();
static void ssRestartTimer();
static void ssUpdateClock();

static void bttfn_setup();
static void bttfn_loop_quick();
#ifdef BTTFN_MC
static bool bttfn_checkmc();
#endif
static void BTTFNCheckPacket();
static bool BTTFNTriggerUpdate();
static void BTTFNSendPacket();
static bool BTTFNConnected();

static bool bttfn_trigger_tt();
static bool bttfn_send_command(uint8_t cmd, uint8_t p1, uint8_t p2);

void main_boot()
{
    // Boot display, keep it dark
    sid.begin();
}

void main_setup()
{
    char *s = LM;
    
    Serial.println(F("Status Indicator Display version " SID_VERSION " " SID_VERSION_EXTRA));

    // Load settings
    loadBrightness();
    loadIdlePat();                    // load idleMode and strictMode
    updateConfigPortalStrictValue();  // Update current CP value
    loadIRLock();

    // Other options
    ssDelay = ssOrigDelay = atoi(settings.ssTimer) * 60 * 1000;
    useGPSS = (atoi(settings.useGPSS) > 0);
    useNM = (atoi(settings.useNM) > 0);
    useFPO = (atoi(settings.useFPO) > 0);
    bttfnTT = (atoi(settings.bttfnTT) > 0);
    ssClock = (atoi(settings.ssClock) > 0);
    ssClockOffinNM = (atoi(settings.ssClockOffNM) > 0);

    skipTTAnim = (atoi(settings.skipTTAnim) > 0);

    doPeaks = (atoi(settings.SApeaks) > 0);

    if((atoi(settings.disDIR) > 0)) 
        maxIRctrls--;
    
    // [formerly started CP here]

    // Determine if Time Circuits Display is connected
    // via wire, and is source of GPIO tt trigger
    TCDconnected = (atoi(settings.TCDpresent) > 0);
    noETTOLead = (atoi(settings.noETTOLead) > 0);

    for(int i = 0; i < BTTFN_REM_MAX_COMMAND+1; i++) {
        bttfnSeqCnt[i] = 1;
    }

    // Init IR feedback LED
    pinMode(IRFeedBackPin, OUTPUT);
    digitalWrite(IRFeedBackPin, LOW);

    // Set up TT button / TCD trigger
    TTKey.attachPress(TTKeyPressed);
    if(!TCDconnected) {
        // If we are in fact a physical button, we need
        // reasonable values for debounce and press
        TTKey.setTiming(TT_DEBOUNCE, TT_PRESS_TIME, TT_HOLD_TIME);
        TTKey.attachLongPressStart(TTKeyHeld);
    } else {
        // If the TCD is connected, we can go more to the edge
        TTKey.setTiming(5, 50, 100000);
        // Long press ignored when TCD is connected
        // IRLearning only possible if "TCD connected by wire" unset.
    }

    #ifdef SID_DBG
    Serial.println(F("Booting IR Receiver"));
    #endif
    ir_remote.begin();

    memset(bttfnDateBuf, 0xff, sizeof(bttfnDateBuf));

    // Initialize BTTF network
    bttfn_setup();
    bttfn_loop();

    // Other inits
    idleDelay2 = 800 + ((int)(esp_random() % 200) - 100);

    for( ; *s; ++s) *s ^= (SBLF_SKIPSHOW + SBLF_STRICT);

    // If "Follow TCD fake power" is set,
    // stay silent and dark

    if(useBTTFN && useFPO && (WiFi.status() == WL_CONNECTED)) {
      
        FPBUnitIsOn = false;
        tcdFPO = fpoOld = true;

        sid.off();

        // Light up IR feedback for 500ms
        startIRfeedback();
        mydelay(500, false);
        endIRfeedback();

        Serial.println("Waiting for TCD fake power on");
    
    } else {

        // Otherwise boot:
        FPBUnitIsOn = true;

        // Play startup sequence
        play_startup();
        
        ssRestartTimer();
        
    }

    #ifdef SID_DBG
    Serial.println(F("main_setup() done"));
    #endif

    // Delete previous IR input, start fresh
    ir_remote.resume();  
}

void main_loop()
{   
    unsigned long now = millis();

    // Reset polling interval; will be overruled in showIdle if applicable
    bttfnSIDPollInt = BTTFN_POLL_INT;

    // Follow TCD fake power
    if(useFPO && (tcdFPO != fpoOld)) {
        if(tcdFPO) {
            // Power off:
            FPBUnitIsOn = false;
            
            if(TTrunning) {
                // Reset to idle
                sa_setAmpFact(100);
                sid.setBrightness(255);
                // ...
            }
            TTrunning = false;

            if(irFeedBack) {
                endIRfeedback();
                irFeedBack = false;
            }
            
            if(IRLearning) {
                endIRLearn(true); // Turns LEDs on
            }
            
            // Display OFF
            sid.off();

            // Backup last mode (idle or sa)
            FPOSAMode = saActive ? 1 : 0;

            // Specials off
            span_stop();
            siddly_stop();
            snake_stop();

            flushDelayedSave();
            
            // FIXME - anything else?
            
        } else {
            // Power on: 
            FPBUnitIsOn = true;
            
            // Display ON, idle
            sid.clearDisplayDirect();
            lastChange = 0;
            sid.setBrightness(255);
            sid.on();

            isTTKeyHeld = isTTKeyPressed = false;
            networkTimeTravel = false;

            // Play startup sequence
            play_startup();

            ssRestartTimer();
            ssActive = ssIsClock = false;

            sidBaseLine = strictBaseLine = 0;
            LMState = LMIdx = id5idx = 0;

            ir_remote.loop();

            // FIXME - anything else?

            // Restore sa mode if active before FPO
            if(FPOSAMode > 0) {
                span_start();
            }
 
        }
        fpoOld = tcdFPO;
    }

    // Discard (incomplete) input from IR after 30 seconds of inactivity
    if(now - lastKeyPressed >= 30*1000) {
        clearInpBuf();
    }

    // IR feedback
    if(irErrFeedBack && (now - irErrFeedBackNow > 250)) {
        irErrFeedBackNow = now;
        irErrFBState++;
        if(irErrFBState > 3) {
            irErrFeedBack = false;
            endIRfeedback();
        } else {
            (irErrFBState & 0x01) ? endIRfeedback() : startIRfeedback();
            irErrFeedBack = true;
        }
    }
    if(irFeedBack && (now - irFeedBackNow > irFeedBackDur)) {
        endIRfeedback();
        irFeedBack = false;
    }

    if(IRLearning) {
        ssRestartTimer();
        if(now - IRFBLearnNow > 200) {
            IRLearnBlink = !IRLearnBlink;
            IRLearnBlink ? endIRfeedback() : startIRfeedback();
            IRFBLearnNow = now;
        }
        if(now - IRLearnNow > 10000) {
            endIRLearn(true);
            #ifdef SID_DBG
            Serial.println("main_loop: IR learning timed out");
            #endif
        }
    }

    // IR Remote loop
    if(FPBUnitIsOn) {
        if(ir_remote.loop()) {
            handleIRinput();
        }
        handleRemoteCommand();
    }

    // Spectrum analyzer / Siddly / Snake loops
    if(FPBUnitIsOn && !TTrunning) {
        //unsigned long now2 = millis();
        sa_loop();    // 17ms: float / 28ms: double FFT [400Khz i2c, Rectangle]
        //now2 = millis() - now2;
        //Serial.printf("%d\n", now2);
        si_loop();
        sn_loop();
    }

    // TT button evaluation
    if(FPBUnitIsOn && !TTrunning) {
        ttkeyScan();
        if(isTTKeyHeld) {
            ssEnd();
            isTTKeyHeld = isTTKeyPressed = false;
            if(!IRLearning) {
                span_stop();
                siddly_stop();
                snake_stop();
                startIRLearn();
                #ifdef SID_DBG
                Serial.println("main_loop: IR learning started");
                #endif
            }
        } else if(isTTKeyPressed) {
            isTTKeyPressed = false;
            if(!TCDconnected && ssActive) {
                // First button press when ss is active only deactivates SS
                ssEnd();
            } else if(IRLearning) {
                endIRLearn(true);
                #ifdef SID_DBG
                Serial.println("main_loop: IR learning aborted");
                #endif
            } else {
                if(TCDconnected) {
                    ssEnd();
                }
                if(TCDconnected || !bttfnTT || !bttfn_trigger_tt()) {
                    timeTravel(TCDconnected, noETTOLead ? 0 : ETTO_LEAD);
                }
            }
        }
    
        // Check for BTTFN/MQTT-induced TT
        if(networkTimeTravel) {
            networkTimeTravel = false;
            ssEnd();
            timeTravel(networkTCDTT, networkLead);
        }
    }

    now = millis();
    
    if(TTrunning) {

        if(extTT) {

            // ***********************************************************************************
            // TT triggered by TCD (BTTFN, GPIO or MQTT) *****************************************
            // ***********************************************************************************

            if(TTP0) {   // Acceleration - runs for ETTO_LEAD ms by default

                if(!networkAbort && (now - TTstart < P0duration)) {

                    if(!TTFDelay || (now - TTfUpdNow >= TTFDelay)) {

                        if(TTFDelay) {
                            TTFDelay = 0;
                            TTfUpdNow = now;
                        } else if(TTFInt && (now - TTfUpdNow >= TTFInt)) {                          
                            if(saActive) {
                                if(TTcnt > 0) {
                                    TTcnt--;
                                    sa_setAmpFact(TTampFacts[TT_AMP_STEPS - 1 - TTcnt]);
                                }
                            } else {
                                if(TTcnt > 0) {
                                    TTcnt--;
                                    if(TTsbFlags & SBLF_STRICT) {
                                        for(int i = 0; i < 10; i++) {
                                            sid.drawBarWithHeight(i, ttledseqfull[TT_SQF_LN - 1 - TTcnt][i]);
                                        }
                                    } else {
                                        for(int i = 0; i < 10; i++) {
                                            sid.drawBarWithHeight(i, ttledseq[TT_SQ_LN - 1 - TTcnt][i]);
                                        }
                                    }
                                    sid.show();
                                }
                            }
                            TTfUpdNow = now;
                        }

                        if(saActive) {
                            sa_loop();
                        }

                    } else {

                        if(saActive) {
                            sa_loop();                            
                        } else {
                            showIdle(true);
                        }

                    }

                } else {

                    if(TTstart == TTfUpdNow) {
                        // If we have skipped P0, set last step of sequence at least
                        // Do this also in sa mode and if strict (pattern is same)
                        for(int i = 0; i < 10; i++) {
                            sid.drawBarWithHeight(i, ttledseq[TT_SQ_LN - 1][i]);
                        }
                        sid.show();
                    }

                    if(saActive) {
                        //span_stop(true);
                        sa_deactivate();
                        TTSAStopped = true;
                    }

                    TTP0 = false;
                    TTP1 = true;
                    sidBaseLine = 19;
                    strictBaseLine = TT_SQF_LN - 1;
                    TTstart = TTfUpdNow = now;
                    TTFInt = 1000 + ((int)(esp_random() % 200) - 100);

                    TTClrBar = TTBarCnt = 0;
                    TTClrBarInc = 1;
                    TTBri = false;

                    TTLMIdx = 0;
                    TTLMTrigger = false;

                }
            }
            if(TTP1) {   // Peak/"time tunnel" - ends with pin going LOW or MQTT "REENTRY"

                if( (networkTCDTT && (!networkReentry && !networkAbort)) || (!networkTCDTT && digitalRead(TT_IN_PIN))) 
                {

                    if(TTFInt && (now - TTfUpdNow >= TTFInt)) {
                        if(TTLMTrigger) {
                            TTLMIdx++;
                            if(!LMTT[TTLMIdx]) TTLMIdx = 0;
                        }
                        showBaseLine(80, TTsbFlags | SBLF_ISTT);
                        TTfUpdNow = now;
                        if(TTsbFlags & SBLF_LMTT) {
                            TTLMTrigger = true;
                            TTFInt = 130;
                        } else {
                            TTFInt = 100 + ((int)(esp_random() % 100) - 50);
                        }
                    }
                    
                } else {

                    TTP1 = false;
                    TTP2 = true;

                    sid.setBrightness(255);

                    TTFInt = 50;
                    TTfUpdNow = now;
                    
                }
            }
            if(TTP2) {   // Reentry - up to us

                // Idle mode: Let showIdle take care of calming us down
                // SA mode: reduce ampFactor gradually

                if(TTSAStopped) {
                    sa_activate(false, 500);
                    TTSAStopped = false;
                }
                if(saActive && sa_setAmpFact(-1) > 100) {
                    if(now - TTfUpdNow >= TTFInt) {
                        TTcnt++;
                        if(TTcnt <= TT_AMP_STEPS) {
                            sa_setAmpFact(TTampFacts[TT_AMP_STEPS - 1 - TTcnt]);
                        }
                        TTfUpdNow = now;
                        TTFInt = 400 + ((int)(esp_random() % 100) - 50);
                    }
                    sa_loop();
                } else {
                    TTP2 = false;
                    TTrunning = false;
                    isTTKeyHeld = isTTKeyPressed = false;
                    ssRestartTimer();
                    sa_setAmpFact(100);
                    LMState = LMIdx = id5idx = 0;
                }

            }
          
        } else {

            // ****************************************************************************
            // TT triggered by button (if TCD not connected), MQTT or IR ******************
            // ****************************************************************************
          
            if(TTP0) {   // Acceleration - runs for P0_DUR ms

                if(now - TTstart < P0_DUR) {

                    // TT acceleration (use gpsSpeed if available)

                    if(!TTFDelay || (now - TTfUpdNow >= TTFDelay)) {

                        TTFDelay = 0;
                        if(TTFInt && (now - TTfUpdNow >= TTFInt)) {
                            if(saActive) {
                                if(TTcnt > 0) {
                                    TTcnt--;
                                    sa_setAmpFact(TTampFacts[TT_AMP_STEPS - 1 - TTcnt]);
                                }
                            } else {
                                if(TTcnt > 0) {
                                    TTcnt--;
                                    if(TTsbFlags & SBLF_STRICT) {
                                        for(int i = 0; i < 10; i++) {
                                            sid.drawBarWithHeight(i, ttledseqfull[TT_SQF_LN - 1 - TTcnt][i]);
                                        }
                                    } else {
                                        for(int i = 0; i < 10; i++) {
                                            sid.drawBarWithHeight(i, ttledseq[TT_SQ_LN - 1 - TTcnt][i]);
                                        }
                                    }
                                    sid.show();
                                }
                            }
                            TTfUpdNow = now;
                        }
                        
                        if(saActive) {
                            sa_loop();
                        }

                    } else {
                        
                        if(saActive) {
                            sa_loop();
                        } else {
                            showIdle(true);
                        }

                    }

                } else {

                    if(saActive) {
                        sa_deactivate();
                        TTSAStopped = true;
                    }

                    TTP0 = false;
                    TTP1 = true;
                    sidBaseLine = 19;
                    strictBaseLine = TT_SQF_LN - 1;
                    TTstart = TTfUpdNow = now;
                    TTFInt = 1000 + ((int)(esp_random() % 200) - 100);

                    TTClrBar = TTBarCnt = 0;
                    TTClrBarInc = 1;
                    TTBri = false;

                    TTLMIdx = 0;
                    TTLMTrigger = false;
                    
                }
            }
            
            if(TTP1) {   // Peak/"time tunnel" - runs for P1_DUR ms

                if(now - TTstart < P1_DUR) {
                    
                    if(TTFInt && (now - TTfUpdNow >= TTFInt)) {
                        if(TTLMTrigger) {
                            TTLMIdx++;
                            if(!LMTT[TTLMIdx]) TTLMIdx = 0;
                        }
                        showBaseLine(80, TTsbFlags | SBLF_ISTT);
                        TTfUpdNow = now;
                        if(TTsbFlags & SBLF_LMTT) {
                            TTLMTrigger = true;
                            TTFInt = 130;
                        } else {
                            TTFInt = 100 + ((int)(esp_random() % 100) - 50);
                        }
                    }
                    
                } else {

                    TTP1 = false;
                    TTP2 = true; 

                    sid.setBrightness(255);

                    TTFInt = 50;
                    TTfUpdNow = now;
                    
                }
            }
            
            if(TTP2) {   // Reentry - up to us

                // Idle mode: Let showIdle take care of calming us down
                // SA mode: reduce ampFactor gradually

                if(TTSAStopped) {
                    sa_activate(false, 500);
                    TTSAStopped = false;
                }
                if(saActive && sa_setAmpFact(-1) > 100) {
                    if(now - TTfUpdNow >= TTFInt) {
                        TTcnt++;
                        if(TTcnt <= TT_AMP_STEPS) {
                            sa_setAmpFact(TTampFacts[TT_AMP_STEPS - 1 - TTcnt]);
                        }
                        TTfUpdNow = now;
                        TTFInt = 400 + ((int)(esp_random() % 100) - 50);
                    }
                    sa_loop();
                } else {
                    TTP2 = false;
                    TTrunning = false;
                    isTTKeyHeld = isTTKeyPressed = false;
                    ssRestartTimer();
                    sa_setAmpFact(100);
                    LMState = LMIdx = id5idx = 0;
                }

            }

        }

    } else if(!siActive && !snActive && !saActive) {    // No TT currently

        if(networkAlarm && !IRLearning) {

            networkAlarm = false;
            
            ssEnd();
            
            if(!FPBUnitIsOn) {
                sid.on();
            }
            
            // play alarm sequence
            showWordSequence("ALARM", 4);
            
            if(!FPBUnitIsOn) {
                sid.off();
            }
        
        } else if(!IRLearning) {

            // Wake up on GPS/RotEnc speed changes
            if(gpsSpeed != oldGpsSpeed) {
                if(FPBUnitIsOn && spdIsRotEnc && gpsSpeed >= 0) {
                    wakeup();
                }
                oldGpsSpeed = gpsSpeed;
            }

            now = millis();

            // "Screen saver"
            if(FPBUnitIsOn) {
                if(!ssActive && ssDelay && (now - ssLastActivity > ssDelay)) {
                    ssStart();
                }
            }
          
            // Default idle sequence
            if(FPBUnitIsOn) {
                if(ssActive && ssClock) {
                    ssUpdateClock();
                } else {
                    showIdle();
                }
            }

        }
        
    } else if(saActive) {
        if(sidNM != oldSidNM) {
            if(sidNM) {
                span_stop();
            }
            oldSidNM = sidNM;
        }
    }

    // Follow TCD night mode
    if(useNM && (tcdNM != nmOld)) {
        if(tcdNM) {
            // NM on: Set Screen Saver timeout to 10 seconds
            ssDelay = 10 * 1000;
            sidNM = true;
        } else {
            // NM off: End Screen Saver; reset timeout to old value
            ssEnd();  // Doesn't do anything if fake power is off
            ssDelay = ssOrigDelay;
            sidNM = false;
        }
        nmOld = tcdNM;
    }

    now = millis();

    // If network is interrupted, return to stand-alone
    if(useBTTFN) {
        if( (lastBTTFNpacket && (now - lastBTTFNpacket > 30*1000)) ||
            (!BTTFNBootTO && !lastBTTFNpacket && (now - powerupMillis > 60*1000)) ) {
            tcdNM = false;
            tcdFPO = false;
            gpsSpeed = -1;
            lastBTTFNpacket = 0;
            BTTFNBootTO = true;
        }
    }

    if(!TTrunning) {
        
        if(brichanged && (now - brichgnow > 10000)) {
            // Save brightness 10 seconds after last change
            brichanged = false;
            saveBrightness();
        } else if(ipachanged && (now - ipachgnow > 10000)) {
            // Save idle pattern 10 seconds after last change
            ipachanged = false;
            saveIdlePat();
        } else if(irlchanged && (now - irlchgnow > 10000)) {
            // Save irlock 10 seconds after last change
            irlchanged = false;
            saveIRLock();
        }
    }
}

void flushDelayedSave()
{
    if(brichanged) {
        brichanged = false;
        saveBrightness();
    }

    if(ipachanged) {
        ipachanged = false;
        saveIdlePat();
    }

    if(irlchanged) {
        irlchanged = false;
        saveIRLock();
    }
}

static void showBaseLine(int variation, uint16_t flags)
{
    const int mods[21][10] = {
        { 130, 90, 10,  80,  10, 110, 100,  15, 120,  90 }, // g 0
        { 130, 90, 10,  80,  10, 110, 100,  15, 100,  90 }, // g 1
        { 130, 90, 20,  80,  15, 110, 100,  15, 120, 100 }, // g 2
        { 110,100, 70,  80,  30,  50, 100,  15, 100, 110 }, // g 3
        { 110,110, 40,  90,  30,  50, 100,  15,  80, 100 }, // g 4
        { 110,110, 30, 120,  30,  50, 110,  15,  50, 100 }, // g 5
        { 100,100, 20, 120,  10,  50, 110,  20,  40, 110 }, // g 6
        { 110,120, 15, 110,  20,  40, 110,  18,  40, 100 }, // g 7
        { 100,100, 15, 110,  20,  50, 100,  15,  50,  90 }, // g 8
        {  90,110,  0, 100,  20,  50, 100,  15,  60, 100 }, // g 9
        {  90,100, 10, 100,  10,  60,  90,  15,  40, 100 }, // g 10
        {  90,100, 10, 100,  10,  90,  90,  15, 110, 100 }, // g 11
        {  90, 90, 20,  90,  15, 100, 100,  50, 100,  90 }, // g 12
        {  90, 90, 20,  90,  15, 100, 100,  50, 100,  90 }, // y 13
        {  90, 80, 10,  80,  15,  90,  80,  50, 100,  80 }, // y 14
        {  90, 80, 10,  80,  15,  90,  80,  50, 100,  80 }, // y 15
        {  90, 80, 10,  80,  15,  90,  80,  50, 100,  80 }, // y 16
        {  90, 70, 20,  70,  15,  70,  70,  40, 100,  70 }, // y 17
        {  90, 70, 20,  70,  15,  70,  70,  40,  90,  70 }, // y 18
        {  90, 60, 25,  60,  15,  80,  60,  40,  90,  60 }, // r 19
        {  90, 90, 70, 100,  90, 110,  90,  60,  95,  80 }  // extra for TT
    };
    const uint8_t maxTTHeight[10] = {
        19, 19, 12, 19, 19, 18, 19,  9, 19, 16
    };

    int bh, a = sidBaseLine, b;
    int vc = (flags & SBLF_ISTT) ? 0 : variation / 2;

    if(!(flags & SBLF_NOBL)) {
          
        if(a < 0) a = 0;
        if(a > 19) a = 19;

        b = (flags & SBLF_ISTT) ? 20 : a;
    
        if(flags & SBLF_REPEAT) {
            // (Never set in strict mode)
            for(int i = 0; i < 10; i++) {
                sid.drawBar(i, 0, oldIdleHeight[i]);
            }
        } else {
            if(!(flags & SBLF_STRICT)) {
                for(int i = 0; i < 10; i++) {
                    bh = a * (mods[b][i] + ((int)(esp_random() % variation)-vc)) / 100;
                    if(bh < 0) bh = 0;
                    if(bh > 19) bh = 19;
                    if((flags & SBLF_LM) && bh < 9) {
                        bh = 9 + (int)(esp_random() % 4);
                    }
                    if(!(flags & SBLF_ISTT) && abs(bh - oldIdleHeight[i]) > 5) {
                        bh = (oldIdleHeight[i] + bh) / 2;
                    }
                    if(flags & SBLF_ISTT) {
                        if(bh > maxTTHeight[i] || (!(flags & SBLF_ANIM))) bh = maxTTHeight[i];
                    }
                    sid.drawBar(i, 0, bh);
                    oldIdleHeight[i] = bh;
                }
            } else {
                for(int i = 0; i < 10; i++) {
                    bh = ttledseqfull[strictBaseLine][i];
                    if(flags & SBLF_ISTT) {
                        if(bh > maxTTHeight[i] + 1 || (!(flags & SBLF_ANIM))) bh = maxTTHeight[i] + 1;
                    }
                    sid.drawBarWithHeight(i, bh);
                    if(bh > 0) bh--;
                    oldIdleHeight[i] = bh;
                }
            }
        }
    
        if(flags & SBLF_ISTT) {
            if(flags & SBLF_ANIM) {
                if(TTClrBarInc && !(flags & SBLF_LMTT)) {
                    sid.clearBar(TTClrBar);
                    TTClrBar += TTClrBarInc;
                    if(TTClrBar >= 10) {
                        TTClrBar = 9;
                        TTClrBarInc = -1;
                    }
                    if(TTClrBar < 0 && TTClrBarInc < 0) {
                        TTClrBarInc = 1;
                        TTClrBar = 0;
                        TTBarCnt++;
                        if(TTBarCnt > 0) TTBri = true;
                    }
                }
            }
            if(TTBri || (flags & SBLF_LMTT)) {
                int temp1 = sid.getBrightness(), temp2 = 3;
                if(temp1 >= 4) temp1 -= 2;
                else { temp1 = 2; temp2 = 0; }
                sid.setBrightnessDirect((esp_random() % temp1) + temp2); //       13) + 3);
            }
            if(flags & SBLF_LMTT) {
                if(LMTT[TTLMIdx]) {
                    sid.drawLetterMask(LMTT[TTLMIdx], 1, 11);
                }
            }
        } 

    }

    if(!(flags & SBLF_SKIPSHOW)) {
        sid.show();
    }

    #ifdef SID_DBG
    //Serial.printf("baseline %d, strict %d\n", sidBaseLine, strictBaseLine);
    #endif
}

static void showIdle(bool freezeBaseLine)
{
    unsigned long now = millis();
    int oldBaseLine = sidBaseLine;
    int oldSBaseLine = strictBaseLine;
    int variation = 20;
    uint16_t sblFlags = 0;

    idleDelay2 = 800 + ((int)(esp_random() % 200) - 100);

    if(useGPSS && gpsSpeed >= 0) {

        if(!bttfnTCDSeqCnt) {
            bttfnSIDPollInt = BTTFN_POLL_INT_FAST;
        }

        usingGPSS = true;
       
        if(!strictMode) {
            if(now - lastChange < 500) 
                return;

            if(!freezeBaseLine) {
                sidBaseLine = (max(10, (int)gpsSpeed) * 20 / 88) - 1;
                if(sidBaseLine > 19) sidBaseLine = 19;
                if(abs(oldBaseLine - sidBaseLine) > 3) {
                    sidBaseLine = (sidBaseLine + oldBaseLine) / 2;
                }
            }
            
        } else {
          
            if((gpsSpeed == prevGPSSpeed) && (now - lastChange < 500))
                return;
            
            if(!freezeBaseLine) {
                strictBaseLine = gpsSpeed * 100 / (88 * 100 / (TT_SQF_LN - 1));
                if(gpsSpeed == prevGPSSpeed) {
                    if(strictBaseLine < 5) {
                        strictBaseLine += (esp_random() % 5);
                    } else if(strictBaseLine > TT_SQF_LN - 4) {
                        // no modify
                    } else {
                        strictBaseLine += (((esp_random() % 5)) - 2);
                    }
                    if(strictBaseLine < 0) strictBaseLine = 0;
                    if(strictBaseLine > TT_SQF_LN-2) strictBaseLine = TT_SQF_LN-2;
                }
                if(strictBaseLine > TT_SQF_LN-1) strictBaseLine = TT_SQF_LN-1;
                if(abs(oldSBaseLine - strictBaseLine) > 3) {
                    strictBaseLine = (strictBaseLine + oldSBaseLine) / 2;
                }
            }
            sblFlags |= SBLF_STRICT;

        }
        
        lastChange = now;
        prevGPSSpeed = gpsSpeed;

    } else if(idleMode == SID_IDLE_BL) {     // "backlot mode"

        if(now - lastChange < idleDelay)
            return;
          
        lastChange = now;

        idleDelay = 90;

        for(int i = 0; i < 10; i++) {
            sid.drawBarWithHeight(i, idle5[id5idx][i]);
        }
        id5idx++;
        if(id5idx >= ID5_STEPS) id5idx = 0;
        
        sidBaseLine = strictBaseLine = 0;
        sblFlags |= SBLF_NOBL;

    } else {
        
        if(now - lastChange < idleDelay)
            return;
          
        lastChange = now;

        if(strictMode) {
            if(idleMode != SID_IDLE_IDC) {
                sblFlags |= SBLF_STRICT;
            }
        }

        switch(idleMode) {
        case SID_IDLE_1:     // higher peaks, tempo as 0
            idleDelay = 800 + ((int)(esp_random() % 200) - 100);
            if(!strictMode) {
                if(!freezeBaseLine) {
                    if(sidBaseLine > 16) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine > 12) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine < 3) {
                        sidBaseLine += (((esp_random() % 3)) + 2);
                    } else {
                        sidBaseLine += (((esp_random() % 5)) - 1);
                    }
                    variation = 40;
                }
            } else {
                if(!freezeBaseLine) {
                    if(strictBaseLine > 40) {
                        strictBaseLine -= (((esp_random() % 5)) + 1);
                        blWayup = false;
                    } else if(strictBaseLine < 10) {
                        strictBaseLine += (((esp_random() % 5)) + 2);
                        blWayup = true;
                    } else {
                        strictBaseLine += (((esp_random() % 7)) - (blWayup ? 2 : 4));
                    }
                } else {
                    if((esp_random() % 5) >= 2) {
                        strictBaseLine ^= 0x01;   // toggle bit 0, nothing more
                    }
                }
            }
            break;
        case SID_IDLE_2:       // Same as 0, but faster
            idleDelay = 300 + ((int)(esp_random() % 200) - 100);
            if(!strictMode) {
                if(!freezeBaseLine) {
                    if(sidBaseLine > 14) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine > 8) {
                        sidBaseLine -= (((esp_random() % 5)) + 1);
                    } else if(sidBaseLine < 3) {
                        sidBaseLine += (((esp_random() % 3)) + 2);
                    } else {
                        sidBaseLine += (((esp_random() % 4)) - 1);
                    }
                }
            } else {
                if(!freezeBaseLine) {
                    if(strictBaseLine > 30) {
                        strictBaseLine -= (((esp_random() % 3)) + 1);
                        blWayup = false;
                    } else if(strictBaseLine < 10) {
                        strictBaseLine += (((esp_random() % 3)) + 2);
                        blWayup = true;
                    } else {
                        strictBaseLine += (((esp_random() % 7)) - (blWayup ? 2 : 4));
                    }
                } else {
                    if((esp_random() % 5) >= 2) {
                        strictBaseLine ^= 0x01;   // toggle bit 0, nothing more
                    }
                }
            }
            break;
        case SID_IDLE_3:     // higher peaks, faster
            idleDelay = 300 + ((int)(esp_random() % 200) - 100);
            if(!strictMode) {
                if(!freezeBaseLine) {
                    if(sidBaseLine > 16) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine > 12) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine < 3) {
                        sidBaseLine += (((esp_random() % 3)) + 2);
                    } else {
                        sidBaseLine += (((esp_random() % 5)) - 1);
                    }
                    variation = 40;
                }
            } else {
                if(!freezeBaseLine) {
                    if(strictBaseLine > 40) {
                        strictBaseLine -= (((esp_random() % 5)) + 1);
                        blWayup = false;
                    } else if(strictBaseLine < 10) {
                        strictBaseLine += (((esp_random() % 5)) + 2);
                        blWayup = true;
                    } else {
                        strictBaseLine += (((esp_random() % 7)) - (blWayup ? 2 : 4));
                    }
                } else {
                    if((esp_random() % 5) >= 2) {
                        strictBaseLine ^= 0x01;   // toggle bit 0, nothing more
                    }
                }
            }
            break;
        case SID_IDLE_IDC:   // with masked text & "identity crisis" tt seq
            idleDelay = 80;
            sblFlags |= (SBLF_LM|SBLF_SKIPSHOW);
            if(now - lastChange2 < idleDelay2) {
                sblFlags |= SBLF_REPEAT;
            } else {
                if(!freezeBaseLine) {
                    if(sidBaseLine > 18) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine < 3) {
                        sidBaseLine += (((esp_random() % 3)) + 2);
                    } else {
                        sidBaseLine += (((esp_random() % 5)) - 1);
                    }
                    variation = 40;
                }
                lastChange2 = now;
                idleDelay2 = 800 + ((int)(esp_random() % 200) - 100);
            }
            break;
        default:
            idleDelay = 800 + ((int)(esp_random() % 200) - 100);
            if(!strictMode) {
                if(!freezeBaseLine) {
                    if(sidBaseLine > 14) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine > 8) {
                        sidBaseLine -= (((esp_random() % 5)) + 1);
                    } else if(sidBaseLine < 3) {
                        sidBaseLine += (((esp_random() % 3)) + 2);
                    } else {
                        sidBaseLine += (((esp_random() % 4)) - 1);
                    }
                }
            } else {
                if(!freezeBaseLine) {
                    if(strictBaseLine > 30) {
                        strictBaseLine -= (((esp_random() % 3)) + 1);
                        blWayup = false;
                    } else if(strictBaseLine < 10) {
                        strictBaseLine += (((esp_random() % 3)) + 2);
                        blWayup = true;
                    } else {
                        strictBaseLine += (((esp_random() % 7)) - (blWayup ? 2 : 4));
                    }
                } else {
                    if((esp_random() % 5) >= 2) {
                        strictBaseLine ^= 0x01;   // toggle bit 0, nothing more
                    }
                }
            }
            break;
        }
        
        if(!freezeBaseLine) {
            if(usingGPSS) {
                // Smoothen
                if(!(sblFlags & SBLF_STRICT)) {
                    if(abs(oldBaseLine - sidBaseLine) > 3) {
                        sidBaseLine = (sidBaseLine + oldBaseLine) / 2;
                    }
                } else {
                    if(abs(oldSBaseLine - strictBaseLine) > 7) {
                        strictBaseLine = (strictBaseLine + oldSBaseLine) / 2;
                    }
                }
                usingGPSS = false;
            }
        }
    }

    if(sidBaseLine < 0) sidBaseLine = 0;
    else if(sidBaseLine > 19) sidBaseLine = 19;
    if(strictBaseLine < 0) strictBaseLine = 0;
    else if(strictBaseLine > TT_SQF_LN-1) strictBaseLine = TT_SQF_LN-1;
    
    showBaseLine(variation, sblFlags);

    if(sblFlags & SBLF_LM) {
        switch(LMState) {
        case 0:
            if(!LM[LMIdx]) LMIdx = 0;
            LMAdvNow = millis();
            LMDelay = (LM[LMIdx] == '.') ? 400 : 1000;
            LMState++;
            LMY = 11;
            break;
        case 1:
        case 2:
            sid.drawLetterMask(LM[LMIdx], 1, LMY);
            if(millis() - LMAdvNow > LMDelay) {
                LMAdvNow = millis();
                if(LMState == 1) {
                    LMState++;
                    LMDelay = 50;
                } else {
                    LMY--;
                    if(LM[LMIdx] == ' ' || LMY < -8) {
                        LMState = 0;
                        LMIdx++;
                    }
                }
            }
            break;
        }
    }

    if(sblFlags & SBLF_SKIPSHOW) {
        sid.show();
    }
}

/*
 * Time travel
 * 
 */

static void timeTravel(bool TCDtriggered, uint16_t P0Dur)
{
    if(TTrunning || IRLearning)
        return;

    bool initScreen = siActive || snActive;

    siddly_stop();
    snake_stop();

    flushDelayedSave();
    
    if(initScreen) {
        lastChange = 0;
        showIdle();
    }
        
    TTrunning = true;
    TTstart = TTfUpdNow = millis();
    TTP0 = true;   // phase 0
    TTSAStopped = false;
    TTsbFlags = skipTTAnim ? 0 : SBLF_ANIM;
    TTLMIdx = 0;
    
    #ifdef SID_DBG
    Serial.printf("TT: baseLine %d, entry %d\n", sidBaseLine, seqEntry[sidBaseLine]);
    #endif

    if(saActive) {
        TTcnt = TT_AMP_STEPS;
    } else if(usingGPSS) {
        if(strictMode) TTsbFlags |= SBLF_STRICT;
        if(TTsbFlags & SBLF_STRICT) {
            TTcnt = TT_SQF_LN - strictBaseLine;
        } else {
            TTcnt = TT_SQ_LN - seqEntry[sidBaseLine];
        }
    } else {
        if(idleMode == SID_IDLE_IDC) {
            TTsbFlags |= SBLF_LMTT;
        } else if(strictMode) {
            TTsbFlags |= SBLF_STRICT;   // yes, even when idleMode == SID_IDLE_BL
        }
        if(TTsbFlags & SBLF_STRICT) {
            TTcnt = TT_SQF_LN - strictBaseLine;
        } else {
            TTcnt = TT_SQ_LN - seqEntry[sidBaseLine];
        }
    }
    
    if(TCDtriggered) {    // TCD-triggered TT (GPIO, BTTFN or MQTT) (synced with TCD)
        extTT = true;
        P0duration = P0Dur;
        #ifdef SID_DBG
        Serial.printf("P0 duration is %d\n", P0duration);
        #endif
        if(TTcnt > 0) {
            if(P0duration > 2500) {
                TTFDelay = P0duration - 2500;
            } else {
                TTFDelay = 0;
            }
            TTFInt = (P0duration - TTFDelay) / (TTcnt + 1);
            if(TTFInt > 0 && TTFInt < 65) {
                if((TTcnt + 1) * 65 <= P0duration) {
                    TTFInt = 65;
                    TTFDelay = P0duration - (TTFInt * (TTcnt + 1));
                }
            }
        } else {
            TTFInt = 0;
            TTFDelay = 0;
        }
    } else {              // button/IR-triggered TT (stand-alone)
        extTT = false;
        TTFDelay = 2500;
        if(TTcnt > 0) {
            TTFInt = (P0_DUR - TTFDelay) / (TTcnt + 1);
            if(TTFInt > 0 && TTFInt < 65) {
                if((TTcnt + 1) * 65 <= P0_DUR) {
                    TTFInt = 65;
                    TTFDelay = P0_DUR - (TTFInt * (TTcnt + 1));
                }
            }
        } else {
            TTFInt = 0;
        }
    }
    
    #ifdef SID_DBG
    Serial.printf("TTFDelay %d  TTFInt %d  TTcnt %d\n", TTFDelay, TTFInt, TTcnt);
    #endif
}

static void play_startup()
{
    const uint8_t q[10] = {
        27, 26, 20, 27, 20, 25, 28, 20, 25, 28
    };
    const uint8_t qs[10] = {
        24, 20, 20, 28, 20, 20, 20, 20, 20, 20
    };
    const uint8_t q4[10] = {
        20, 20, 20, 20, 20, 27, 27, 27, 27, 27
    };
    uint8_t w[10];
    uint8_t oldBri = sid.getBrightness();

    sid.clearBuf();
    sid.clearDisplayDirect();

    sid.setBrightnessDirect(0);

    // Growing line
    for(int i = 0; i < 5; i++) {
        sid.drawDot(4 - i, 10);
        sid.drawDot(5 + i, 10);
        sid.show();
        if(oldBri >= (i + 1) * 2) sid.setBrightnessDirect((i + 1) * 2);
        mydelay(20 - (i*2), false);
    }

    if(oldBri >= 12) sid.setBrightnessDirect(12);
    
    mydelay(50, false);
    
    sid.setBrightness(255);

    // Fill from center
    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 10; j++) {
            sid.drawBar(j, 10 - i, 10 + i);
        }
        sid.show();
        mydelay(30 - (i*2), false);
    }
    
    for(int i = 0; i < 10; i++) {
        oldIdleHeight[i] = 0;
    }
    
    // Shrink like idle pattern
    if(idleMode == SID_IDLE_BL) {
        memcpy(w, q4, sizeof(w));
    } else if(strictMode && idleMode != SID_IDLE_IDC) {
        memcpy(w, qs, sizeof(w));
    } else {
        memcpy(w, q, sizeof(w));
    }
    for(int i = 0; i < 28/2; i++) {
        for(int j = 0; j < 10; j++) {
            sid.drawBarWithHeight(j, w[j]);
            if(w[j] >= 2) w[j] -= 2;
        }
        sid.show();
        mydelay(30 + (i*5), false);
    }
}

void setIdleMode(int idleNo)
{
    uint16_t temp = idleMode;
    
    idleMode = idleNo;
    if(temp != idleMode) {
        if(idleMode == SID_IDLE_IDC) {
            LMState = LMIdx = 0;
        } else if(idleMode == SID_IDLE_BL) {
            id5idx = 0;
        }
    }
    ipachanged = true;
    ipachgnow = millis();
}

static void toggleStrictMode()
{
    strictMode = !strictMode;
    ipachanged = true;
    ipachgnow = millis();
    updateConfigPortalStrictValue();
}

/*
 * IR remote input handling
 */

static void startIRfeedback()
{
    digitalWrite(IRFeedBackPin, HIGH);
    irFeedBackDur = IR_FEEDBACK_DUR;
    irErrFeedBack = false;
}

static void endIRfeedback()
{
    digitalWrite(IRFeedBackPin, LOW);
}

static void startIRErrFeedback()
{
    startIRfeedback();
    irErrFeedBack = true;
    irErrFeedBackNow = millis();
    irErrFBState = 0;
    irFeedBack = false;
}

static void backupIR()
{
    for(int i = 0; i < NUM_IR_KEYS; i++) {
        backupIRcodes[i] = remote_codes[i][1];
    }
}

static void restoreIRbackup()
{
    for(int i = 0; i < NUM_IR_KEYS; i++) {
        remote_codes[i][1] = backupIRcodes[i];
    }
}

static void startIRLearn()
{
    // Play LEARN START sequence
    showWordSequence("GO", 4);
    IRLearning = true;
    IRLearnIndex = 0;
    IRLearnNow = IRFBLearnNow = millis();
    IRLearnBlink = false;
    backupIR();
    showChar(IRLearnKeys[IRLearnIndex]);
    ir_remote.loop();     // Ignore IR received in the meantime
}

static void endIRLearn(bool restore)
{
    // TODO Restore display
    IRLearning = false;
    endIRfeedback();
    if(restore) {
        restoreIRbackup();
    }
    ir_remote.loop();     // Ignore IR received in the meantime
}

static void handleIRinput()
{
    uint32_t myHash = ir_remote.readHash();
    uint16_t i, j;
    bool done = false;
    
    Serial.printf("handleIRinput: Received IR code 0x%lx\n", myHash);

    if(IRLearning) {
        endIRfeedback();
        remote_codes[IRLearnIndex++][1] = myHash;
        if(IRLearnIndex == NUM_IR_KEYS) {
            // Play LEARN DONE sequence
            fadeOutChar();
            showWordSequence("DONE", 2);
            IRLearning = false;
            saveIRKeys();
            #ifdef SID_DBG
            Serial.println("handleIRinput: All IR keys learned, and saved");
            #endif
        } else {
            // Play LEARN NEXT sequence
            fadeOutChar();
            showChar(IRLearnKeys[IRLearnIndex]);
            #ifdef SID_DBG
            Serial.println("handleIRinput: IR key learned");
            #endif
        }
        if(IRLearning) {
            IRLearnNow = millis();
        } else {
            endIRLearn(false);
        }
        return;
    }

    for(i = 0; i < NUM_IR_KEYS; i++) {
        for(j = 0; j < maxIRctrls; j++) {
            if(remote_codes[i][j] == myHash) {
                #ifdef SID_DBG
                Serial.printf("handleIRinput: key %d\n", i);
                #endif
                handleIRKey(i);
                done = true;
                break;
            }
        }
        if(done) break;
    }
}

static void clearInpBuf()
{
    inputIndex = 0;
    inputBuffer[0] = '\0';
    inputRecord = false;
}

static void recordKey(int key)
{
    if(inputIndex < INPUTLEN_MAX) {
        inputBuffer[inputIndex++] = key + '0';
        inputBuffer[inputIndex] = '\0';
    }
}

static uint8_t read2digs(uint8_t idx)
{
    return ((inputBuffer[idx] - '0') * 10) + (inputBuffer[idx+1] - '0');
}

/*
static void doKey1()
{
}
static void doKey2()
{
}
static void doKey3()
{
}
static void doKey4()
{
}
static void doKey5()
{
}
static void doKey6()
{
}
static void doKey7()
{
}
static void doKey8()
{
}
static void doKey9()
{
}
*/

static void handleIRKey(int key)
{
    uint16_t temp;
    int16_t tempi;
    int doInpReaction = 0;
    unsigned long now = millis();

    if(ssActive) {
        if(!irLocked || key == 11) {
            ssEnd();
            return;
        }
    }

    if(!irLocked) {
        ssRestartTimer();

        startIRfeedback();
        irFeedBack = true;
        irFeedBackNow = now;
    }
    lastKeyPressed = now;
    
    // If we are in "recording" mode, just record and bail
    if(inputRecord && key >= 0 && key <= 9) {
        recordKey(key);
        return;
    }  else if(remMode) {
        // Remote mode for TCD keypad: (Must not be started while irlocked - user wouldn't be able to unlock IR)
        if(key == 11) {   // # quits
            remMode = remHoldKey = false;
            clearInpBuf();    // Relevant if initiated by TCD (6096)
            bttfn_send_command(BTTFN_REMCMD_KP_BYE, 0, 0);
            irFeedBackDur = 1000;
        } else if(key == 10) {   // * means the following key is "held" on TCD keypad
            remHoldKey = true;
        } else {
            uint8_t rkey = (key >= 0 && key <= 9) ? key + '0' : ((key == 16) ? rkey = 'E' : 0);
            if(rkey) {
                bttfn_send_command(BTTFN_REMCMD_KP_KEY, rkey, remHoldKey ? BTTFN_KP_KS_HOLD : BTTFN_KP_KS_PRESSED);
            }
            remHoldKey = false;
        }
        return;
    }
    
    switch(key) {
    case 0:                           // 0: time travel           si: fall down   sn: -
        if(irLocked) return;
        if(siActive) {
            si_fallDown();
        } else if (snActive) {
            // Nothing
        } else {
            if(!bttfnTT || !bttfn_trigger_tt()) {
                timeTravel(false, ETTO_LEAD);
            }
        }
        break;
    case 1:                           // 1: si/sn: quit
        if(irLocked) return;
        if(siActive) {
            siddly_stop(); 
        } else if(snActive) {
            snake_stop(); 
        }
        break;
    case 2:                           // 2:
        if(irLocked) return;
        break;
    case 3:                           // 3: si/sn: new game (restart)
        if(irLocked) return;
        if(siActive) {
            si_newGame();
        } else if(snActive) {
            sn_newGame();
        }
        break;
    case 4:                           // 4:
        if(irLocked) return;
        break;
    case 5:                           // 5:
        if(irLocked) return;
        break;
    case 6:                           // 6:
        if(irLocked) return;
        break;
    case 7:                           // 7:
        if(irLocked) return;
        break;
    case 8:                           // 8:
        if(irLocked) return;
        break;
    case 9:                           // 9: si/sn: pause game (toggle)
        if(irLocked) return;
        if(siActive) {
            si_pause();
        } else if(snActive) {
            sn_pause();
        }
        break;
    case 10:                          // * - start code input
        clearInpBuf();
        inputRecord = true;
        break;
    case 11:                          // # - abort code input
        clearInpBuf();
        break;
    case 12:                          // arrow up: inc brightness     si: rotate sn: move up
        if(irLocked) return;
        if(siActive) {
            si_rotate();
        } else if(snActive) {
            sn_moveUp();
        } else if(saActive) {
            // nothing
        } else {
            inc_bri();
            brichanged = true;
            brichgnow = now;
        }
        break;
    case 13:                          // arrow down: dec brightness  si/sn: move down
        if(irLocked) return;
        if(siActive) {
            si_moveDown();
        } else if(snActive) {
            sn_moveDown();
        } else if(saActive) {
            // nothing
        } else {
            dec_bri();
            brichanged = true;
            brichgnow = now;
        }
        break;
    case 14:                          // arrow left:                si/sn: move left
        if(irLocked) return;
        if(siActive) {
            si_moveLeft();
        } else if(snActive) {
            sn_moveLeft();
        } else {
            // NOTHING
        }
        break;
    case 15:                          // arrow right:               si/sn: move right
        if(irLocked) return;
        if(siActive) {
            si_moveRight();
        } else if(snActive) {
            sn_moveRight();
        } else {
            // NOTHING
        }
        break;
    case 16:                          // ENTER: Execute code command
        doInpReaction = execute(true);
        clearInpBuf();
        break;
    default:
        if(!irLocked) {
            doInpReaction = -1;
        }
    }

    if(doInpReaction < 0) {
        startIRErrFeedback();
    } else if(doInpReaction) {
        irFeedBackDur = 1000;
    }
}

static void handleRemoteCommand()
{
    uint32_t command = commandQueue[oCmdIdx];
    int      doInpReaction = 0;

    if(!command)
        return;

    commandQueue[oCmdIdx] = 0;
    oCmdIdx++;
    oCmdIdx &= 0x0f;

    if(ssActive) {
        ssEnd();
    }
    ssRestartTimer();

    // Some translation
    if(command < 10) {
      
        // <10 are translated to direct-key-actions.
        // Right now, there are no direct-key actions suitable 
        // for BTTFN remote control.
        /*
        switch(command) {
        case 1:
            doKey1();
            break;
        case 2:
            doKey2();
            break;
        case 3:
            doKey3();
            break;
        case 4:
            doKey4();
            break;
        case 5:
            doKey5();
            break;
        case 6:
            doKey6();
            break;
        case 7:
            doKey7();
            break;
        case 8:
            doKey8();
            break;
        case 9:
            doKey9();
            break;
        }
        */

        doInpReaction = -1;
        
    } else {

        // Backup current IR input buffer
        memcpy(inputBackup, inputBuffer, sizeof(inputBuffer));
    
        if(command < 100) {
          
            sprintf(inputBuffer, "%02d", command);
            
        } else if(command < 1000) {
          
            sprintf(inputBuffer, "%03d", command);
            
        } else if(command < 100000) {
    
            sprintf(inputBuffer, "%05d", command);
            
        } else {
          
            sprintf(inputBuffer, "%06d", command);
    
        }
    
        doInpReaction = execute(false);

        // Restore current IR input buffer
        memcpy(inputBuffer, inputBackup, sizeof(inputBuffer));

    }

    if(doInpReaction < 0) {
        startIRErrFeedback();
    } else if(doInpReaction > 0) {
        startIRfeedback();
        irFeedBack = true;
        irFeedBackNow = millis();
        irFeedBackDur = 1000;
    }
}

static int execute(bool isIR)
{
    int  inputReaction = 0;
    bool isIRLocked = isIR ? irLocked : false;
    uint16_t temp;
    unsigned long now = millis();

    switch(strlen(inputBuffer)) {
    case 1:
        if(!isIRLocked) {
            inputReaction = -1;
        }
        break;
    case 2:
        temp = atoi(inputBuffer);
        if(temp >= 10 && temp <= 19) {            // *10-*15 idle pattern
            if(!isIRLocked) {
                if(temp <= (10 + SID_MAX_IDLE_MODE)) {
                    setIdleMode(temp - 10);
                    inputReaction = 1;
                } else {
                    inputReaction = -1;
                }
            }
        } else {
            switch(temp) {
            case 20:                              // *20 idle mode
                if(!isIRLocked) {
                    if(!TTrunning) {
                        span_stop();
                        siddly_stop();
                        snake_stop();
                        inputReaction = 1;
                    } else inputReaction = -1;
                }
                break;
            case 21:                              // *21 sa mode
                if(!isIRLocked) {
                    if(!TTrunning) {
                        siddly_stop();
                        snake_stop();
                        span_start();
                        inputReaction = 1;
                    } else inputReaction = -1;
                }
                break;
            case 22:                              // *22 siddly
                if(!isIRLocked) {
                    if(!TTrunning) {
                        span_stop();
                        snake_stop();
                        siddly_start();
                        inputReaction = 1;
                    } else inputReaction = -1;
                }
                break;
            case 23:                              // *23 snake
                if(!isIRLocked) {
                    if(!TTrunning) {
                        siddly_stop();
                        span_stop();
                        snake_start();
                        inputReaction = 1;
                    } else inputReaction = -1;
                }
                break;
            case 60:                              // *60  enable/disable strict mode
                if(!isIRLocked) {
                    if(!TTrunning) {
                        toggleStrictMode();
                        inputReaction = 1;
                    } else inputReaction = -1;
                }
                break;
            case 61:                              // *61  enable/disable "peaks" in Spectrum Analyzer
                if(!isIRLocked) {
                    if(!TTrunning) {
                        doPeaks = !doPeaks;
                        inputReaction = 1;
                    } else inputReaction = -1;
                }
                break;
            case 70:                              // *70 taken by FC IR lock sequence
              // Stay silent
              break;
            case 71:                              // *71 lock/unlock ir
                if(remMode) {
                    // Need to quit remote mode before locking ir
                    // (Use case: Enter 6071 on TCD keypad during remMode)
                    remMode = remHoldKey = false;                    
                    bttfn_send_command(BTTFN_REMCMD_KP_BYE, 0, 0);
                }
                irLocked = !irLocked;
                irlchanged = true;
                irlchgnow = now;
                if(!irLocked) {
                    // Start IR feedback here, since wasn't done above
                    startIRfeedback();
                    irFeedBack = true;
                    irFeedBackNow = now;
                }
                inputReaction = 1;
                break;
            case 90:                              // *90  display IP address
                if(!isIRLocked) {
                    if(!TTrunning) {
                        uint8_t a, b, c, d;
                        char ipbuf[16];
                        wifi_getIP(a, b, c, d);
                        sprintf(ipbuf, "%d.%d.%d.%d", a, b, c, d);
                        span_stop();
                        siddly_stop();
                        snake_stop();
                        flushDelayedSave();
                        showWordSequence(ipbuf, 5);
                        ir_remote.loop(); // Flush IR afterwards
                    } else inputReaction = -1;
                }
                break;
            case 96:                              // *96  enter TCD keypad remote control mode
                if(!irLocked) {                   //      yes, 'irLocked' - must not be entered while IR is locked
                    if(!TTrunning) {
                        if(BTTFNConnected() && TCDSupportsRemKP && remoteAllowed) {
                            remMode = true;
                            inputReaction = 1;
                        } else {
                            inputReaction = -1;
                        }
                    } else inputReaction = -1;
                }
                break;
            default:
                if(!isIRLocked) {
                    inputReaction = -1;
                }
            }
        }
        break;
    case 3:
        if(!isIRLocked) {
            temp = atoi(inputBuffer);
            if(temp >= 400 && temp <= 415) {
                if(!TTrunning) {
                    sid.setBrightness(temp - 400);
                    brichanged = true;
                    brichgnow = now;
                    inputReaction = 1;
                } else inputReaction = -1;
            } else {
                inputReaction = -1;
            }
        }
        break;
    case 5:
        if(!isIRLocked) {
            if(!strcmp(inputBuffer, "64738")) {
                allOff();
                endIRfeedback();
                flushDelayedSave();
                unmount_fs();
                delay(50);
                esp_restart();
            }
            inputReaction = -1;
        }
        break;
    case 6:
        if(!isIRLocked) {
            if(!TTrunning) {
                if(!strcmp(inputBuffer, "123456")) {
                    flushDelayedSave();
                    deleteIpSettings();               // *123456OK deletes IP settings
                    if(settings.appw[0]) {
                        settings.appw[0] = 0;             // and clears AP mode WiFi password
                        write_settings();
                    }
                    inputReaction = 1;
                } else if(!strcmp(inputBuffer, "654321")) {
                    deleteIRKeys();                   // *654321OK deletes learned IR remote
                    for(int i = 0; i < NUM_IR_KEYS; i++) {
                        remote_codes[i][1] = 0;
                    }
                    inputReaction = 1;
                } else {
                    inputReaction = -1;
                }
            } else inputReaction = -1;
        }
        break;
    default:
        if(!isIRLocked) {
            inputReaction = -1;
        }
    }
    
    return inputReaction;
}

/*
 * Modes of operation
 */

static void span_start()
{
    sid.clearDisplayDirect();
    sa_activate();
}

static void span_stop(bool skipClearDisplay)
{
    if(saActive) {
        sa_deactivate();
        if(skipClearDisplay) {
            sid.clearDisplayDirect();
        }
        LMState = LMIdx = id5idx = 0;
    }
}

static void siddly_start()
{
    sid.clearDisplayDirect();
    si_init();
}

static void siddly_stop()
{
    if(siActive) {
        si_end();
        sid.clearDisplayDirect();
        LMState = LMIdx = id5idx = 0;
    }
}

static void snake_start()
{
    sid.clearDisplayDirect();
    sn_init();
}

static void snake_stop()
{
    if(snActive) {
        sn_end();
        sid.clearDisplayDirect();
        LMState = LMIdx = id5idx = 0;
    }
}

void switch_to_sa()
{
    snake_stop();
    siddly_stop();
    span_start();
}

void switch_to_idle()
{
    snake_stop();
    siddly_stop();
    span_stop();
}

/*
 * Helpers
 */

static void inc_bri()
{
    uint8_t cb = sid.getBrightness();
    if(cb == 15)
        return;
    sid.setBrightness(cb + 1);
}

static void dec_bri()
{
    uint8_t cb = sid.getBrightness();
    if(!cb)
        return;
    sid.setBrightness(cb - 1);
}

void showWaitSequence(bool force)
{
    // Show a "wait" symbol
    ssEnd();
    if(force) sid.on();
    sid.clearDisplayDirect();
    sid.drawLetterAndShow('&', 0, 8);
}

void endWaitSequence()
{
    sid.clearDisplayDirect();
    LMState = LMIdx = id5idx = 0;
}

void allOff()
{
    sid.clearDisplayDirect();
}

static void fadeOut()
{
    int a = sid.getBrightness();
    while(a >= 0) {
        sid.setBrightnessDirect(a);
        a--;
        mydelay(10, false);
    }
}

void showWordSequence(const char *text, int speed)
{
    const int speedDelay[6] = { 100, 200, 300, 400, 500, 1000 };

    if(speed < 0) speed = 0;
    if(speed > 5) speed = 5;
    
    sid.clearDisplayDirect();
    for(int i = 0; i < strlen(text); i++) {
        sid.drawLetterAndShow(text[i], 0, 8);
        sid.setBrightness(255);
        mydelay(speedDelay[speed], false);
        fadeOut();
        mydelay(50, false);
    }
    sid.clearDisplayDirect();
    sid.setBrightness(255);
    LMState = LMIdx = id5idx = 0;
    ir_remote.loop();     // Ignore IR received in the meantime
}

static void showChar(const char text)
{
    sid.clearDisplayDirect();
    sid.drawLetterAndShow(text, 0, 8);
}

static void fadeOutChar()
{
    fadeOut();
    mydelay(50, false);
    sid.clearDisplayDirect();
    sid.setBrightness(255);
    LMState = LMIdx = id5idx = 0;
    ir_remote.loop();     // Ignore IR received in the meantime
}

void populateIRarray(uint32_t *irkeys, int index)
{
    for(int i = 0; i < NUM_IR_KEYS; i++) {
        remote_codes[i][index] = irkeys[i]; 
    }
}

void copyIRarray(uint32_t *irkeys, int index)
{
    for(int i = 0; i < NUM_IR_KEYS; i++) {
        irkeys[i] = remote_codes[i][index];
    }
}

// TT button

static void ttkeyScan()
{
    TTKey.scan();  // scan the tt button
}

static void TTKeyPressed()
{
    isTTKeyPressed = true;
}

static void TTKeyHeld()
{
    isTTKeyHeld = true;
}

// "Screen saver"

static void ssDoClock()
{
    const int ssClkx[4] = {0, 1, 1, 0};
    const int ssClky[4] = {1, 1, 2, 2};
    ssClkPos++;
    ssClkPos&=0x03;
    sid.drawClockAndShow(bttfnDateBuf, ssClkx[ssClkPos], ssClky[ssClkPos]);
    memcpy(ssDateBuf, bttfnDateBuf, sizeof(bttfnDateBuf));
    if(sidNM) {
        if(ssClockOffinNM) {
            sid.off();
        } else {
            sid.setBrightnessDirect(0);
        }
    } else {
        sid.setBrightness(255);
    }
    ssNMbuf = sidNM;
}

static void ssStart()
{
    if(ssActive)
        return;

    if(ssClock && (bttfnDateBuf[0] != 0xff)) {
        ssDoClock();
        ssIsClock = true;
    } else {
        // Display off
        sid.off();
        ssIsClock = false;
    }

    ssActive = true;
}

static void ssRestartTimer()
{
    ssLastActivity = millis();
}

static void ssEnd()
{
    if(!FPBUnitIsOn)
        return;
        
    ssRestartTimer();
    
    if(!ssActive)
        return;

    if(ssIsClock) {
        sid.clearDisplayDirect();
    }
    
    sid.on();
    sid.setBrightness(255);

    ssActive = false;
}

static void ssUpdateClock()
{
    if(ssIsClock) {
        if(millis() - bttfnDateNow > 5000) {
            // Lost contact - switch off clock
            ssIsClock = false;
            sid.off();
        } else {
            if(memcmp(ssDateBuf+4, bttfnDateBuf+4, 2) || (ssNMbuf != sidNM)) {
                ssDoClock();
            }
        }
    } else if(millis() - bttfnDateNow < 2000) {
        // Regained contact, switch clock on again
        ssDoClock();
        ssIsClock = true;
        sid.on();
    }
}

// Prepare TT: Stop games, disable s-s
void prepareTT()
{
    // Prepare for time travel
    ssEnd();
    siddly_stop();
    snake_stop();
}

// Wakeup: Sent by TCD upon entering dest date,
// return from tt, triggering delayed tt via ETT
// For audio-visually synchronized behavior
void wakeup()
{
    // End screen saver
    ssEnd();
}

/*
 *  Do this whenever we are caught in a while() loop
 *  This allows other stuff to proceed
 *  withIR is needed for shorter delays; it allows
 *  filtering out repeated keys. If IR is flushed
 *  after the delay, withIR is not needed.
 */
static void myloop(bool withIR)
{
    wifi_loop();
    bttfn_loop_quick();
    if(withIR) ir_remote.loop();
}

/*
 * MyDelay:
 * Calls myloop() periodically
 */
void mydelay(unsigned long mydel, bool withIR)
{
    unsigned long startNow = millis();
    myloop(withIR);
    while(millis() - startNow < mydel) {
        delay(10);
        myloop(withIR);
    }
}


/*
 * Basic Telematics Transmission Framework (BTTFN)
 */

static void addCmdQueue(uint32_t command)
{
    #ifdef SID_DBG
    Serial.printf("Received command %d\n", command);
    #endif
    
    if(!command) return;

    commandQueue[iCmdIdx] = command;
    iCmdIdx++;
    iCmdIdx &= 0x0f;
}

static void bttfn_setup()
{
    useBTTFN = false;

    // string empty? Disable BTTFN.
    if(!settings.tcdIP[0])
        return;

    haveTCDIP = isIp(settings.tcdIP);
    
    if(!haveTCDIP) {
        #ifdef BTTFN_MC
        tcdHostNameHash = 0;
        unsigned char *s = (unsigned char *)settings.tcdIP;
        for ( ; *s; ++s) tcdHostNameHash = 37 * tcdHostNameHash + tolower(*s);
        #else
        return;
        #endif
    } else {
        bttfnTcdIP.fromString(settings.tcdIP);
    }
    
    sidUDP = &bttfUDP;
    sidUDP->begin(BTTF_DEFAULT_LOCAL_PORT);

    #ifdef BTTFN_MC
    sidMcUDP = &bttfMcUDP;
    sidMcUDP->beginMulticast(bttfnMcIP, BTTF_DEFAULT_LOCAL_PORT + 2);
    #endif
    
    BTTFNfailCount = 0;
    useBTTFN = true;
}

void bttfn_loop()
{
    #ifdef BTTFN_MC
    int t = 10;
    #endif
    
    if(!useBTTFN)
        return;

    #ifdef BTTFN_MC
    while(bttfn_checkmc() && t--) {}
    #endif

    BTTFNCheckPacket();
    
    if(!BTTFNPacketDue) {
        // If WiFi status changed, trigger immediately
        if(!BTTFNWiFiUp && (WiFi.status() == WL_CONNECTED)) {
            BTTFNUpdateNow = 0;
        }
        if((!BTTFNUpdateNow) || (millis() - BTTFNUpdateNow > bttfnSIDPollInt)) {
            BTTFNTriggerUpdate();
        }
    }
}

static void bttfn_loop_quick()
{
    #ifdef BTTFN_MC
    int t = 10;
    #endif
    
    if(!useBTTFN)
        return;

    #ifdef BTTFN_MC
    while(bttfn_checkmc() && t--) {}
    #endif
}

static bool check_packet(uint8_t *buf)
{
    // Basic validity check
    if(memcmp(BTTFUDPBuf, BTTFUDPHD, 4))
        return false;

    uint8_t a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += BTTFUDPBuf[i] ^ 0x55;
    }
    if(BTTFUDPBuf[BTTF_PACKET_SIZE - 1] != a)
        return false;

    return true;
}

static void handle_tcd_notification(uint8_t *buf)
{
    uint32_t seqCnt;

    #ifdef SID_DBG
    Serial.printf("Received notification %d\n", buf[5]);
    #endif
    
    switch(buf[5]) {
    case BTTFN_NOT_PREPARE:
        // Prepare for TT. Comes at some undefined point,
        // an undefined time before the actual tt, and
        // may not come at all.
        // We don't ignore this if TCD is connected by wire,
        // because this signal does not come via wire.
        if(!TTrunning && !IRLearning) {
            prepareTT();
        }
        break;
    case BTTFN_NOT_TT:
        // Trigger Time Travel (if not running already)
        // Ignore command if TCD is connected by wire
        if(!TCDconnected && !TTrunning && !IRLearning) {
            networkTimeTravel = true;
            networkTCDTT = true;
            networkReentry = false;
            networkAbort = false;
            networkLead = buf[6] | (buf[7] << 8);
        }
        break;
    case BTTFN_NOT_REENTRY:
        // Start re-entry (if TT currently running)
        // Ignore command if TCD is connected by wire
        if(!TCDconnected && TTrunning && networkTCDTT) {
            networkReentry = true;
        }
        break;
    case BTTFN_NOT_ABORT_TT:
        // Abort TT (if TT currently running)
        // Ignore command if TCD is connected by wire
        if(!TCDconnected && TTrunning && networkTCDTT) {
            networkAbort = true;
        }
        break;
    case BTTFN_NOT_ALARM:
        networkAlarm = true;
        // Eval this at our convenience
        break;
    case BTTFN_NOT_SID_CMD:
        addCmdQueue(GET32(buf, 6));
        break;
    case BTTFN_NOT_WAKEUP:
        if(!TTrunning && !IRLearning) {
            wakeup();
        }
        break;
    case BTTFN_NOT_SPD:
        seqCnt = GET32(buf, 12);
        if(seqCnt == 1 || seqCnt > bttfnTCDSeqCnt) {
            gpsSpeed = (int16_t)(buf[6] | (buf[7] << 8));
            if(gpsSpeed > 88) gpsSpeed = 88;
            switch(buf[8] | (buf[9] << 8)) {
            case BTTFN_SSRC_GPS:
                spdIsRotEnc = false;
                break;
            default:
                spdIsRotEnc = true;
            }
        } else {
            #ifdef SID_DBG
            Serial.printf("Out-of-sequence packet received from TCD %d %d\n", seqCnt, bttfnTCDSeqCnt);
            #endif
        }
        bttfnTCDSeqCnt = seqCnt;
        break;
    }
}

#ifdef BTTFN_MC
static bool bttfn_checkmc()
{
    int psize = sidMcUDP->parsePacket();

    if(!psize) {
        return false;
    }
    
    sidMcUDP->read(BTTFMCBuf, BTTF_PACKET_SIZE);

    #ifdef SID_DBG
    Serial.printf("Received multicast packet from %s\n", sidMcUDP->remoteIP().toString());
    #endif

    if(haveTCDIP) {
        if(bttfnTcdIP != sidMcUDP->remoteIP())
            return false;
    } else {
        // Do not use tcdHostNameHash; let DISCOVER do its work
        // and wait for a result.
        return false;
    }

    if(!check_packet(BTTFMCBuf))
        return false;

    if((BTTFMCBuf[4] & 0x4f) == (BTTFN_VERSION | 0x40)) {

        // A notification from the TCD
        handle_tcd_notification(BTTFMCBuf);
    
    } else {
      
        return false;

    }

    return true;
}
#endif

// Check for pending packet and parse it
static void BTTFNCheckPacket()
{
    unsigned long mymillis = millis();
    
    int psize = sidUDP->parsePacket();
    if(!psize) {
        if(BTTFNPacketDue) {
            if((mymillis - BTTFNTSRQAge) > 700) {
                // Packet timed out
                BTTFNPacketDue = false;
                // Immediately trigger new request for
                // the first 10 timeouts, after that
                // the new request is only triggered
                // in greater intervals via bttfn_loop().
                if(haveTCDIP && BTTFNfailCount < 10) {
                    BTTFNfailCount++;
                    BTTFNUpdateNow = 0;
                }
            }
        }
        return;
    }
    
    sidUDP->read(BTTFUDPBuf, BTTF_PACKET_SIZE);

    if(!check_packet(BTTFUDPBuf))
        return;

    if((BTTFUDPBuf[4] & 0x4f) == (BTTFN_VERSION | 0x40)) {

        // A notification from the TCD
        handle_tcd_notification(BTTFUDPBuf);
      
    } else {

        // (Possibly) a response packet
    
        if(GET32(BTTFUDPBuf, 6) != BTTFUDPID)
            return;
    
        // Response marker missing or wrong version, bail
        if((BTTFUDPBuf[4] & 0x8f) != (BTTFN_VERSION | 0x80))
            return;

        BTTFNfailCount = 0;
    
        // If it's our expected packet, no other is due for now
        BTTFNPacketDue = false;

        #ifdef BTTFN_MC
        if(BTTFUDPBuf[5] & 0x80) {
            if(!haveTCDIP) {
                bttfnTcdIP = sidUDP->remoteIP();
                haveTCDIP = true;
                #ifdef SID_DBG
                Serial.printf("Discovered TCD IP %d.%d.%d.%d\n", bttfnTcdIP[0], bttfnTcdIP[1], bttfnTcdIP[2], bttfnTcdIP[3]);
                #endif
            } else {
                #ifdef SID_DBG
                Serial.println("Internal error - received unexpected DISCOVER response");
                #endif
            }
        }
        #endif

        if(BTTFUDPBuf[5] & 0x01) {
            memcpy(bttfnDateBuf, &BTTFUDPBuf[10], sizeof(bttfnDateBuf));
            bttfnDateNow = mymillis;
        }

        if(BTTFUDPBuf[5] & 0x02) {
            gpsSpeed = (int16_t)(BTTFUDPBuf[18] | (BTTFUDPBuf[19] << 8));
            if(gpsSpeed > 88) gpsSpeed = 88;
            spdIsRotEnc = (BTTFUDPBuf[26] & (0x80|0x20)) ? true : false;    // Speed is from RotEnc or Remote
        }

        if(BTTFUDPBuf[5] & 0x10) {
            tcdNM  = (BTTFUDPBuf[26] & 0x01) ? true : false;
            tcdFPO = (BTTFUDPBuf[26] & 0x02) ? true : false;   // 1 means fake power off
            remoteAllowed = (BTTFUDPBuf[26] & 0x08) ? true : false;
        } else {
            tcdNM = false;
            tcdFPO = false;
            remoteAllowed = false;
        }

        if(BTTFUDPBuf[5] & 0x40) {
            bttfnReqStatus &= ~0x40;     // Do no longer poll capabilities
            #ifdef BTTFN_MC
            if(BTTFUDPBuf[31] & 0x01) {
                bttfnMcMarker = BTTFN_SUP_MC;
                bttfnReqStatus &= ~0x02; // Do no longer poll speed, comes over multicast
            }
            #endif
            if(BTTFUDPBuf[31] & 0x08) {
                TCDSupportsRemKP = true;
            }
        }

        lastBTTFNpacket = mymillis;
    }
}

// Send a new data request
static bool BTTFNTriggerUpdate()
{
    BTTFNPacketDue = false;

    BTTFNUpdateNow = millis();

    if(WiFi.status() != WL_CONNECTED) {
        BTTFNWiFiUp = false;
        return false;
    }

    BTTFNWiFiUp = true;

    // Send new packet
    BTTFNSendPacket();
    BTTFNTSRQAge = millis();
    
    BTTFNPacketDue = true;
    
    return true;
}

static void BTTFNPreparePacket()
{
    memset(BTTFUDPBuf, 0, BTTF_PACKET_SIZE);

    // ID
    memcpy(BTTFUDPBuf, BTTFUDPHD, 4);

    // Tell the TCD about our hostname (0-term., 13 bytes total)
    strncpy((char *)BTTFUDPBuf + 10, settings.hostName, 12);
    BTTFUDPBuf[10+12] = 0;

    BTTFUDPBuf[10+13] = BTTFN_TYPE_SID;

    // Version, MC-marker
    #ifdef BTTFN_MC
    BTTFUDPBuf[4] = BTTFN_VERSION | bttfnMcMarker;  
    #else
    BTTFUDPBuf[4] = BTTFN_VERSION;
    #endif

    // Remote-ID
    SET32(BTTFUDPBuf, 35, myRemID);                 
}

static void BTTFNDispatch()
{
    uint8_t a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += BTTFUDPBuf[i] ^ 0x55;
    }
    BTTFUDPBuf[BTTF_PACKET_SIZE - 1] = a;

    #ifdef BTTFN_MC
    if(haveTCDIP) {
    #endif  
        sidUDP->beginPacket(bttfnTcdIP, BTTF_DEFAULT_LOCAL_PORT);
    #ifdef BTTFN_MC    
    } else {
        #ifdef SID_DBG
        Serial.printf("Sending multicast (hostname hash %x)\n", tcdHostNameHash);
        #endif
        sidUDP->beginPacket(bttfnMcIP, BTTF_DEFAULT_LOCAL_PORT + 1);
    }
    #endif
    sidUDP->write(BTTFUDPBuf, BTTF_PACKET_SIZE);
    sidUDP->endPacket();
}

static void BTTFNSendPacket()
{   
    BTTFNPreparePacket();
    
    // Serial
    BTTFUDPID = (uint32_t)millis();
    SET32(BTTFUDPBuf, 6, BTTFUDPID);

    // Request flags
    BTTFUDPBuf[5] = bttfnReqStatus;                

    #ifdef BTTFN_MC
    if(!haveTCDIP) {
        BTTFUDPBuf[5] |= 0x80;
        SET32(BTTFUDPBuf, 31, tcdHostNameHash);
    }
    #endif

    BTTFNDispatch();
}

static bool BTTFNConnected()
{
    if(!useBTTFN)
        return false;

    #ifdef BTTFN_MC
    if(!haveTCDIP)
        return false;
    #endif

    if(WiFi.status() != WL_CONNECTED)
        return false;

    if(!lastBTTFNpacket)
        return false;

    return true;
}

static bool bttfn_trigger_tt()
{
    if(!BTTFNConnected())
        return false;

    if(TTrunning || IRLearning)
        return false;

    BTTFNPreparePacket();

    // Trigger BTTFN-wide TT
    BTTFUDPBuf[5] = 0x80;           

    BTTFNDispatch();

    #ifdef SID_DBG
    Serial.println("Triggered BTTFN-wide TT");
    #endif

    return true;
}

static bool bttfn_send_command(uint8_t cmd, uint8_t p1, uint8_t p2)
{
    if(!TCDSupportsRemKP || !remoteAllowed)
        return false;
        
    if(!BTTFNConnected())
        return false;

    BTTFNPreparePacket();
    
    BTTFUDPBuf[5] = 0x00;

    SET32(BTTFUDPBuf, 6, bttfnSeqCnt[cmd]);
    bttfnSeqCnt[cmd]++;
    if(!bttfnSeqCnt[cmd]) bttfnSeqCnt[cmd]++;

    BTTFUDPBuf[25] = cmd;
    BTTFUDPBuf[26] = p1;
    BTTFUDPBuf[27] = p2;

    BTTFNDispatch();

    return true;
}
