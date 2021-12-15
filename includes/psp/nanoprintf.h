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

#ifndef NANOPRINTF_IMPLEMENTATION

/* The interface of nanoprintf begins here, to be compiled only if
   NANOPRINTF_IMPLEMENTATION is not defined. In a multi-file library what
   follows would be the public-facing nanoprintf.h. */

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

// Compilers can warn when printf formatting strings are incorrect.
#if defined(__clang__)
  #define NPF_PRINTF_ATTR(FORMAT_INDEX, VARGS_INDEX) \
    __attribute__((__format__(__printf__, FORMAT_INDEX, VARGS_INDEX)))
#elif defined(__GNUC__) || defined(__GNUG__)
  #define NPF_PRINTF_ATTR(FORMAT_INDEX, VARGS_INDEX) \
    __attribute__((format(printf, FORMAT_INDEX, VARGS_INDEX)))
#else
  #define NPF_PRINTF_ATTR(FORMAT_INDEX, VARGS_INDEX)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Public API

NPF_VISIBILITY int npf_snprintf(char *buffer, size_t bufsz, const char *format,
                                ...) NPF_PRINTF_ATTR(3, 4);

NPF_VISIBILITY int npf_vsnprintf(char *buffer, size_t bufsz, char const *format,
                                 va_list vlist) NPF_PRINTF_ATTR(3, 0);

typedef void (*npf_putc)(int c, void *ctx);
NPF_VISIBILITY int npf_pprintf(npf_putc pc, void *pc_ctx, char const *format,
                               ...) NPF_PRINTF_ATTR(3, 4);

NPF_VISIBILITY int npf_vpprintf(npf_putc pc, void *pc_ctx, char const *format,
                                va_list vlist) NPF_PRINTF_ATTR(3, 0);

#ifdef NANOPRINTF_EXPOSE_INTERNALS

// Configuration

// Pick reasonable defaults if nothing's been configured.
#if !defined(NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS) &&   \
    !defined(NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS) &&       \
    !defined(NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS) &&       \
    !defined(NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS)
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#endif

// If anything's been configured, everything must be configured.
#ifndef NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

#ifndef NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

#ifndef NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

#ifndef NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

#ifndef NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

// Ensure flags are compatible.
#if (NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 0)
  #error Precision format specifiers must be enabled if float support is enabled.
#endif

// intmax_t / uintmax_t require stdint from c99 / c++11
#ifndef _MSC_VER
  #ifdef __cplusplus
    #if __cplusplus < 201103L
      #error nanoprintf requires C++11 or later.
    #endif
  #else
    #if __STDC_VERSION__ < 199409L
      #error nanoprintf requires C99 or later.
    #endif
  #endif
#endif

// Implementation Details (prototype / config helper functions)

#include <inttypes.h>
#include <stdint.h>

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
typedef enum {
  NPF_FMT_SPEC_FIELD_WIDTH_NONE,
  NPF_FMT_SPEC_FIELD_WIDTH_STAR,
  NPF_FMT_SPEC_FIELD_WIDTH_LITERAL
} npf_format_spec_field_width_t;
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
typedef enum {
  NPF_FMT_SPEC_PRECISION_NONE,
  NPF_FMT_SPEC_PRECISION_STAR,
  NPF_FMT_SPEC_PRECISION_LITERAL
} npf_format_spec_precision_t;
#endif

typedef enum {
  NPF_FMT_SPEC_LEN_MOD_NONE,
  NPF_FMT_SPEC_LEN_MOD_SHORT,       /* 'h' */
  NPF_FMT_SPEC_LEN_MOD_LONG,        /* 'l' */
  NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE, /* 'L' */
  NPF_FMT_SPEC_LEN_MOD_CHAR         /* 'hh' */
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  ,
  NPF_FMT_SPEC_LEN_MOD_LARGE_LONG_LONG, /* 'll' */
  NPF_FMT_SPEC_LEN_MOD_LARGE_INTMAX,    /* 'j' */
  NPF_FMT_SPEC_LEN_MOD_LARGE_SIZET,     /* 'z' */
  NPF_FMT_SPEC_LEN_MOD_LARGE_PTRDIFFT   /* 't' */
#endif
} npf_format_spec_length_modifier_t;

