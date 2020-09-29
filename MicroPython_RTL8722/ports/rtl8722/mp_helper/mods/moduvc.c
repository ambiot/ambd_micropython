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

#include "usb.h"
#include "uvc_intf.h"

STATIC struct uvc_buf_context buf;

STATIC mp_obj_t uvc_init(void) {
    _usb_init();
    if (wait_usb_ready() < 0) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "USB driver init failed"));
    }
    if (uvc_stream_init() < 0) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "USB stream init failed"));
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(uvc_init_obj, uvc_init);

STATIC mp_obj_t uvc_deinit(void) {
    uvc_stream_off();
    uvc_stream_free();
    _usb_deinit();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(uvc_deinit_obj, uvc_deinit);

STATIC mp_obj_t uvc_is_ready(void) {
    if (uvc_is_stream_ready() < 0)
        return mp_const_false;
    else
        return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(uvc_is_ready_obj, uvc_is_ready);

STATIC mp_obj_t uvc_enable(void) {
    if (uvc_stream_on() < 0)
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "UVC enable stream failed"));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(uvc_enable_obj, uvc_enable);

STATIC mp_obj_t uvc_disable(void) {
    uvc_stream_off();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(uvc_disable_obj, uvc_disable);

STATIC mp_obj_t uvc_frame(mp_obj_t buf_in) {
    int16_t ret = 0;

    mp_buffer_info_t bufinfo;

    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_WRITE);

    if (bufinfo.len < buf.len)
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Bytearray size is smaller than queue size"));

    memcpy(bufinfo.buf, buf.data, buf.len);
    
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uvc_frame_obj, uvc_frame);

STATIC mp_obj_t uvc_enqueue(void) {
    int16_t ret = 0;
    ret = uvc_qbuf(&buf);

    if (ret < 0)
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Enqueue failed"));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(uvc_enqueue_obj, uvc_enqueue);

STATIC mp_obj_t uvc_dequeue(void) {
    int16_t ret = 0;

    ret = uvc_dqbuf(&buf);

    if (ret < 0)
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Dequeue failed"));

    if (uvc_buf_check(&buf) < 0)
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Check queue failed"));

    return mp_obj_new_int(buf.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(uvc_dequeue_obj, uvc_dequeue);

STATIC mp_obj_t uvc_format(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_format,                     MP_ARG_INT, {.u_int  = UVC_FORMAT_MJPEG} },
        { MP_QSTR_width,    MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int  = 320} },
        { MP_QSTR_height,   MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int  = 240} },
        { MP_QSTR_frate,    MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int  = 30} },
        { MP_QSTR_ratio,    MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int  = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args); 
    if (uvc_set_param(args[0].u_int, args[1].u_int, args[2].u_int, args[3].u_int, args[4].u_int) < 0)
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "UVC set parameter failed"));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(uvc_format_obj, 0, uvc_format);

/*****************************************************************************
 *                              External variables
 * ***************************************************************************/
STATIC const mp_map_elem_t uvc_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__),      MP_OBJ_NEW_QSTR(MP_QSTR_uvc) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_init),          (mp_obj_t)&uvc_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_deinit),        (mp_obj_t)&uvc_deinit_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_ready),      (mp_obj_t)&uvc_is_ready_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_enable),        (mp_obj_t)&uvc_enable_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_disable),       (mp_obj_t)&uvc_disable_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_format),        (mp_obj_t)&uvc_format_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_frame),         (mp_obj_t)&uvc_frame_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_enqueue),       (mp_obj_t)&uvc_enqueue_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_dequeue),       (mp_obj_t)&uvc_dequeue_obj },


    { MP_OBJ_NEW_QSTR(MP_QSTR_FMT_MJPEG),     MP_OBJ_NEW_SMALL_INT(UVC_FORMAT_MJPEG) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_FMT_H264),      MP_OBJ_NEW_SMALL_INT(UVC_FORMAT_H264) },
};
STATIC MP_DEFINE_CONST_DICT(uvc_module_globals, uvc_module_globals_table);

const mp_obj_module_t mp_uvc_module = {
    .base    = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&uvc_module_globals,
};
