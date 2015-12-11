/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

 
#ifndef __MANuVR_AUDIO_H__
#define __MANuVR_AUDIO_H__


#if defined(__MK20DX256__) | defined(__MK20DX128__) | defined(STM32F4XX)
// computes limit((val >> rshift), 2**bits)
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift) __attribute__((always_inline, unused));
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift)
{
	int32_t out;
	asm volatile("ssat %0, %1, %2, asr %3" : "=r" (out) : "I" (bits), "r" (val), "I" (rshift));
	return out;
}

// computes ((a[31:0] * b[15:0]) >> 16)
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulwb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[31:0] * b[31:16]) >> 16)
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulwt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (((int64_t)a[31:0] * (int64_t)b[31:0]) >> 32)
static inline int32_t multiply_32x32_rshift32(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_32x32_rshift32(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("smmul %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
static inline int32_t multiply_32x32_rshift32_rounded(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_32x32_rshift32_rounded(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("smmulr %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes sum + (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
static inline int32_t multiply_accumulate_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_accumulate_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("smmlar %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
}

// computes sum - (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
static inline int32_t multiply_subtract_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_subtract_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("smmlsr %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
}


// computes (a[31:16] | (b[31:16] >> 16))
static inline uint32_t pack_16t_16t(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16t_16t(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("pkhtb %0, %1, %2, asr #16" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (a[31:16] | b[15:0])
static inline uint32_t pack_16t_16b(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16t_16b(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("pkhtb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[15:0] << 16) | b[15:0])
static inline uint32_t pack_16b_16b(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16b_16b(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("pkhbt %0, %1, %2, lsl #16" : "=r" (out) : "r" (b), "r" (a));
	return out;
}

// computes ((a[15:0] << 16) | b[15:0])
static inline uint32_t pack_16x16(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16x16(int32_t a, int32_t b)
{
	int32_t out;
	asm volatile("pkhbt %0, %1, %2, lsl #16" : "=r" (out) : "r" (b), "r" (a));
	return out;
}

// computes (((a[31:16] + b[31:16]) << 16) | (a[15:0 + b[15:0]))
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("qadd16 %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (sum + ((a[31:0] * b[15:0]) >> 16))
static inline int32_t signed_multiply_accumulate_32x16b(int32_t sum, int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_accumulate_32x16b(int32_t sum, int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smlawb %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
}

// computes (sum + ((a[31:0] * b[31:16]) >> 16))
static inline int32_t signed_multiply_accumulate_32x16t(int32_t sum, int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_accumulate_32x16t(int32_t sum, int32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smlawt %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
	return out;
}

// computes logical and, forces compiler to allocate register and use single cycle instruction
static inline uint32_t logical_and(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline uint32_t logical_and(uint32_t a, uint32_t b)
{
	asm volatile("and %0, %1" : "+r" (a) : "r" (b));
	return a;
}

// computes ((a[15:0] * b[15:0]) + (a[31:16] * b[31:16]))
static inline int32_t multiply_16tx16t_add_16bx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16t_add_16bx16b(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smuad %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[15:0] * b[31:16]) + (a[31:16] * b[15:0]))
static inline int32_t multiply_16tx16b_add_16bx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16b_add_16bx16t(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smuadx %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[15:0] * b[15:0])
static inline int32_t multiply_16bx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16bx16b(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulbb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[15:0] * b[31:16])
static inline int32_t multiply_16bx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16bx16t(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smulbt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[31:16] * b[15:0])
static inline int32_t multiply_16tx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16b(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smultb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes ((a[31:16] * b[31:16])
static inline int32_t multiply_16tx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16t(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("smultt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (a - b), result saturated to 32 bit integer range
static inline int32_t substract_32_saturate(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t substract_32_saturate(uint32_t a, uint32_t b)
{
	int32_t out;
	asm volatile("qsub %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}



#elif defined(__PIC32MX2XX__) | defined(__PIC32MX3XX__) | defined(__PIC32MZ__)
/*
* I have ported these fxns to MIPS, and am integrating them into ManuvrOS for the
*   sake of benchmarking. 
*
* It would probably be better to re-code the client code that uses these fxns to take
*   better advantage of MIPS pipelining rather than try to emulate a split-word register
*   organization, as is the goal of these ARM-specific DSP instructions.
*                               ---J. Ian Lindsay   Sat Jul 04 23:58:16 MST 2015
*/

// computes limit((val >> rshift), 2**bits)
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift) __attribute__((always_inline, unused));
static inline int32_t signed_saturate_rshift(int32_t val, int bits, int rshift) {
	//asm volatile("ssat %0, %1, %2, asr %3" : "=r" (out) : "I" (bits), "r" (val), "I" (rshift));
	int32_t out = (val >> rshift);
	uint32_t pow = (1 << bits) - 1;
	return ((((int32_t)pow) > out) ? out : pow);
}

// computes ((a[31:0] * b[15:0]) >> 16)
// Finished.
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_32x16b(int32_t a, uint32_t b)
{
  int32_t out;
  asm volatile("sll %0, %2, 0x10 \n \
    mult %1, %0  \n \
    mfhi %0" : "=r" (out) : "r" (a), "r" (b));
  return out;
}

// computes ((a[31:0] * b[31:16]) >> 16)
// Finished.
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_32x16t(int32_t a, uint32_t b)
{
  int32_t out;
  uint32_t mask_value = 0xFFFF0000;
  asm volatile("and %0, %2, %3 \n \
    mult %1, %0  \n \
    mfhi %0" : "=r" (out) : "r" (a), "r" (b), "r" (mask_value));
  return out;
}

// computes (((int64_t)a[31:0] * (int64_t)b[31:0]) >> 32)
// Finished.
static inline int32_t multiply_32x32_rshift32(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_32x32_rshift32(int32_t a, int32_t b) {
  int32_t out;
  asm volatile("mult %1, %2\n mfhi %0" : "=r" (out) : "r" (a), "r" (b));
  return out;
}

// computes (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
// Note: For PIC32, we are better-off looking at it this way...
//        (0x8000000 + ((int64_t)a[31:0] * (int64_t)b[31:0]) >> 32)
// Finished.
static inline int32_t multiply_32x32_rshift32_rounded(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_32x32_rshift32_rounded(int32_t a, int32_t b) {
  int32_t out;
  uint32_t temp = 0;
  asm volatile("mthi %3 \n \
    lw %3, 0x80000000   \n \
    mtlo %3             \n \
    madd %1, %2         \n \
    mfhi %0" : "=r" (out) : "r" (a), "r" (b), "r" (temp));
  return out;
}

// computes sum + (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
static inline int32_t multiply_accumulate_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_accumulate_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b) {
  int32_t out;
  uint32_t temp = 0;
  asm volatile("mthi %3 \n \
    lw %3, 0x80000000   \n \
    mtlo %3             \n \
    madd %1, %2         \n \
    mfhi %3             \n \
    addu %0, %4, %3" : "=r" (out) : "r" (a), "r" (b), "r" (temp), "r" (sum));
  return out;
}

// computes sum - (((int64_t)a[31:0] * (int64_t)b[31:0] + 0x8000000) >> 32)
static inline int32_t multiply_subtract_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_subtract_32x32_rshift32_rounded(int32_t sum, int32_t a, int32_t b) {
  //asm volatile("smmlsr %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
  int32_t out;
  uint32_t temp = 0;
  asm volatile("mthi %3 \n \
    lw %3, 0x80000000   \n \
    mtlo %3             \n \
    madd %1, %2         \n \
    mfhi %3             \n \
    subu %0, %4, %3" : "=r" (out) : "r" (a), "r" (b), "r" (temp), "r" (sum));
  return out;
}


// computes (a[31:16] | (b[31:16] >> 16))
static inline uint32_t pack_16t_16t(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16t_16t(int32_t a, int32_t b) {
  //asm volatile("pkhtb %0, %1, %2, asr #16" : "=r" (out) : "r" (a), "r" (b));
  uint32_t out;
  uint32_t temp0;
  asm volatile("srl %3, %1, 0x10 \n \
    srl %0, %2, 0x10 \n \
    ins %0, %3, 0x10, 0x10" : "=r" (out) : "r" (a), "r" (b), "r" (temp0));
  return out;
}

// computes (a[31:16] | b[15:0])
// Finished.
static inline uint32_t pack_16t_16b(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16t_16b(int32_t a, int32_t b) {
  //asm volatile("pkhtb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
  uint32_t out;
  asm volatile("addi %0, %1, 0x00000000 \n \
    ins %0, %2, 0x00, 0x10" : "=r" (out) : "r" (a), "r" (b));
  return out;
}

// computes ((a[15:0] << 16) | b[15:0])
// Finished.
static inline uint32_t pack_16b_16b(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16b_16b(int32_t a, int32_t b) {
  //asm volatile("pkhbt %0, %1, %2, lsl #16" : "=r" (out) : "r" (b), "r" (a));
  uint32_t out;
  asm volatile("andi %0, %2, 0x0000ffff \n \
    ins %0, %1, 0x10, 0x10" : "=r" (out) : "r" (a), "r" (b));
  return out;
}

// computes ((a[15:0] << 16) | b[15:0])
// Finished. But why is it the same as the FXN above?
static inline uint32_t pack_16x16(int32_t a, int32_t b) __attribute__((always_inline, unused));
static inline uint32_t pack_16x16(int32_t a, int32_t b) {
  //asm volatile("pkhbt %0, %1, %2, lsl #16" : "=r" (out) : "r" (b), "r" (a));
  uint32_t out;
  asm volatile("andi %0, %2, 0x0000ffff \n \
    ins %0, %1, 0x10, 0x10" : "=r" (out) : "r" (a), "r" (b));
  return out;
}

// computes (((a[31:16] + b[31:16]) << 16) | (a[15:0 + b[15:0]))
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline uint32_t signed_add_16_and_16(uint32_t a, uint32_t b)
{
	int32_t out;
	//asm volatile("qadd16 %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

// computes (sum + ((a[31:0] * b[15:0]) >> 16))
static inline int32_t signed_multiply_accumulate_32x16b(int32_t sum, int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_accumulate_32x16b(int32_t sum, int32_t a, uint32_t b) {
  //asm volatile("smlawb %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
  int32_t out;
  uint32_t temp0 = 0;
  asm volatile("mthi %3 \n \
    mtlo %4             \n \
    sll %4, %2, 0x10    \n \
    madd %1, %4         \n \
    mfhi %0" : "=r" (out) : "r" (a), "r" (b), "r" (sum), "r" (temp0));
  return out;
}

// computes (sum + ((a[31:0] * b[31:16]) >> 16))
static inline int32_t signed_multiply_accumulate_32x16t(int32_t sum, int32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t signed_multiply_accumulate_32x16t(int32_t sum, int32_t a, uint32_t b) {
  //asm volatile("smlawt %0, %2, %3, %1" : "=r" (out) : "r" (sum), "r" (a), "r" (b));
  int32_t out;
  uint32_t temp0      = 0x00000000;
  uint32_t mask_value = 0xFFFF0000;
  asm volatile("mthi %3 \n \
    mtlo %4             \n \
    and %4, %2, %5      \n \
    madd %1, %4         \n \
    mfhi %0" : "=r" (out) : "r" (a), "r" (b), "r" (sum), "r" (temp0), "r" (mask_value));
  return out;
}

// computes logical AND.
// Finished
static inline uint32_t logical_and(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline uint32_t logical_and(uint32_t a, uint32_t b) {
  asm volatile("and %0, %1, %2" : "=r" (a), "+r" (a), "+r" (b));
  return a;
}

// computes ((a[15:0] * b[15:0]) + (a[31:16] * b[31:16]))
static inline int32_t multiply_16tx16t_add_16bx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16t_add_16bx16b(uint32_t a, uint32_t b) {
  //asm volatile("smuad %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
  int32_t out;
  uint32_t temp0;
  uint32_t temp1;
  uint32_t mask_value = 0xffff0000;
  asm volatile("sll %3, %1, 0x10 \n \
    sll %4, %2, 0x10             \n \
    mult %4, %3                  \n \
    and %3, %1, %5               \n \
    and %4, %2, %5               \n \
    madd %3, %4                  \n \
    mfhi %0" : "=r" (out) : "r" (a), "r" (b), "r" (temp0), "r" (temp1), "r" (mask_value));
  return out;
}

// computes ((a[15:0] * b[31:16]) + (a[31:16] * b[15:0]))
static inline int32_t multiply_16tx16b_add_16bx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16b_add_16bx16t(uint32_t a, uint32_t b) {
  //asm volatile("smuadx %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
  int32_t out;
  uint32_t temp0;
  uint32_t temp1;
  uint32_t mask_value = 0xffff0000;
  asm volatile("sll %3, %1, 0x10 \n \
    and %4, %2, %5               \n \
    mult %4, %3                  \n \
    and %3, %1, %5               \n \
    sll %4, %2, 0x10             \n \
    madd %3, %4                  \n \
    mfhi %0" : "=r" (out) : "r" (a), "r" (b), "r" (temp0), "r" (temp1), "r" (mask_value));
  return out;
}

// computes ((a[15:0] * b[15:0])
// Finished.
static inline int32_t multiply_16bx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16bx16b(uint32_t a, uint32_t b){
  //asm volatile("smulbb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
  int32_t out;
  uint32_t temp0;
  uint32_t temp1;
  asm volatile("sll %1, %2, 0x10 \n \
    sll %3, %4, 0x10 \n \
    mult %1, %3         \n \
    mfhi %0" : "=r" (out) : "r" (temp0), "r" (a), "r" (temp1), "r" (b));
  return out;
}

// computes ((a[15:0] * b[31:16])
// Finished.
static inline int32_t multiply_16bx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16bx16t(uint32_t a, uint32_t b) {
  //asm volatile("smulbt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
  int32_t out;
  uint32_t temp0;
  uint32_t temp1;
  uint32_t mask_value = 0xffff0000;
  asm volatile("sll %1, %2, 0x10 \n \
    and %3, %4, %5 \n \
    mult %1, %3         \n \
    mflo %0" : "=r" (out) : "r" (temp0), "r" (a), "r" (temp1), "r" (b), "r" (mask_value));
  return out;
}

// computes ((a[31:16] * b[15:0])
// Finished.
static inline int32_t multiply_16tx16b(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16b(uint32_t a, uint32_t b) {
  //asm volatile("smultb %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
  int32_t out;
  uint32_t temp0;
  uint32_t temp1;
  uint32_t mask_value = 0xffff0000;
  asm volatile("and %1, %2, %5 \n \
    sll %3, %4, 0x10 \n \
    mult %1, %3         \n \
    mfhi %0" : "=r" (out) : "r" (temp0), "r" (a), "r" (temp1), "r" (b), "r" (mask_value));
  return out;
}

// computes ((a[31:16] * b[31:16])
// Finished.
static inline int32_t multiply_16tx16t(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t multiply_16tx16t(uint32_t a, uint32_t b) {
  //asm volatile("smultt %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
  int32_t out;
  uint32_t temp0;
  uint32_t temp1;
  uint32_t mask_value = 0xffff0000;
  asm volatile("and %1, %2, %5 \n \
    and %3, %4, %5 \n \
    mult %1, %3         \n \
    mfhi %0" : "=r" (out) : "r" (temp0), "r" (a), "r" (temp1), "r" (b), "r" (mask_value));
  return out;
}

// computes (a - b), result saturated to 32 bit integer range
static inline int32_t substract_32_saturate(uint32_t a, uint32_t b) __attribute__((always_inline, unused));
static inline int32_t substract_32_saturate(uint32_t a, uint32_t b)
{
	int32_t out;
	//asm volatile("qsub %0, %1, %2" : "=r" (out) : "r" (a), "r" (b));
	return out;
}

#else  // Generics

#endif





class ManuvrAudio : public EventReceiver {
  public:

    ManuvrAudio();
    ~ManuvrAudio();

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrEvent*);
    int8_t callback_proc(ManuvrEvent *);
    void procDirectDebugInstruction(StringBuilder *);
    const char* getReceiverName();
    void printDebug(StringBuilder*);

    volatile static ManuvrAudio* INSTANCE;


  protected:
    int8_t bootComplete();


  private:

};

#endif  // __MANuVR_AUDIO_H__
