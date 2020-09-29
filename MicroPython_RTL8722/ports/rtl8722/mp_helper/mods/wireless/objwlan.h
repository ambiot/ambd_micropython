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
#ifndef OBJWLAN_H_
#define OBJWLAN_H_

#include "py/mpconfig.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "wireless.h"
#include "modnetwork.h"
#include "objnetif.h"

#include "wifi_conf.h"

#define WLAN_MIN_SSID_LEN       3
#define WLAN_MAX_SSID_LEN       32

#define WLAN_WEP_MIN_PASS_CHAR_LEN  1
#define WLAN_WEP_MAX_PASS_CHAR_LEN  13

#define WLAN_WPA_MIN_PASS_CHAR_LEN  8
#define WLAN_WPA_MAC_PASS_CHAR_LEN  63

#define WLAN_PROMISC_ENABLE 0

extern const mp_obj_type_t wlan_type;

void wlan_init0(void);
void validate_wlan_mode(uint8_t mode);
void validate_ssid(mp_uint_t len);
void validate_key(int8_t **key, uint8_t *key_len, uint32_t *sec_type, mp_obj_t obj);
void validate_channel(uint8_t channel);

typedef struct {
    mp_obj_base_t base;
    uint8_t       mode;                       // WLAN mode, STA / AP / STA_AP 
    int32_t       security_type;              // WLAN security mode, WPA2 / WEP ...
    uint8_t       channel;                    // WLAN channel 0 ~ 11
    int8_t        ssid[WLAN_MAX_SSID_LEN];    // WLAN STA mode ssid
#if WLAN_PROMISC_ENABLE
    uint8_t       promisc_level;
#endif
    netif_obj_t   netif[];
} wlan_obj_t;

#endif  // OBJWLAN_H_
