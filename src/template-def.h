#include "macro.h"

#if !defined(SUFFIX)
#if defined(REQUIRE_SUFFIX)
#error "No SUFFIX definition provided."
#else
#define SUFFIX
#endif
#endif

#if !defined(PREFIX)
#if defined(REQUIRE_PREFIX)
#error "No PREFIX definition provided."
#else
#define PREFIX
#endif
#endif

#if !defined(TYPE_0)
#if defined(REQUIRE_TYPE_0)
#error "Required template argument TYPE_0 not defined"
#else
#define DEFINE_STRUCT
#endif
#endif

#if !defined(TYPE_1) && defined(REQUIRE_TYPE_1)
#error "Required template argument TYPE_1 not defined"
#endif

#if !defined(TYPE_2) && defined(REQUIRE_TYPE_2)
#error "Required template argument TYPE_2 not defined"
#endif

#if !defined(TYPE_3) && defined(REQUIRE_TYPE_3)
#error "Required template argument TYPE_3 not defined"
#endif

#if !defined(TYPE_4) && defined(REQUIRE_TYPE_4)
#error "Required template argument TYPE_4 not defined"
#endif

#if !defined(FUNCTION_0) && defined(REQUIRE_FUNCTION_0)
#error "Required function argument FUNCTION_0 not provided"
#else
#endif

#if !defined(FUNCTION_1) && defined(REQUIRE_FUNCTION_1)
#error "Required function argument FUNCTION_1 not provided"
#else
#endif

#if !defined(FUNCTION_2) && defined(REQUIRE_FUNCTION_2)
#error "Required function argument FUNCTION_2 not provided"
#endif

#if !defined(FUNCTION_3) && defined(REQUIRE_FUNCTION_3)
#error "Required function argument FUNCTION_3 not provided"
#endif

#if !defined(FUNCTION_4) && defined(REQUIRE_FUNCTION_4)
#error "Required function argument FUNCTION_4 not provided"
#endif

#if !defined(C_SOURCE) && !defined(C_HEADER) && !defined(HEADER_ONLY)
#error "Neither C_SOURCE, C_HEADER or HEADER_ONLY defined."
#endif

#if defined(HEADER_ONLY)
#define INLINE static inline
#define C_SOURCE
#define C_HEADER
#else
#define INLINE
#endif

#if defined(C_SOURCE) && !defined(C_HEADER)
#define C_HEADER
#endif

#define TEMPLATE_DEF
