#if !defined(ANY_H_)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "string_view.h"

/**
 * "any", a union that can contain most of the primitive C types
 */
union any {

  char c;
  wchar_t wc;
  bool b;

  signed char sc;
  short h;
  int i;
  long l;
  long long ll;

  unsigned char uc;
  unsigned short uh;
  unsigned int u;
  unsigned long ul;
  unsigned long long ull;

  int8_t i8;
  int16_t i16;
  int32_t i32;
  int64_t i64;

  uint8_t u8;
  uint16_t u16;
  uint32_t u32;
  uint64_t u64;

  size_t uz;
  ptrdiff_t iz;

  void *ptr;
  char *str;
  const char *cstr;
  intptr_t iptr;
  uintptr_t uptr;

  float f;
  double d;
  long double ld;

  string_view_t sv;
};

#define ANY_H_
#endif
