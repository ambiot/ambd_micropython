/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

#include <stdio.h>
#include <stdint.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "extmod/machine_mem.h"

#include "py/objint.h"

extern const struct _mp_obj_module_t mp_module_umachine;
extern const struct _mp_obj_module_t mp_module_uos;
extern const struct _mp_obj_module_t mp_module_utime;
extern const struct _mp_obj_module_t mp_module_uwireless;
extern const struct _mp_obj_module_t mp_module_usocket;

STATIC const mp_map_elem_t ameba_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), 	MP_OBJ_NEW_QSTR(MP_QSTR_modules) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_machine),     MP_OBJ_FROM_PTR(&mp_module_umachine) },   
    { MP_OBJ_NEW_QSTR(MP_QSTR_os),          MP_OBJ_FROM_PTR(&mp_module_uos) },        
    { MP_OBJ_NEW_QSTR(MP_QSTR_time),        MP_OBJ_FROM_PTR(&mp_module_utime) },   
    { MP_OBJ_NEW_QSTR(MP_QSTR_wireless),    MP_OBJ_FROM_PTR(&mp_module_uwireless) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_socket),    MP_OBJ_FROM_PTR(&mp_module_usocket) },
};

STATIC MP_DEFINE_CONST_DICT(ameba_module_globals, ameba_module_globals_table);

const mp_obj_module_t mp_module_modules = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&ameba_module_globals,
};
