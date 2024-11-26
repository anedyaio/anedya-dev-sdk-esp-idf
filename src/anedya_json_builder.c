#include <stddef.h>
#include "anedya_json_builder.h"

/** Add a character at the end of a string.
 * @param dest Pointer to the null character of the string
 * @param ch Value to be added.
 * @param end_marker Pointer to remaining length of dest
 * @return Pointer to the null character of the destination string. */
static char *chtoa(char *dest, char ch, size_t *end_marker)
{
    if (*end_marker != 0)
    {
        --*end_marker;
        *dest = ch;
        *++dest = '\0';
    }
    return dest;
}

/** Copy a null-terminated string.
 * @param dest Destination memory block.
 * @param src Source string.
 * @param end_marker Pointer to remaining length of dest
 * @return Pointer to the null character of the destination string. */
static char *atoa(char *dest, char const *src, size_t *end_marker)
{
    for (; *src != '\0' && *end_marker != 0; ++dest, ++src, --*end_marker)
        *dest = *src;
    *dest = '\0';
    return dest;
}

/* Open a JSON object in a JSON string. */
char *anedya_json_objOpen(char *dest, char const *name, size_t *end_marker)
{
    if (NULL == name)
        dest = chtoa(dest, '{', end_marker);
    else
    {
        dest = chtoa(dest, '\"', end_marker);
        dest = atoa(dest, name, end_marker);
        dest = atoa(dest, "\":{", end_marker);
    }
    return dest;
}

/* Close a JSON object in a JSON string. */
char *anedya_json_objClose(char *dest, size_t *end_marker)
{
    if (dest[-1] == ',')
    {
        --dest;
        ++*end_marker;
    }
    return atoa(dest, "},", end_marker);
}

/* Open an array in a JSON string. */
char *anedya_json_arrOpen(char *dest, char const *name, size_t *end_marker)
{
    if (NULL == name)
        dest = chtoa(dest, '[', end_marker);
    else
    {
        dest = chtoa(dest, '\"', end_marker);
        dest = atoa(dest, name, end_marker);
        dest = atoa(dest, "\":[", end_marker);
    }
    return dest;
}

/* Close an array in a JSON string. */
char *anedya_json_arrClose(char *dest, size_t *end_marker)
{
    if (dest[-1] == ',')
    {
        --dest;
        ++*end_marker;
    }
    return atoa(dest, "],", end_marker);
}

/** Add the name of a text property.
 * @param dest Destination memory.
 * @param name The name of the property.
 * @param end_marker Pointer to remaining length of dest
 * @return Pointer to the next char. */
static char *strname(char *dest, char const *name, size_t *end_marker)
{
    dest = chtoa(dest, '\"', end_marker);
    if (NULL != name)
    {
        dest = atoa(dest, name, end_marker);
        dest = atoa(dest, "\":\"", end_marker);
    }
    return dest;
}

/** Get the hexadecimal digit of the least significant nibble of a integer. */
static int nibbletoch(int nibble)
{
    return "0123456789ABCDEF"[nibble % 16u];
}

/** Get the escape character of a non-printable.
 * @param ch Character source.
 * @return The escape character or null character if error. */
static int escape(int ch)
{
    int i;
    static struct
    {
        char code;
        char ch;
    } const pair[] = {
        {'\"', '\"'},
        {'\\', '\\'},
        {'/', '/'},
        {'b', '\b'},
        {'f', '\f'},
        {'n', '\n'},
        {'r', '\r'},
        {'t', '\t'},
    };
    for (i = 0; i < sizeof pair / sizeof *pair; ++i)
        if (ch == pair[i].ch)
            return pair[i].code;
    return '\0';
}

/** Copy a null-terminated string inserting escape characters if needed.
 * @param dest Destination memory block.
 * @param src Source string.
 * @param len Max length of source. < 0 for unlimit.
 * @param end_marker Pointer to remaining length of dest
 * @return Pointer to the null character of the destination string. */
static char *atoesc(char *dest, char const *src, int len, size_t *end_marker)
{
    int i;
    for (i = 0; src[i] != '\0' && (i < len || 0 > len) && *end_marker != 0; ++dest, ++i, --*end_marker)
    {
        if (src[i] >= ' ' && src[i] != '\"' && src[i] != '\\' && src[i] != '/')
            *dest = src[i];
        else
        {
            if (*end_marker != 0)
            {
                *dest++ = '\\';
                --*end_marker;
                int const esc = escape(src[i]);
                if (esc)
                {
                    if (*end_marker != 0)
                        *dest = esc;
                }
                else
                {
                    if (*end_marker != 0)
                    {
                        --*end_marker;
                        *dest++ = 'u';
                    }
                    if (*end_marker != 0)
                    {
                        --*end_marker;
                        *dest++ = '0';
                    }
                    if (*end_marker != 0)
                    {
                        --*end_marker;
                        *dest++ = '0';
                    }
                    if (*end_marker != 0)
                    {
                        --*end_marker;
                        *dest++ = nibbletoch(src[i] / 16);
                    }
                    if (*end_marker != 0)
                    {
                        --*end_marker;
                        *dest++ = nibbletoch(src[i]);
                    }
                }
            }
        }

        if (*end_marker == 0)
            break;
    }
    *dest = '\0';
    return dest;
}

