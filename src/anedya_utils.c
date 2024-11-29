#include "anedya_models.h"
#include "stdio.h"


static uint8_t unhex_char(unsigned char s)
{
    if (0x30 <= s && s <= 0x39) { /* 0-9 */
        return s - 0x30;
    } else if (0x41 <= s && s <= 0x46) { /* A-F */
        return s - 0x41 + 0xa;
    } else if (0x61 <= s && s <= 0x69) { /* a-f */
        return s - 0x61 + 0xa;
    } else {
        /* invalid string */
        return 0xff;
    }
}

static int unhex(unsigned char *s, size_t s_len, unsigned char *r)
{
    int i;

    for (i = 0; i < s_len; i += 2) {
        uint8_t h = unhex_char(s[i]);
        uint8_t l = unhex_char(s[i + 1]);
        if (0xff == h || 0xff == l) {
            return -1;
        }
        r[i / 2] = (h << 4) | (l & 0xf);
    }

    return 0;
}

anedya_err_t _anedya_uuid_parse(const char *in, anedya_uuid_t uuid)
{
    const char *p = in;
    uint8_t *op = (uint8_t *)uuid;

    if (0 != unhex((unsigned char *)p, 8, op)) {
        return ANEDYA_ERR_INVALID_UUID;
    }
    p += 8;
    op += 4;

    for (int i = 0; i < 3; i++) {
        if ('-' != *p++ || 0 != unhex((unsigned char *)p, 4, op)) {
            return ANEDYA_ERR_INVALID_UUID;
        }
        p += 4;
        op += 2;
    }

    if ('-' != *p++ || 0 != unhex((unsigned char *)p, 12, op)) {
        return ANEDYA_ERR_INVALID_UUID;
    }
    p += 12;
    op += 6;

    return ANEDYA_OK;
}

void _anedya_uuid_marshal(const anedya_uuid_t uuid, char *out)
{
    snprintf(out, 37,
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-"
             "%02x%02x-%02x%02x%02x%02x%02x%02x",
             uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11],
             uuid[12], uuid[13], uuid[14], uuid[15]);
}

int _anedya_strcmp(const char* x, const char* y)
{
    //int flag = 0;
 
    // Iterate a loop till the end
    // of both the strings
    while (*x != '\0' || *y != '\0') {
        if (*x == *y) {
            x++;
            y++;
        }
 
        // If two characters are not same
        // print the difference and exit
        else if ((*x == '\0' && *y != '\0')
                 || (*x != '\0' && *y == '\0')
                 || *x != *y) {
            //flag = 1;
            return 1;
        }
    }
 
    return 0;
}

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static int mod_table[] = {0, 2, 1};
static const char decoding_table[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x3f, 
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 
    0x3c, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
    0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 
    0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 
    0x17, 0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 
    0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 
    0x31, 0x32, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

unsigned int _anedya_base64_encode(unsigned char *data, unsigned char *output) {
    
    unsigned int input_length = strlen((const char*)data);
    unsigned int output_length = 4 * ((input_length + 2) / 3);

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        output[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        output[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        output[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        output[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        output[output_length - 1 - i] = '=';

    return output_length;
}

unsigned int _anedya_base64_decode(unsigned char *encoded, char *output) {

    // build_decoding_table();
    int input_length = strlen((const char *)encoded);
    if (input_length % 4 != 0) return 0;

    unsigned int output_length = input_length / 4 * 3;
    if (encoded[input_length - 1] == '=') (output_length)--;
    if (encoded[input_length - 2] == '=') (output_length)--;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = encoded[i] == '=' ? 0 & i++ : decoding_table[encoded[i++]];
        uint32_t sextet_b = encoded[i] == '=' ? 0 & i++ : decoding_table[encoded[i++]];
        uint32_t sextet_c = encoded[i] == '=' ? 0 & i++ : decoding_table[encoded[i++]];
        uint32_t sextet_d = encoded[i] == '=' ? 0 & i++ : decoding_table[encoded[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < output_length) output[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < output_length) output[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < output_length) output[j++] = (triple >> 0 * 8) & 0xFF;
    }
    
    return output_length;
}