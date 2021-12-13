/*  minIni - Multi-Platform INI file parser, suitable for embedded systems
 *
 *  These routines are in part based on the article "Multiplatform .INI Files"
 *  by Joseph J. Graf in the March 1994 issue of Dr. Dobb's Journal.
 *
 *  Copyright (c) CompuPhase, 2008-2021
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy
 *  of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations
 *  under the License.
 *
 *  Version: $Id: minIni.c 53 2015-01-18 13:35:11Z thiadmer.riemersma@gmail.com $
 */

#if (defined _UNICODE || defined __UNICODE__ || defined UNICODE) && !defined INI_ANSIONLY
# if !defined UNICODE   /* for Windows */
#   define UNICODE
# endif
# if !defined _UNICODE  /* for C library */
#   define _UNICODE
# endif
#endif

#define MININI_IMPLEMENTATION
#include "minIni.h"
#if defined NDEBUG
  #define assert(e)
#else
  #include <assert.h>
#endif

#if !defined __T || defined INI_ANSIONLY
  #include <ctype.h>
  #include <string.h>
  #include <stdlib.h>
  #define TCHAR     char
  #define __T(s)    s
  #define _tcscat   strcat
  #define _tcschr   strchr
  #define _tcscmp   strcmp
  #define _tcscpy   strcpy
  #define _tcsicmp  stricmp
  #define _tcslen   strlen
  #define _tcsncmp  strncmp
  #define _tcsnicmp strnicmp
  #define _tcsrchr  strrchr
  #define _tcstol   strtol
  #define _tcstod   strtod
  #define _totupper toupper
  #define _stprintf sprintf
  #define _tfgets   fgets
  #define _tfputs   fputs
  #define _tfopen   fopen
  #define _tremove  remove
  #define _trename  rename
#endif

#if defined __linux || defined __linux__
  #define __LINUX__
#elif defined FREEBSD && !defined __FreeBSD__
  #define __FreeBSD__
#elif defined(_MSC_VER)
  #pragma warning(disable: 4996)	/* for Microsoft Visual C/C++ */
#endif
#if !defined strnicmp && !defined PORTABLE_STRNICMP
  #if defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__ || defined __NetBSD__ || defined __DragonFly__ || defined __GNUC__
    #define strnicmp  strncasecmp
  #endif
#endif
#if !defined _totupper
  #define _totupper toupper
#endif

#if !defined INI_LINETERM
  #if defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__ || defined __NetBSD__ || defined __DragonFly__
    #define INI_LINETERM    __T("\n")
  #else
    #define INI_LINETERM    __T("\r\n")
  #endif
#endif
#if !defined INI_FILETYPE
  #error Missing definition for INI_FILETYPE.
#endif

#if !defined sizearray
  #define sizearray(a)    (sizeof(a) / sizeof((a)[0]))
#endif

enum quote_option {
  QUOTE_NONE,
  QUOTE_ENQUOTE,
  QUOTE_DEQUOTE,
};

#if defined PORTABLE_STRNICMP
int strnicmp(const TCHAR *s1, const TCHAR *s2, size_t n)
{
  while (n-- != 0 && (*s1 || *s2)) {
    register int c1, c2;
    c1 = *s1++;
    if ('a' <= c1 && c1 <= 'z')
      c1 += ('A' - 'a');
    c2 = *s2++;
    if ('a' <= c2 && c2 <= 'z')
      c2 += ('A' - 'a');
    if (c1 != c2)
      return c1 - c2;
  }
  return 0;
}
#endif /* PORTABLE_STRNICMP */

static TCHAR *skipleading(const TCHAR *str)
{
  assert(str != NULL);
  while ('\0' < *str && *str <= ' ')
    str++;
  return (TCHAR *)str;
}

static TCHAR *skiptrailing(const TCHAR *str, const TCHAR *base)
{
  assert(str != NULL);
  assert(base != NULL);
  while (str > base && '\0' < *(str-1) && *(str-1) <= ' ')
    str--;
  return (TCHAR *)str;
}

static TCHAR *striptrailing(TCHAR *str)
{
  TCHAR *ptr = skiptrailing(_tcschr(str, '\0'), str);
  assert(ptr != NULL);
  *ptr = '\0';
  return str;
}

