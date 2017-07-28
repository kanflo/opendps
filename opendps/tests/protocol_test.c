#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "uframe.h"
#include "protocol.h"


#define VERBOSE_ERRORS


#define MAX_FRAME_SIZE  (128)

static bool test_frame(uint8_t *payload, uint32_t length, bool expect_success);

#define RUN_FRAMING_TEST(f); \
    test_frame(f, sizeof(f), true) ? (printf(" " #f " pass\n"), g_num_pass++) : (printf(" " #f " failed\n"), g_num_fail++);

#define RUN_PROTOCOL_TEST(f) \
    f( #f) != g_expecting_failure ? (printf(" " #f " pass\n"), g_num_pass++) : (printf(" " #f " failed\n"), g_num_fail++);


// Used to test protocol creation
static uint32_t g_max_frame_size;
static bool g_expecting_failure;
static uint32_t g_num_pass = 0;
static uint32_t g_num_fail = 0;

#ifdef VERBOSE_ERRORS
// http://stackoverflow.com/questions/7775991/how-to-get-hexdump-of-a-structure-data
static void hexdump(char *desc, void *addr, int len)
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;
    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);
    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n",len);
        return;
    }
    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).
        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);
            // Output the offset.
            printf ("  %04x ", i);
        }
        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);
        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }
    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }
    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}
#endif // VERBOSE_ERRORS

#define EXTRACT_PAYLOAD() \
    int32_t res = uframe_extract_payload(f, len); \
    if (res < 0) { \
        if (!g_expecting_failure) { \
            printf("%s: extract failed with %d\n", test_name, res); \
        } \
        return false; \
    }

#define COMPARE(n, a, b) \
    if ((a) != (b)) { \
        printf(" %s: comparison #%d mismatch: %d != %d (0x%04x != 0x%04x)\n", (test_name), (n), (a), (b), (a), (b)); \
        return false; \
    }

static bool test_frame(uint8_t *payload, uint32_t length, bool expect_success)
{
    DECLARE_FRAME(MAX_FRAME_SIZE);
    for (uint32_t i = 0; i < length; ++i) {
        PACK8(payload[i]);
    }
    FINISH_FRAME();

    int32_t res = uframe_extract_payload((uint8_t*) _buffer, _length);
    if (res < 0 && expect_success) {
#ifdef VERBOSE_ERRORS
        printf("Unexpected error %d\n", res);
        hexdump("payload", payload, length);
#endif // VERBOSE_ERRORS
        return false;
    }

    if ((length != res) && expect_success) {
#ifdef VERBOSE_ERRORS
        printf("Length mismatch (payload %d, frame %d)\n", length, res);
        hexdump("payload", payload, length);
#endif // VERBOSE_ERRORS
        return false;
    }
    for (uint32_t i = 0; i < length; i++) {
        if (payload[i] != _buffer[i]) {
#ifdef VERBOSE_ERRORS
            printf("Payload mismatch at %d (p:%02x f:%02d)\n", i, payload[i], _buffer[i]);
            hexdump("payload", payload, length);
            hexdump("frame", _buffer, res);
#endif // VERBOSE_ERRORS
            return false;
        }
    }
    return true;
}

static bool test_small_frame(char *test_name)
{
    uint8_t frame[FRAME_OVERHEAD(MAX_FRAME_SIZE)], *f = (uint8_t*) frame;
    command_t out1, in1 = cmd_ping;
    uint8_t out2, in2 = 0x42;
    uint8_t out3 = 0xff;
    uint32_t len = protocol_create_response(f, g_max_frame_size, in1, in2);
    if (len == 0) {
        if (!g_expecting_failure) {
            printf(" %s: frame creation failed\n", test_name);
        }
        return false;
    }
    EXTRACT_PAYLOAD();
    DECLARE_UNPACK(frame, res);
    UNPACK8(out1);
    UNPACK8(out2);
    UNPACK8(out3);
    COMPARE(1, in1 | cmd_response, out1);
    COMPARE(2, in2, out2);
    COMPARE(3, 0, out3); // There is no third byte in the frame
    return true;
}

static bool test_ping(char *test_name)
{
    uint8_t frame[FRAME_OVERHEAD(MAX_FRAME_SIZE)], *f = (uint8_t*) frame;
    uint32_t len = protocol_create_ping(f, g_max_frame_size);
    if (len == 0) {
        if (!g_expecting_failure) {
            printf(" %s: frame creation failed\n", test_name);
        }
        return false;
    }
    EXTRACT_PAYLOAD();
    return true;
}

