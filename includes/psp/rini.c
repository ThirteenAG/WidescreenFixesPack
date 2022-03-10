/**
 * @file rini.c
 * @author LloydLabs
 * @date 10/2/2017
 * @brief Rini is a tiny, super fast, .ini file parser programmed from scratch in C99.
 * Rini (really small ini parser).
 * All other parsers rely on libc functions such as 'strtok', however Rini is in complete raw C code designed to handle the 'ini' format thus is much faster and efficent.
 * This is the first parser I've made completely from scratch, feedback is welcome.
 *
 * @see https://github.com/LloydLabs/librini
 * @see https://en.wikipedia.org/wiki/INI_file
 */

#include "rini.h"

/**
 * Seeks to the location of the start of the [parent], sets the @param config_buf to the location the definition ends at.
 * @param parent The parent [section] to find the offset of.
 * @param config_buf The contents of the ini configuration, we need this so that we can find the offset to the parent.
 * @param size The size of the configuration buffer.
 * @return Returns an address to where the [parent] starts.
 */
static char* rini_seek_section(const char* parent, char* config_buf,
                               unsigned size)
{
    unsigned buf_size = 0;
    bool head_found = false;

    char current_head[MAX_NAME];
    memset(current_head, 0, MAX_NAME);

    char* head_buf = (char*)current_head;

    for (unsigned total_read = 0; total_read < size; total_read++, config_buf++)
    {
        if (*config_buf == '[')
        {
            if (total_read++ >= size)
            {
                break;
            }

            config_buf++;

            for ( ; *config_buf != ']' && total_read < size; config_buf++, buf_size++, total_read++)
            {
                if ((buf_size + 1) >= MAX_NAME)
                {
                    break;
                }

                *head_buf++ = *config_buf;
            }

            if (total_read++ >= size)
            {
                break;
            }

            config_buf++;

            if (strncmp(parent, (const char*)current_head, MAX_NAME) == 0)
            {
                head_found = true;
                break;
            }
            else
            {
                CLEAN_PTR(current_head, sizeof(current_head), head_buf);
            }
        }
    }

    return (head_found ? config_buf : NULL);
}

/**
 * https://en.wikipedia.org/wiki/INI_file#Escape_characters
 * checks if a character is of a special type and must be escaped
 * @param c The character to check
 * @return This will return 1 on success
 */
static bool rini_is_escaped(char c)
{
    const char escape_chars[] = {
            '"', ';', '#', ':',
            '=', '\\'
    };

    for (unsigned i = 0; i < ARRAY_SIZE(escape_chars); i++)
    {
        if (c == escape_chars[i])
        {
            return true;
        }
    }

    return false;
}

/**
 * attempts to parse a string of a given size from a human-like boolean value.
 * @see bool_type_t
 * @param buf The alleged boolean value.
 * @param buf_size The size of the buffer.
 * @return Will return BOOL_KEY_ERROR on error, else, a boolean value.
 */
static bool_type_t rini_get_bool(char* buf, unsigned buf_size)
{
    if (buf == NULL || buf_size > (MAX_BOOL_KEY_SIZE - 1))
    {
        return BOOL_KEY_ERROR;
    }

    bool_keys_t bool_keys[] = {
            { BOOL_KEY_TRUE, "1", 1 }, { BOOL_KEY_TRUE, "yes", 3 },
            { BOOL_KEY_TRUE, "on", 2 }, { BOOL_KEY_TRUE, "true", 4 },
            { BOOL_KEY_TRUE, "y", 1 },
            { BOOL_KEY_FALSE, "0", 1 },  { BOOL_KEY_FALSE, "no", 2 },
            { BOOL_KEY_FALSE, "off", 3 }, { BOOL_KEY_FALSE, "false", 5 },
            { BOOL_KEY_FALSE, "n", 1 }
    };

    char bool_key[MAX_BOOL_KEY_SIZE];
    memset(bool_key, 0, MAX_BOOL_KEY_SIZE);

    memcpy(bool_key, buf, buf_size);

    for (unsigned i = 0; i < ARRAY_SIZE(bool_keys); i++)
    {
        if (bool_keys[i].size > buf_size)
        {
            continue;
        }

        if (memcmp(bool_key, bool_keys[i].key, bool_keys[i].size) == 0)
        {
            return bool_keys[i].val;
        }
    }

    return BOOL_KEY_ERROR;
}

