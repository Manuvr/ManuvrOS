/*
File:   ManuvrGPS.h
Author: J. Ian Lindsay
Date:   2016.06.29

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This is a basic class for parsing NMEA sentences from a GPS and emitting
  them as annotated messages.

This class in unidirectional in the sense that it only reads from the
  associated transport. Hardware that has bidirectional capability for
  whatever reason can extend this class into something with a non-trivial
  fromCounterparty() method.

This class was an adaption to Manuvr based on Kosma Moczek's minmea.
https://github.com/cloudyourcar/minmea
*/

/*
* Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
* This program is free software. It comes without any warranty, to the extent
* permitted by applicable law. You can redistribute it and/or modify it under
* the terms of the Do What The Fuck You Want To Public License, Version 2, as
* published by Sam Hocevar. See the COPYING file for more details.
*/


#ifndef __MANUVR_BASIC_GPS_H__
#define __MANUVR_BASIC_GPS_H__

#include <DataStructures/BufferPipe.h>
#include <Drivers/Sensors/SensorWrapper.h>
#include <Kernel.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <math.h>


#define MINMEA_MAX_LENGTH          140
#define MANUVR_MSG_GPS_LOCATION    0x2039

/* These are integer representations of the three-letter sentence IDs. */
#define MINMEA_INT_SENTENCE_CODE_RMC   0x00524d43
#define MINMEA_INT_SENTENCE_CODE_GGA   0x00474741
#define MINMEA_INT_SENTENCE_CODE_GSA   0x00475341
#define MINMEA_INT_SENTENCE_CODE_GLL   0x00474c4c
#define MINMEA_INT_SENTENCE_CODE_GST   0x00475354
#define MINMEA_INT_SENTENCE_CODE_GSV   0x00475356
#define MINMEA_INT_SENTENCE_CODE_VTG   0x00565447



/*******************************************************************************
* Undigested GPS types. Gradually morph these into Manuvr.                     *
*******************************************************************************/

enum minmea_sentence_id {
    MINMEA_INVALID = -1,
    MINMEA_UNKNOWN = 0,
    MINMEA_SENTENCE_RMC,
    MINMEA_SENTENCE_GGA,
    MINMEA_SENTENCE_GSA,
    MINMEA_SENTENCE_GLL,
    MINMEA_SENTENCE_GST,
    MINMEA_SENTENCE_GSV,
    MINMEA_SENTENCE_VTG,
};

enum minmea_gll_status {
    MINMEA_GLL_STATUS_DATA_VALID     = 'A',
    MINMEA_GLL_STATUS_DATA_NOT_VALID = 'V',
};

// FAA mode added to some fields in NMEA 2.3.
enum minmea_faa_mode {
    MINMEA_FAA_MODE_AUTONOMOUS   = 'A',
    MINMEA_FAA_MODE_DIFFERENTIAL = 'D',
    MINMEA_FAA_MODE_ESTIMATED    = 'E',
    MINMEA_FAA_MODE_MANUAL       = 'M',
    MINMEA_FAA_MODE_SIMULATED    = 'S',
    MINMEA_FAA_MODE_NOT_VALID    = 'N',
    MINMEA_FAA_MODE_PRECISE      = 'P',
};

enum minmea_gsa_mode {
    MINMEA_GPGSA_MODE_AUTO   = 'A',
    MINMEA_GPGSA_MODE_FORCED = 'M',
};

enum minmea_gsa_fix_type {
    MINMEA_GPGSA_FIX_NONE = 1,
    MINMEA_GPGSA_FIX_2D = 2,
    MINMEA_GPGSA_FIX_3D = 3,
};

struct minmea_float {
    int_least32_t value;
    int_least32_t scale;
};

struct minmea_date {
    int day;
    int month;
    int year;
};

struct minmea_time {
    int hours;
    int minutes;
    int seconds;
    int microseconds;
};

struct minmea_sentence_rmc {
    struct minmea_time time;
    bool valid;
    struct minmea_float latitude;
    struct minmea_float longitude;
    struct minmea_float speed;
    struct minmea_float course;
    struct minmea_date date;
    struct minmea_float variation;
};

struct minmea_sentence_gga {
    struct minmea_time time;
    struct minmea_float latitude;
    struct minmea_float longitude;
    int fix_quality;
    int satellites_tracked;
    struct minmea_float hdop;
    struct minmea_float altitude; char altitude_units;
    struct minmea_float height; char height_units;
    int dgps_age;
};

struct minmea_sentence_gll {
    struct minmea_float latitude;
    struct minmea_float longitude;
    struct minmea_time time;
    char status;
    char mode;
};

struct minmea_sentence_gst {
    struct minmea_time time;
    struct minmea_float rms_deviation;
    struct minmea_float semi_major_deviation;
    struct minmea_float semi_minor_deviation;
    struct minmea_float semi_major_orientation;
    struct minmea_float latitude_error_deviation;
    struct minmea_float longitude_error_deviation;
    struct minmea_float altitude_error_deviation;
};

