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
 * @file template-undef.h
 *
 * This file is to be included after all the contents of a template header file
 * that includes template-def.h
 *
 * The purpose of this file is to #undef all template parameters and
 * intermediate preprocessor definitions to ensure they do not leak into the
 * global scope.
 */

#if !defined(TEMPLATE_DEF)
#error "template-undef.h may only be included after template-def.h"
#else
#undef TEMPLATE_DEF
#endif

#if defined(SUFFIX)
#undef SUFFIX
#endif

#if defined(REQUIRE_SUFFIX)
#undef REQUIRE_SUFFIX
#endif

#if defined(PREFIX)
#undef PREFIX
#endif

#if defined(REQUIRE_PREFIX)
#undef REQUIRE_PREFIX
#endif

#if defined(TYPE_0)
#undef TYPE_0
#endif

#if defined(DEFINE_STRUCT)
#undef DEFINE_STRUCT
#endif

#if defined(REQUIRE_TYPE_0)
#undef REQUIRE_TYPE_0
#endif

#if defined(TYPE_1)
#undef TYPE_1
#endif

#if defined(REQUIRE_TYPE_1)
#undef REQUIRE_TYPE_1
#endif

#if defined(TYPE_2)
#undef TYPE_2
#endif

#if defined(REQUIRE_TYPE_2)
#undef REQUIRE_TYPE_2
#endif

#if defined(TYPE_3)
#undef TYPE_3
#endif

#if defined(REQUIRE_TYPE_3)
#undef REQUIRE_TYPE_3
#endif

#if defined(TYPE_4)
#undef TYPE_4
#endif

#if defined(REQUIRE_TYPE_4)
#undef REQUIRE_TYPE_4
#endif

#if defined(FUNCTION_0)
#undef FUNCTION_0
#endif

#if defined(REQUIRE_FUNCTION_0)
#undef REQUIRE_FUNCTION_0
#endif

#if defined(FUNCTION_1)
#undef FUNCTION_1
#endif

#if defined(REQUIRE_FUNCTION_1)
#undef REQUIRE_FUNCTION_1
#endif

#if defined(FUNCTION_2)
#undef FUNCTION_2
#endif

#if defined(REQUIRE_FUNCTION_2)
#undef REQUIRE_FUNCTION_2
#endif

#if defined(FUNCTION_3)
#undef FUNCTION_3
#endif

#if defined(REQUIRE_FUNCTION_3)
#undef REQUIRE_FUNCTION_3
#endif

#if defined(FUNCTION_4)
#undef FUNCTION_4
#endif

#if defined(REQUIRE_FUNCTION_4)
#undef REQUIRE_FUNCTION_4
#endif

#if defined(C_SOURCE)
#undef C_SOURCE
#endif

#if defined(C_HEADER)
#undef C_HEADER
#endif

#if defined(HEADER_ONLY)
#undef HEADER_ONLY
#endif

#if defined(INLINE)
#undef INLINE
#endif
