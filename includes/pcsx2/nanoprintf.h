/*
  nanoprintf: a tiny embeddable printf replacement written in C.
  https://github.com/charlesnicholson/nanoprintf
  charles.nicholson+nanoprintf@gmail.com

  LICENSE:
  --------
  This is free and unencumbered software released into the public domain.

  Anyone is free to copy, modify, publish, use, compile, sell, or
  distribute this software, either in source code form or as a compiled
  binary, for any purpose, commercial or non-commercial, and by any
  means.

  In jurisdictions that recognize copyright laws, the author or authors
  of this software dedicate any and all copyright interest in the
  software to the public domain. We make this dedication for the benefit
  of the public at large and to the detriment of our heirs and
  successors. We intend this dedication to be an overt act of
  relinquishment in perpetuity of all present and future rights to this
  software under copyright law.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  For more information, please refer to <http://unlicense.org>
*/

#ifndef NANOPRINTF_H_INCLUDED
#define NANOPRINTF_H_INCLUDED

#include <stdarg.h>
#include <stddef.h>

// Define this to fully sandbox nanoprintf inside of a translation unit.
#ifdef NANOPRINTF_VISIBILITY_STATIC
  #define NPF_VISIBILITY static
#else
  #define NPF_VISIBILITY extern
#endif

#if defined(__clang__)
  #define NPF_PRINTF_ATTR(FORMAT_INDEX, VARGS_INDEX) \
    __attribute__((__format__(__printf__, FORMAT_INDEX, VARGS_INDEX)))
#elif defined(__GNUC__) || defined(__GNUG__)
  #define NPF_PRINTF_ATTR(FORMAT_INDEX, VARGS_INDEX) \
    __attribute__((format(printf, FORMAT_INDEX, VARGS_INDEX)))
#else
  #define NPF_PRINTF_ATTR(FORMAT_INDEX, VARGS_INDEX)
#endif

// Public API

#ifdef __cplusplus
extern "C" {
#endif

NPF_VISIBILITY int npf_snprintf(char *buffer, size_t bufsz, const char *format,
                                ...) NPF_PRINTF_ATTR(3, 4);

NPF_VISIBILITY int npf_vsnprintf(char *buffer, size_t bufsz, char const *format,
                                 va_list vlist) NPF_PRINTF_ATTR(3, 0);

typedef void (*npf_putc)(int c, void *ctx);
NPF_VISIBILITY int npf_pprintf(npf_putc pc, void *pc_ctx, char const *format,
                               ...) NPF_PRINTF_ATTR(3, 4);

NPF_VISIBILITY int npf_vpprintf(npf_putc pc, void *pc_ctx, char const *format,
                                va_list vlist) NPF_PRINTF_ATTR(3, 0);

#ifdef __cplusplus
}
#endif

#endif // NANOPRINTF_H_INCLUDED

/* The implementation of nanoprintf begins here, to be compiled only if
   NANOPRINTF_IMPLEMENTATION is defined. In a multi-file library what follows would
   be nanoprintf.c. */

#ifdef NANOPRINTF_IMPLEMENTATION

#ifndef NANOPRINTF_IMPLEMENTATION_INCLUDED
#define NANOPRINTF_IMPLEMENTATION_INCLUDED

#include <inttypes.h>
#include <stdint.h>

// Pick reasonable defaults if nothing's been configured.
#if !defined(NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS)
  #define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
  #define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
  #define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
  #define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 0
  #define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 0
  #define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#endif

// If anything's been configured, everything must be configured.
#ifndef NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

// Ensure flags are compatible.
#if (NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 0)
  #error Precision format specifiers must be enabled if float support is enabled.
#endif

#if defined(NANOPRINTF_SNPRINTF_SAFE_EMPTY_STRING_ON_OVERFLOW) && \
    defined(NANOPRINTF_SNPRINTF_SAFE_TRIM_STRING_ON_OVERFLOW)
  #error snprintf safety flags are mutually exclusive.
#endif

// intmax_t / uintmax_t require stdint from c99 / c++11
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  #ifndef _MSC_VER
    #ifdef __cplusplus
      #if __cplusplus < 201103L
        #error large format specifier support requires C++11 or later.
      #endif
    #else
      #if __STDC_VERSION__ < 199409L
        #error nanoprintf requires C99 or later.
      #endif
    #endif
  #endif
#endif

// Figure out if we can disable warnings with pragmas.
#ifdef __clang__
  #define NANOPRINTF_CLANG 1
  #define NANOPRINTF_GCC_PAST_4_6 0
