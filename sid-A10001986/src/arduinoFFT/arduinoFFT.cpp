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

#include "arduinoFFT.h"

arduinoFFT::arduinoFFT(void)
{
}

arduinoFFT::arduinoFFT(FTYPE *vReal, FTYPE *vImag, uint16_t samples, FTYPE samplingFrequency)
{
    this->_vReal = vReal;
    this->_vImag = vImag;
    this->_samples = samples;
    this->_samplingFrequency = samplingFrequency;
    this->_power = Exponent(samples);
}

arduinoFFT::~arduinoFFT(void)
{
}

void arduinoFFT::Compute(FTYPE *vReal, FTYPE *vImag, uint16_t samples, FFTDirection dir)
{
    Compute(vReal, vImag, samples, Exponent(samples), dir);
}

void arduinoFFT::Compute(FFTDirection dir)
{
    // Computes in-place complex-to-complex FFT /
    // Reverse bits /
    uint16_t j = 0;
    for(uint16_t i = 0; i < (this->_samples - 1); i++) {
        if (i < j) {
            Swap(&this->_vReal[i], &this->_vReal[j]);
            if (dir == FFT_REVERSE)
                Swap(&this->_vImag[i], &this->_vImag[j]);
        }
        uint16_t k = (this->_samples >> 1);
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    // Compute the FFT
    FTYPE c1 = -1.0f;
    FTYPE c2 = 0.0f;
    uint16_t l2 = 1;
    for (uint8_t l = 0; (l < this->_power); l++) {
        uint16_t l1 = l2;
        l2 <<= 1;
        FTYPE u1 = 1.0f;
        FTYPE u2 = 0.0f;
        for (j = 0; j < l1; j++) {
            for (uint16_t i = j; i < this->_samples; i += l2) {
                uint16_t i1 = i + l1;
                FTYPE t1 = u1 * this->_vReal[i1] - u2 * this->_vImag[i1];
                FTYPE t2 = u1 * this->_vImag[i1] + u2 * this->_vReal[i1];
                this->_vReal[i1] = this->_vReal[i] - t1;
                this->_vImag[i1] = this->_vImag[i] - t2;
                this->_vReal[i] += t1;
                this->_vImag[i] += t2;
            }
            FTYPE z = ((u1 * c1) - (u2 * c2));
            u2 = ((u1 * c2) + (u2 * c1));
            u1 = z;
        }

        c2 = FFT_SQRT((1.0f - c1) / 2.0f);
        c1 = FFT_SQRT((1.0f + c1) / 2.0f);

        if (dir == FFT_FORWARD) {
            c2 = -c2;
        }
    }

    // Scaling for reverse transform /
    if (dir != FFT_FORWARD) {
        for (uint16_t i = 0; i < this->_samples; i++) {
            this->_vReal[i] /= this->_samples;
            this->_vImag[i] /= this->_samples;
        }
    }
}

void arduinoFFT::Compute(FTYPE *vReal, FTYPE *vImag, uint16_t samples, uint8_t power, FFTDirection dir)
{
    // Computes in-place complex-to-complex FFT
    // Reverse bits

    uint16_t j = 0;
    for (uint16_t i = 0; i < (samples - 1); i++) {
        if (i < j) {
            Swap(&vReal[i], &vReal[j]);
            if (dir == FFT_REVERSE)
                Swap(&vImag[i], &vImag[j]);
        }
        uint16_t k = (samples >> 1);
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    // Compute the FFT

    FTYPE c1 = -1.0f;
    FTYPE c2 = 0.0f;
    uint16_t l2 = 1;
    for (uint8_t l = 0; (l < power); l++) {
        uint16_t l1 = l2;
        l2 <<= 1;
        FTYPE u1 = 1.0f;
        FTYPE u2 = 0.0f;
        for (j = 0; j < l1; j++) {
            for (uint16_t i = j; i < samples; i += l2) {
                uint16_t i1 = i + l1;
                FTYPE t1 = u1 * vReal[i1] - u2 * vImag[i1];
                FTYPE t2 = u1 * vImag[i1] + u2 * vReal[i1];
                vReal[i1] = vReal[i] - t1;
                vImag[i1] = vImag[i] - t2;
                vReal[i] += t1;
                vImag[i] += t2;
            }
            FTYPE z = ((u1 * c1) - (u2 * c2));
            u2 = ((u1 * c2) + (u2 * c1));
            u1 = z;
        }

        c2 = FFT_SQRT((1.0f - c1) / 2.0f);
        c1 = FFT_SQRT((1.0f + c1) / 2.0f);

        if (dir == FFT_FORWARD) {
            c2 = -c2;
        }
    }

    // Scaling for reverse transform
    if (dir != FFT_FORWARD) {
        for (uint16_t i = 0; i < samples; i++) {
            vReal[i] /= samples;
            vImag[i] /= samples;
        }
    }
}

void arduinoFFT::ComplexToMagnitude()
{
    // vM is half the size of vReal and vImag
    for (uint16_t i = 0; i < this->_samples; i++) {
        this->_vReal[i] = FFT_SQRT(sq(this->_vReal[i]) + sq(this->_vImag[i]));
    }
}

void arduinoFFT::ComplexToMagnitude(FTYPE *vReal, FTYPE *vImag, uint16_t samples)
{
    // vM is half the size of vReal and vImag
    for (uint16_t i = 0; i < samples; i++) {
        vReal[i] = FFT_SQRT(sq(vReal[i]) + sq(vImag[i]));
    }
}

void arduinoFFT::DCRemoval()
{
    // calculate the mean of vData
    FTYPE mean = 0.0f;
    for (uint16_t i = 0; i < this->_samples; i++) {
        mean += this->_vReal[i];
    }

    mean /= this->_samples;

    // Subtract the mean from vData
    for (uint16_t i = 0; i < this->_samples; i++) {
        this->_vReal[i] -= mean;
    }
}

void arduinoFFT::DCRemoval(FTYPE *vData, uint16_t samples)
{
    // calculate the mean of vData
    FTYPE mean = 0.0f;
    for (uint16_t i = 0; i < samples; i++) {
        mean += vData[i];
    }

    mean /= (float)samples;

    // Subtract the mean from vData
    for (uint16_t i = 0; i < samples; i++) {
        vData[i] -= mean;
    }
}

#ifdef INCL_WINDOWING
void arduinoFFT::Windowing(FFTWindow windowType, FFTDirection dir)
{
    // Weighing factors are computed once before multiple use of FFT
    // The weighing function is symmetric; half the weighs are recorded
    FTYPE samplesMinusOne = (FTYPE(this->_samples) - 1.0f);

    for (uint16_t i = 0; i < (this->_samples >> 1); i++) {
        FTYPE indexMinusOne = FTYPE(i);
        FTYPE ratio = (indexMinusOne / samplesMinusOne);
        FTYPE weighingFactor = 1.0f;
        // Compute and record weighting factor
        switch (windowType) {
        case FFT_WIN_TYP_RECTANGLE:
            weighingFactor = 1.0f;
            break;
        case FFT_WIN_TYP_HAMMING:
            weighingFactor = 0.54f - (0.46f * FFT_COS(twoPi * ratio));
            break;
        case FFT_WIN_TYP_HANN:
            weighingFactor = 0.54f * (1.0f - FFT_COS(twoPi * ratio));
            break;
        case FFT_WIN_TYP_TRIANGLE:
            weighingFactor = 1.0f - ((2.0f * fabs(indexMinusOne - (samplesMinusOne / 2.0f))) / samplesMinusOne);
            break;
        case FFT_WIN_TYP_NUTTALL:
            weighingFactor = 0.355768f - (0.487396f * (FFT_COS(twoPi * ratio))) +
                             (0.144232f * (FFT_COS(fourPi * ratio))) -
                             (0.012604f * (FFT_COS(sixPi * ratio)));
            break;
        case FFT_WIN_TYP_BLACKMAN:
            weighingFactor = 0.42323f - (0.49755f * (FFT_COS(twoPi * ratio))) +
                             (0.07922f * (FFT_COS(fourPi * ratio)));
            break;
        case FFT_WIN_TYP_BLACKMAN_NUTTALL:
            weighingFactor = 0.3635819f - (0.4891775f * (FFT_COS(twoPi * ratio))) +
                             (0.1365995f * (FFT_COS(fourPi * ratio))) -
                             (0.0106411f * (FFT_COS(sixPi * ratio)));
            break;
        case FFT_WIN_TYP_BLACKMAN_HARRIS: // blackman harris
            weighingFactor = 0.35875f - (0.48829f * (FFT_COS(twoPi * ratio))) +
                             (0.14128f * (FFT_COS(fourPi * ratio))) -
                             (0.01168f * (FFT_COS(sixPi * ratio)));
            break;
        case FFT_WIN_TYP_FLT_TOP: // flat top
            weighingFactor = 0.2810639f - (0.5208972f * FFT_COS(twoPi * ratio)) +
                             (0.1980399f * FFT_COS(fourPi * ratio));
            break;
        case FFT_WIN_TYP_WELCH: // welch
            weighingFactor = 1.0f - sq((indexMinusOne - samplesMinusOne / 2.0f) / (samplesMinusOne / 2.0f));
            break;
        }
        if (dir == FFT_FORWARD) {
            this->_vReal[i] *= weighingFactor;
            this->_vReal[this->_samples - (i + 1)] *= weighingFactor;
        } else {
            this->_vReal[i] /= weighingFactor;
            this->_vReal[this->_samples - (i + 1)] /= weighingFactor;
        }
    }
}

void arduinoFFT::Windowing(FTYPE *vData, uint16_t samples, FFTWindow windowType, FFTDirection dir)
{
    // Weighing factors are computed once before multiple use of FFT
    // The weighing function is symetric; half the weighs are recorded

    FTYPE samplesMinusOne = (FTYPE(samples) - 1.0f);
    for (uint16_t i = 0; i < (samples >> 1); i++) {
        FTYPE indexMinusOne = FTYPE(i);
        FTYPE ratio = (indexMinusOne / samplesMinusOne);
        FTYPE weighingFactor = 1.0f;
        // Compute and record weighting factor
        switch (windowType) {
        case FFT_WIN_TYP_RECTANGLE: // rectangle (box car)
            weighingFactor = 1.0f;
            break;
        case FFT_WIN_TYP_HAMMING: // hamming
            weighingFactor = 0.54f - (0.46f * FFT_COS(twoPi * ratio));
            break;
        case FFT_WIN_TYP_HANN: // hann
            weighingFactor = 0.54f * (1.0f - FFT_COS(twoPi * ratio));
            break;
        case FFT_WIN_TYP_TRIANGLE: // triangle (Bartlett)
            weighingFactor = 1.0f - ((2.0f * fabs(indexMinusOne - (samplesMinusOne / 2.0f))) / samplesMinusOne);
            break;
        case FFT_WIN_TYP_NUTTALL: // nuttall
            weighingFactor = 0.355768f - (0.487396f * (FFT_COS(twoPi * ratio))) +
                             (0.144232f * (FFT_COS(fourPi * ratio))) -
                             (0.012604f * (FFT_COS(sixPi * ratio)));
            break;
        case FFT_WIN_TYP_BLACKMAN: // blackman
            weighingFactor = 0.42323f - (0.49755f * (FFT_COS(twoPi * ratio))) +
                             (0.07922f * (FFT_COS(fourPi * ratio)));
            break;
        case FFT_WIN_TYP_BLACKMAN_NUTTALL: // blackman nuttall
            weighingFactor = 0.3635819f - (0.4891775f * (FFT_COS(twoPi * ratio))) +
                             (0.1365995f * (FFT_COS(fourPi * ratio))) -
                             (0.0106411f * (FFT_COS(sixPi * ratio)));
            break;
        case FFT_WIN_TYP_BLACKMAN_HARRIS: // blackman harris
            weighingFactor = 0.35875f - (0.48829f * (FFT_COS(twoPi * ratio))) +
                             (0.14128f * (FFT_COS(fourPi * ratio))) -
                             (0.01168f * (FFT_COS(sixPi * ratio)));
            break;
        case FFT_WIN_TYP_FLT_TOP: // flat top
            weighingFactor = 0.2810639f - (0.5208972f * FFT_COS(twoPi * ratio)) +
                             (0.1980399f * FFT_COS(fourPi * ratio));
            break;
        case FFT_WIN_TYP_WELCH:
            weighingFactor = 1.0f - sq((indexMinusOne - samplesMinusOne / 2.0f) / (samplesMinusOne / 2.0f));
            break;
        }
        if (dir == FFT_FORWARD) {
            vData[i] *= weighingFactor;
            vData[samples - (i + 1)] *= weighingFactor;
        } else {
            vData[i] /= weighingFactor;
            vData[samples - (i + 1)] /= weighingFactor;
        }
    }
}
#endif // INCL_WINDOWING

#ifdef INCL_MAJORPEAK
FTYPE arduinoFFT::MajorPeak()
{
    FTYPE maxY = 0.0f;
    uint16_t IndexOfMaxY = 0;

    // If sampling_frequency = 2 * max_frequency in signal,
    // value would be stored at position samples/2
    for (uint16_t i = 1; i < ((this->_samples >> 1) + 1); i++) {
        if ((this->_vReal[i - 1] < this->_vReal[i]) && (this->_vReal[i] > this->_vReal[i + 1])) {
            if (this->_vReal[i] > maxY) {
                maxY = this->_vReal[i];
                IndexOfMaxY = i;
            }
        }
    }

    FTYPE delta = 0.5f *
          ((this->_vReal[IndexOfMaxY - 1] - this->_vReal[IndexOfMaxY + 1]) /
           (this->_vReal[IndexOfMaxY - 1] - (2.0f * this->_vReal[IndexOfMaxY]) +
            this->_vReal[IndexOfMaxY + 1]));

    FTYPE interpolatedX = ((IndexOfMaxY + delta) * this->_samplingFrequency) / (this->_samples - 1);

    if (IndexOfMaxY ==
        (this->_samples >> 1)) // To improve calculation on edge values

    interpolatedX = ((IndexOfMaxY + delta) * this->_samplingFrequency) / (this->_samples);

    // returned value: interpolated frequency peak apex
    return (interpolatedX);
}

void arduinoFFT::MajorPeak(FTYPE *f, FTYPE *v)
{
    FTYPE maxY = 0.0f;
    uint16_t IndexOfMaxY = 0;

    // If sampling_frequency = 2 * max_frequency in signal,
    // value would be stored at position samples/2
    for (uint16_t i = 1; i < ((this->_samples >> 1) + 1); i++) {
        if ((this->_vReal[i - 1] < this->_vReal[i]) && (this->_vReal[i] > this->_vReal[i + 1])) {
            if (this->_vReal[i] > maxY) {
                maxY = this->_vReal[i];
                IndexOfMaxY = i;
            }
        }
    }

    FTYPE delta = 0.5f *
                    ((this->_vReal[IndexOfMaxY - 1] - this->_vReal[IndexOfMaxY + 1]) /
                     (this->_vReal[IndexOfMaxY - 1] - (2.0f * this->_vReal[IndexOfMaxY]) +
                      this->_vReal[IndexOfMaxY + 1]));

    FTYPE interpolatedX = ((IndexOfMaxY + delta) * this->_samplingFrequency) / (this->_samples - 1);

    if (IndexOfMaxY ==
        (this->_samples >> 1)) // To improve calculation on edge values

    interpolatedX = ((IndexOfMaxY + delta) * this->_samplingFrequency) / (this->_samples);

    // returned value: interpolated frequency peak apex
    *f = interpolatedX;
    *v = fabs(this->_vReal[IndexOfMaxY - 1] - (2.0f * this->_vReal[IndexOfMaxY]) + this->_vReal[IndexOfMaxY + 1]);
}

FTYPE arduinoFFT::MajorPeak(FTYPE *vD, uint16_t samples, FTYPE samplingFrequency)
{
    FTYPE maxY = 0.0f;
    uint16_t IndexOfMaxY = 0;

    // If sampling_frequency = 2 * max_frequency in signal,
    // value would be stored at position samples/2
    for (uint16_t i = 1; i < ((samples >> 1) + 1); i++) {
        if ((vD[i - 1] < vD[i]) && (vD[i] > vD[i + 1])) {
            if (vD[i] > maxY) {
                maxY = vD[i];
                IndexOfMaxY = i;
            }
        }
    }
    FTYPE delta = 0.5f * ((vD[IndexOfMaxY - 1] - vD[IndexOfMaxY + 1]) /
                         (vD[IndexOfMaxY - 1] - (2.0f * vD[IndexOfMaxY]) + vD[IndexOfMaxY + 1]));

    FTYPE interpolatedX = ((IndexOfMaxY + delta) * samplingFrequency) / (samples - 1);

    if (IndexOfMaxY == (samples >> 1)) // To improve calculation on edge values
        interpolatedX = ((IndexOfMaxY + delta) * samplingFrequency) / (samples);

    // returned value: interpolated frequency peak apex
    return (interpolatedX);
}

void arduinoFFT::MajorPeak(FTYPE *vD, uint16_t samples, FTYPE samplingFrequency, FTYPE *f, FTYPE *v)
{
    FTYPE maxY = 0.0f;
    uint16_t IndexOfMaxY = 0;

    // If sampling_frequency = 2 * max_frequency in signal,
    // value would be stored at position samples/2
    for (uint16_t i = 1; i < ((samples >> 1) + 1); i++) {
        if ((vD[i - 1] < vD[i]) && (vD[i] > vD[i + 1])) {
            if (vD[i] > maxY) {
                maxY = vD[i];
                IndexOfMaxY = i;
            }
        }
    }

    FTYPE delta = 0.5f *
                    ((vD[IndexOfMaxY - 1] - vD[IndexOfMaxY + 1]) /
                     (vD[IndexOfMaxY - 1] - (2.0f * vD[IndexOfMaxY]) + vD[IndexOfMaxY + 1]));

    FTYPE interpolatedX = ((IndexOfMaxY + delta) * samplingFrequency) / (samples - 1);

    if (IndexOfMaxY == (samples >> 1)) // To improve calculation on edge values
        interpolatedX = ((IndexOfMaxY + delta) * samplingFrequency) / (samples);

    // returned value: interpolated frequency peak apex
    *f = interpolatedX;
    *v = fabs(vD[IndexOfMaxY - 1] - (2.0f * vD[IndexOfMaxY]) + vD[IndexOfMaxY + 1]);
}

FTYPE arduinoFFT::MajorPeakParabola()
{
    FTYPE maxY = 0.0f;
    uint16_t IndexOfMaxY = 0;

    // If sampling_frequency = 2 * max_frequency in signal,
    // value would be stored at position samples/2
    for (uint16_t i = 1; i < ((this->_samples >> 1) + 1); i++) {
        if ((this->_vReal[i - 1] < this->_vReal[i]) && (this->_vReal[i] > this->_vReal[i + 1])) {
            if (this->_vReal[i] > maxY) {
                maxY = this->_vReal[i];
                IndexOfMaxY = i;
            }
        }
    }

    FTYPE freq = 0.0f;
    if (IndexOfMaxY > 0) {
        // Assume the three points to be on a parabola
        FTYPE a, b, c;
        Parabola(IndexOfMaxY - 1, this->_vReal[IndexOfMaxY - 1], IndexOfMaxY,
                 this->_vReal[IndexOfMaxY], IndexOfMaxY + 1,
                 this->_vReal[IndexOfMaxY + 1], &a, &b, &c);

        // Peak is at the middle of the parabola
        FTYPE x = -b / (2.0f * a);

        // And magnitude is at the extrema of the parabola if you want It...
        // FTYPE y = a*x*x+b*x+c;

        // Convert to frequency
        freq = (x * this->_samplingFrequency) / (this->_samples);
    }

    return freq;
}

void arduinoFFT::Parabola(FTYPE x1, FTYPE y1, FTYPE x2, FTYPE y2, FTYPE x3,
                          FTYPE y3, FTYPE *a, FTYPE *b, FTYPE *c)
{
    FTYPE reversed_denom = 1.0f / ((x1 - x2) * (x1 - x3) * (x2 - x3));

    *a = (x3 * (y2 - y1) + x2 * (y1 - y3) + x1 * (y3 - y2)) * reversed_denom;
    *b = (x3 * x3 * (y1 - y2) + x2 * x2 * (y3 - y1) + x1 * x1 * (y2 - y3)) * reversed_denom;
    *c = (x2 * x3 * (x2 - x3) * y1 + x3 * x1 * (x3 - x1) * y2 + x1 * x2 * (x1 - x2) * y3) * reversed_denom;
}
#endif // INCL_MAJORPEAK


uint8_t arduinoFFT::Exponent(uint16_t value)
{
    // Calculates the base 2 logarithm of a value
    uint8_t result = 0;

    while (((value >> result) & 1) != 1) result++;

    return (result);
}

// Private functions

void arduinoFFT::Swap(FTYPE *x, FTYPE *y)
{
    FTYPE temp = *x;
    *x = *y;
    *y = temp;
}