static TCHAR *ini_strncpy(TCHAR *dest, const TCHAR *source, size_t maxlen, enum quote_option option)
{
  size_t d, s;

  assert(maxlen>0);
  assert(source != NULL && dest != NULL);
  assert((dest < source || (dest == source && option != QUOTE_ENQUOTE)) || dest > source + strlen(source));
  if (option == QUOTE_ENQUOTE && maxlen < 3)
    option = QUOTE_NONE;  /* cannot store two quotes and a terminating zero in less than 3 characters */

  switch (option) {
  case QUOTE_NONE:
    for (d = 0; d < maxlen - 1 && source[d] != '\0'; d++)
      dest[d] = source[d];
    assert(d < maxlen);
    dest[d] = '\0';
    break;
  case QUOTE_ENQUOTE:
    d = 0;
    dest[d++] = '"';
    for (s = 0; source[s] != '\0' && d < maxlen - 2; s++, d++) {
      if (source[s] == '"') {
        if (d >= maxlen - 3)
          break;  /* no space to store the escape character plus the one that follows it */
        dest[d++] = '\\';
      }
      dest[d] = source[s];
    }
    dest[d++] = '"';
    dest[d] = '\0';
    break;
  case QUOTE_DEQUOTE:
    for (d = s = 0; source[s] != '\0' && d < maxlen - 1; s++, d++) {
      if ((source[s] == '"' || source[s] == '\\') && source[s + 1] == '"')
        s++;
      dest[d] = source[s];
    }
    dest[d] = '\0';
    break;
  default:
    assert(0);
  }

  return dest;
}

static TCHAR *cleanstring(TCHAR *string, enum quote_option *quotes)
{
  int isstring;
  TCHAR *ep;

  assert(string != NULL);
  assert(quotes != NULL);

  /* Remove a trailing comment */
  isstring = 0;
  for (ep = string; *ep != '\0' && ((*ep != ';' && *ep != '#') || isstring); ep++) {
    if (*ep == '"') {
      if (*(ep + 1) == '"')
        ep++;                 /* skip "" (both quotes) */
      else
        isstring = !isstring; /* single quote, toggle isstring */
    } else if (*ep == '\\' && *(ep + 1) == '"') {
      ep++;                   /* skip \" (both quotes */
    }
  }
  assert(ep != NULL && (*ep == '\0' || *ep == ';' || *ep == '#'));
  *ep = '\0';                 /* terminate at a comment */
  striptrailing(string);
  /* Remove double quotes surrounding a value */
  *quotes = QUOTE_NONE;
  if (*string == '"' && (ep = _tcschr(string, '\0')) != NULL && *(ep - 1) == '"') {
    string++;
    *--ep = '\0';
    *quotes = QUOTE_DEQUOTE;  /* this is a string, so remove escaped characters */
  }
  return string;
}

static int getkeystring(INI_FILETYPE *fp, const TCHAR *Section, const TCHAR *Key,
                        int idxSection, int idxKey, TCHAR *Buffer, int BufferSize,
                        INI_FILEPOS *mark)
{
  TCHAR *sp, *ep;
  int len, idx;
  enum quote_option quotes;
  TCHAR LocalBuffer[INI_BUFFERSIZE];

  assert(fp != NULL);
  /* Move through file 1 line at a time until a section is matched or EOF. If
   * parameter Section is NULL, only look at keys above the first section. If
   * idxSection is positive, copy the relevant section name.
   */
  len = (Section != NULL) ? (int)_tcslen(Section) : 0;
  if (len > 0 || idxSection >= 0) {
    assert(idxSection >= 0 || Section != NULL);
    idx = -1;
    do {
      do {
        if (!ini_read(LocalBuffer, INI_BUFFERSIZE, fp))
          return 0;
        sp = skipleading(LocalBuffer);
        ep = _tcsrchr(sp, ']');
      } while (*sp != '[' || ep == NULL);
      /* When arrived here, a section was found; now optionally skip leading and
       * trailing whitespace.
       */
      assert(sp != NULL && *sp == '[');
      sp = skipleading(sp + 1);
      assert(ep != NULL && *ep == ']');
      ep = skiptrailing(ep, sp);
    } while ((((int)(ep-sp) != len || Section == NULL || _tcsnicmp(sp, Section, len) != 0) && ++idx != idxSection));
    if (idxSection >= 0) {
      if (idx == idxSection) {
        assert(ep != NULL);
        *ep = '\0'; /* the end of the section name was found earlier */
        ini_strncpy(Buffer, sp, BufferSize, QUOTE_NONE);
        return 1;
      }
      return 0; /* no more section found */
    }
  }

  /* Now that the section has been found, find the entry.
   * Stop searching upon leaving the section's area.
   */
  assert(Key != NULL || idxKey >= 0);
  len = (Key != NULL) ? (int)_tcslen(Key) : 0;
  idx = -1;
  do {
    if (mark != NULL)
      ini_tell(fp, mark);   /* optionally keep the mark to the start of the line */
    if (!ini_read(LocalBuffer,INI_BUFFERSIZE,fp) || *(sp = skipleading(LocalBuffer)) == '[')
      return 0;
    sp = skipleading(LocalBuffer);
    ep = _tcschr(sp, '=');  /* Parse out the equal sign */
    if (ep == NULL)
      ep = _tcschr(sp, ':');
  } while (*sp == ';' || *sp == '#' || ep == NULL
           || ((len == 0 || (int)(skiptrailing(ep,sp)-sp) != len || _tcsnicmp(sp,Key,len) != 0) && ++idx != idxKey));
  if (idxKey >= 0) {
    if (idx == idxKey) {
      assert(ep != NULL);
      assert(*ep == '=' || *ep == ':');
      *ep = '\0';
      striptrailing(sp);
      ini_strncpy(Buffer, sp, BufferSize, QUOTE_NONE);
      return 1;
    }
    return 0;   /* no more key found (in this section) */
  }

  /* Copy up to BufferSize chars to buffer */
  assert(ep != NULL);
  assert(*ep == '=' || *ep == ':');
  sp = skipleading(ep + 1);
  sp = cleanstring(sp, &quotes);  /* Remove a trailing comment */
  ini_strncpy(Buffer, sp, BufferSize, quotes);
  return 1;
}