#else
  #define NANOPRINTF_CLANG 0
  #if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 6)))
    #define NANOPRINTF_GCC_PAST_4_6 1
  #else
    #define NANOPRINTF_GCC_PAST_4_6 0
  #endif
#endif

#if NANOPRINTF_CLANG || NANOPRINTF_GCC_PAST_4_6
  #define NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS 1
#else
  #define NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS 0
#endif

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-function"
  #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
  #ifdef __cplusplus
    #pragma GCC diagnostic ignored "-Wold-style-cast"
  #endif
  #pragma GCC diagnostic ignored "-Wpadded"
  #pragma GCC diagnostic ignored "-Wfloat-equal"
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
    #pragma GCC diagnostic ignored "-Wcovered-switch-default"
  #elif NANOPRINTF_GCC_PAST_4_6
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  #endif
#endif

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable:4514) // unreferenced inline function removed
  #pragma warning(disable:4505) // unreferenced function removed
  #pragma warning(disable:4820) // padding after data member
  #pragma warning(disable:5039) // extern "C" throw
  #pragma warning(disable:5045) // spectre mitigation
  #pragma warning(disable:4701) // possibly uninitialized
  #pragma warning(disable:4706) // assignment in conditional
  #pragma warning(disable:4710) // not inlined
  #pragma warning(disable:4711) // selected for inline
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) || \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
typedef enum {
  NPF_FMT_SPEC_OPT_NONE,
  NPF_FMT_SPEC_OPT_LITERAL,
  NPF_FMT_SPEC_OPT_STAR,
} npf_fmt_spec_opt_t;
#endif

typedef enum {
  NPF_FMT_SPEC_LEN_MOD_NONE,
  NPF_FMT_SPEC_LEN_MOD_SHORT,       // 'h'
  NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE, // 'L'
  NPF_FMT_SPEC_LEN_MOD_CHAR,        // 'hh'
  NPF_FMT_SPEC_LEN_MOD_LONG         // 'l'
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  ,
  NPF_FMT_SPEC_LEN_MOD_LARGE_LONG_LONG, // 'll'
  NPF_FMT_SPEC_LEN_MOD_LARGE_INTMAX,    // 'j'
  NPF_FMT_SPEC_LEN_MOD_LARGE_SIZET,     // 'z'
  NPF_FMT_SPEC_LEN_MOD_LARGE_PTRDIFFT   // 't'
#endif
} npf_format_spec_length_modifier_t;

typedef enum {
  NPF_FMT_SPEC_CONV_PERCENT,      // '%'
  NPF_FMT_SPEC_CONV_CHAR,         // 'c'
  NPF_FMT_SPEC_CONV_STRING,       // 's'
  NPF_FMT_SPEC_CONV_SIGNED_INT,   // 'i', 'd'
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
  NPF_FMT_SPEC_CONV_BINARY,       // 'b'
#endif
  NPF_FMT_SPEC_CONV_OCTAL,        // 'o'
  NPF_FMT_SPEC_CONV_HEX_INT,      // 'x', 'X'
  NPF_FMT_SPEC_CONV_UNSIGNED_INT, // 'u'
  NPF_FMT_SPEC_CONV_POINTER       // 'p'
#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
  , NPF_FMT_SPEC_CONV_WRITEBACK   // 'n'
#endif
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
  , NPF_FMT_SPEC_CONV_FLOAT_DECIMAL // 'f', 'F'
#endif
} npf_format_spec_conversion_t;

typedef struct {
  char prepend;          // ' ' or '+'
  char alt_form;         // '#'

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  npf_fmt_spec_opt_t field_width_opt;
  int field_width;
  char left_justified;   // '-'
  char leading_zero_pad; // '0'
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
  npf_fmt_spec_opt_t prec_opt;
  int prec;
#endif

  npf_format_spec_length_modifier_t length_modifier;
  npf_format_spec_conversion_t conv_spec;
  char case_adjust;
} npf_format_spec_t;

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 0
  typedef long npf_int_t;
  typedef unsigned long npf_uint_t;
#else
  typedef intmax_t npf_int_t;
  typedef uintmax_t npf_uint_t;
#endif

typedef struct {
  char *dst;
  size_t len;
  size_t cur;
} npf_bufputc_ctx_t;

static int npf_parse_format_spec(char const *format, npf_format_spec_t *out_spec);
static void npf_bufputc(int c, void *ctx);
static void npf_bufputc_nop(int c, void *ctx);
static int npf_itoa_rev(char *buf, npf_int_t i);
static int npf_utoa_rev(char *buf, npf_uint_t i, unsigned base, unsigned case_adjust);

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
static int npf_fsplit_abs(float f,
                          uint64_t *out_int_part,
                          uint64_t *out_frac_part,
                          int *out_frac_base10_neg_e);
