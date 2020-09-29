/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Chester Tseng
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*****************************************************************************
 *                              Header includes
 * ***************************************************************************/
// micropython headers
#include "py/mpstate.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "rtc_api.h"
#include "timeutils.h"
#include "objrtc.h"
#include "utime_mphal.h"

/*****************************************************************************
 *                              External variables
 * ***************************************************************************/

STATIC mp_obj_t time_time(void) {
    time_t time_sec = rtc_read();
    return mp_obj_new_int_from_uint(time_sec);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(time_time_obj, time_time);

STATIC mp_obj_t time_localtime(mp_uint_t n_args, const mp_obj_t *args) {
    timeutils_struct_time_t tm;
    mp_int_t seconds = 0;

    if (n_args == 0 || args[0] == mp_const_none) {
        seconds = rtc_read();
    } else {
        seconds = mp_obj_get_int(args[0]);
    }
    timeutils_seconds_since_2000_to_struct_time(seconds, &tm);
    mp_obj_t tuple[8] = {
        tuple[0] = mp_obj_new_int(tm.tm_year),
        tuple[1] = mp_obj_new_int(tm.tm_mon),
        tuple[2] = mp_obj_new_int(tm.tm_mday),
        tuple[3] = mp_obj_new_int(tm.tm_hour),
        tuple[4] = mp_obj_new_int(tm.tm_min),
        tuple[5] = mp_obj_new_int(tm.tm_sec),
        tuple[6] = mp_obj_new_int(tm.tm_wday),
        tuple[7] = mp_obj_new_int(tm.tm_yday),
    };

    return mp_obj_new_tuple(8, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_localtime_obj, 0, 1, time_localtime);

STATIC mp_obj_t time_mktime(mp_obj_t tuple) {

    mp_uint_t len;
    mp_obj_t *elem;

    mp_obj_get_array(tuple, &len, &elem);

    // localtime generates a tuple of len 8. CPython uses 9, so we accept both.
    if (len < 8 || len > 9) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "mktime needs a tuple of length 8 or 9 (%d given)", len));
    }

    return mp_obj_new_int_from_uint(timeutils_mktime(mp_obj_get_int(elem[0]),
            mp_obj_get_int(elem[1]), mp_obj_get_int(elem[2]), mp_obj_get_int(elem[3]),
            mp_obj_get_int(elem[4]), mp_obj_get_int(elem[5])));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(time_mktime_obj, time_mktime);

STATIC const mp_map_elem_t time_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__),   MP_OBJ_NEW_QSTR(MP_QSTR_utime) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_time),       MP_OBJ_FROM_PTR(&time_time_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_localtime),  MP_OBJ_FROM_PTR(&time_localtime_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_mktime),     MP_OBJ_FROM_PTR(&time_mktime_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sleep),      MP_OBJ_FROM_PTR(&mp_utime_sleep_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sleep_ms),   MP_OBJ_FROM_PTR(&mp_utime_sleep_ms_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sleep_us),   MP_OBJ_FROM_PTR(&mp_utime_sleep_us_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ticks_ms),   MP_OBJ_FROM_PTR(&mp_utime_ticks_ms_obj) },
#if 0 // Not support us level
    { MP_OBJ_NEW_QSTR(MP_QSTR_ticks_us),   MP_OBJ_FROM_PTR(&mp_utime_ticks_us_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ticks_cpu),  MP_OBJ_FROM_PTR(&mp_utime_ticks_cpu_obj) },
#endif
    { MP_OBJ_NEW_QSTR(MP_QSTR_ticks_add),  MP_OBJ_FROM_PTR(&mp_utime_ticks_add_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ticks_diff), MP_OBJ_FROM_PTR(&mp_utime_ticks_diff_obj) },
};
STATIC MP_DEFINE_CONST_DICT(time_module_globals, time_module_globals_table);

const mp_obj_module_t mp_module_utime = {
    .base    = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&time_module_globals,
};
