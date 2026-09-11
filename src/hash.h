// ================================================================================
// Copyright © 2026 William Jaarma
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ================================================================================

/**
 * @file hash.h
 *
 * Various hash and equals functions for most of the common C-types.
 *
 * @note Some of the functions in this file was AI generated
 */

#if !defined(HASH_H_)

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static inline size_t hash_combine(size_t lhs, size_t rhs) {
  lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
  return lhs;
}

/*
 * Integer hashing
 *
 * The mixer is based on splitmix64.  The result is truncated to size_t
 * when size_t is 32 bits, which is still a good hash.
 */
static inline size_t hash_u64(uint64_t x) {
  x += UINT64_C(0x9e3779b97f4a7c15);
  x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
  x ^= x >> 31;

#if SIZE_MAX > UINT32_MAX
  return (size_t)x;
#else
  return (size_t)(x ^ (x >> 32));
#endif
}

/* Built-in integer types */

static inline size_t hash_bool(_Bool x) { return hash_u64((uint64_t)x); }

static inline int equals_bool(_Bool x, _Bool y) { return x == y; }

static inline size_t hash_char(char x) {
  return hash_u64((uint64_t)(unsigned char)x);
}

static inline size_t hash_wchar(wchar_t x) { return hash_u64((uint64_t)x); }

static inline int equals_wchar(wchar_t x, wchar_t y) { return x == y; }

static inline int equals_char(char x, char y) { return x == y; }

