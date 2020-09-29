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

#include "objspi.h"

#define SPI_MAX 2       //max 2 sets of SPI supported on Ameba D
static uint8_t id = 0; // default SPI idx id is 0 

spi_t mp_spi_obj[SPI_MAX];  // MBED obj 

STATIC spi_obj_t spi_obj[2] = {
    {.base.type = &spi_type, .unit = 0, .bits = 8, .baudrate = SPI_DEFAULT_BAUD_RATE, .pol = SCPOL_INACTIVE_IS_LOW, .pha = SCPH_TOGGLES_IN_MIDDLE },
    {.base.type = &spi_type, .unit = 1, .bits = 8, .baudrate = SPI_DEFAULT_BAUD_RATE, .pol = SCPOL_INACTIVE_IS_LOW, .pha = SCPH_TOGGLES_IN_MIDDLE },
};


STATIC void spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    spi_obj_t *self = self_in;
    if (id == 0) {
        mp_printf(print, "SPI(%d, baudrate=%u, bits=%d, MOSI=PB_18, MISO=PB_19, SCLK=PB_20, CS=PB_21 )", 
            self->unit, self->baudrate, self->bits);
    } else {
        mp_printf(print, "SPI(%d, baudrate=%u, bits=%d, MOSI=PB_4, MISO=PB_5, SCLK=PB_6, CS=PB_7 )", 
            self->unit, self->baudrate, self->bits);
    }
}


STATIC mp_obj_t spi_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw,
        const mp_obj_t *all_args) {
    enum { ARG_unit, ARG_baudrate, ARG_pol, ARG_pha, ARG_bits, ARG_firstbit, ARG_miso, ARG_mosi, ARG_sck };
    const mp_arg_t spi_init_args[] = {
        { MP_QSTR_unit,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_baudrate, MP_ARG_INT,                  {.u_int = SPI_DEFAULT_BAUD_RATE} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SCPOL_INACTIVE_IS_LOW} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SCPH_TOGGLES_IN_MIDDLE} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MICROPY_PY_MACHINE_SPI_MSB} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ },
        { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ },
    };
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(spi_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), spi_init_args, args);

    if (args[ARG_unit].u_int > 1)
        mp_raise_ValueError("Invalid SPI unit");

    id = args[ARG_unit].u_int;
    //printf("id is %d\n", id);

    if (args[ARG_unit].u_int == 0) {

        mp_spi_obj[id].spi_idx = MBED_SPI0;
        spi_init(&mp_spi_obj[id],  SPI_0_MOSI, SPI_0_MISO, SPI_0_SCLK, SPI_0_CS);
        spi_format(&mp_spi_obj[id], 8, 0, 0);                      // 8 bits, mode 0[polarity=0,phase=0], and master-role
        spi_frequency(&mp_spi_obj, SPI_DEFAULT_BAUD_RATE);      // default 2M baud rate
        //printf("SPI0 init finished\n");

    } else if (args[ARG_unit].u_int == 1) {

        mp_spi_obj[id].spi_idx = MBED_SPI1;
        spi_init(&mp_spi_obj[id], SPI_1_MOSI, SPI_1_MISO, SPI_1_SCLK, SPI_1_CS);
        spi_format(&mp_spi_obj[id], 8, 0, 0);
        spi_frequency(&mp_spi_obj[id], SPI_DEFAULT_BAUD_RATE);
        //printf("SPI1 init finished\n");
    }

    spi_obj_t *self  = &spi_obj[args[ARG_unit].u_int];
    self->baudrate = args[ARG_baudrate].u_int;
    self->bits     = args[ARG_bits].u_int;

    return (mp_obj_t)self;
}


STATIC void spi_stop(mp_obj_base_t *self_in) {
    spi_free(&mp_spi_obj[id]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(spi_stop_obj, spi_stop);


STATIC int spi_read(mp_obj_base_t *self_in) {
    return spi_master_write(&mp_spi_obj[id], NULL);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(spi_read_obj, spi_read);


STATIC void spi_write(mp_obj_base_t *self_in, mp_obj_t value_in) {
    spi_obj_t *self = (spi_obj_t*)self_in;
    mp_int_t value = mp_obj_get_int(value_in);

    spi_master_write(&mp_spi_obj[id], value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(spi_write_obj, spi_write);


STATIC const mp_map_elem_t spi_locals_dict_table[] = {
    // basic SPI operations
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&spi_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&spi_write_obj) },

};
STATIC MP_DEFINE_CONST_DICT(spi_locals_dict, spi_locals_dict_table);


const mp_obj_type_t spi_type = {
    { &mp_type_type },
    .name        = MP_QSTR_SPI,
    .print       = spi_print,
    .make_new    = spi_make_new,
    .locals_dict = (mp_obj_dict_t *)&spi_locals_dict,
};
