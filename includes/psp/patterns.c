#include "patterns.h"
#include <string.h>

void PatternSetGameBaseAddress(uintptr_t addr, size_t size)
{
    pattern.text_addr = addr;
    pattern.text_size = size;
}

void remove_spaces_and_format(char* str)
{
    int count = 0;
    for (int i = 0; str[i]; i++)
    {
        if (str[i] == '?')
            str[count++] = '?';
        if (str[i] != ' ')
            str[count++] = str[i];
    }
    str[count] = '\0';
}

uint8_t hextobin(const char* str, uint8_t* bytes, size_t blen, uint8_t* wildcards)
{
    uint8_t  pos;
    uint8_t  idx0;
    uint8_t  idx1;

    static const uint8_t hashmap[] =
    {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 01234567
      0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 89:;<=>?
      0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, // @ABCDEFG
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // HIJKLMNO
    };

    memset(bytes, 0, blen);
    for (pos = 0; ((pos < (blen * 2)) && (pos < strlen(str))); pos += 2)
    {
        idx0 = ((uint8_t)str[pos + 0] & 0x1F) ^ 0x10;
        idx1 = ((uint8_t)str[pos + 1] & 0x1F) ^ 0x10;
        bytes[pos / 2] = (uint8_t)(hashmap[idx0] << 4) | hashmap[idx1];
        if (idx0 == 0xf && idx1 == 0xf)
            wildcards[pos / 2] = 1;
        else
            wildcards[pos / 2] = 0;
    };

    return pos / 2;
}

uint8_t* bytes_find(uint8_t* haystack, size_t haystackLen, uint8_t* needle, size_t needleLen, uint8_t* wildcards) {
    size_t len = needleLen;
    size_t limit = haystackLen - len;
    for (size_t i = 0; i <= limit; i++) {
        size_t k = 0;
        for (; k < len; k++) {
            if ((needle[k] != haystack[i + k]) && wildcards[k] != 1) break;
        }
        if (k == len) return haystack + i;
    }
    return NULL;
}

uint8_t* bytes_find_nth(size_t count, uint8_t* haystack, size_t haystackLen, uint8_t* needle, size_t needleLen, uint8_t* wildcards) {
    size_t n = 0;
    size_t len = needleLen;
    size_t limit = haystackLen - len;
    for (size_t i = 0; i <= limit; i++) {
        size_t k = 0;
        for (; k < len; k++) {
            if ((needle[k] != haystack[i + k]) && wildcards[k] != 1) break;
        }
        if (k == len)
        {
            if (n == count)
                return haystack + i;
            else
                n++;
        }
    }
    return NULL;
}

uintptr_t range_get(size_t count, uintptr_t range_start, size_t range_size, const char* pattern_str, int32_t offset)
{
    if (pattern_str[0] == '\0')
        return 0;
    char str[strlen(pattern_str)];
    strcpy(str, pattern_str);
    pattern_str = str;
    uint8_t wc[strlen(pattern_str)];
    remove_spaces_and_format(pattern_str);
    size_t len = strlen(pattern_str);
    uint8_t buf[len];
    uint8_t size = hextobin(pattern_str, buf, len, wc);
    uint8_t* result = bytes_find_nth(count, range_start, range_size, buf, size, wc);
    if (result)
        return result + offset;
    else
        return 0;
}

uintptr_t range_get_first(uintptr_t range_start, size_t range_size, const char* pattern_str, int32_t offset)
{
    return range_pattern.get(0, range_start, range_size, pattern_str, offset);
}

uintptr_t get(size_t count, const char* pattern_str, int32_t offset)
{
    return range_pattern.get(count, pattern.text_addr, pattern.text_size, pattern_str, offset);
}

uintptr_t get_first(const char* pattern_str, int32_t offset)
{
    return range_pattern.get_first(pattern.text_addr, pattern.text_size, pattern_str, offset);
}

struct range_pattern_t range_pattern =
{
    .get = range_get,
    .get_first = range_get_first
};

struct pattern_t pattern =
{
    .text_addr = 0,
    .text_size = 0,
    .get = get,
    .get_first = get_first,
    .SetGameBaseAddress = PatternSetGameBaseAddress
};