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

#define M_NAME(base) M_CAT3(PREFIX, base, SUFFIX)
#define M_TYPENAME M_NAME(TYPE_0)

#define MACRO_H_
#endif