/**
 * The main parsing logic is conducted here, first of all the key is parsed from the input,
 * we then make sure the key is valid. Then, from here, we skip the '=' character, and then go on to
 * parse the value. Once the value has been parsed, we do the relevant operations to convert it into
 * the data type specified in the @param val_type.
 *
 * @param node In all cases with Rini, this will be the line to parse, this function does not handle line parsing.
 * @param name The name to the key to parse.
 * @param out The buffer to send the output to. The type is of a void pointer as the type can be variable, as defined by @param val_type.
 * @param out_size The size of the output buffer. We must know this in order to make sure that the parsed data is no greater than this value.
 * @param val_type The type of value that we are parsing, the relevant conversions will take place towards the end of the function.
 * @param size The overall size of the configuration buffer, this will make sure that the @param out_size is not greater than our overall size.
 * @return Returns 1 on success.
 */
static bool rini_get_node(char* node, char* name, void* out,
                         unsigned out_size, value_types_t val_type,
                         unsigned size)
{
    if (out == NULL)
    {
        return false;
    }

    if (val_type == INT_VAL && out_size < sizeof(int))
    {
        return false;
    }

    char name_parsed[MAX_NAME], int_str[MAX_INT_STR_SIZE], bool_str[MAX_BOOL_KEY_SIZE];

    memset(name_parsed, 0, MAX_NAME);
    memset(int_str, 0, MAX_INT_STR_SIZE);
    memset(bool_str, 0, MAX_BOOL_KEY_SIZE);

    char* name_buf = (char*)name_parsed, *int_buf = (char*)int_str, *bool_buf = (char*)bool_str, *node_buf = node;
    unsigned buf_size = 0, val_size = 1;

    for ( ; PTR_NOT_END(node_buf) && buf_size < size; node_buf++, buf_size++)
    {
        for ( ; *node_buf != '=' && buf_size < size; node_buf++, buf_size++)
        {
            if ((buf_size + 1) >= MAX_NAME)
            {
                return false;
            }

            *name_buf++ = *node_buf;
        }

        if (strncmp(name_parsed, name, strlen(name)) == 0)
        {
            break;
        }
    }

    if (*node_buf != '=')
    {
        return false;
    }

    if (buf_size++ > size)
    {
        return false;
    }

    node_buf++;

    parser_flags_t parser_flags = FLAGS_DEFAULT;

    char* val_buf = (char*)out;
    for ( ; PTR_NOT_END(node_buf) && buf_size < size; node_buf++, buf_size++, val_size++)
    {
        if (parser_flags & END_EARLY)
        {
            break;
        }

        if ((parser_flags & LAST_ESCAPE_CHAR) == 0)
        {
            if (*node_buf == '#' || *node_buf == ';')
            {
                parser_flags |= END_EARLY;
                continue;
            }
        }

        switch (val_type)
        {
            case STRING_VAL:
                if ((val_size + 1) > out_size)
                {
                    return false;
                }

                if (buf_size == 0)
                {
                    if (*node_buf == '"')
                    {
                        parser_flags |= EXPECT_QUOTE;
                        continue;
                    }
                }

                if (*node_buf == '\\')
                {
                    parser_flags |= LAST_ESCAPE_CHAR;
                    break;
                }

                if (parser_flags & LAST_ESCAPE_CHAR)
                {
                    parser_flags |= ESCAPE_NOT_FOUND;
                    if (rini_is_escaped(*node_buf))
                    {
                        *val_buf++ = *node_buf;
                        parser_flags &= ~ESCAPE_NOT_FOUND;
                    }

                    if (parser_flags & ESCAPE_NOT_FOUND)
                    {
                        *val_buf++ = '\\';
                        node_buf--;
                        buf_size--;

                        parser_flags &= ~ESCAPE_NOT_FOUND;
                        continue;
                    }

                    parser_flags &= ~LAST_ESCAPE_CHAR;
                }
                else if (*node_buf == '"' && (parser_flags & EXPECT_QUOTE))
                {
                    parser_flags |= END_EARLY;
                    continue;
                }
                else
                {
                    *val_buf++ = *node_buf;
                }
                break;

            case BOOL_VAL:
                if (val_size > (MAX_BOOL_KEY_SIZE - 1))
                {
                    return false;
                }

                *bool_buf++ = *node_buf;
                break;

            case INT_VAL:
                if (val_size > (MAX_INT_STR_SIZE - 1))
                {
                    return false;
                }

                if (buf_size == 0 && *node_buf == '-')
                {
                    *int_buf++ = *node_buf;
                    continue;
                }

                if (*node_buf < '0' || *node_buf > '9')
                {
                    if (*node_buf != ' ')
                        return false;
                }

                *int_buf++ = *node_buf;
                break;
        }
    }

    if (val_type == STRING_VAL)
    {
        *val_buf++ = 0;
    }
    else if (val_type == INT_VAL)
    {
        *int_buf++ = 0;
        int_buf = (char*)int_str;

        int numb_buf;
        char* conv_buf;
        if ((numb_buf = (int)strtol(int_buf, &conv_buf, 10)) < 0)
        {
            return false;
        }

        if (conv_buf == NULL)
        {
            return false;
        }

        memcpy(out, &numb_buf, sizeof(int));
    }
    else if (val_type == BOOL_VAL)
    {
        bool_type_t bool_val = BOOL_KEY_ERROR;
        if ((bool_val = rini_get_bool(bool_str, val_size)) == BOOL_KEY_ERROR)
        {
            return false;
        }

        *val_buf = bool_val;
    }

    return true;
}