static int npf_ftoa_rev(char *buf, float f, char case_adj, int *out_frac_chars);
#endif

#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
static int npf_bin_len(npf_uint_t i);
#endif

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  #ifdef _MSC_VER
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;
  #else
    #include <sys/types.h>
  #endif
#endif

#ifdef _MSC_VER
  #include <intrin.h>
#endif

static int npf_min(int x, int y) { return (x < y) ? x : y; }
static int npf_max(int x, int y) { return (x > y) ? x : y; }

int npf_parse_format_spec(char const *format, npf_format_spec_t *out_spec) {
  char const *cur = format;

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  out_spec->left_justified = 0;
  out_spec->leading_zero_pad = 0;
#endif
  out_spec->case_adjust = 'a'-'A'; // lowercase
  out_spec->prepend = 0;
  out_spec->alt_form = 0;

  while (*++cur) { // cur points at the leading '%' character
    switch (*cur) { // Optional flags
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
      case '-':
        out_spec->left_justified = '-';
        out_spec->leading_zero_pad = 0;
        continue;
      case '0':
        out_spec->leading_zero_pad = !out_spec->left_justified;
        continue;
#endif
      case '+':
        out_spec->prepend = '+';
        continue;
      case ' ':
        if (out_spec->prepend == 0) { out_spec->prepend = ' '; }
        continue;
      case '#':
        out_spec->alt_form = '#';
        continue;
      default: break;
    }
    break;
  }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  out_spec->field_width_opt = NPF_FMT_SPEC_OPT_NONE;
  if (*cur == '*') {
    out_spec->field_width_opt = NPF_FMT_SPEC_OPT_STAR;
    ++cur;
  } else {
    out_spec->field_width = 0;
    while ((*cur >= '0') && (*cur <= '9')) {
      out_spec->field_width_opt = NPF_FMT_SPEC_OPT_LITERAL;
      out_spec->field_width = (out_spec->field_width * 10) + (*cur++ - '0');
    }
  }
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
  out_spec->prec = 0;
  out_spec->prec_opt = NPF_FMT_SPEC_OPT_NONE;
  if (*cur == '.') {
    ++cur;
    if (*cur == '*') {
      out_spec->prec_opt = NPF_FMT_SPEC_OPT_STAR;
      ++cur;
    } else {
      if (*cur == '-') {
        ++cur;
        out_spec->prec_opt = NPF_FMT_SPEC_OPT_NONE;
      } else {
        out_spec->prec_opt = NPF_FMT_SPEC_OPT_LITERAL;
      }
      while ((*cur >= '0') && (*cur <= '9')) {
        out_spec->prec = (out_spec->prec * 10) + (*cur++ - '0');
      }
    }
  }
#endif

  out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_NONE;
  switch (*cur++) { // Length modifier
    case 'h':
      out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_SHORT;
      if (*cur == 'h') {
        out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_CHAR;
        ++cur;
      }
      break;
    case 'l':
      out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LONG;
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
      if (*cur == 'l') {
        out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_LONG_LONG;
        ++cur;
      }
#endif
      break;
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    case 'L':
      out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE;
      break;
#endif
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    case 'j':
      out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_INTMAX;
      break;
    case 'z':
      out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_SIZET;
      break;
    case 't':
      out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_PTRDIFFT;
      break;
#endif
    default: --cur; break;
  }

  switch (*cur++) { // Conversion specifier
    case '%':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_PERCENT;
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      out_spec->prec_opt = NPF_FMT_SPEC_OPT_NONE;
#endif
      break;
    case 'c':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_CHAR;
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      out_spec->prec_opt = NPF_FMT_SPEC_OPT_NONE;
#endif
      break;
    case 's':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_STRING;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
      out_spec->leading_zero_pad = 0;
#endif
      break;

    case 'i':
    case 'd':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_SIGNED_INT;
      break;

    case 'o':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_OCTAL;
      break;
    case 'u':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_UNSIGNED_INT;
      break;

    case 'X':
      out_spec->case_adjust = 0;
    case 'x':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_HEX_INT;
      break;

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    case 'F':
      out_spec->case_adjust = 0;
    case 'f':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_DECIMAL;
      if (out_spec->prec_opt == NPF_FMT_SPEC_OPT_NONE) { out_spec->prec = 6; }
      break;
#endif

#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
    case 'n':
      // todo: reject string if flags or width or precision exist
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_WRITEBACK;
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      out_spec->prec_opt = NPF_FMT_SPEC_OPT_NONE;
#endif
      break;
#endif

    case 'p':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_POINTER;
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      out_spec->prec_opt = NPF_FMT_SPEC_OPT_NONE;
#endif
      break;

#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
    case 'B':
      out_spec->case_adjust = 0;
    case 'b':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_BINARY;
      break;
#endif

    default: return 0;
  }

  return (int)(cur - format);
}