typedef enum {
  NPF_FMT_SPEC_CONV_PERCENT,      /* '%' */
  NPF_FMT_SPEC_CONV_CHAR,         /* 'c' */
  NPF_FMT_SPEC_CONV_STRING,       /* 's' */
  NPF_FMT_SPEC_CONV_SIGNED_INT,   /* 'i', 'd' */
  NPF_FMT_SPEC_CONV_OCTAL,        /* 'o' */
  NPF_FMT_SPEC_CONV_HEX_INT,      /* 'x', 'X' */
  NPF_FMT_SPEC_CONV_UNSIGNED_INT, /* 'u' */
  NPF_FMT_SPEC_CONV_POINTER       /* 'p' */
#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
  ,
  NPF_FMT_SPEC_CONV_WRITEBACK /* 'n' */
#endif
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
  ,
  NPF_FMT_SPEC_CONV_FLOAT_DECIMAL /* 'f', 'F' */
#endif
} npf_format_spec_conversion_t;

typedef enum {
  NPF_FMT_SPEC_CONV_CASE_LOWER,
  NPF_FMT_SPEC_CONV_CASE_UPPER
} npf_format_spec_conversion_case_t;

typedef struct {
  // optional flags
  char prepend_sign;     /* '+' */
  char prepend_space;    /* ' ' */
  char alternative_form; /* '#' */

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  // field width
  npf_format_spec_field_width_t field_width_type;
  int field_width;
  char left_justified;   /* '-' */
  char leading_zero_pad; /* '0' */
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
  // precision
  npf_format_spec_precision_t precision_type;
  int precision;
#endif

  // length modifier for specifying argument size
  npf_format_spec_length_modifier_t length_modifier;

  // conversion specifiers
  npf_format_spec_conversion_t conv_spec;
  npf_format_spec_conversion_case_t conv_spec_case;
} npf_format_spec_t;

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 0
  typedef long npf_int_t;
  typedef unsigned long npf_uint_t;
#else
  typedef intmax_t npf_int_t;
  typedef uintmax_t npf_uint_t;
#endif

NPF_VISIBILITY int npf_parse_format_spec(char const *format, npf_format_spec_t *out_spec);

typedef struct {
  char *dst;
  size_t len;
  size_t cur;
} npf_bufputc_ctx_t;

NPF_VISIBILITY void npf_bufputc(int c, void *ctx);
NPF_VISIBILITY void npf_bufputc_nop(int c, void *ctx);
NPF_VISIBILITY int npf_itoa_rev(char *buf, npf_int_t i);
NPF_VISIBILITY int npf_utoa_rev(char *buf, npf_uint_t i, unsigned base,
                                npf_format_spec_conversion_case_t cc);
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
NPF_VISIBILITY int npf_fsplit_abs(float f, uint64_t *out_int_part,
                                  uint64_t *out_frac_part,
                                  int *out_frac_base10_neg_e);
NPF_VISIBILITY int npf_ftoa_rev(char *buf, float f, unsigned base,
                                npf_format_spec_conversion_case_t cc,
                                int *out_frac_chars);
#endif

#endif // NANOPRINTF_EXPOSE_INTERNALS

#ifdef __cplusplus
}
#endif

#endif // NANOPRINTF_H_INCLUDED

#else // NANOPRINTF_IMPLEMENTATION

/* The implementation of nanoprintf begins here, to be compiled only if
   NANOPRINTF_IMPLEMENTATION is defined. In a multi-file library what follows would
   be nanoprintf.c. */

#undef NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_EXPOSE_INTERNALS
#include "nanoprintf.h"
#undef NANOPRINTF_EXPOSE_INTERNALS
#define NANOPRINTF_IMPLEMENTATION

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
  #include <math.h>
#endif

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  #ifdef _MSC_VER
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;
  #else
    #include <sys/types.h>
  #endif
#endif

#define NPF_MIN(x, y) ((x) < (y) ? (x) : (y))
#define NPF_MAX(x, y) ((x) > (y) ? (x) : (y))