/** ini_gets()
 * \param Section     the name of the section to search for
 * \param Key         the name of the entry to find the value of
 * \param DefValue    default string in the event of a failed read
 * \param Buffer      a pointer to the buffer to copy into
 * \param BufferSize  the maximum number of characters to copy
 * \param Filename    the name and full path of the .ini file to read from
 *
 * \return            the number of characters copied into the supplied buffer
 */
int ini_gets(const TCHAR *Section, const TCHAR *Key, const TCHAR *DefValue,
             TCHAR *Buffer, int BufferSize, const TCHAR *Filename)
{
  INI_FILETYPE fp;
  int ok = 0;

  if (Buffer == NULL || BufferSize <= 0 || Key == NULL)
    return 0;
  if (ini_openread(Filename, &fp)) {
    ok = getkeystring(&fp, Section, Key, -1, -1, Buffer, BufferSize, NULL);
    (void)ini_close(&fp);
  }
  if (!ok)
    ini_strncpy(Buffer, (DefValue != NULL) ? DefValue : __T(""), BufferSize, QUOTE_NONE);
  return (int)_tcslen(Buffer);
}

/** ini_getl()
 * \param Section     the name of the section to search for
 * \param Key         the name of the entry to find the value of
 * \param DefValue    the default value in the event of a failed read
 * \param Filename    the name of the .ini file to read from
 *
 * \return            the value located at Key
 */
long ini_getl(const TCHAR *Section, const TCHAR *Key, long DefValue, const TCHAR *Filename)
{
  TCHAR LocalBuffer[64];
  int len = ini_gets(Section, Key, __T(""), LocalBuffer, sizearray(LocalBuffer), Filename);
  return (len == 0) ? DefValue
                    : ((len >= 2 && _totupper((int)LocalBuffer[1]) == 'X') ? _tcstol(LocalBuffer, NULL, 16)
                                                                           : _tcstol(LocalBuffer, NULL, 10));
}

#if defined INI_REAL
/** ini_getf()
 * \param Section     the name of the section to search for
 * \param Key         the name of the entry to find the value of
 * \param DefValue    the default value in the event of a failed read
 * \param Filename    the name of the .ini file to read from
 *
 * \return            the value located at Key
 */
INI_REAL ini_getf(const TCHAR *Section, const TCHAR *Key, INI_REAL DefValue, const TCHAR *Filename)
{
  TCHAR LocalBuffer[64];
  int len = ini_gets(Section, Key, __T(""), LocalBuffer, sizearray(LocalBuffer), Filename);
  return (len == 0) ? DefValue : ini_atof(LocalBuffer);
}
#endif

/** ini_getbool()
 * \param Section     the name of the section to search for
 * \param Key         the name of the entry to find the value of
 * \param DefValue    default value in the event of a failed read; it should
 *                    zero (0) or one (1).
 * \param Filename    the name and full path of the .ini file to read from
 *
 * A true boolean is found if one of the following is matched:
 * - A string starting with 'y' or 'Y'
 * - A string starting with 't' or 'T'
 * - A string starting with '1'
 *
 * A false boolean is found if one of the following is matched:
 * - A string starting with 'n' or 'N'
 * - A string starting with 'f' or 'F'
 * - A string starting with '0'
 *
 * \return            the true/false flag as interpreted at Key
 */