int npf_itoa_rev(char *buf, npf_int_t i) {
  int n = 0;
  int const sign = (i >= 0) ? 1 : -1;
  do { *buf++ = (char)('0' + (sign * (i % 10))); i /= 10; ++n; } while (i);
  return n;
}

int npf_utoa_rev(char *buf, npf_uint_t i, unsigned base, unsigned case_adj) {
  int n = 0;
  do {
    unsigned const d = (unsigned)(i % base);
    *buf++ = (char)((d < 10) ? ('0' + d) : ('A' + case_adj + (d - 10)));
    i /= base;
    ++n;
  } while (i);
  return n;
}

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
enum {
  NPF_MANTISSA_BITS = 23,
  NPF_EXPONENT_BITS = 8,
  NPF_EXPONENT_BIAS = 127,
  NPF_FRACTION_BIN_DIGITS = 64,
  NPF_MAX_FRACTION_DEC_DIGITS = 8
};

int npf_fsplit_abs(float f, uint64_t *out_int_part, uint64_t *out_frac_part,
                   int *out_frac_base10_neg_exp) {
  /* conversion algorithm by Wojciech Muła (zdjęcia@garnek.pl)
     http://0x80.pl/notesen/2015-12-29-float-to-string.html
     grisu2 (https://bit.ly/2JgMggX) and ryu (https://bit.ly/2RLXSg0)
     are fast + precise + round, but require large lookup tables. */

  uint32_t f_bits; { // union-cast is UB, let compiler optimize byte-copy loop.
    char const *src = (char const *)&f;
    char *dst = (char *)&f_bits;
    for (unsigned i = 0; i < sizeof(f_bits); ++i) { dst[i] = src[i]; }
  }

  int const exponent =
    ((int)((f_bits >> NPF_MANTISSA_BITS) & ((1u << NPF_EXPONENT_BITS) - 1u)) -
      NPF_EXPONENT_BIAS) - NPF_MANTISSA_BITS;

  if (exponent >= (64 - NPF_MANTISSA_BITS)) { return 0; } // value is out of range

  uint32_t const implicit_one = 1u << NPF_MANTISSA_BITS;
  uint32_t const mantissa = f_bits & (implicit_one - 1);
  uint32_t const mantissa_norm = mantissa | implicit_one;

  if (exponent > 0) {
    *out_int_part = (uint64_t)mantissa_norm << exponent;
  } else if (exponent < 0) {
    if (-exponent > NPF_MANTISSA_BITS) {
      *out_int_part = 0;
    } else {
      *out_int_part = mantissa_norm >> -exponent;
    }
  } else {
    *out_int_part = mantissa_norm;
  }

  uint64_t frac; {
    int const shift = NPF_FRACTION_BIN_DIGITS + exponent - 4;
    if ((shift >= (NPF_FRACTION_BIN_DIGITS - 4)) || (shift < 0)) {
      frac = 0;
    } else {
      frac = ((uint64_t)mantissa_norm) << shift;
    }
    // multiply off the leading one's digit
    frac &= 0x0fffffffffffffffllu;
    frac *= 10;
  }

  { // Count the number of 0s at the beginning of the fractional part.
    int frac_base10_neg_exp = 0;
    while (frac && ((frac >> (NPF_FRACTION_BIN_DIGITS - 4))) == 0) {
      ++frac_base10_neg_exp;
      frac &= 0x0fffffffffffffffllu;
      frac *= 10;
    }
    *out_frac_base10_neg_exp = frac_base10_neg_exp;
  }

  { // Convert the fractional part to base 10.
    unsigned frac_part = 0;
    for (int i = 0; frac && (i < NPF_MAX_FRACTION_DEC_DIGITS); ++i) {
      frac_part *= 10;
      frac_part += (unsigned)(frac >> (NPF_FRACTION_BIN_DIGITS - 4));
      frac &= 0x0fffffffffffffffllu;
      frac *= 10;
    }
    *out_frac_part = frac_part;
  }
  return 1;
}

