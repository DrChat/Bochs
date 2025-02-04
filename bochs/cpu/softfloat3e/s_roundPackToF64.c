/*============================================================================

This C source file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3e, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014, 2015, 2017 The Regents of the University of
California.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions, and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions, and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. Neither the name of the University nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS", AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE
DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

#include <stdbool.h>
#include <stdint.h>
#include "internals.h"
#include "softfloat.h"

float64_t
 softfloat_roundPackToF64(bool sign, int16_t exp, uint64_t sig, struct softfloat_status_t *status)
{
    uint8_t roundingMode;
    bool roundNearEven;
    uint16_t roundIncrement, roundBits;
    bool isTiny;
    uint64_t uiZ;

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    roundingMode = softfloat_getRoundingMode(status);
    roundNearEven = (roundingMode == softfloat_round_near_even);
    roundIncrement = 0x200;
    if (! roundNearEven && (roundingMode != softfloat_round_near_maxMag)) {
        roundIncrement =
            (roundingMode == (sign ? softfloat_round_min : softfloat_round_max)) ? 0x3FF : 0;
    }
    roundBits = sig & 0x3FF;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if (0x7FD <= (uint16_t) exp) {
        if (exp < 0) {
            /*----------------------------------------------------------------
            *----------------------------------------------------------------*/
            isTiny = (exp < -1) || (sig + roundIncrement < UINT64_C(0x8000000000000000));
            if (isTiny && ! softfloat_isMaskedException(status, softfloat_flag_underflow)) {
                softfloat_raiseFlags(status, softfloat_flag_underflow);
                exp += 1536;
            }
            else {
                sig = softfloat_shiftRightJam64(sig, -exp);
                exp = 0;
                roundBits = sig & 0x3FF;
                if (isTiny) {
                    if (softfloat_flushUnderflowToZero(status)) {
                        softfloat_raiseFlags(status, softfloat_flag_underflow | softfloat_flag_inexact);
                        return packToF64UI(sign, 0, 0);
                    }
                    if (roundBits) softfloat_raiseFlags(status, softfloat_flag_underflow);
                }
            }
        } else if ((0x7FD < exp) || (UINT64_C(0x8000000000000000) <= sig + roundIncrement)) {
            /*----------------------------------------------------------------
            *----------------------------------------------------------------*/
            softfloat_raiseFlags(status, softfloat_flag_overflow);
            if (roundBits || softfloat_isMaskedException(status, softfloat_flag_overflow)) {
                softfloat_raiseFlags(status, softfloat_flag_inexact);
            }
            uiZ = packToF64UI(sign, 0x7FF, 0) - ! roundIncrement;
            return uiZ;
        }
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    sig = (sig + roundIncrement)>>10;
    if (roundBits) {
        softfloat_raiseFlags(status, softfloat_flag_inexact);
    }
    sig &= ~(uint64_t) (! (roundBits ^ 0x200) & roundNearEven);
    if (! sig) exp = 0;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 packReturn:
    return packToF64UI(sign, exp, sig);
}
