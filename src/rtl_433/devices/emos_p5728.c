/** @file
    EMOS P5728 Doorbell.

    Changes done by ErNis <ly3ph@qsl.lt>. Copyright is
    retained by Robert Fraczkiewicz.

    Copyright (C) 2017 Robert Fraczkiewicz <aromring@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/
/**
EMOS P5728 Doorbell.

A count enables us to determine the quality of radio transmission.

*/

#include "decoder.h"

static int emos_p5728_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
    data_t *data;
    int row;
    int device;
    uint8_t *bytes;
    char const *alert = "Unknown";

    //decoder_log(decoder, 0, __func__, "EMOS P5728 Doorbell");

    // The device transmits many rows, check for 10 matching rows.
    row = bitbuffer_find_repeated_row(bitbuffer, 10, 18);
    if (row < 0) {
        return DECODE_ABORT_EARLY;
    }
    bytes = bitbuffer->bb[row];

    if (bitbuffer->bits_per_row[row] != 18) {
        return DECODE_ABORT_LENGTH;
    }

    bitbuffer_invert(bitbuffer);

    // No need to decode/extract values for simple test
    if ((!bytes[0] && !bytes[1] && !bytes[2])
       || (bytes[0] == 0xff && bytes[1] == 0xff && bytes[2] == 0xff)) {
        decoder_log(decoder, 2, __func__, "DECODE_FAIL_SANITY data all 0x00 or 0xFF");
        return DECODE_FAIL_SANITY;
    }

    device = bytes[0] << 16 | bytes[1] << 8 | bytes[2];

    decoder_logf(decoder, 0, __func__, "device: %x", device);

    if (device == 0xd79fc0) {
        alert = "Normal";
    } else {
        decoder_logf(decoder, 1, __func__, "unknown device: %x", device);
        return DECODE_FAIL_OTHER;
    }

    /* clang-format off */
    data = data_make(
            "model",         "",            DATA_STRING, "EMOS P5728 Doorbell",
            "subtype",       "Class",       DATA_STRING, "Doorbell",
            "id",            "Id",          DATA_FORMAT, "%x",   DATA_INT,    device,
            "alert",         "Alert",       DATA_FORMAT, "%s",   DATA_STRING, alert,
            NULL);
    /* clang-format on */

    decoder_output_data(decoder, data);
    return 1;
}

static char const *const output_fields[] = {
        "model",
        "subtype",
        "id",
        "alert",
        NULL,
};

r_device const emos_p5728 = {
        .name        = "EMOS P5728 Doorbell",
        .modulation  = OOK_PULSE_PWM,
        .short_width = 288,
        .long_width  = 872,
        .gap_limit   = 0,
        .reset_limit = 5380,
        .sync_width  = 1188,
        .decode_fn   = &emos_p5728_decode,
        .fields      = output_fields,
};