int npf_ftoa_rev(char *buf, float f, char case_adj, int *out_frac_chars) {
  uint32_t f_bits; { // union-cast is UB, let compiler optimize byte-copy loop.
    char const *src = (char const *)&f;
    char *dst = (char *)&f_bits;
    for (unsigned i = 0; i < sizeof(f_bits); ++i) { dst[i] = src[i]; }
  }

  if ((uint8_t)(f_bits >> 23) == 0xFF) {
    if (f_bits & 0x7fffff) {
      for (int i = 0; i < 3; ++i) { *buf++ = (char)("NAN"[i] + case_adj); }
    } else {
      for (int i = 0; i < 3; ++i) { *buf++ = (char)("FNI"[i] + case_adj); }
    }
    return -3;
  }

  uint64_t int_part, frac_part;
  int frac_base10_neg_exp;
  if (npf_fsplit_abs(f, &int_part, &frac_part, &frac_base10_neg_exp) == 0) {
    for (int i = 0; i < 3; ++i) { *buf++ = (char)("ROO"[i] + case_adj); }
    return -3;
  }

  char *dst = buf;

  while (frac_part) { // write the fractional digits
    *dst++ = (char)('0' + (frac_part % 10));
    frac_part /= 10;
  }

  // write the 0 digits between the . and the first fractional digit
  while (frac_base10_neg_exp-- > 0) { *dst++ = '0'; }
  *out_frac_chars = (int)(dst - buf);
  *dst++ = '.';

  // write the integer digits
  do { *dst++ = (char)('0' + (int_part % 10)); int_part /= 10; } while (int_part);
  return (int)(dst - buf);
}

#endif // NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS

#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
int npf_bin_len(npf_uint_t u) {
  // Return the length of the binary string format of 'u', preferring intrinsics.
  if (!u) { return 1; }

#ifdef _MSC_VER // Win64, use _BSR64 for everything. If x86, use _BSR when non-large.
  #ifdef _M_X64
    #define NPF_HAVE_BUILTIN_CLZ
    #define NPF_CLZ _BitScanReverse64
  #elif NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 0
    #define NPF_HAVE_BUILTIN_CLZ
    #define NPF_CLZ _BitScanReverse
  #endif
  #ifdef NPF_HAVE_BUILTIN_CLZ
    unsigned long idx;
    NPF_CLZ(&idx, u);
    return (int)(idx + 1);
  #endif
#elif defined(NANOPRINTF_CLANG) || defined(NANOPRINTF_GCC_PAST_4_6)
  #define NPF_HAVE_BUILTIN_CLZ
  #if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    #define NPF_CLZ(X) ((sizeof(long long) * 8) - (size_t)__builtin_clzll(X))
  #else
    #define NPF_CLZ(X) ((sizeof(long) * 8) - (size_t)__builtin_clzl(X))
  #endif
  return (int)NPF_CLZ(u);
#endif

#ifndef NPF_HAVE_BUILTIN_CLZ
  int n;
  for (n = 0; u; ++n, u >>= 1); // slow but small software fallback
  return n;
#else
  #undef NPF_HAVE_BUILTIN_CLZ
  #undef NPF_CLZ
#endif
}
#endif

void npf_bufputc(int c, void *ctx) {
  npf_bufputc_ctx_t *bpc = (npf_bufputc_ctx_t *)ctx;
  if (bpc->cur < bpc->len) { bpc->dst[bpc->cur++] = (char)c; }
}

void npf_bufputc_nop(int c, void *ctx) { (void)c; (void)ctx; }

typedef struct npf_cnt_putc_ctx {
  npf_putc pc;
  void *ctx;
  int n;
} npf_cnt_putc_ctx_t;

static void npf_putc_cnt(int c, void *ctx) {
  npf_cnt_putc_ctx_t *pc_cnt = (npf_cnt_putc_ctx_t *)ctx;
  ++pc_cnt->n;
  pc_cnt->pc(c, pc_cnt->ctx); // sibling-call optimization
}

#define NPF_PUTC(VAL) do { npf_putc_cnt((int)(VAL), &pc_cnt); } while (0)

#define NPF_EXTRACT(MOD, CAST_TO, EXTRACT_AS) \
  case NPF_FMT_SPEC_LEN_MOD_##MOD: val = (CAST_TO)va_arg(args, EXTRACT_AS); break

#define NPF_WRITEBACK(MOD, TYPE) \
  case NPF_FMT_SPEC_LEN_MOD_##MOD: *(va_arg(args, TYPE *)) = (TYPE)pc_cnt.n; break

