/*

	FFT library
	Copyright (C) 2010 Didier Longueville
	Copyright (C) 2014 Enrique Condes

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	Adapted by Thomas Winischhofer (A10001986) in 2023/2025

*/

#ifndef ArduinoFFT_h
#define ArduinoFFT_h

//#define INCL_WINDOWING
//#define INCL_MAJORPEAK

//#define FFT_DOUBLE

#ifdef FFT_DOUBLE
#define FTYPE double
#define FFT_COS cos
#define FFT_SQRT sqrt
#error "Literals are marked float"
#else
#define FTYPE float
#define FFT_COS cosf
#define FFT_SQRT sqrtf
#endif

#include "Arduino.h"

enum class FFTDirection { Reverse, Forward };

enum class FFTWindow {
    Rectangle,        // (Box car)
    Hamming,
    Hann,
    Triangle,         // (Bartlett)
    Nuttall,
    Blackman,
    Blackman_Nuttall,
    Blackman_Harris,
    Flat_top,
    Welch
};

/* Custom constants */
#define FFT_FORWARD FFTDirection::Forward
#define FFT_REVERSE FFTDirection::Reverse

/* Windowing type */
#define FFT_WIN_TYP_RECTANGLE        FFTWindow::Rectangle
#define FFT_WIN_TYP_HAMMING          FFTWindow::Hamming
#define FFT_WIN_TYP_HANN             FFTWindow::Hann
#define FFT_WIN_TYP_TRIANGLE         FFTWindow::Triangle
#define FFT_WIN_TYP_NUTTALL          FFTWindow::Nuttall
#define FFT_WIN_TYP_BLACKMAN         FFTWindow::Blackman
#define FFT_WIN_TYP_BLACKMAN_NUTTALL FFTWindow::Blackman_Nuttall
#define FFT_WIN_TYP_BLACKMAN_HARRIS  FFTWindow::Blackman_Harris
#define FFT_WIN_TYP_FLT_TOP          FFTWindow::Flat_top
#define FFT_WIN_TYP_WELCH            FFTWindow::Welch

/* Mathematial constants */
#define twoPi   6.28318531f
#define fourPi 12.56637061f
#define sixPi  18.84955593f

class arduinoFFT {
    public:
        /* Constructors */
        arduinoFFT(void);
        arduinoFFT(FTYPE *vReal, FTYPE *vImag, uint16_t samples, FTYPE samplingFrequency);

        /* Destructor */
        ~arduinoFFT(void);

        /* Functions */
        uint8_t Exponent(uint16_t value);

        void  ComplexToMagnitude();
        void  ComplexToMagnitude(FTYPE *vReal, FTYPE *vImag, uint16_t samples);

        void  Compute(FFTDirection dir);
        void  Compute(FTYPE *vReal, FTYPE *vImag, uint16_t samples, FFTDirection dir);
        void  Compute(FTYPE *vReal, FTYPE *vImag, uint16_t samples, uint8_t power, FFTDirection dir);

        void  DCRemoval();
        void  DCRemoval(FTYPE *vData, uint16_t samples);

        #ifdef INCL_WINDOWING
        void  Windowing(FTYPE *vData, uint16_t samples, FFTWindow windowType, FFTDirection dir);
        void  Windowing(FFTWindow windowType, FFTDirection dir);
        #endif

        #ifdef INCL_MAJORPEAK
        FTYPE MajorPeak();
        void  MajorPeak(FTYPE *f, FTYPE *v);
        FTYPE MajorPeak(FTYPE *vD, uint16_t samples, FTYPE samplingFrequency);
        void  MajorPeak(FTYPE *vD, uint16_t samples, FTYPE samplingFrequency, FTYPE *f, FTYPE *v);
        FTYPE MajorPeakParabola();
        #endif

    private:
        /* Variables */
        uint16_t  _samples;
        FTYPE     _samplingFrequency;
        FTYPE *   _vReal;
        FTYPE *   _vImag;
        uint8_t   _power;

        /* Functions */
        void Swap(FTYPE *x, FTYPE *y);

        #ifdef INCL_MAJORPEAK
        void Parabola(FTYPE x1, FTYPE y1, FTYPE x2, FTYPE y2, FTYPE x3, FTYPE y3, FTYPE *a, FTYPE *b, FTYPE *c);
        #endif
};

#endif

