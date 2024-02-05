/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * Spectrum Analyzer
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
#include "src/arduinoFFT/arduinoFFT.h"
#include <driver/i2s.h>
#include <driver/adc.h>
#include <soc/i2s_reg.h>
#include "sid_main.h"

#define NUMBANDS      11    // Number of bands ("bins" in FFT-speak)
#define DISPLAYBANDS  10    // Displayed number of bands
#define LEDS_PER_BAR  20    // Height of bar

#define NUMSAMPLES  1024    // Size of sample block
#define SAMPLERATE 32000    // Sampling frequency

#define PEAK_HOLD    500    // ms - Peak hold time
#define PEAK_FALL    100    // ms - Peak fall speed

//#define SA_DBG_WRITEOUT   // For debugging

static const i2s_port_t I2S_PORT = I2S_NUM_0;

static int32_t rawSamples[NUMSAMPLES];
static FTYPE vReal[NUMSAMPLES];
static FTYPE vImag[NUMSAMPLES];

static FTYPE freqBands[NUMBANDS] = { 0 };

// 32 = 32ms * 32 = 1 sec
// 64 = 32ms * 64 = 2 secs
// 128 = 32ms * 128 = 4 secs
#define FQ_HIST 128
static int histIdx = 0;
static FTYPE freqBandsHistory[FQ_HIST][NUMBANDS] = { 0 };

// The frequency bands
// First one is "garbage bin", not used for display
static const int freqSteps[NUMBANDS] = {
    80,  100,  150,  250,  430,  600, 1000, 2000, 4000, 7000, 10000
};

// Noise threshold per band. Lower bands have more noise.
static const int minTreshold[NUMBANDS] = {
    0, 5000, 5000, 5000, 3000, 1000, 1000, 1000, 1000, 1000, 1000
};

static const uint8_t maxTTHeight[10] = {
    20, 20, 13, 20, 20, 20, 20, 10, 20, 17
};

static int oldHeight[DISPLAYBANDS]  = { 0 };

static uint8_t       peaks[DISPLAYBANDS]     = { 0 };
static unsigned long newPeak[DISPLAYBANDS]   = { 0 };
static unsigned long peakTimer[DISPLAYBANDS] = { 0 };

bool        saActive = false;
static bool sa_avail = false;
bool        doPeaks  = false;
static bool startFlag = false;
static bool initFlag = false;
static bool initDisplay = true;
static unsigned long lastTime  = 0;
static unsigned long lastStart = 0;
static unsigned long startDelay = 0;

int         ampFact = 100;

#if defined(SID_DBG) && defined(SA_DBG_WRITEOUT)
#include "sid_settings.h"
#include <SD.h>
#include <FS.h>
static File outFile;
static bool outFileOpen = false;
#endif

static const i2s_pin_config_t i2sPins = {
    .bck_io_num   = I2S_BCLK_PIN,
    .ws_io_num    = I2S_LRCLK_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_DIN_PIN
};

static const i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = SAMPLERATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 4,
    .dma_buf_len          = 1024,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
};   

static bool sa_setup()
{
    esp_err_t err;

    if(sa_avail)
        return true;

    err = i2s_driver_install(I2S_PORT, &i2s_config,  0, NULL);
    if(err != ESP_OK) {
        #ifdef SID_DBG
        Serial.printf("sa_setup: Failed to install i2s driver (%d)\n", err);
        #endif
        return false;
    }

    // For SPH0645
    REG_SET_BIT(I2S_TIMING_REG(I2S_PORT), BIT(9));
    REG_SET_BIT(I2S_CONF_REG(I2S_PORT), I2S_RX_MSB_SHIFT);

    i2s_set_pin(I2S_PORT, &i2sPins);

    sa_avail = true;

    #if defined(SID_DBG) && defined(SA_DBG_WRITEOUT)
    if(haveSD) {
        outFile = SD.open("/sidsa.pcm", FILE_WRITE);
        outFileOpen = true;
    }
    #endif

    return true;
}

#if 0   // Unused
void sa_remove()
{
    if(!sa_avail)
        return;
        
    i2s_driver_uninstall(I2S_PORT);
    sa_avail = false;
}
#endif

// internal resume/stop

static void sa_resume(bool initDisp, unsigned long start_Delay)
{
    if(!sa_avail)
        sa_setup();
    else 
        i2s_start(I2S_PORT);

    lastTime = lastStart = millis();
    startFlag = true;
    startDelay = start_Delay;
    initFlag = false;
    initDisplay = initDisp;
}

static void sa_stop()
{
    i2s_stop(I2S_PORT);
}

// Externally called activate/deactivate

void sa_activate(bool init, unsigned long start_Delay)
{
    sa_resume(init, start_Delay);

    if(sa_avail)
        saActive = true;
}

void sa_deactivate()
{
    if(sa_avail)
        sa_stop();

    saActive = false;

    #if defined(SID_DBG) && defined(SA_DBG_WRITEOUT)
    outFile.close();
    outFileOpen = false;
    #endif
}

// Set amplification factor

int sa_setAmpFact(int newAmpFact)
{
    int old = ampFact;

    if(newAmpFact >= 0) {
        ampFact = newAmpFact;
    }

    return old;
}

// The loop