int npf_vpprintf(npf_putc pc, void *pc_ctx, char const *format, va_list args) {
  npf_format_spec_t fs;
  char const *cur = format;
  npf_cnt_putc_ctx_t pc_cnt;
  pc_cnt.pc = pc;
  pc_cnt.ctx = pc_ctx;
  pc_cnt.n = 0;

  while (*cur) {
    int const fs_len = (*cur != '%') ? 0 : npf_parse_format_spec(cur, &fs);
    if (!fs_len) { NPF_PUTC(*cur++); continue; }
    cur += fs_len;

    // Extract star-args immediately
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    if (fs.field_width_opt == NPF_FMT_SPEC_OPT_STAR) {
      fs.field_width_opt = NPF_FMT_SPEC_OPT_LITERAL;
      fs.field_width = va_arg(args, int);
      if (fs.field_width < 0) {
        fs.field_width = -fs.field_width;
        fs.left_justified = 1;
      }
    }
#endif
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    if (fs.prec_opt == NPF_FMT_SPEC_OPT_STAR) {
      fs.prec_opt = NPF_FMT_SPEC_OPT_NONE;
      fs.prec = va_arg(args, int);
      if (fs.prec >= 0) { fs.prec_opt = NPF_FMT_SPEC_OPT_LITERAL; }
    }
#endif

    union { char cbuf_mem[32]; npf_uint_t binval; } u;
    char *cbuf = u.cbuf_mem, sign_c = 0;
    int cbuf_len = 0, need_0x = 0;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    int field_pad = 0;
    char pad_c = 0;
#endif
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    int prec_pad = 0;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    int zero = 0;
#endif
#endif
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    int frac_chars = 0, inf_or_nan = 0;
#endif

    // Extract and convert the argument to string, point cbuf at the text.
    switch (fs.conv_spec) {
      case NPF_FMT_SPEC_CONV_PERCENT:
        *cbuf = '%';
        ++cbuf_len;
        break;

      case NPF_FMT_SPEC_CONV_CHAR:
        *cbuf = (char)va_arg(args, int);
        ++cbuf_len;
        break;

      case NPF_FMT_SPEC_CONV_STRING: {
        cbuf = va_arg(args, char *);
        for (char const *s = cbuf; *s; ++s, ++cbuf_len); // strlen
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
        if (fs.prec_opt == NPF_FMT_SPEC_OPT_LITERAL) {
          cbuf_len = npf_min(fs.prec, cbuf_len); // prec truncates strings
        }
#endif
      } break;

      case NPF_FMT_SPEC_CONV_SIGNED_INT: {
        npf_int_t val = 0;
        switch (fs.length_modifier) {
          NPF_EXTRACT(NONE, int, int);
          NPF_EXTRACT(SHORT, short, int);
          NPF_EXTRACT(LONG_DOUBLE, int, int);
          NPF_EXTRACT(CHAR, char, int);
          NPF_EXTRACT(LONG, long, long);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
          NPF_EXTRACT(LARGE_LONG_LONG, long long, long long);
          NPF_EXTRACT(LARGE_INTMAX, intmax_t, intmax_t);
          NPF_EXTRACT(LARGE_SIZET, ssize_t, ssize_t);
          NPF_EXTRACT(LARGE_PTRDIFFT, ptrdiff_t, ptrdiff_t);
#endif
          default: break;
        }

        sign_c = (val < 0) ? '-' : fs.prepend;

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
        zero = !val;
#endif
        // special case, if prec and value are 0, skip
        if (!val && (fs.prec_opt == NPF_FMT_SPEC_OPT_LITERAL) && !fs.prec) {
          cbuf_len = 0;
        } else
#endif
        { cbuf_len = npf_itoa_rev(cbuf, val); }
      } break;

#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
      case NPF_FMT_SPEC_CONV_BINARY:
#endif
      case NPF_FMT_SPEC_CONV_OCTAL:
      case NPF_FMT_SPEC_CONV_HEX_INT:
      case NPF_FMT_SPEC_CONV_UNSIGNED_INT: {
        npf_uint_t val = 0;

        switch (fs.length_modifier) {
          NPF_EXTRACT(NONE, unsigned, unsigned);
          NPF_EXTRACT(SHORT, unsigned short, unsigned);
          NPF_EXTRACT(LONG_DOUBLE, unsigned, unsigned);
          NPF_EXTRACT(CHAR, unsigned char, unsigned);
          NPF_EXTRACT(LONG, unsigned long, unsigned long);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
          NPF_EXTRACT(LARGE_LONG_LONG, unsigned long long, unsigned long long);
          NPF_EXTRACT(LARGE_INTMAX, uintmax_t, uintmax_t);
          NPF_EXTRACT(LARGE_SIZET, size_t, size_t);
          NPF_EXTRACT(LARGE_PTRDIFFT, size_t, size_t);
#endif
          default: break;
        }

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
        zero = !val;
#endif
        if (!val && (fs.prec_opt == NPF_FMT_SPEC_OPT_LITERAL) && !fs.prec) {
          // Zero value and explicitly-requested zero precision means "print nothing".
          if ((fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL) && fs.alt_form) {
            fs.prec = 1; // octal special case, print a single '0'
          }
        } else
#endif
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
        if (fs.conv_spec == NPF_FMT_SPEC_CONV_BINARY) {
          cbuf_len = npf_bin_len(val); u.binval = val;
        } else
#endif
        {
          unsigned const base = (fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL) ?
            8u : ((fs.conv_spec == NPF_FMT_SPEC_CONV_HEX_INT) ? 16u : 10u);
          cbuf_len = npf_utoa_rev(cbuf, val, base, (unsigned)fs.case_adjust);
        }

        if (val && fs.alt_form && (fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL)) {
          cbuf[cbuf_len++] = '0'; // OK to add leading octal '0' immediately.
        }

        if (val && fs.alt_form) { // 0x or 0b but can't write it yet.
          if (fs.conv_spec == NPF_FMT_SPEC_CONV_HEX_INT) { need_0x = 'X'; }
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
          else if (fs.conv_spec == NPF_FMT_SPEC_CONV_BINARY) { need_0x = 'B'; }
#endif
          if (need_0x) { need_0x += fs.case_adjust; }
        }
      } break;

      case NPF_FMT_SPEC_CONV_POINTER: {
        cbuf_len =
          npf_utoa_rev(cbuf, (npf_uint_t)(uintptr_t)va_arg(args, void *), 16, 'a'-'A');
        need_0x = 'x';
      } break;

#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
      case NPF_FMT_SPEC_CONV_WRITEBACK:
        switch (fs.length_modifier) {
          NPF_WRITEBACK(NONE, int);
          NPF_WRITEBACK(SHORT, short);
          NPF_WRITEBACK(LONG, long);
          NPF_WRITEBACK(LONG_DOUBLE, double);
          NPF_WRITEBACK(CHAR, signed char);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
          NPF_WRITEBACK(LARGE_LONG_LONG, long long);
          NPF_WRITEBACK(LARGE_INTMAX, intmax_t);
          NPF_WRITEBACK(LARGE_SIZET, size_t);
          NPF_WRITEBACK(LARGE_PTRDIFFT, ptrdiff_t);
#endif
          default: break;
        } break;
#endif

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      case NPF_FMT_SPEC_CONV_FLOAT_DECIMAL: {
        float val;
        if (fs.length_modifier == NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE) {
          val = (float)va_arg(args, long double);
        } else {
          val = (float)va_arg(args, double);
        }

        sign_c = (val < 0.f) ? '-' : fs.prepend;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
        zero = (val == 0.f);
#endif
        cbuf_len = npf_ftoa_rev(cbuf, val, fs.case_adjust, &frac_chars);

        if (cbuf_len < 0) {
          cbuf_len = -cbuf_len;
          inf_or_nan = 1;
        } else {
          int const prec_adj = npf_max(0, frac_chars - fs.prec);
          cbuf += prec_adj;
          cbuf_len -= prec_adj;
        }
      } break;
#endif
      default: break;
    }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // Compute the field width pad character
    if (fs.field_width_opt == NPF_FMT_SPEC_OPT_LITERAL) {
      if (fs.leading_zero_pad) { // '0' flag is only legal with numeric types
        if ((fs.conv_spec != NPF_FMT_SPEC_CONV_STRING) &&
            (fs.conv_spec != NPF_FMT_SPEC_CONV_CHAR) &&
            (fs.conv_spec != NPF_FMT_SPEC_CONV_PERCENT)) {
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
          if ((fs.prec_opt == NPF_FMT_SPEC_OPT_LITERAL) && !fs.prec && zero) {
            pad_c = ' ';
          } else
#endif
          { pad_c = '0'; }
        }
      } else { pad_c = ' '; }
    }
#endif

    // Compute the number of bytes to truncate or '0'-pad.
    if (fs.conv_spec != NPF_FMT_SPEC_CONV_STRING) {
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      if (!inf_or_nan) { // float precision is after the decimal point
        int const prec_start =
          (fs.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_DECIMAL) ? frac_chars : cbuf_len;
        prec_pad = npf_max(0, fs.prec - prec_start);
      }
#elif NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      prec_pad = npf_max(0, fs.prec - cbuf_len);
#endif
    }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // Given the full converted length, how many pad bytes?
    field_pad = fs.field_width - cbuf_len - !!sign_c;
    if (need_0x) { field_pad -= 2; }

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    if ((fs.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_DECIMAL) && !fs.prec && !fs.alt_form) {
      ++field_pad; // 0-pad, no decimal point.
    }
#endif
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    field_pad -= prec_pad;
#endif
    field_pad = npf_max(0, field_pad);
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // Apply right-justified field width if requested
    if (!fs.left_justified && pad_c) { // If leading zeros pad, sign goes first.
      if (pad_c == '0') {
        if (sign_c) { NPF_PUTC(sign_c); sign_c = 0; }
        // Pad byte is '0', write '0x' before '0' pad chars.
        if (need_0x) { NPF_PUTC('0'); NPF_PUTC(need_0x); }
      }
      while (field_pad-- > 0) { NPF_PUTC(pad_c); }
      // Pad byte is ' ', write '0x' after ' ' pad chars but before number.
      if ((pad_c != '0') && need_0x) { NPF_PUTC('0'); NPF_PUTC(need_0x); }
    } else
#endif
    { if (need_0x) { NPF_PUTC('0'); NPF_PUTC(need_0x); } } // no pad, '0x' requested.

    // Write the converted payload
    if (fs.conv_spec == NPF_FMT_SPEC_CONV_STRING) {
      for (int i = 0; i < cbuf_len; ++i) { NPF_PUTC(cbuf[i]); }
    } else {
      if (sign_c) { NPF_PUTC(sign_c); }
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      if (fs.conv_spec != NPF_FMT_SPEC_CONV_FLOAT_DECIMAL) {
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
        while (prec_pad-- > 0) { NPF_PUTC('0'); } // int precision leads.
#endif

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      } else {
        // if 0 precision, skip the fractional part and '.'
        // if 0 prec + alternative form, keep the '.'
        if (!fs.prec && !fs.alt_form) { ++cbuf; --cbuf_len; }
      }
#endif

#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
      if (fs.conv_spec == NPF_FMT_SPEC_CONV_BINARY) {
        while (cbuf_len) { NPF_PUTC('0' + ((u.binval >> --cbuf_len) & 1)); }
      } else
#endif
      { while (cbuf_len-- > 0) { NPF_PUTC(cbuf[cbuf_len]); } } // payload is reversed

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      // real precision comes after the number.
      if ((fs.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_DECIMAL) && !inf_or_nan) {
        while (prec_pad-- > 0) { NPF_PUTC('0'); }
      }
#endif
    }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    if (fs.left_justified && pad_c) { // Apply left-justified field width
      while (field_pad-- > 0) { NPF_PUTC(pad_c); }
    }
#endif
  }

  return pc_cnt.n;
}