int ini_getbool(const TCHAR *Section, const TCHAR *Key, int DefValue, const TCHAR *Filename)
{
  TCHAR LocalBuffer[2] = __T("");
  int ret;

  ini_gets(Section, Key, __T(""), LocalBuffer, sizearray(LocalBuffer), Filename);
  LocalBuffer[0] = (TCHAR)_totupper((int)LocalBuffer[0]);
  if (LocalBuffer[0] == 'Y' || LocalBuffer[0] == '1' || LocalBuffer[0] == 'T')
    ret = 1;
  else if (LocalBuffer[0] == 'N' || LocalBuffer[0] == '0' || LocalBuffer[0] == 'F')
    ret = 0;
  else
    ret = DefValue;

  return(ret);
}

/** ini_getsection()
 * \param idx         the zero-based sequence number of the section to return
 * \param Buffer      a pointer to the buffer to copy into
 * \param BufferSize  the maximum number of characters to copy
 * \param Filename    the name and full path of the .ini file to read from
 *
 * \return            the number of characters copied into the supplied buffer
 */
int  ini_getsection(int idx, TCHAR *Buffer, int BufferSize, const TCHAR *Filename)
{
  INI_FILETYPE fp;
  int ok = 0;

  if (Buffer == NULL || BufferSize <= 0 || idx < 0)
    return 0;
  if (ini_openread(Filename, &fp)) {
    ok = getkeystring(&fp, NULL, NULL, idx, -1, Buffer, BufferSize, NULL);
    (void)ini_close(&fp);
  }
  if (!ok)
    *Buffer = '\0';
  return (int)_tcslen(Buffer);
}

/** ini_getkey()
 * \param Section     the name of the section to browse through, or NULL to
 *                    browse through the keys outside any section
 * \param idx         the zero-based sequence number of the key to return
 * \param Buffer      a pointer to the buffer to copy into
 * \param BufferSize  the maximum number of characters to copy
 * \param Filename    the name and full path of the .ini file to read from
 *
 * \return            the number of characters copied into the supplied buffer
 */
int  ini_getkey(const TCHAR *Section, int idx, TCHAR *Buffer, int BufferSize, const TCHAR *Filename)
{
  INI_FILETYPE fp;
  int ok = 0;

  if (Buffer == NULL || BufferSize <= 0 || idx < 0)
    return 0;
  if (ini_openread(Filename, &fp)) {
    ok = getkeystring(&fp, Section, NULL, -1, idx, Buffer, BufferSize, NULL);
    (void)ini_close(&fp);
  }
  if (!ok)
    *Buffer = '\0';
  return (int)_tcslen(Buffer);
}

/** ini_hassection()
 * \param Section     the name of the section to search for
 * \param Filename    the name of the .ini file to read from
 *
 * \return            1 if the section is found, 0 if not found
 */
int ini_hassection(const mTCHAR *Section, const mTCHAR *Filename)
{
  TCHAR LocalBuffer[32];  /* dummy buffer */
  INI_FILETYPE fp;
  int ok = 0;

  if (ini_openread(Filename, &fp)) {
    ok = getkeystring(&fp, Section, NULL, -1, 0, LocalBuffer, sizearray(LocalBuffer), NULL);
    (void)ini_close(&fp);
  }
  return ok;
}

/** ini_haskey()
 * \param Section     the name of the section to search for
 * \param Key         the name of the entry to find the value of
 * \param Filename    the name of the .ini file to read from
 *
 * \return            1 if the key is found, 0 if not found
 */
int ini_haskey(const mTCHAR *Section, const mTCHAR *Key, const mTCHAR *Filename)
{
  TCHAR LocalBuffer[32];  /* dummy buffer */
  INI_FILETYPE fp;
  int ok = 0;

  if (ini_openread(Filename, &fp)) {
    ok = getkeystring(&fp, Section, Key, -1, -1, LocalBuffer, sizearray(LocalBuffer), NULL);
    (void)ini_close(&fp);
  }
  return ok;
}


#if !defined INI_NOBROWSE
/** ini_browse()
 * \param Callback    a pointer to a function that will be called for every
 *                    setting in the INI file.
 * \param UserData    arbitrary data, which the function passes on the
 *                    \c Callback function
 * \param Filename    the name and full path of the .ini file to read from
 *
 * \return            1 on success, 0 on failure (INI file not found)
 *
 * \note              The \c Callback function must return 1 to continue
 *                    browsing through the INI file, or 0 to stop. Even when the
 *                    callback stops the browsing, this function will return 1
 *                    (for success).
 */
