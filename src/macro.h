#if !defined(MACRO_H_)

#define M_EXPAND_2(...) __VA_ARGS__
#define M_EXPAND_1(...) M_EXPAND_2(__VA_ARGS__)
#define M_EXPAND(...) M_EXPAND_1(__VA_ARGS__)

#define M_CAT_2(x, y) x##y
#define M_CAT_1(x, y) M_CAT_2(x, y)
#define M_CAT(x, y) M_CAT_1(x, y)

#define M_CAT3_2(x, y, z) x##y##z
#define M_CAT3_1(x, y, z) M_CAT3_2(x, y, z)
#define M_CAT3(x, y, z) M_CAT3_1(x, y, z)

#define M_STR_2(...) #__VA_ARGS__
#define M_STR_1(...) M_STR_2(__VA_ARGS__)
#define M_STR(...) M_STR_1(__VA_ARGS__)

#define M_ARG_COUNT_ARGN(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11,     \
                         _12, _13, _14, _15, _16, ...)                         \
  _16
#define M_ARG_COUNT(...)                                                       \
  M_ARG_COUNT_ARGN(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4,  \
                   3, 2, 1)

// Expands a macro for every argumet in the variadic argument list.
//
// The macro should accept these positional arguments:
//  1. The index of the current parameter
//  2. The token T if this is the first parameter, else the F token
//  3. The token T if this is the last parameter, else the F token
//  4. The current parameter
//
// clang-format off
#define M_FOREACH(m, ...)                                                      \
  M_CAT(M_FOREACH_, M_ARG_COUNT(__VA_ARGS__))(m, __VA_ARGS__)

#define M_FOREACH_1(m, _1) \
  m(1, T, T, _1)
#define M_FOREACH_2(m, _1, _2) \
  m(1, T, F, _1) \
  m(2, F, T, _2)
#define M_FOREACH_3(m, _1, _2, _3) \
  m(1, T, F, _1) \
  m(2, F, F, _2) \
  m(3, F, T, _3)
#define M_FOREACH_4(m, _1, _2, _3, _4) \
  m(1, T, F, _1) \
  m(2, F, F, _2) \
  m(3, F, F, _3) \
  m(4, F, T, _4)
#define M_FOREACH_5(m, _1, _2, _3, _4, _5) \
  m(1, T, F, _1) \
  m(2, F, F, _2) \
  m(3, F, F, _3) \
  m(4, F, F, _4) \
  m(5, F, T, _5)
#define M_FOREACH_6(m, _1, _2, _3, _4, _5, _6) \
  m(1, T, F, _1) \
  m(2, F, F, _2) \
  m(3, F, F, _3) \
  m(4, F, F, _4) \
  m(5, F, F, _5) \
  m(6, F, T, _6)
#define M_FOREACH_7(m, _1, _2, _3, _4, _5, _6, _7) \
  m(1, T, F, _1) \
  m(2, F, F, _2) \
  m(3, F, F, _3) \
  m(4, F, F, _4) \
  m(5, F, F, _5) \
  m(6, F, F, _6)  \
  m(7, F, T, _7)
#define M_FOREACH_8(m, _1, _2, _3, _4, _5, _6, _7, _8) \
  m(1, T, F, _1) \
  m(2, F, F, _2) \
  m(3, F, F, _3) \
  m(4, F, F, _4) \
  m(5, F, F, _5) \
  m(6, F, F, _6) \
  m(7, F, F, _7) \
  m(8, F, T, _8)
#define M_FOREACH_9(m, _1, _2, _3, _4, _5, _6, _7, _8, _9) \
  m(1, T, F, _1) \
  m(2, F, F, _2) \
  m(3, F, F, _3) \
  m(4, F, F, _4) \
  m(5, F, F, _5) \
  m(6, F, F, _6) \
  m(7, F, F, _7) \
  m(8, F, F, _8) \
  m(9, F, T, _9)
#define M_FOREACH_10(m, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A) \
  m( 1, T, F, _1) \
  m( 2, F, F, _2) \
  m( 3, F, F, _3) \
  m( 4, F, F, _4) \
  m( 5, F, F, _5) \
  m( 6, F, F, _6) \
  m( 7, F, F, _7) \
  m( 8, F, F, _8) \
  m( 9, F, F, _9) \
  m(10, F, T, _A)