struct minmea_sentence_gsa {
    char mode;
    int fix_type;
    int sats[12];
    struct minmea_float pdop;
    struct minmea_float hdop;
    struct minmea_float vdop;
};

struct minmea_sat_info {
    int nr;
    int elevation;
    int azimuth;
    int snr;
};

struct minmea_sentence_gsv {
    int total_msgs;
    int msg_nr;
    int total_sats;
    struct minmea_sat_info sats[4];
};

struct minmea_sentence_vtg {
    struct minmea_float true_track_degrees;
    struct minmea_float magnetic_track_degrees;
    struct minmea_float speed_knots;
    struct minmea_float speed_kph;
    enum minmea_faa_mode faa_mode;
};

/**
 * Rescale a fixed-point value to a different scale. Rounds towards zero.
 */
static inline int_least32_t minmea_rescale(struct minmea_float *f, int_least32_t new_scale)
{
    if (f->scale == 0)
        return 0;
    if (f->scale == new_scale)
        return f->value;
    if (f->scale > new_scale)
        return (f->value + ((f->value > 0) - (f->value < 0)) * f->scale/new_scale/2) / (f->scale/new_scale);
    else
        return f->value * (new_scale/f->scale);
}

/**
 * Convert a fixed-point value to a floating-point value.
 * Returns NaN for "unknown" values.
 */
static inline float minmea_tofloat(struct minmea_float *f)
{
    if (f->scale == 0)
        return NAN;
    return (float) f->value / (float) f->scale;
}

/**
 * Convert a raw coordinate to a floating point DD.DDD... value.
 * Returns NaN for "unknown" values.
 */
static inline float minmea_tocoord(struct minmea_float *f)
{
    if (f->scale == 0)
        return NAN;
    int_least32_t degrees = f->value / (f->scale * 100);
    int_least32_t minutes = f->value % (f->scale * 100);
    return (float) degrees + (float) minutes / (60 * f->scale);
}


/*
* A BufferPipe that is specialized for parsing NMEA.
* After enough successful parsing, this class will emit messages into the
*   Kernel's general message queue containing framed high-level GPS data.
*/
class ManuvrGPS : public BufferPipe, public SensorWrapper {
  public:
    ManuvrGPS();
    ManuvrGPS(BufferPipe*);
    ~ManuvrGPS();

    /* Override from BufferPipe. */
    virtual int8_t fromCounterparty(ManuvrPipeSignal, void*);
    virtual int8_t toCounterparty(StringBuilder* buf, int8_t mm);
    virtual int8_t fromCounterparty(StringBuilder* buf, int8_t mm);

    /* Overrides from SensorWrapper */
    SensorError init();
    SensorError readSensor();
    SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    void printDebug(StringBuilder*);


  protected:
    const char* pipeName();


  private:
    uint32_t       _sentences_parsed   = 0;
    uint32_t       _sentences_rejected = 0;
    StringBuilder  _accumulator;

    void _class_init();

    /**
    * Tries to empty the accumulator, parsing sentences iteratively.
    */
    bool _attempt_parse();

    /**
    * Calculate raw sentence checksum. Does not check sentence integrity.
    */
    uint8_t _checksum(const char *sentence);

    /**
    * Check sentence validity and checksum. Returns true for valid sentences.
    */
    bool _check(const char *sentence, bool strict);

    /**
    * Determine talker identifier.
    */
    bool _talker_id(char talker[3], const char *sentence);

    /**
    * Determine sentence identifier.
    */
    enum minmea_sentence_id _sentence_id(const char *sentence, bool strict);

    /*
    * Parse a specific type of sentence. Return true on success.
    */
    bool _parse_rmc(struct minmea_sentence_rmc *frame, const char *sentence);
    bool _parse_gga(struct minmea_sentence_gga *frame, const char *sentence);
    bool _parse_gsa(struct minmea_sentence_gsa *frame, const char *sentence);
    bool _parse_gll(struct minmea_sentence_gll *frame, const char *sentence);
    bool _parse_gst(struct minmea_sentence_gst *frame, const char *sentence);
    bool _parse_gsv(struct minmea_sentence_gsv *frame, const char *sentence);
    bool _parse_vtg(struct minmea_sentence_vtg *frame, const char *sentence);

    /**
    * Scanf-like processor for NMEA sentences. Supports the following formats:
    * c - single character (char *)
    * d - direction, returned as 1/-1, default 0 (int *)
    * f - fractional, returned as value + scale (int *, int *)
    * i - decimal, default zero (int *)
    * s - string (char *)
    * t - talker identifier and type (char *)
    * T - date/time stamp (int *, int *, int *)
    * Returns true on success. See library source code for details.
    */
    bool _scan(const char *sentence, const char *format, ...);

    const char* _get_string_by_sentence_id(enum minmea_sentence_id);

    /**
    * Convert GPS UTC date/time representation to a UNIX timestamp.
    */
    int _gettime(struct timespec *ts, const struct minmea_date *date, const struct minmea_time *time_);
};

#endif   // __MANUVR_BASIC_GPS_H__