#undef NPF_PUTC
#undef NPF_EXTRACT
#undef NPF_WRITEBACK

int npf_pprintf(npf_putc pc, void *pc_ctx, char const *format, ...) {
  va_list val;
  va_start(val, format);
  int const rv = npf_vpprintf(pc, pc_ctx, format, val);
  va_end(val);
  return rv;
}

int npf_snprintf(char *buffer, size_t bufsz, const char *format, ...) {
  va_list val;
  va_start(val, format);
  int const rv = npf_vsnprintf(buffer, bufsz, format, val);
  va_end(val);
  return rv;
}

int npf_vsnprintf(char *buffer, size_t bufsz, char const *format, va_list vlist) {
  npf_bufputc_ctx_t bufputc_ctx;
  bufputc_ctx.dst = buffer;
  bufputc_ctx.len = bufsz;
  bufputc_ctx.cur = 0;

  npf_putc const pc = buffer ? npf_bufputc : npf_bufputc_nop;
  int const n = npf_vpprintf(pc, &bufputc_ctx, format, vlist);
  pc('\0', &bufputc_ctx);

#ifdef NANOPRINTF_SNPRINTF_SAFE_EMPTY_STRING_ON_OVERFLOW
  if (bufsz && (n >= (int)bufsz)) { buffer[0] = '\0'; }
#elif defined(NANOPRINTF_SNPRINTF_SAFE_TRIM_STRING_ON_OVERFLOW)
  if (bufsz && (n >= (int)bufsz)) { buffer[bufsz - 1] = '\0'; }
#endif

  return n;
}

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

#endif // NANOPRINTF_IMPLEMENTATION_INCLUDED
#endif // NANOPRINTF_IMPLEMENTATION