#define M_FOREACH_11(m, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A, _B) \
  m( 1, T, F, _1) \
  m( 2, F, F, _2) \
  m( 3, F, F, _3) \
  m( 4, F, F, _4) \
  m( 5, F, F, _5) \
  m( 6, F, F, _6) \
  m( 7, F, F, _7) \
  m( 8, F, F, _8) \
  m( 9, F, F, _9) \
  m(10, F, F, _A) \
  m(11, F, T, _B)
#define M_FOREACH_12(m, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A, _B, _C) \
  m( 1, T, F, _1) \
  m( 2, F, F, _2) \
  m( 3, F, F, _3) \
  m( 4, F, F, _4) \
  m( 5, F, F, _5) \
  m( 6, F, F, _6) \
  m( 7, F, F, _7) \
  m( 8, F, F, _8) \
  m( 9, F, F, _9) \
  m(10, F, F, _A) \
  m(11, F, F, _B) \
  m(12, F, T, _C)
#define M_FOREACH_13(m, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A, _B, _C, _D) \
  m( 1, T, F, _1) \
  m( 2, F, F, _2) \
  m( 3, F, F, _3) \
  m( 4, F, F, _4) \
  m( 5, F, F, _5) \
  m( 6, F, F, _6) \
  m( 7, F, F, _7) \
  m( 8, F, F, _8) \
  m( 9, F, F, _9) \
  m(10, F, F, _A) \
  m(11, F, F, _B) \
  m(12, F, F, _C) \
  m(13, F, T, _D)
#define M_FOREACH_14(m, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A, _B, _C, _D, _E) \
  m( 1, T, F, _1) \
  m( 2, F, F, _2) \
  m( 3, F, F, _3) \
  m( 4, F, F, _4) \
  m( 5, F, F, _5) \
  m( 6, F, F, _6) \
  m( 7, F, F, _7) \
  m( 8, F, F, _8) \
  m( 9, F, F, _9) \
  m(10, F, F, _A) \
  m(11, F, F, _B) \
  m(12, F, F, _C) \
  m(13, F, F, _D) \
  m(14, F, T, _E)
#define M_FOREACH_15(m, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A, _B, _C, _D, _E, _F) \
  m( 1, T, F, _1) \
  m( 2, F, F, _2) \
  m( 3, F, F, _3) \
  m( 4, F, F, _4) \
  m( 5, F, F, _5) \
  m( 6, F, F, _6) \
  m( 7, F, F, _7) \
  m( 8, F, F, _8) \
  m( 9, F, F, _9) \
  m(10, F, F, _A) \
  m(11, F, F, _B) \
  m(12, F, F, _C) \
  m(13, F, F, _D) \
  m(14, F, F, _E) \
  m(15, F, T, _F)

// clang-format on

#define M_DELIM_F(Delim) Delim
#define M_DELIM_T(Delim)
#define M_DELIM(Lst, Delim) M_CAT(M_DELIM_, Lst)(Delim)

#define M_COMMA_F ,
#define M_COMMA_T
#define M_COMMA(Lst) M_CAT(M_COMMA_, Lst)

#define M_ENUM_MEMB(I, Fst, Lst, M) M M_COMMA(Lst)
#define M_ENUM_TOSTR(I, Fst, Lst, M)                                           \
  case M:                                                                      \
    return #M;

// Define an enum with an automatically generated to_cstr function.
//
// Typename : Name of the enum
// __VA_ARGS__ : Enumeration values
//
#define M_ENUM(Typename, ...)                                                  \
  enum Typename { M_FOREACH(M_ENUM_MEMB, __VA_ARGS__) };                       \
  static inline const char *M_CAT(Typename, _to_cstr)(enum Typename value) {   \
    switch (value) {                                                           \
      M_FOREACH(M_ENUM_TOSTR, __VA_ARGS__)                                     \
    default:                                                                   \
      return "<unknown>";                                                      \
    }                                                                          \
  }

#define M_NAME(base) M_CAT3(PREFIX, base, SUFFIX)
#define M_TYPENAME M_NAME(TYPE_0)

#define MACRO_H_
#endif