int ini_browse(INI_CALLBACK Callback, void *UserData, const TCHAR *Filename)
{
  TCHAR LocalBuffer[INI_BUFFERSIZE];
  int lenSec, lenKey;
  enum quote_option quotes;
  INI_FILETYPE fp;

  if (Callback == NULL)
    return 0;
  if (!ini_openread(Filename, &fp))
    return 0;

  LocalBuffer[0] = '\0';   /* copy an empty section in the buffer */
  lenSec = (int)_tcslen(LocalBuffer) + 1;
  for ( ;; ) {
    TCHAR *sp, *ep;
    if (!ini_read(LocalBuffer + lenSec, INI_BUFFERSIZE - lenSec, &fp))
      break;
    sp = skipleading(LocalBuffer + lenSec);
    /* ignore empty strings and comments */
    if (*sp == '\0' || *sp == ';' || *sp == '#')
      continue;
    /* see whether we reached a new section */
    ep = _tcsrchr(sp, ']');
    if (*sp == '[' && ep != NULL) {
      sp = skipleading(sp + 1);
      ep = skiptrailing(ep, sp);
      *ep = '\0';
      ini_strncpy(LocalBuffer, sp, INI_BUFFERSIZE, QUOTE_NONE);
      lenSec = (int)_tcslen(LocalBuffer) + 1;
      continue;
    }
    /* not a new section, test for a key/value pair */
    ep = _tcschr(sp, '=');    /* test for the equal sign or colon */
    if (ep == NULL)
      ep = _tcschr(sp, ':');
    if (ep == NULL)
      continue;               /* invalid line, ignore */
    *ep++ = '\0';             /* split the key from the value */
    striptrailing(sp);
    ini_strncpy(LocalBuffer + lenSec, sp, INI_BUFFERSIZE - lenSec, QUOTE_NONE);
    lenKey = (int)_tcslen(LocalBuffer + lenSec) + 1;
    /* clean up the value */
    sp = skipleading(ep);
    sp = cleanstring(sp, &quotes);  /* Remove a trailing comment */
    ini_strncpy(LocalBuffer + lenSec + lenKey, sp, INI_BUFFERSIZE - lenSec - lenKey, quotes);
    /* call the callback */
    if (!Callback(LocalBuffer, LocalBuffer + lenSec, LocalBuffer + lenSec + lenKey, UserData))
      break;
  }

  (void)ini_close(&fp);
  return 1;
}
#endif /* INI_NOBROWSE */

#if ! defined INI_READONLY
static void ini_tempname(TCHAR *dest, const TCHAR *source, int maxlength)
{
  TCHAR *p;

  ini_strncpy(dest, source, maxlength, QUOTE_NONE);
  p = _tcschr(dest, '\0');
  assert(p != NULL);
  *(p - 1) = '~';
}

static enum quote_option check_enquote(const TCHAR *Value)
{
  const TCHAR *p;

  /* run through the value, if it has trailing spaces, or '"', ';' or '#'
   * characters, enquote it
   */
  assert(Value != NULL);
  for (p = Value; *p != '\0' && *p != '"' && *p != ';' && *p != '#'; p++)
    /* nothing */;
  return (*p != '\0' || (p > Value && *(p - 1) == ' ')) ? QUOTE_ENQUOTE : QUOTE_NONE;
}

static void writesection(TCHAR *LocalBuffer, const TCHAR *Section, INI_FILETYPE *fp)
{
  if (Section != NULL && _tcslen(Section) > 0) {
    TCHAR *p;
    LocalBuffer[0] = '[';
    ini_strncpy(LocalBuffer + 1, Section, INI_BUFFERSIZE - 4, QUOTE_NONE);  /* -1 for '[', -1 for ']', -2 for '\r\n' */
    p = _tcschr(LocalBuffer, '\0');
    assert(p != NULL);
    *p++ = ']';
    _tcscpy(p, INI_LINETERM); /* copy line terminator (typically "\n") */
    if (fp != NULL)
      (void)ini_write(LocalBuffer, fp);
  }
}

static void writekey(TCHAR *LocalBuffer, const TCHAR *Key, const TCHAR *Value, INI_FILETYPE *fp)
{
  TCHAR *p;
  enum quote_option option = check_enquote(Value);
  ini_strncpy(LocalBuffer, Key, INI_BUFFERSIZE - 3, QUOTE_NONE);  /* -1 for '=', -2 for '\r\n' */
  p = _tcschr(LocalBuffer, '\0');
  assert(p != NULL);
  *p++ = '=';
  ini_strncpy(p, Value, INI_BUFFERSIZE - (p - LocalBuffer) - 2, option); /* -2 for '\r\n' */
  p = _tcschr(LocalBuffer, '\0');
  assert(p != NULL);
  _tcscpy(p, INI_LINETERM); /* copy line terminator (typically "\n") */
  if (fp != NULL)
    (void)ini_write(LocalBuffer, fp);
}