void sa_loop()
{
    size_t bytesRead = 0;
    unsigned long now = millis();
    int mmaxi = 0, band = 0;
    FTYPE mmax = 1.0;
    
    if(!saActive || !sa_avail)
        return;
    
    if(lastTime && (now - lastTime < (NUMSAMPLES * 1000 / SAMPLERATE)))
        return;

    lastTime = now;

    // Read i2c data  - do I need a timeout? FIXME
    i2s_read(I2S_PORT, (void *)rawSamples, sizeof(rawSamples), &bytesRead, portMAX_DELAY);

    if(bytesRead != sizeof(rawSamples)) {
        // what now?
        #ifdef SID_DBG
        Serial.println("bytesRead != sizeof(rawSamples)");
        #endif
    }

    #if defined(SID_DBG) && defined(SA_DBG_WRITEOUT)
    
    if(outFileOpen) {
        outFile.write((uint8_t *)&rawSamples[0], NUMSAMPLES * 4);
    }
    
    #else

    // Convert; clear vImag
    for(int i = 0; i < NUMSAMPLES; i++) {
        vReal[i] = (FTYPE)(rawSamples[i] / 16384); // do NOT shift; result of shifting negative integer is undefined
        vImag[i] = 0.0;
    }

    // Do the FFT
    arduinoFFT FFT = arduinoFFT(vReal, vImag, NUMSAMPLES, SAMPLERATE);

    // Remove hum and dc offset
    FFT.DCRemoval();

    // Windowing: "Rectangle" does fine for our purpose
    // and since this does effectively nothing, skip it.
    //FFT.Windowing(FFT_WIN_TYP_RECTANGLE, FFT_FORWARD);
    //FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    
    FFT.Compute(FFT_FORWARD);
    
    //FFT.ComplexToMagnitude(); // Covers entire array, half would do
    FFT.ComplexToMagnitude(vReal, vImag, NUMSAMPLES/2);

    // Fill frequency bands
    // Max freq = Half of sampling rate => (SAMPLERATE / 2)
    // vReal only filled half because of this => (NUMSAMPLES / 2)
    band = 0;
    for(int i = 3; i < NUMSAMPLES / 2; i++) {
        int freq = (i - 2) * (SAMPLERATE / 2) / (NUMSAMPLES / 2);
        if(freq >= freqSteps[band]) {
            band++;
            if(band == NUMBANDS) break;
            else freqBands[band] = 0.0;
        }
        if(band && (vReal[i] > minTreshold[band])) {
            freqBands[band] += vReal[i];      
        }
    }

    // Store absolute band sums to our history table
    for(int i = 1; i < NUMBANDS; i++) {
        freqBandsHistory[histIdx][i] = freqBands[i];
    }
    histIdx++;
    histIdx &= (FQ_HIST-1);

    // Find maximum in history table for scaling each bar
    for(int i = 1; i < NUMBANDS; i++) {
        mmax = 1.0;
        for(int j = 0; j < FQ_HIST; j++) {
            if(mmax < freqBandsHistory[j][i]) mmax = freqBandsHistory[j][i];
        }
        freqBands[i] /= mmax;
    }

    now = millis();

    if(startFlag) {

        if(now - lastStart < startDelay) {
            if(!initFlag) {
                for(int i = 0; i < DISPLAYBANDS; i++) {
                    peaks[i] = 0;
                    newPeak[i] = now;
                    peakTimer[i] = PEAK_HOLD;
                    oldHeight[i] = 1;
                    if(initDisplay) {
                        sid.drawBarWithHeight(i, 1);
                    }
                }
                if(initDisplay) {
                    sid.show();
                }
                initFlag = true;
            }
        } else {
            startFlag = false;
            histIdx = 0;
            for(int i = 0; i < FQ_HIST; i++) {
                for(int j = 1; j < NUMBANDS; j++) {
                    freqBandsHistory[i][j] = 0.0;
                }
            }
        }

    } else {

        // Calculate bar heights
        for(int i = 0; i < DISPLAYBANDS; i++) {
            int height = (int)(freqBands[i+1] * (FTYPE)(LEDS_PER_BAR - 1));
    
            if(ampFact != 100) {
                if(!height) height = 1;
                height = height * ampFact / 100;
                if(height > maxTTHeight[i]) height = maxTTHeight[i];
            }
    
            if(height > LEDS_PER_BAR) height = LEDS_PER_BAR;
            if(!height) height = 1;
      
            // Smoothen jumps in downward direction
            if(height < oldHeight[i]) {
                if(oldHeight[i] - height > 10) height = (oldHeight[i] + height) / 2;
                else                           height = oldHeight[i] - 1;
            }
    
            // Now do peak
            if(height - 1 > peaks[i]) {
                peaks[i] = min(LEDS_PER_BAR - 1, height - 1);
                newPeak[i] = now;
                peakTimer[i] = PEAK_HOLD;
            }
        
            oldHeight[i] = height;

            // Draw bars & peaks
            sid.drawBarWithHeight(i, oldHeight[i]);
            if(doPeaks && peaks[i] > oldHeight[i] - 1) {
                sid.drawDot(i, peaks[i]);
            }
        }

        // Put result on display
        sid.show();
    }

    now = millis();

    // Make peaks fall down
    for(int i = 0; i < DISPLAYBANDS; i++) {
        if(newPeak[i] && now - newPeak[i] > peakTimer[i]) {
            if(peaks[i] > 0) {
                peakTimer[i] = PEAK_FALL;
                newPeak[i] = now;
                peaks[i]--;
            } else {
                newPeak[i] = 0;
            }
        }
    }

    #endif
}