/* Add a text property in a JSON string. */
char *anedya_json_nstr(char *dest, char const *name, char const *value, int len, size_t *end_marker)
{
    dest = strname(dest, name, end_marker);
    dest = atoesc(dest, value, len, end_marker);
    dest = atoa(dest, "\",", end_marker);
    return dest;
}

/** Add the name of a primitive property.
 * @param dest Destination memory.
 * @param name The name of the property.
 * @param end_marker Pointer to remaining length of dest
 * @return Pointer to the next char. */
static char *primitivename(char *dest, char const *name, size_t *end_marker)
{
    if (NULL == name)
        return dest;
    dest = chtoa(dest, '\"', end_marker);
    dest = atoa(dest, name, end_marker);
    dest = atoa(dest, "\":", end_marker);
    return dest;
}

/*  Add a boolean property in a JSON string. */
char *anedya_json_bool(char *dest, char const *name, int value, size_t *end_marker)
{
    dest = primitivename(dest, name, end_marker);
    dest = atoa(dest, value ? "true," : "false,", end_marker);
    return dest;
}

/* Add a null property in a JSON string. */
char *anedya_json_null(char *dest, char const *name, size_t *end_marker)
{
    dest = primitivename(dest, name, end_marker);
    dest = atoa(dest, "null,", end_marker);
    return dest;
}

/* Used to finish the root JSON object. After call json_objClose(). */
char *anedya_json_end(char *dest, size_t *end_marker)
{
    if (',' == dest[-1])
    {
        dest[-1] = '\0';
        --dest;
        ++*end_marker;
    }
    return dest;
}

#ifdef NO_SPRINTF

static char *format(char *dest, int len, int isnegative)
{
    if (isnegative)
        dest[len++] = '-';
    dest[len] = '\0';
    int head = 0;
    int tail = len - 1;
    while (head < tail)
    {
        char tmp = dest[head];
        dest[head] = dest[tail];
        dest[tail] = tmp;
        ++head;
        --tail;
    }
    return dest + len;
}

#define numtoa(func, type, utype)             \
    static char *func(char *dest, type val)   \
    {                                         \
        enum                                  \
        {                                     \
            base = 10                         \
        };                                    \
        if (0 == val)                         \
            return chtoa(dest, '0');          \
        int const isnegative = 0 > val;       \
        utype num = isnegative ? -val : val;  \
        int len = 0;                          \
        while (0 != num)                      \
        {                                     \
            int rem = num % base;             \
            dest[len++] = rem + '0';          \
            num = num / base;                 \
        }                                     \
        return format(dest, len, isnegative); \
    }

#define json_num(func, func2, type)                      \
    char *func(char *dest, char const *name, type value) \
    {                                                    \
        dest = primitivename(dest, name);                \
        dest = func2(dest, value);                       \
        dest = chtoa(dest, ',');                         \
        return dest;                                     \
    }

#define ALL_TYPES                          \
    X(int, int, unsigned int)              \
    X(long, long, unsigned long)           \
    X(uint, unsigned int, unsigned int)    \
    X(ulong, unsigned long, unsigned long) \
    X(verylong, long long, unsigned long long)

#define X(name, type, utype) numtoa(name##toa, type, utype)
ALL_TYPES
#undef X

#define X(name, type, utype) json_num(json_##name, name##toa, type)
ALL_TYPES
#undef X

char *anedya_json_double(char *dest, char const *name, double value)
{
    return json_verylong(dest, name, value);
}

#else

#include <stdio.h>

#define ALL_TYPES                              \
    X(anedya_json_int, int, "%d")              \
    X(anedya_json_long, long, "%ld")           \
    X(anedya_json_uint, unsigned int, "%u")    \
    X(anedya_json_ulong, unsigned long, "%lu") \
    X(anedya_json_verylong, long long, "%lld") \
    X(anedya_json_double, double, "%g")

#define json_num(funcname, type, fmt)                                            \
    char *funcname(char *dest, char const *name, type value, size_t *end_marker) \
    {                                                                            \
        int digitLen;                                                            \
        dest = primitivename(dest, name, end_marker);                            \
        digitLen = snprintf(dest, *end_marker, fmt, value);                      \
        if (digitLen >= (int)*end_marker + 1)                                    \
        {                                                                        \
            digitLen = (int)*end_marker;                                         \
        }                                                                        \
        *end_marker -= (size_t)digitLen;                                         \
        dest += digitLen;                                                        \
        dest = chtoa(dest, ',', end_marker);                                     \
        return dest;                                                             \
    }

#define X(name, type, fmt) json_num(name, type, fmt)
ALL_TYPES
#undef X

#endif