static int cache_accum(const TCHAR *string, int *size, int max)
{
  int len = (int)_tcslen(string);
  if (*size + len >= max)
    return 0;
  *size += len;
  return 1;
}

static int cache_flush(TCHAR *buffer, int *size,
                      INI_FILETYPE *rfp, INI_FILETYPE *wfp, INI_FILEPOS *mark)
{
  int terminator_len = (int)_tcslen(INI_LINETERM);
  int pos = 0, pos_prev = -1;

  (void)ini_seek(rfp, mark);
  assert(buffer != NULL);
  buffer[0] = '\0';
  assert(size != NULL);
  assert(*size <= INI_BUFFERSIZE);
  while (pos < *size && pos != pos_prev) {
    pos_prev = pos;     /* to guard against zero bytes in the INI file */
    (void)ini_read(buffer + pos, INI_BUFFERSIZE - pos, rfp);
    while (pos < *size && buffer[pos] != '\0')
      pos++;            /* cannot use _tcslen() because buffer may not be zero-terminated */
  }
  if (buffer[0] != '\0') {
    assert(pos > 0 && pos <= INI_BUFFERSIZE);
    if (pos == INI_BUFFERSIZE)
      pos--;
    buffer[pos] = '\0'; /* force zero-termination (may be left unterminated in the above while loop) */
    (void)ini_write(buffer, wfp);
  }
  ini_tell(rfp, mark);  /* update mark */
  *size = 0;
  /* return whether the buffer ended with a line termination */
  return (pos > terminator_len) && (_tcscmp(buffer + pos - terminator_len, INI_LINETERM) == 0);
}

static int close_rename(INI_FILETYPE *rfp, INI_FILETYPE *wfp, const TCHAR *filename, TCHAR *buffer)
{
  (void)ini_close(rfp);
  (void)ini_close(wfp);
  (void)ini_tempname(buffer, filename, INI_BUFFERSIZE);
  #if defined ini_remove || defined INI_REMOVE
    (void)ini_remove(filename);
  #endif
  (void)ini_rename(buffer, filename);
  return 1;
}

/** ini_puts()
 * \param Section     the name of the section to write the string in
 * \param Key         the name of the entry to write, or NULL to erase all keys in the section
 * \param Value       a pointer to the buffer the string, or NULL to erase the key
 * \param Filename    the name and full path of the .ini file to write to
 *
 * \return            1 if successful, otherwise 0
 */