int npf_parse_format_spec(char const *format, npf_format_spec_t *out_spec) {
  char const *cur = format;

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  out_spec->left_justified = 0;
  out_spec->leading_zero_pad = 0;
#endif
  out_spec->prepend_sign = 0;
  out_spec->prepend_space = 0;
  out_spec->alternative_form = 0;
  out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_NONE;

  // cur points at the leading '%' character
  while (*++cur) {
    switch (*cur) { // Optional flags
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
      case '-':
        out_spec->left_justified = 1;
        out_spec->leading_zero_pad = 0;
        continue;
      case '0':
        out_spec->leading_zero_pad = !out_spec->left_justified;
        continue;
#endif
      case '+':
        out_spec->prepend_sign = 1;
        out_spec->prepend_space = 0;
        continue;
      case ' ':
        out_spec->prepend_space = !out_spec->prepend_sign;
        continue;
      case '#':
        out_spec->alternative_form = 1;
        continue;
      default:
        break;
    }
    break;
  }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  // Minimum field width
  out_spec->field_width_type = NPF_FMT_SPEC_FIELD_WIDTH_NONE;
  if (*cur == '*') {
    // '*' modifiers require more varargs
    out_spec->field_width_type = NPF_FMT_SPEC_FIELD_WIDTH_STAR;
    ++cur;
  } else {
    out_spec->field_width = 0;
    if ((*cur >= '0') && (*cur <= '9')) {
      out_spec->field_width_type = NPF_FMT_SPEC_FIELD_WIDTH_LITERAL;
    }
    while ((*cur >= '0') && (*cur <= '9')) {
      out_spec->field_width = (out_spec->field_width * 10) + (*cur++ - '0');
    }
  }
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
  // Precision
  out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
  if (*cur == '.') {
    ++cur;
    if (*cur == '*') {
      out_spec->precision_type = NPF_FMT_SPEC_PRECISION_STAR;
      ++cur;
    } else if (*cur == '-') {
      /* ignore negative precision */
      out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
      ++cur;
      while ((*cur >= '0') && (*cur <= '9')) {
        ++cur;
      }
    } else {
      out_spec->precision = 0;
      out_spec->precision_type = NPF_FMT_SPEC_PRECISION_LITERAL;
      while ((*cur >= '0') && (*cur <= '9')) {
        out_spec->precision = (out_spec->precision * 10) + (*cur++ - '0');
      }
    }
  }
#endif

  // Length modifier
  switch (*cur++) {
    case 'h':
      if (*cur == 'h') {
        out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_CHAR;
        ++cur;
      } else {
        out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_SHORT;
      }
      break;
    case 'l':
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
      if (*cur == 'l') {
        out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_LONG_LONG;
        ++cur;
      } else
#endif
      out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LONG;
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
    default:
      --cur;
      break;
  }

  // Conversion specifier
  switch (*cur++) {
    case '%':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_PERCENT;
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
#endif
      break;
    case 'c':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_CHAR;
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
#endif
      break;
    case 's':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_STRING;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
      out_spec->leading_zero_pad = 0;
#endif
      break;
    case 'i':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_SIGNED_INT;
      break;
    case 'd':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_SIGNED_INT;
      break;
    case 'o':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_OCTAL;
      break;
    case 'x':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_HEX_INT;
      out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
      break;
    case 'X':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_HEX_INT;
      out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
      break;
    case 'u':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_UNSIGNED_INT;
      break;
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    case 'f':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_DECIMAL;
      out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
      break;
    case 'F':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_DECIMAL;
      out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
      break;
#endif
#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
    case 'n':
      /* todo: reject string if flags or width or precision exist */
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_WRITEBACK;
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
#endif
      break;
#endif
    case 'p':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_POINTER;
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
#endif
      break;
    default:
      return 0;
  }

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
  if ((out_spec->precision_type == NPF_FMT_SPEC_PRECISION_NONE) ||
      (out_spec->precision_type == NPF_FMT_SPEC_PRECISION_STAR)) {
    switch (out_spec->conv_spec) {
      case NPF_FMT_SPEC_CONV_PERCENT:
      case NPF_FMT_SPEC_CONV_CHAR:
      case NPF_FMT_SPEC_CONV_STRING:
      case NPF_FMT_SPEC_CONV_POINTER:
#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
      case NPF_FMT_SPEC_CONV_WRITEBACK:
#endif
        out_spec->precision = 0;
        break;
      case NPF_FMT_SPEC_CONV_SIGNED_INT:
      case NPF_FMT_SPEC_CONV_OCTAL:
      case NPF_FMT_SPEC_CONV_HEX_INT:
      case NPF_FMT_SPEC_CONV_UNSIGNED_INT:
        out_spec->precision = 1;
        break;
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      case NPF_FMT_SPEC_CONV_FLOAT_DECIMAL:
        out_spec->precision = 6;
        break;
#endif
      default:
        break;
    }
  }
#endif

  return (int)(cur - format);
}