static bool test_response(char *test_name)
{
    uint8_t frame[FRAME_OVERHEAD(MAX_FRAME_SIZE)], *f = (uint8_t*) frame;
    command_t out1, in1 = cmd_ping;
    uint8_t out2, in2 = 0x42;
    uint32_t len = protocol_create_response(f, g_max_frame_size, in1, in2);
    if (len == 0) {
        if (!g_expecting_failure) {
            printf(" %s: frame creation failed\n", test_name);
        }
        return false;
    }
    EXTRACT_PAYLOAD();
    len = protocol_unpack_response(f, res, &out1, &out2);
    if (len == 0) {
        printf("%s: unpack response failed with %d\n", test_name, res);
        return false;
    }
    COMPARE(1, in1 | cmd_response, out1);
    COMPARE(2, in2, out2);
    return true;
}

static bool test_vout(char *test_name)
{
    uint8_t frame[FRAME_OVERHEAD(MAX_FRAME_SIZE)], *f = (uint8_t*) frame;
    uint16_t out, in = 0x1234;
    uint32_t len = protocol_create_vout(f, g_max_frame_size, in);
    if (len == 0) {
        if (!g_expecting_failure) {
            printf(" %s: frame creation failed\n", test_name);
        }
        return false;
    }
    EXTRACT_PAYLOAD();
    if (!protocol_unpack_vout(f, res, &out)) {
        printf("%s: unpack response failed\n", test_name);
        return false;
    }
    COMPARE(1, in, out);
    return true;
}

static bool test_ilimit(char *test_name)
{
    uint8_t frame[FRAME_OVERHEAD(MAX_FRAME_SIZE)], *f = (uint8_t*) frame;
    uint16_t out, in = 0x1234;
    uint32_t len = protocol_create_ilimit(f, g_max_frame_size, in);
    if (len == 0) {
        if (!g_expecting_failure) {
            printf(" %s: frame creation failed\n", test_name);
        }
        return false;
    }
    EXTRACT_PAYLOAD();
    if (!protocol_unpack_ilimit(f, res, &out)) {
        printf("%s: unpack response failed\n", test_name);
        return false;
    }
    COMPARE(1, in, out);
    return true;
}

static bool test_status(char *test_name)
{
    uint8_t frame[FRAME_OVERHEAD(MAX_FRAME_SIZE)], *f = (uint8_t*) frame;
    uint32_t len = protocol_create_status(f, g_max_frame_size);
    command_t out;
    if (len == 0) {
        if (!g_expecting_failure) {
            printf(" %s: frame creation failed\n", test_name);
        }
        return false;
    }
    EXTRACT_PAYLOAD();
    DECLARE_UNPACK(frame, res);
    UNPACK8(out);
    COMPARE(1, cmd_status, out);
    return true;
}

static bool test_status_response(char *test_name)
{
    uint8_t frame[FRAME_OVERHEAD(MAX_FRAME_SIZE)], *f = (uint8_t*) frame;
    uint16_t out1, in1 = 0x0123;
    uint16_t out2, in2 = 0x4567;
    uint16_t out3, in3 = 0x89ab;
    uint16_t out4, in4 = 0xcdef;
    uint16_t out5, in5 = 0x1234;
    uint8_t out6, in6 = 1;

    uint32_t len = protocol_create_status_response(f, g_max_frame_size, in1, in2, in3, in4, in5, in6);
    if (len == 0) {
        if (!g_expecting_failure) {
            printf(" %s: frame creation failed\n", test_name);
        }
        return false;
    }
    EXTRACT_PAYLOAD();
    if (!protocol_unpack_status_response(f, res, &out1, &out2, &out3, &out4, &out5, &out6)) {
        printf("%s: unpack response failed\n", test_name);
        return false;
    }
    COMPARE(1, in1, out1);
    COMPARE(2, in2, out2);
    COMPARE(3, in3, out3);
    COMPARE(4, in4, out4);
    COMPARE(5, in5, out5);
    COMPARE(6, in6, out6);
    return true;
}

static bool test_wifi(char *test_name)
{
    uint8_t frame[FRAME_OVERHEAD(MAX_FRAME_SIZE)], *f = (uint8_t*) frame;
    wifi_status_t out, in = wifi_connected;
    uint32_t len = protocol_create_wifi_status(f, g_max_frame_size, in);
    if (len == 0) {
        if (!g_expecting_failure) {
            printf(" %s: frame creation failed\n", test_name);
        }
        return false;
    }
    EXTRACT_PAYLOAD();
    if (!protocol_unpack_wifi_status(f, res, &out)) {
        printf("%s: unpack response failed\n", test_name);
        return false;
    }
    COMPARE(1, in, out);
    return true;
}