int ini_puts(const TCHAR *Section, const TCHAR *Key, const TCHAR *Value, const TCHAR *Filename)
{
  INI_FILETYPE rfp;
  INI_FILETYPE wfp;
  INI_FILEPOS mark;
  INI_FILEPOS head, tail;
  TCHAR *sp, *ep;
  TCHAR LocalBuffer[INI_BUFFERSIZE];
  int len, match, flag, cachelen;

  assert(Filename != NULL);
  if (!ini_openread(Filename, &rfp)) {
    /* If the .ini file doesn't exist, make a new file */
    if (Key != NULL && Value != NULL) {
      if (!ini_openwrite(Filename, &wfp))
        return 0;
      writesection(LocalBuffer, Section, &wfp);
      writekey(LocalBuffer, Key, Value, &wfp);
      (void)ini_close(&wfp);
    }
    return 1;
  }

  /* If parameters Key and Value are valid (so this is not an "erase" request)
   * and the setting already exists, there are two short-cuts to avoid rewriting
   * the INI file.
   */
  if (Key != NULL && Value != NULL) {
    match = getkeystring(&rfp, Section, Key, -1, -1, LocalBuffer, sizearray(LocalBuffer), &head);
    if (match) {
      /* if the current setting is identical to the one to write, there is
       * nothing to do.
       */
      if (_tcscmp(LocalBuffer,Value) == 0) {
        (void)ini_close(&rfp);
        return 1;
      }
      /* if the new setting has the same length as the current setting, and the
       * glue file permits file read/write access, we can modify in place.
       */
      #if defined ini_openrewrite || defined INI_OPENREWRITE
        /* we already have the start of the (raw) line, get the end too */
        ini_tell(&rfp, &tail);
        /* create new buffer (without writing it to file) */
        writekey(LocalBuffer, Key, Value, NULL);
        if (_tcslen(LocalBuffer) == (size_t)(tail - head)) {
          /* length matches, close the file & re-open for read/write, then
           * write at the correct position
           */
          (void)ini_close(&rfp);
          if (!ini_openrewrite(Filename, &wfp))
            return 0;
          (void)ini_seek(&wfp, &head);
          (void)ini_write(LocalBuffer, &wfp);
          (void)ini_close(&wfp);
          return 1;
        }
      #endif
    }
    /* key not found, or different value & length -> proceed */
  } else if (Key != NULL && Value == NULL) {
    /* Conversely, for a request to delete a setting; if that setting isn't
       present, just return */
    match = getkeystring(&rfp, Section, Key, -1, -1, LocalBuffer, sizearray(LocalBuffer), NULL);
    if (!match) {
      (void)ini_close(&rfp);
      return 1;
    }
    /* key found -> proceed to delete it */
  }

  /* Get a temporary file name to copy to. Use the existing name, but with
   * the last character set to a '~'.
   */
  (void)ini_close(&rfp);
  ini_tempname(LocalBuffer, Filename, INI_BUFFERSIZE);
  if (!ini_openwrite(LocalBuffer, &wfp))
    return 0;
  /* In the case of (advisory) file locks, ini_openwrite() may have been blocked
   * on the open, and after the block is lifted, the original file may have been
   * renamed, which is why the original file was closed and is now reopened */
  if (!ini_openread(Filename, &rfp)) {
    /* If the .ini file doesn't exist any more, make a new file */
    assert(Key != NULL && Value != NULL);
    writesection(LocalBuffer, Section, &wfp);
    writekey(LocalBuffer, Key, Value, &wfp);
    (void)ini_close(&wfp);
    return 1;
  }

  (void)ini_tell(&rfp, &mark);
  cachelen = 0;

  /* Move through the file one line at a time until a section is
   * matched or until EOF. Copy to temp file as it is read.
   */
  len = (Section != NULL) ? (int)_tcslen(Section) : 0;
  if (len > 0) {
    do {
      if (!ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp)) {
        /* Failed to find section, so add one to the end */
        flag = cache_flush(LocalBuffer, &cachelen, &rfp, &wfp, &mark);
        if (Key!=NULL && Value!=NULL) {
          if (!flag)
            (void)ini_write(INI_LINETERM, &wfp);  /* force a new line behind the last line of the INI file */
          writesection(LocalBuffer, Section, &wfp);
          writekey(LocalBuffer, Key, Value, &wfp);
        }
        return close_rename(&rfp, &wfp, Filename, LocalBuffer);  /* clean up and rename */
      }
      /* Check whether this line is a section */
      sp = skipleading(LocalBuffer);
      ep = _tcsrchr(sp, ']');
      match = (*sp == '[' && ep != NULL);
      if (match) {
        /* A section was found, skip leading and trailing whitespace */
        assert(sp != NULL && *sp == '[');
        sp = skipleading(sp + 1);
        assert(ep != NULL && *ep == ']');
        ep = skiptrailing(ep, sp);
        match = ((int)(ep-sp) == len && _tcsnicmp(sp, Section, len) == 0);
      }
      /* Copy the line from source to dest, but not if this is the section that
       * we are looking for and this section must be removed
       */
      if (!match || Key != NULL) {
        if (!cache_accum(LocalBuffer, &cachelen, INI_BUFFERSIZE)) {
          cache_flush(LocalBuffer, &cachelen, &rfp, &wfp, &mark);
          (void)ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp);
          cache_accum(LocalBuffer, &cachelen, INI_BUFFERSIZE);
        }
      }
    } while (!match);
  }
  cache_flush(LocalBuffer, &cachelen, &rfp, &wfp, &mark);
  /* when deleting a section, the section head that was just found has not been
   * copied to the output file, but because this line was not "accumulated" in
   * the cache, the position in the input file was reset to the point just
   * before the section; this must now be skipped (again)
   */
  if (Key == NULL) {
    (void)ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp);
    (void)ini_tell(&rfp, &mark);
  }

  /* Now that the section has been found, find the entry. Stop searching
   * upon leaving the section's area. Copy the file as it is read
   * and create an entry if one is not found.
   */
  len = (Key != NULL) ? (int)_tcslen(Key) : 0;
  for( ;; ) {
    if (!ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp)) {
      /* EOF without an entry so make one */
      flag = cache_flush(LocalBuffer, &cachelen, &rfp, &wfp, &mark);
      if (Key!=NULL && Value!=NULL) {
        if (!flag)
          (void)ini_write(INI_LINETERM, &wfp);  /* force a new line behind the last line of the INI file */
        writekey(LocalBuffer, Key, Value, &wfp);
      }
      return close_rename(&rfp, &wfp, Filename, LocalBuffer);  /* clean up and rename */
    }
    sp = skipleading(LocalBuffer);
    ep = _tcschr(sp, '='); /* Parse out the equal sign */
    if (ep == NULL)
      ep = _tcschr(sp, ':');
    match = (ep != NULL && len > 0 && (int)(skiptrailing(ep,sp)-sp) == len && _tcsnicmp(sp,Key,len) == 0);
    if ((Key != NULL && match) || *sp == '[')
      break;  /* found the key, or found a new section */
    /* copy other keys in the section */
    if (Key == NULL) {
      (void)ini_tell(&rfp, &mark);  /* we are deleting the entire section, so update the read position */
    } else {
      if (!cache_accum(LocalBuffer, &cachelen, INI_BUFFERSIZE)) {
        cache_flush(LocalBuffer, &cachelen, &rfp, &wfp, &mark);
        (void)ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp);
        cache_accum(LocalBuffer, &cachelen, INI_BUFFERSIZE);
      }
    }
  }
  /* the key was found, or we just dropped on the next section (meaning that it
   * wasn't found); in both cases we need to write the key, but in the latter
   * case, we also need to write the line starting the new section after writing
   * the key
   */
  flag = (*sp == '[');
  cache_flush(LocalBuffer, &cachelen, &rfp, &wfp, &mark);
  if (Key != NULL && Value != NULL)
    writekey(LocalBuffer, Key, Value, &wfp);
  /* cache_flush() reset the "read pointer" to the start of the line with the
   * previous key or the new section; read it again (because writekey() destroyed
   * the buffer)
   */
  (void)ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp);
  if (flag) {
    /* the new section heading needs to be copied to the output file */
    cache_accum(LocalBuffer, &cachelen, INI_BUFFERSIZE);
  } else {
    /* forget the old key line */
    (void)ini_tell(&rfp, &mark);
  }
  /* Copy the rest of the INI file */
  while (ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp)) {
    if (!cache_accum(LocalBuffer, &cachelen, INI_BUFFERSIZE)) {
      cache_flush(LocalBuffer, &cachelen, &rfp, &wfp, &mark);
      (void)ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp);
      cache_accum(LocalBuffer, &cachelen, INI_BUFFERSIZE);
    }
  }
  cache_flush(LocalBuffer, &cachelen, &rfp, &wfp, &mark);
  return close_rename(&rfp, &wfp, Filename, LocalBuffer);  /* clean up and rename */
}