void npf_bufputc(int c, void *ctx) {
  npf_bufputc_ctx_t *bpc = (npf_bufputc_ctx_t *)ctx;
  if (bpc->cur < bpc->len - 1) {
    bpc->dst[bpc->cur++] = (char)c;
  }
}

void npf_bufputc_nop(int c, void *ctx) {
  (void)c;
  (void)ctx;
}

int npf_itoa_rev(char *buf, npf_int_t i) {
  char *dst = buf;
  if (i == 0) {
    *dst++ = '0';
  } else {
    int const neg = (i < 0) ? -1 : 1;
    while (i) {
      *dst++ = (char)('0' + (neg * (i % 10)));
      i /= 10;
    }
  }
  return (int)(dst - buf);
}

int npf_utoa_rev(char *buf, npf_uint_t i, unsigned base,
                 npf_format_spec_conversion_case_t cc) {
  char *dst = buf;
  if (i == 0) {
    *dst++ = '0';
  } else {
    unsigned const base_c = (cc == NPF_FMT_SPEC_CONV_CASE_LOWER) ? 'a' : 'A';
    while (i) {
      unsigned const d = (unsigned)(i % base);
      i /= base;
      *dst++ = (d < 10) ? (char)('0' + d) : (char)(base_c + (d - 10));
    }
  }
  return (int)(dst - buf);
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

  // union-cast is UB, so copy through char*, compiler can optimize.
  uint32_t f_bits; {
    char const *src = (char const *)&f;
    char *dst = (char *)&f_bits;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
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

int npf_ftoa_rev(char *buf, float f, unsigned base,
                 npf_format_spec_conversion_case_t cc, int *out_frac_chars) {
  char const case_c = (cc == NPF_FMT_SPEC_CONV_CASE_LOWER) ? 'a' - 'A' : 0;

  if (f != f) {
    *buf++ = (char)('N' + case_c);
    *buf++ = (char)('A' + case_c);
    *buf++ = (char)('N' + case_c);
    return -3;
  }
  if (f == INFINITY) {
    *buf++ = (char)('F' + case_c);
    *buf++ = (char)('N' + case_c);
    *buf++ = (char)('I' + case_c);
    return -3;
  }

  uint64_t int_part, frac_part;
  int frac_base10_neg_exp;
  if (npf_fsplit_abs(f, &int_part, &frac_part, &frac_base10_neg_exp) == 0) {
    *buf++ = (char)('R' + case_c);
    *buf++ = (char)('O' + case_c);
    *buf++ = (char)('O' + case_c);
    return -3;
  }

  unsigned const base_c = (cc == NPF_FMT_SPEC_CONV_CASE_LOWER) ? 'a' : 'A';
  char *dst = buf;

  // write the fractional digits
  while (frac_part) {
    unsigned const d = (unsigned)(frac_part % base);
    frac_part /= base;
    *dst++ = (d < 10) ? (char)('0' + d) : (char)(base_c + (d - 10));
  }

  // write the 0 digits between the . and the first fractional digit
  while (frac_base10_neg_exp-- > 0) { *dst++ = '0'; }

  *out_frac_chars = (int)(dst - buf);
  *dst++ = '.';

  // write the integer digits
  if (int_part == 0) {
    *dst++ = '0';
  } else {
    while (int_part) {
      unsigned const d = (unsigned)(int_part % base);
      int_part /= base;
      *dst++ = (d < 10) ? (char)('0' + d) : (char)(base_c + (d - 10));
    }
  }
  return (int)(dst - buf);
}
#endif

#define NPF_PUTC(VAL) do { pc((VAL), pc_ctx); ++n; } while (0)

#define NPF_EXTRACT(MOD, CAST_TO, EXTRACT_AS) \
  case NPF_FMT_SPEC_LEN_MOD_##MOD: val = (CAST_TO)va_arg(vlist, EXTRACT_AS); break

#define NPF_WRITEBACK(MOD, TYPE) \
  case NPF_FMT_SPEC_LEN_MOD_##MOD: *(va_arg(vlist, TYPE *)) = (TYPE)n; break

int npf_vpprintf(npf_putc pc, void *pc_ctx, char const *format, va_list vlist) {
  npf_format_spec_t fs;
  char const *cur = format;
  int n = 0, sign = 0, i;

  while (*cur) {
    if (*cur != '%') { // Non-format character, write directly
      NPF_PUTC(*cur++);
      continue;
    }

    // Might be a format run, try to parse
    int const fs_len = npf_parse_format_spec(cur, &fs);
    if (fs_len == 0) { // Invalid format specifier, write and continue
      NPF_PUTC(*cur++);
      continue;
    }

    // Format specifier, convert and write argument
    char cbuf_mem[32], *cbuf = cbuf_mem, sign_c;
    int cbuf_len = 0;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    int field_pad = 0;
    char pad_c;
#endif
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    int prec_pad = 0;
#endif
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    int frac_chars = 0, inf_or_nan = 0;
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    if (fs.field_width_type == NPF_FMT_SPEC_FIELD_WIDTH_STAR) {
      // If '*' was used as field width, read it from args.
      int const field_width = va_arg(vlist, int);
      fs.field_width_type = NPF_FMT_SPEC_FIELD_WIDTH_LITERAL;
      if (field_width >= 0) {
        fs.field_width = field_width;
      } else { // Negative field width is left-justified.
        fs.field_width = -field_width;
        fs.left_justified = 1;
      }
    }
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    if (fs.precision_type == NPF_FMT_SPEC_PRECISION_STAR) {
      // If '*' was used as precision, read from args.
      int const precision = va_arg(vlist, int);
      if (precision >= 0) {
        fs.precision_type = NPF_FMT_SPEC_PRECISION_LITERAL;
        fs.precision = precision;
      } else { // Negative precision is ignored.
      fs.precision_type = NPF_FMT_SPEC_PRECISION_NONE;
      }
    }
#endif

    // Convert the argument to string and point cbuf at it
    switch (fs.conv_spec) {
      case NPF_FMT_SPEC_CONV_PERCENT:
        *cbuf = '%';
        cbuf_len = 1;
        break;

      case NPF_FMT_SPEC_CONV_CHAR: // 'c'
        *cbuf = (char)va_arg(vlist, int);
        cbuf_len = 1;
        break;

      case NPF_FMT_SPEC_CONV_STRING: { // 's'
        char *s = va_arg(vlist, char *);
        cbuf = s;
        while (*s) ++s;
        cbuf_len = (int)(s - cbuf);
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
        if (fs.precision_type == NPF_FMT_SPEC_PRECISION_LITERAL) {
          // precision modifier truncates strings
          cbuf_len = NPF_MIN(fs.precision, cbuf_len);
        }
#endif
      } break;

      case NPF_FMT_SPEC_CONV_SIGNED_INT: { /* 'i', 'd' */
        npf_int_t val = 0;
        switch (fs.length_modifier) {
          NPF_EXTRACT(NONE, int, int);
          NPF_EXTRACT(SHORT, short, int);
          NPF_EXTRACT(LONG, long, long);
          NPF_EXTRACT(LONG_DOUBLE, int, int);
          NPF_EXTRACT(CHAR, char, int);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
          NPF_EXTRACT(LARGE_LONG_LONG, long long, long long);
          NPF_EXTRACT(LARGE_INTMAX, intmax_t, intmax_t);
          NPF_EXTRACT(LARGE_SIZET, ssize_t, ssize_t);
          NPF_EXTRACT(LARGE_PTRDIFFT, ptrdiff_t, ptrdiff_t);
#endif
          default:
            break;
        }

        sign = (val < 0) ? -1 : 1;

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
        // special case, if prec and value are 0, skip
        if (!val && !fs.precision &&
            (fs.precision_type == NPF_FMT_SPEC_PRECISION_LITERAL)) {
          cbuf_len = 0;
        } else
#endif
        { cbuf_len = npf_itoa_rev(cbuf, val); }
      } break;

      case NPF_FMT_SPEC_CONV_OCTAL:          // 'o'
      case NPF_FMT_SPEC_CONV_HEX_INT:        // 'x', 'X'
      case NPF_FMT_SPEC_CONV_UNSIGNED_INT: { // 'u'
        sign = 0;
        unsigned const base = (fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL) ?
          8 : ((fs.conv_spec == NPF_FMT_SPEC_CONV_HEX_INT) ? 16 : 10);
        npf_uint_t val = 0;

        switch (fs.length_modifier) {
          NPF_EXTRACT(NONE, unsigned, unsigned);
          NPF_EXTRACT(SHORT, unsigned short, unsigned);
          NPF_EXTRACT(LONG, unsigned long, unsigned long);
          NPF_EXTRACT(LONG_DOUBLE, unsigned, unsigned);
          NPF_EXTRACT(CHAR, unsigned char, unsigned);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
          NPF_EXTRACT(LARGE_LONG_LONG, unsigned long long, unsigned long long);
          NPF_EXTRACT(LARGE_INTMAX, uintmax_t, uintmax_t);
          NPF_EXTRACT(LARGE_SIZET, size_t, size_t);
          NPF_EXTRACT(LARGE_PTRDIFFT, size_t, size_t);
#endif
          default:
            break;
        }

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
        if (!val && !fs.precision) {
          if ((fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL) && fs.alternative_form) {
            fs.precision = 1; // octal special case, print a single '0'
          } else if (fs.precision_type == NPF_FMT_SPEC_PRECISION_LITERAL) {
            cbuf_len = 0; // 0 value + 0 precision, print nothing
          }
        } else
#endif
        { cbuf_len = npf_utoa_rev(cbuf, val, base, fs.conv_spec_case); }

        // alt form adds '0' octal or '0x' hex prefix
        if (val && fs.alternative_form) {
          if (fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL) {
            cbuf[cbuf_len++] = '0';
          } else if (fs.conv_spec == NPF_FMT_SPEC_CONV_HEX_INT) {
            cbuf[cbuf_len++] = (fs.conv_spec_case == NPF_FMT_SPEC_CONV_CASE_LOWER) ?
              'x' : 'X';
            cbuf[cbuf_len++] = '0';
          }
        }
      } break;

      case NPF_FMT_SPEC_CONV_POINTER: { // 'p'
        cbuf_len = npf_utoa_rev(cbuf, (npf_uint_t)(uintptr_t)va_arg(vlist, void *),
          16, NPF_FMT_SPEC_CONV_CASE_LOWER);
        cbuf[cbuf_len++] = 'x';
        cbuf[cbuf_len++] = '0';
      } break;

#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
      case NPF_FMT_SPEC_CONV_WRITEBACK: /* 'n' */
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
          default:
            break;
        } break;
#endif

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      case NPF_FMT_SPEC_CONV_FLOAT_DECIMAL: { // 'f', 'F'
        float val;
        if (fs.length_modifier == NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE) {
          val = (float)va_arg(vlist, long double);
        } else {
          val = (float)va_arg(vlist, double);
        }

        sign = (val < 0) ? -1 : 1;
        cbuf_len = npf_ftoa_rev(cbuf, val, 10, fs.conv_spec_case, &frac_chars);

        if (cbuf_len < 0) {
          cbuf_len = -cbuf_len;
          inf_or_nan = 1;
        } else {
          if (frac_chars > fs.precision) { // truncate low frac digits for precision
            cbuf += (frac_chars - fs.precision);
            cbuf_len -= (frac_chars - fs.precision);
          }
        }
      } break;
#endif
      default:
        break;
    }

    // Compute the leading symbol (+, -, ' ')
    sign_c = 0;
    if (sign == -1) {
      sign_c = '-';
    } else if (sign == 1) {
      if (fs.prepend_sign) { sign_c = '+'; }
      else if (fs.prepend_space) { sign_c = ' '; }
    }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // Compute the field width pad character
    pad_c = 0;
    if (fs.field_width_type == NPF_FMT_SPEC_FIELD_WIDTH_LITERAL) {
      if (fs.leading_zero_pad) { // '0' flag is only legal with numeric types
        if ((fs.conv_spec != NPF_FMT_SPEC_CONV_STRING) &&
            (fs.conv_spec != NPF_FMT_SPEC_CONV_CHAR) &&
            (fs.conv_spec != NPF_FMT_SPEC_CONV_PERCENT)) {
          pad_c = '0';
        }
      } else {
        pad_c = ' ';
      }
    }
#endif

    // Compute the number of bytes to truncate or '0'-pad.
    if (fs.conv_spec != NPF_FMT_SPEC_CONV_STRING) {
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      if (!inf_or_nan) { // float precision is after the decimal point
        int const precision_start =
          (fs.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_DECIMAL) ? frac_chars : cbuf_len;
        prec_pad = NPF_MAX(0, fs.precision - precision_start);
      }
#elif NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      prec_pad = NPF_MAX(0, fs.precision - cbuf_len);
#endif
    }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // Given the full converted length, how many pad bytes?
    field_pad = fs.field_width - cbuf_len - !!sign_c;
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    if (fs.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_DECIMAL) {
      field_pad += (!fs.precision && !fs.alternative_form); // 0-pad, no decimal point.
    }
#endif // NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    field_pad -= prec_pad;
#endif // NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS
    field_pad = NPF_MAX(0, field_pad);
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // Apply right-justified field width if requested
    if (!fs.left_justified && pad_c) { // If leading zeros pad, sign goes first.
      if ((sign_c == '-' || sign_c == '+') && pad_c == '0') {
        NPF_PUTC(sign_c);
        sign_c = 0;
      }
      while (field_pad-- > 0) { NPF_PUTC(pad_c); }
    }
#endif
    // Write the converted payload
    if (fs.conv_spec == NPF_FMT_SPEC_CONV_STRING) {
      for (i = 0; i < cbuf_len; ++i) {
        NPF_PUTC(cbuf[i]);
      }
    } else {
      if (sign_c) { NPF_PUTC(sign_c); }
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      if (fs.conv_spec != NPF_FMT_SPEC_CONV_FLOAT_DECIMAL) {
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
        // integral precision comes before the number.
        while (prec_pad-- > 0) { NPF_PUTC('0'); }
#endif

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      } else {
        // if 0 precision, skip the fractional part and '.'
        // if 0 prec + alternative form, keep the '.'
        if (fs.precision == 0) {
          cbuf += !fs.alternative_form;
          cbuf_len -= !fs.alternative_form;
        }
      }
#endif
      // *toa_rev leaves payloads reversed
      while (cbuf_len-- > 0) { NPF_PUTC(cbuf[cbuf_len]); }

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      // real precision comes after the number.
      if ((fs.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_DECIMAL) && !inf_or_nan) {
        while (prec_pad-- > 0) { NPF_PUTC('0'); }
      }
#endif
    }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // Apply left-justified field width if requested
    if (fs.left_justified && pad_c) {
      while (field_pad-- > 0) { NPF_PUTC(pad_c); }
    }
#endif
    cur += fs_len;
  }

  NPF_PUTC('\0');
  return n - 1;
}

#undef NPF_PUTC
#undef NPF_EXTRACT
#undef NPF_WRITEBACK

int npf_pprintf(npf_putc pc, void *pc_ctx, char const *format, ...) {
  va_list val;
  int rv;
  va_start(val, format);
  rv = npf_vpprintf(pc, pc_ctx, format, val);
  va_end(val);
  return rv;
}

int npf_snprintf(char *buffer, size_t bufsz, const char *format, ...) {
  va_list val;
  int rv;
  va_start(val, format);
  rv = npf_vsnprintf(buffer, bufsz, format, val);
  va_end(val);
  return rv;
}

int npf_vsnprintf(char *buffer, size_t bufsz, char const *format, va_list vlist) {
  npf_bufputc_ctx_t bufputc_ctx;
  bufputc_ctx.dst = buffer;
  bufputc_ctx.len = bufsz;
  bufputc_ctx.cur = 0;
  if (buffer && bufsz) { buffer[bufsz - 1] = 0; }

  return
    npf_vpprintf(buffer ? npf_bufputc : npf_bufputc_nop, &bufputc_ctx, format, vlist);
}

#endif // NANOPRINTF_IMPLEMENTATION