/**
 * this function will parse a key from the given parent, key and configuration.
 * @see rini_seek_section
 * @see rini_get_node
 * @param parent The parent [section], NULL if no section.
 * @param key The name of the key to find
 * @param config The configuration to parse
 * @param config_size The size of the configuration
 * @param out The buffer to recieve the key's value.
 * @param out_size The size of the buffer to recieve the key's value.
 * @param type The type of data that the key holds.
 * @return On success 1 is returned
 */
int rini_get_key(const char* parent, const char* key, const char* config,
                 unsigned config_size, const void* out, unsigned out_size,
                 value_types_t type)
{
    char* config_buf = (char*)config;

    if (key != NULL)
    {
        if ((config_buf = rini_seek_section(parent, config_buf, config_size)) == NULL)
        {
            return false;
        }
    }

    char line[MAX_LINE_SIZE(out_size)];
    memset(line, 0, sizeof(line));

    char* line_buf = (char*)line;
    unsigned line_size = 0;

    for (unsigned total_read = 0; total_read < config_size; config_buf++, total_read++)
    {
        line_size = 0;
        for ( ; PTR_NOT_END(config_buf) && total_read < config_size; config_buf++, line_size++, total_read++)
        {
            if ((line_size + 1) >= MAX_LINE_SIZE(out_size))
            {
                return false;
            }

            if (line_size == 0)
            {
                if (*config_buf == '#' || *config_buf == ';')
                {
                    continue;
                }

                if (*config_buf == '[')
                {
                    break;
                }
            }

            *line_buf++ = *config_buf;
        }

        if (rini_get_node(line, key, (void*)out, out_size, type, line_size) == 1)
        {
            return true;
        }

        CLEAN_PTR(line, MAX_NAME, line_buf);
    }

    return false;
}