/* Ansi C "itoa" based on Kernighan & Ritchie's "Ansi C" book. */
#define ABS(v)  ((v) < 0 ? -(v) : (v))

static void strreverse(TCHAR *str)
{
  int i, j;
  for (i = 0, j = (int)_tcslen(str) - 1; i < j; i++, j--) {
    TCHAR t = str[i];
    str[i] = str[j];
    str[j] = t;
  }
}

static void long2str(long value, TCHAR *str)
{
  int i = 0;
  long sign = value;

  /* generate digits in reverse order */
  do {
    int n = (int)(value % 10);          /* get next lowest digit */
    str[i++] = (TCHAR)(ABS(n) + '0');   /* handle case of negative digit */
  } while (value /= 10);                /* delete the lowest digit */
  if (sign < 0)
    str[i++] = '-';
  str[i] = '\0';

  strreverse(str);
}

/** ini_putl()
 * \param Section     the name of the section to write the value in
 * \param Key         the name of the entry to write
 * \param Value       the value to write
 * \param Filename    the name and full path of the .ini file to write to
 *
 * \return            1 if successful, otherwise 0
 */
int ini_putl(const TCHAR *Section, const TCHAR *Key, long Value, const TCHAR *Filename)
{
  TCHAR LocalBuffer[32];
  long2str(Value, LocalBuffer);
  return ini_puts(Section, Key, LocalBuffer, Filename);
}

#if defined INI_REAL
/** ini_putf()
 * \param Section     the name of the section to write the value in
 * \param Key         the name of the entry to write
 * \param Value       the value to write
 * \param Filename    the name and full path of the .ini file to write to
 *
 * \return            1 if successful, otherwise 0
 */
int ini_putf(const TCHAR *Section, const TCHAR *Key, INI_REAL Value, const TCHAR *Filename)
{
  TCHAR LocalBuffer[64];
  ini_ftoa(LocalBuffer, Value);
  return ini_puts(Section, Key, LocalBuffer, Filename);
}
#endif /* INI_REAL */
#endif /* !INI_READONLY */