static inline size_t hash_schar(signed char x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_schar(signed char x, signed char y) { return x == y; }

static inline size_t hash_uchar(unsigned char x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_uchar(unsigned char x, unsigned char y) {
  return x == y;
}

static inline size_t hash_short(short x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_short(short x, short y) { return x == y; }

static inline size_t hash_ushort(unsigned short x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_ushort(unsigned short x, unsigned short y) {
  return x == y;
}

static inline size_t hash_int(int x) { return hash_u64((uint64_t)(int64_t)x); }

static inline int equals_int(int x, int y) { return x == y; }

static inline size_t hash_uint(unsigned int x) { return hash_u64((uint64_t)x); }

static inline int equals_uint(unsigned int x, unsigned int y) { return x == y; }

static inline size_t hash_long(long x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_long(long x, long y) { return x == y; }

static inline size_t hash_ulong(unsigned long x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_ulong(unsigned long x, unsigned long y) {
  return x == y;
}

static inline size_t hash_llong(long long x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_llong(long long x, long long y) { return x == y; }

static inline size_t hash_ullong(unsigned long long x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_ullong(unsigned long long x, unsigned long long y) {
  return x == y;
}

/*
 * stdint.h
 *
 * These aliases are useful when the typedef names themselves are used
 * as the hash-table's key type.
 */

static inline size_t hash_int8(int8_t x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_int8(int8_t x, int8_t y) { return x == y; }

static inline size_t hash_uint8(uint8_t x) { return hash_u64((uint64_t)x); }

static inline int equals_uint8(uint8_t x, uint8_t y) { return x == y; }

static inline size_t hash_int16(int16_t x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_int16(int16_t x, int16_t y) { return x == y; }

static inline size_t hash_uint16(uint16_t x) { return hash_u64((uint64_t)x); }

static inline int equals_uint16(uint16_t x, uint16_t y) { return x == y; }

static inline size_t hash_int32(int32_t x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_int32(int32_t x, int32_t y) { return x == y; }

static inline size_t hash_uint32(uint32_t x) { return hash_u64((uint64_t)x); }

static inline int equals_uint32(uint32_t x, uint32_t y) { return x == y; }

static inline size_t hash_int64(int64_t x) { return hash_u64((uint64_t)x); }

static inline int equals_int64(int64_t x, int64_t y) { return x == y; }

static inline size_t hash_uint64(uint64_t x) { return hash_u64(x); }

static inline int equals_uint64(uint64_t x, uint64_t y) { return x == y; }

/* least-width integer types */

static inline size_t hash_int_least8(int_least8_t x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_int_least8(int_least8_t x, int_least8_t y) {
  return x == y;
}

static inline size_t hash_uint_least8(uint_least8_t x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_uint_least8(uint_least8_t x, uint_least8_t y) {
  return x == y;
}

static inline size_t hash_int_least16(int_least16_t x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_int_least16(int_least16_t x, int_least16_t y) {
  return x == y;
}

static inline size_t hash_uint_least16(uint_least16_t x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_uint_least16(uint_least16_t x, uint_least16_t y) {
  return x == y;
}

static inline size_t hash_int_least32(int_least32_t x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_int_least32(int_least32_t x, int_least32_t y) {
  return x == y;
}

static inline size_t hash_uint_least32(uint_least32_t x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_uint_least32(uint_least32_t x, uint_least32_t y) {
  return x == y;
}

static inline size_t hash_int_least64(int_least64_t x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_int_least64(int_least64_t x, int_least64_t y) {
  return x == y;
}

static inline size_t hash_uint_least64(uint_least64_t x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_uint_least64(uint_least64_t x, uint_least64_t y) {
  return x == y;
}

/* fast-width integer types */

static inline size_t hash_int_fast8(int_fast8_t x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_int_fast8(int_fast8_t x, int_fast8_t y) {
  return x == y;
}

static inline size_t hash_uint_fast8(uint_fast8_t x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_uint_fast8(uint_fast8_t x, uint_fast8_t y) {
  return x == y;
}

static inline size_t hash_int_fast16(int_fast16_t x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_int_fast16(int_fast16_t x, int_fast16_t y) {
  return x == y;
}

static inline size_t hash_uint_fast16(uint_fast16_t x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_uint_fast16(uint_fast16_t x, uint_fast16_t y) {
  return x == y;
}

static inline size_t hash_int_fast32(int_fast32_t x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_int_fast32(int_fast32_t x, int_fast32_t y) {
  return x == y;
}

static inline size_t hash_uint_fast32(uint_fast32_t x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_uint_fast32(uint_fast32_t x, uint_fast32_t y) {
  return x == y;
}

static inline size_t hash_int_fast64(int_fast64_t x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_int_fast64(int_fast64_t x, int_fast64_t y) {
  return x == y;
}

static inline size_t hash_uint_fast64(uint_fast64_t x) {
  return hash_u64((uint64_t)x);
}

static inline int equals_uint_fast64(uint_fast64_t x, uint_fast64_t y) {
  return x == y;
}

/* Maximum-width integers */

static inline size_t hash_intmax(intmax_t x) { return hash_u64((uint64_t)x); }

static inline int equals_intmax(intmax_t x, intmax_t y) { return x == y; }

static inline size_t hash_uintmax(uintmax_t x) { return hash_u64((uint64_t)x); }

static inline int equals_uintmax(uintmax_t x, uintmax_t y) { return x == y; }

/* stddef.h */

static inline size_t hash_size(size_t x) { return hash_u64((uint64_t)x); }

static inline int equals_size(size_t x, size_t y) { return x == y; }

static inline size_t hash_ptrdiff(ptrdiff_t x) {
  return hash_u64((uint64_t)(int64_t)x);
}

static inline int equals_ptrdiff(ptrdiff_t x, ptrdiff_t y) { return x == y; }

/*
 * Floating-point hashing
 *
 * Hash the object representation rather than converting to an integer.
 * This preserves distinctions between values such as 1.5 and 1.75.
 *
 * +0.0 and -0.0 compare equal, so they are deliberately normalized to
 * the same hash.
 */

static inline size_t hash_float(float x) {
  uint32_t bits;

  if (x == 0.0f)
    return 0;

  memcpy(&bits, &x, sizeof bits);
  return hash_u64((uint64_t)bits);
}

static inline int equals_float(float x, float y) { return x == y; }

static inline size_t hash_double(double x) {
  uint64_t bits;

  if (x == 0.0)
    return 0;

  memcpy(&bits, &x, sizeof bits);
  return hash_u64(bits);
}

static inline int equals_double(double x, double y) { return x == y; }

static inline size_t hash_long_double(long double x) {
  unsigned char bytes[sizeof x];
  size_t h = (size_t)UINT64_C(0xcbf29ce484222325);
  size_t i;

  if (x == 0.0L)
    return 0;

  memcpy(bytes, &x, sizeof bytes);

  /*
   * FNV-1a over the representation.  This is primarily intended for
   * implementations where equal long doubles have stable object
   * representations.
   */
  for (i = 0; i < sizeof bytes; ++i) {
    h ^= (size_t)bytes[i];
#if SIZE_MAX > UINT32_MAX
    h *= (size_t)UINT64_C(0x100000001b3);
#else
    h *= (size_t)UINT32_C(0x01000193);
#endif
  }

  return h;
}

static inline int equals_long_double(long double x, long double y) {
  return x == y;
}

static inline size_t hash_voidp(const void *p) {
  return hash_uintmax((uintptr_t)p);
}

static inline int equals_voidp(const void *x, const void *y) { return x == y; }

#define HASH_H_
#endif