static bool test_lock(char *test_name)
{
    uint8_t frame[FRAME_OVERHEAD(MAX_FRAME_SIZE)], *f = (uint8_t*) frame;
    uint8_t out, in = 1;
    uint32_t len = protocol_create_lock(f, g_max_frame_size, in);
    if (len == 0) {
        if (!g_expecting_failure) {
            printf(" %s: frame creation failed\n", test_name);
        }
        return false;
    }
    EXTRACT_PAYLOAD();
    if (!protocol_unpack_lock(f, res, &out)) {
        printf("%s: unpack response failed\n", test_name);
        return false;
    }
    COMPARE(1, in, out);
    return true;
}

static bool test_ocp(char *test_name)
{
    uint8_t frame[FRAME_OVERHEAD(MAX_FRAME_SIZE)], *f = (uint8_t*) frame;
    uint16_t out, in = _EOF;
    uint32_t len = protocol_create_ocp(f, g_max_frame_size, in);
    if (len == 0) {
        if (!g_expecting_failure) {
            printf(" %s: frame creation failed\n", test_name);
        }
        return false;
    }
    EXTRACT_PAYLOAD();
    if (!protocol_unpack_ocp(f, res, &out)) {
        printf("%s: unpack response failed\n", test_name);
        return false;
    }
    COMPARE(1, in, out);
    return true;
}

static bool test_power_enable(char *test_name)
{
    uint8_t frame[FRAME_OVERHEAD(MAX_FRAME_SIZE)], *f = (uint8_t*) frame;
    uint8_t out, in = 1;
    uint32_t len = protocol_create_power_enable(f, g_max_frame_size, in);

    if (len == 0) {
        if (!g_expecting_failure) {
            printf(" %s: frame creation failed\n", test_name);
        }
        return false;
    }
    EXTRACT_PAYLOAD();
    if (!protocol_unpack_power_enable(f, res, &out)) {
        printf("%s: unpack response failed\n", test_name);
        return false;
    }
    COMPARE(1, in, out);
    return true;
}

// Unit testing failed to find this test where the crc is escaped :-/
static void crc_escape_test(void)
{
    uint8_t frame_buffer[32];
    uint32_t len = protocol_create_status_response(frame_buffer, sizeof(frame_buffer), 7750, 5000, 0, 0, 250, 0);
    int32_t res = uframe_extract_payload(frame_buffer, len);
    printf(" crc_escape_test ");
    if (res > 0) {
        printf("pass\n");
        g_num_pass++;
    } else {
        printf("fail\n");
        g_num_fail++;
    }
}

int main(int argc, char const *argv[])
{
    uint8_t f1[] = {0x40};
    uint8_t f2[] = {_DLE};
    uint8_t f3[] = {_DLE, 1};
    uint8_t f4[] = {_DLE, _SOF};
    uint8_t f5[] = {_DLE, _XOR};
    uint8_t f6[] = {_SOF, _EOF};

    // Test framing wrf SOF/EOF and escaping
    RUN_FRAMING_TEST(f1);
    RUN_FRAMING_TEST(f2);
    RUN_FRAMING_TEST(f3);
    RUN_FRAMING_TEST(f4);
    RUN_FRAMING_TEST(f5);
    RUN_FRAMING_TEST(f6);

    // Test the protocol suite by creating and extracting frames
    g_expecting_failure = false;
    g_max_frame_size = FRAME_OVERHEAD(MAX_FRAME_SIZE);
    RUN_PROTOCOL_TEST(test_small_frame);
    RUN_PROTOCOL_TEST(test_ping);
    RUN_PROTOCOL_TEST(test_response);
    RUN_PROTOCOL_TEST(test_vout);
    RUN_PROTOCOL_TEST(test_ilimit);
    RUN_PROTOCOL_TEST(test_status);
    RUN_PROTOCOL_TEST(test_status_response);
    RUN_PROTOCOL_TEST(test_wifi);
    RUN_PROTOCOL_TEST(test_lock);
    RUN_PROTOCOL_TEST(test_ocp);
    RUN_PROTOCOL_TEST(test_power_enable);


    g_max_frame_size = 2;
    g_expecting_failure = true;
    RUN_PROTOCOL_TEST(test_small_frame);
    RUN_PROTOCOL_TEST(test_ping);
    RUN_PROTOCOL_TEST(test_response);
    RUN_PROTOCOL_TEST(test_vout);
    RUN_PROTOCOL_TEST(test_ilimit);
    RUN_PROTOCOL_TEST(test_status);
    RUN_PROTOCOL_TEST(test_status_response);
    RUN_PROTOCOL_TEST(test_wifi);
    RUN_PROTOCOL_TEST(test_lock);
    RUN_PROTOCOL_TEST(test_ocp);

    crc_escape_test();

    printf("\n");
    if (g_num_fail == 0) {
        printf("All tests passed\n");
    } else if (g_num_pass == 0) {
        printf("All tests failed!\n");
    } else {
        printf ("%d/%d test failed\n", g_num_fail, g_num_pass);
    }
    printf("\n");

	return 0;
}
