/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Chester Tseng
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

#include "objwlan.h"
#include "modnetwork.h"
#include "objnetif.h"

/*****************************************************************************
 *                              Local variables
 * ***************************************************************************/
STATIC xSemaphoreHandle xSTAConnectAPSema;
rtw_mode_t wifi_mode;

extern struct netif xnetif[NET_IF_NUM];

STATIC wlan_obj_t wlan_obj = {
    .base.type      = &wlan_type,
    .mode           = RTW_MODE_STA,
    .security_type  = RTW_SECURITY_OPEN,
    .channel        = 6,
    .netif          = {
        { .base.type = &netif_type, .piface = &xnetif[0], .index = 0 },
        { .base.type = &netif_type, .piface = &xnetif[1], .index = 1 },
    }
};

/*****************************************************************************
 *                              Local functions
 * ***************************************************************************/
void validate_wlan_mode(uint8_t mode) {
    if (mode != RTW_MODE_AP &&
        mode != RTW_MODE_STA &&
        mode != RTW_MODE_STA_AP 
        ) {
            mp_raise_ValueError("Invalid WLAN mode");
    }
}

void validate_ssid(mp_uint_t len) {

    if ((len > WLAN_MAX_SSID_LEN) || (len < WLAN_MIN_SSID_LEN)) {
        mp_raise_ValueError("Invalid SSID length");
    }

}

void validate_key(int8_t **key, uint8_t *key_len, uint32_t *sec_type, mp_obj_t obj) {
    mp_obj_t *sec;

    if (obj != mp_const_none) {

        mp_obj_get_array_fixed_n(obj, 2, &sec);

        *sec_type = mp_obj_get_int(sec[0]);
        *key = mp_obj_str_get_data(sec[1], key_len);

        if (*sec_type != RTW_SECURITY_OPEN &&
            *sec_type != RTW_SECURITY_WEP_PSK &&
            *sec_type != RTW_SECURITY_WEP_SHARED &&
            *sec_type != RTW_SECURITY_WPA_TKIP_PSK &&
            *sec_type != RTW_SECURITY_WPA_AES_PSK &&
            *sec_type != RTW_SECURITY_WPA2_TKIP_PSK &&
            *sec_type != RTW_SECURITY_WPA2_AES_PSK &&
            *sec_type != RTW_SECURITY_WPA2_MIXED_PSK &&
            *sec_type != RTW_SECURITY_WPA_WPA2_MIXED &&
            *sec_type != RTW_SECURITY_WPS_OPEN &&
            *sec_type != RTW_SECURITY_WPS_SECURE) {
                mp_raise_ValueError("Invalid security type");
        }

        if (*sec_type == RTW_SECURITY_WPA_TKIP_PSK ||
            *sec_type == RTW_SECURITY_WPA_AES_PSK ||
            *sec_type == RTW_SECURITY_WPA2_TKIP_PSK ||
            *sec_type == RTW_SECURITY_WPA2_AES_PSK ||
            *sec_type == RTW_SECURITY_WPA2_MIXED_PSK ||
            *sec_type == RTW_SECURITY_WPA_WPA2_MIXED) {
            if (*key_len > WLAN_WPA_MAC_PASS_CHAR_LEN ||
                *key_len < WLAN_WPA_MIN_PASS_CHAR_LEN) {
                mp_raise_ValueError("Wrong WPA passphrase length");
            }
        }

        if (*sec_type == RTW_SECURITY_WEP_SHARED || 
            *sec_type == RTW_SECURITY_WEP_PSK) { 
            int8_t *key_temp = *key;

            if (*key_len > WLAN_WEP_MAX_PASS_CHAR_LEN ||
                *key_len < WLAN_WEP_MIN_PASS_CHAR_LEN) {
                mp_raise_ValueError("Wrong WEP key length");
            }

            for (uint8_t i=0; i<*key_len; i++) {
                //TODO(Chester) WEP key is digital char only?
                if (!unichar_isdigit(key_temp[i])) {
                    mp_raise_ValueError("Wrong WEP key value");
                }
            }

            //TODO(Chester) Should consider generate WEP key in bytearray format
            uint8_t wep_key[WLAN_WEP_MAX_PASS_CHAR_LEN];
            memset(wep_key, 0x0, sizeof(wep_key));
            for (uint8_t i=0, idx=0; i < strlen(key_temp); i++) {
                wep_key[idx] += unichar_xdigit_value(key_temp[i]);
                if ((i % 2) == 0) {
                    wep_key[idx] *= 16;
                } else {
                    idx++;
                }
            }
            memcpy(*key, wep_key, sizeof(wep_key));
            *key_len /= 2;
        }
    } else {
        *key = NULL;
        *key_len = 0;
    }
}

void validate_channel(uint8_t channel) {
    if (channel < 1 || channel > 11) {
        mp_raise_ValueError("Invalid WLAN channel");
    }
}

void wlan_init0(void) {
    // to wlan init here
    init_event_callback_list();
    xSTAConnectAPSema = xSemaphoreCreateBinary();

    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;

    netif_obj_t netif_obj_0 = wlan_obj.netif[0];

    netif_obj_t netif_obj_1 = wlan_obj.netif[1];

    netif_add(netif_obj_0.piface, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
    
    netif_add(netif_obj_1.piface, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

    netif_set_up(netif_obj_0.piface);

    netif_set_up(netif_obj_1.piface);

    netif_set_default(netif_obj_0.piface);
}

STATIC const qstr wlan_scan_info_fields[] = {
        MP_QSTR_ssid, MP_QSTR_bssid, MP_QSTR_rssi,
        MP_QSTR_bss, MP_QSTR_security, MP_QSTR_wps,
        MP_QSTR_channel, MP_QSTR_band
};

STATIC mp_obj_t wlan_scan(mp_obj_t self_in, mp_obj_t scan_type_in, mp_obj_t bss_type_in) {

    uint16_t scan_type = mp_obj_get_int(scan_type_in);
    uint16_t bss_type = mp_obj_get_int(bss_type_in);
    uint16_t flags = ((RTW_SCAN_COMMAMD << 4 | scan_type) | (bss_type << 8));
    wext_set_scan(WLAN0_NAME, NULL, 0, flags);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(wlan_scan_obj, wlan_scan);

STATIC mp_obj_t wlan_start_ap(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_ssid, ARG_auth};
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_ssid, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_auth, MP_ARG_REQUIRED | MP_ARG_OBJ },
    };
    int16_t ret = RTW_ERROR;

    wlan_obj_t *self = pos_args[0];

    if (self->mode != RTW_MODE_AP && self->mode != RTW_MODE_STA_AP) {
        mp_raise_ValueError("Only AP and STA_AP can start ap");
    }

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int8_t *ssid = NULL;
    mp_uint_t ssid_len = 0;

    ssid = mp_obj_str_get_data(args[ARG_ssid].u_obj, &ssid_len);

    validate_ssid(ssid_len);

    int8_t   *key = NULL;
    uint32_t  security_type = 0;
    mp_uint_t key_len;

    validate_key((uint8_t *)&key, &key_len, &security_type, args[ARG_auth].u_obj);

    if (key != NULL) {
        self->security_type = security_type;
    }

    if (is_promisc_enabled()) {
        promisc_set(0, NULL, 0);
    }

    const char *ifname = WLAN0_NAME;

    if (self->mode == RTW_MODE_STA_AP)
        ifname = WLAN1_NAME;

    ret = wext_set_mode(ifname, IW_MODE_MASTER);

    if (ret != RTW_SUCCESS) {
        mp_raise_OSError("[WLAN] Set master mode error");
    }
    ret = wext_set_channel(ifname, self->channel);

    if (ret != RTW_SUCCESS) {
        mp_raise_OSError("[WLAN] Set channel error");
    }

    switch (self->security_type) {
        case RTW_SECURITY_OPEN:
            ret = RTW_SUCCESS;
            break;
        case RTW_SECURITY_WPA2_AES_PSK:
            ret = wext_set_auth_param(ifname, IW_AUTH_80211_AUTH_ALG, IW_AUTH_ALG_OPEN_SYSTEM);
            if (ret == RTW_SUCCESS)
                ret = wext_set_key_ext(ifname, IW_ENCODE_ALG_CCMP, NULL, 0, 0, 0, 0, NULL, 0);

            if (ret == RTW_SUCCESS) 
                ret = wext_set_passphrase(ifname, key, key_len);
            break;
        default:
            ret = RTW_ERROR;
            break;
    }

    if (ret != RTW_SUCCESS)
        mp_raise_OSError("[WLAN] Set passphrase error");

    ret = wext_set_ap_ssid(ifname, ssid, ssid_len);

    if (ret != RTW_SUCCESS) 
      mp_raise_OSError("[WLAN] Set ssid error");

    dhcps_deinit();

    if (self->mode == RTW_MODE_AP) {
        dhcps_init(&xnetif[0]);
    } else if(self->mode == RTW_MODE_STA_AP) {
        dhcps_init(&xnetif[1]);
    } else {
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(wlan_start_ap_obj, 0, wlan_start_ap);

STATIC mp_obj_t wlan_rssi(mp_obj_t self_in) {

    int16_t rssi = 0;

    wlan_obj_t *self = self_in;

    int16_t ret = wext_get_rssi(WLAN0_NAME, &rssi);

    if (ret != RTW_SUCCESS) 
      mp_raise_OSError("[WLAN] Get RSSO failed");

    return mp_obj_new_int(rssi);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(wlan_rssi_obj, wlan_rssi);

STATIC mp_obj_t wlan_mac(mp_uint_t n_args, const mp_obj_t *args) {

    wlan_obj_t *self = args[0];

    int8_t rtl_cmd[32];
    int8_t mac_str[17];
    uint8_t mac_str_len = 0;
    int16_t ret = RTW_ERROR;

    if (n_args == 1) {
        sprintf(rtl_cmd, "read_mac");
        ret = wext_private_command_with_retval(WLAN0_NAME, rtl_cmd, mac_str, sizeof(mac_str));
        if (ret != RTW_SUCCESS) {
            mp_raise_msg(&mp_type_OSError, "Get MAC address error");
        }
        return mp_obj_new_str(mac_str, sizeof(mac_str));
    }
    else {
        //TODO(Chester) Should parse MAC string here
        int8_t *mac_ptr = mp_obj_str_get_data(args[1], &mac_str_len);

        // format is 00:00:00:00:00:00 = 6 * 2 + 5 = 17 chars
        if (mac_str_len > (ETH_ALEN * 2 + 5)) {
            mp_raise_ValueError("Wrong MAC address length");
        }
        sprintf(rtl_cmd, "write_mac %s", mac_ptr);
        ret = wext_private_command(WLAN0_NAME, rtl_cmd, 0);
        if (ret != RTW_SUCCESS) {
            mp_raise_msg(&mp_type_OSError, "Set MAC address error");
        }
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(wlan_mac_obj, 1, 2, wlan_mac);

STATIC mp_obj_t wlan_rf(mp_obj_t self_in, mp_obj_t enable_in) {

    wlan_obj_t *self = self_in;

    int16_t ret = RTW_ERROR;

    if (enable_in == mp_const_true) {
        ret = rltk_wlan_rf_on();
    } else if (enable_in == mp_const_false) {
        ret = rltk_wlan_rf_off();
    } else {
        mp_raise_ValueError("Invalid value");
    }

    if (ret != RTW_SUCCESS)
        mp_raise_OSError(ret);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(wlan_rf_obj, wlan_rf);

STATIC mp_obj_t wlan_channel(mp_uint_t n_args, const mp_obj_t *args) {
    wlan_obj_t *self = args[0];
    int16_t ret = RTW_ERROR;
    uint8_t channel = 0;

    if (n_args == 1) {
        ret = wext_get_channel(WLAN0_NAME, &channel);
        if (ret != RTW_SUCCESS) {
            mp_raise_msg(&mp_type_OSError, "Get WLAN channel error");
        }
        return mp_obj_new_int(channel);
    }
    else {
        channel = mp_obj_get_int(args[1]);
        validate_channel(channel);
        ret = wext_set_channel(WLAN0_NAME, channel);
        self->channel = channel;
        if (ret != RTW_SUCCESS) {
            mp_raise_msg(&mp_type_OSError, "Set WLAN channel error");
        }
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(wlan_channel_obj, 1, 2, wlan_channel);

STATIC mp_obj_t wlan_connect(mp_uint_t n_args, const mp_obj_t *pos_args,
        mp_map_t *kw_args) {
    enum { ARG_ssid, ARG_auth };
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_ssid,    MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_auth,    MP_ARG_REQUIRED | MP_ARG_OBJ },
    };
    
    wlan_obj_t *self = pos_args[0];

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args),
            allowed_args, args);

    if ((self->mode != RTW_MODE_STA) && (self->mode != RTW_MODE_STA_AP)) {
        mp_raise_ValueError("Only STA / STA_AP mode support wlan connect");
    }

    mp_uint_t ssid_len = 0;

    int8_t *ssid = mp_obj_str_get_data(args[ARG_ssid].u_obj, &ssid_len);

    validate_ssid(ssid_len);

    if (ssid != NULL) {
        memcpy(self->ssid, ssid, ssid_len);
    }

    int8_t *key = NULL;
    uint32_t security_type = 0;
    mp_uint_t key_len;
    validate_key((uint8_t *)&key, &key_len, &security_type, args[ARG_auth].u_obj);
    if (key != NULL) {
        self->security_type = security_type;
    }

    // Override the key and ken_len in open security type
    if (security_type == RTW_SECURITY_OPEN) {
        key = NULL;
        key_len = 0;
    }

    int16_t ret = RTW_ERROR;

    switch(security_type) {
        case RTW_SECURITY_OPEN:
            ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_NONE, NULL, 0, 0, 0, 0, NULL, 0);
            break;
        case RTW_SECURITY_WEP_PSK:
        case RTW_SECURITY_WEP_SHARED:
            ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG,
                    IW_AUTH_ALG_SHARED_KEY);
            break;
        case RTW_SECURITY_WPA_TKIP_PSK:
        case RTW_SECURITY_WPA2_TKIP_PSK:
            ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG,
                    IW_AUTH_ALG_OPEN_SYSTEM);
            if (ret == RTW_SUCCESS) 
                ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_TKIP, NULL,
                        0, 0, 0, 0, NULL, 0);
            if (ret == RTW_SUCCESS) 
                ret = wext_set_passphrase(WLAN0_NAME, key, key_len);
            break;
        case RTW_SECURITY_WPA_AES_PSK:
        case RTW_SECURITY_WPA2_AES_PSK:
        case RTW_SECURITY_WPA2_MIXED_PSK:
        case RTW_SECURITY_WPA_WPA2_MIXED:
            ret = wext_set_auth_param(WLAN0_NAME, IW_AUTH_80211_AUTH_ALG,
                    IW_AUTH_ALG_OPEN_SYSTEM);
            if (ret == RTW_SUCCESS) 
                ret = wext_set_key_ext(WLAN0_NAME, IW_ENCODE_ALG_CCMP, NULL,
                        0, 0, 0, 0, NULL, 0);
            if (ret == RTW_SUCCESS)
                ret = wext_set_passphrase(WLAN0_NAME, key, key_len);
            break;
        default:
            ret = RTW_ERROR;
            break;
    }

    if (ret == RTW_SUCCESS)
        ret = wext_set_ssid(WLAN0_NAME, ssid, ssid_len);
    
    if (xSemaphoreTake(xSTAConnectAPSema, 30000) != pdTRUE) {
        mp_raise_msg(&mp_type_OSError, "Connect to AP timeout");
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(wlan_connect_obj, 1, wlan_connect);

STATIC mp_obj_t wlan_disconnect(mp_obj_t self_in) {
    const uint8_t null_bssid[ETH_ALEN + 2] = {0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00};

    wlan_obj_t *self = self_in;

    if (self->mode != RTW_MODE_STA)
        mp_raise_OSError("Disconnect only support STA mode");

    int16_t ret = wext_set_bssid(WLAN0_NAME, null_bssid);

    if (ret != RTW_SUCCESS) {
        mp_raise_msg(&mp_type_OSError, "Disconnect error");
    }
    
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(wlan_disconnect_obj, wlan_disconnect);

STATIC mp_obj_t wlan_on(mp_uint_t n_args, const mp_obj_t *args) {
    int16_t ret = RTW_ERROR;

    wlan_obj_t *self = args[0];

    if (rltk_wlan_running(WLAN0_IDX)) {
        mp_raise_msg(&mp_type_OSError, "WLAN has already running, please turn it off first.");
    }

    int mode = self->mode;

    if (n_args > 1) {
        mode = mp_obj_get_int(args[1]);
        self->mode = mode;
        wifi_mode = mode;
    }

    // Init WLAN0 first
    ret = rltk_wlan_init(WLAN0_IDX, self->mode);

    // Init WLAN1 second if mode is RTW_MODE_STA_AP
    if (self->mode == RTW_MODE_STA_AP) {
        ret = rltk_wlan_init(WLAN1_IDX, self->mode);
    }

    if (ret != RTW_SUCCESS) {
        mp_raise_msg(&mp_type_OSError, "WLAN init error");
    }

    rltk_wlan_start(WLAN0_IDX);

    if (self->mode == RTW_MODE_STA_AP) {
        rltk_wlan_start(WLAN1_IDX);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(wlan_on_obj, 1, 2, wlan_on);

STATIC mp_obj_t wlan_off(mp_obj_t self_in) {

    wlan_obj_t *self = self_in;

    rltk_wlan_deinit();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(wlan_off_obj, wlan_off);

STATIC mp_obj_t wlan_wifi_is_running(mp_obj_t self_in, mp_obj_t idx_in) {

    mp_uint_t index = mp_obj_get_int(idx_in);

    if (index > 1) 
        mp_raise_ValueError("Invalid WLAN index");

    wlan_obj_t *self = self_in;

    if (rltk_wlan_running(index))
        return mp_const_true;
    
    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(wlan_wifi_is_running_obj, wlan_wifi_is_running);

STATIC mp_obj_t wlan_get_interface(mp_obj_t self_in, mp_obj_t idx_in) {

    mp_uint_t index = mp_obj_get_int(idx_in);

    if (index > 1) 
        mp_raise_ValueError("Invalid WLAN index");

    wlan_obj_t *self = self_in;

    netif_obj_t *netif = &self->netif[index];

    return netif;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(wlan_get_interface_obj, wlan_get_interface);

STATIC mp_obj_t wlan_is_connect_to_ap(mp_obj_t self_in) {

    wlan_obj_t *self = self_in;

    int16_t ret = rltk_wlan_is_connected_to_ap();

    if (ret != RTW_SUCCESS)
        return mp_const_false;

    return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(wlan_is_connect_to_ap_obj, wlan_is_connect_to_ap);

void wifi_event_scan_result_report_hdl (char *buf, int buf_len, int flags,
        void *userfunc) {

    mp_obj_t func = MP_OBJ_FROM_PTR(userfunc);

    if (func != mp_const_none) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {

            //TODO Should gc_lock ?
            
            /*
             *  Make a new tuple and send it to callback function, I decide not to do
             *  gc_lock because we need to pass new data to outside of callback
             */
            rtw_scan_result_t** ptr = (rtw_scan_result_t**) buf;

            mp_obj_t tuple[8];
            mp_obj_t attrtuple;

            tuple[0] = mp_obj_new_str((*ptr)->SSID.val, (*ptr)->SSID.len);
            tuple[1] = mp_obj_new_bytes((*ptr)->BSSID.octet, ETH_ALEN);
            tuple[2] = mp_obj_new_int((*ptr)->signal_strength);
            tuple[3] = mp_obj_new_int((*ptr)->bss_type);
            tuple[4] = mp_obj_new_int((*ptr)->security);
            tuple[5] = mp_obj_new_int((*ptr)->wps_type);
            tuple[6] = mp_obj_new_int((*ptr)->channel);
            tuple[7] = mp_obj_new_int((*ptr)->band);

            attrtuple = mp_obj_new_attrtuple(wlan_scan_info_fields, 8, tuple);

            mp_call_function_1(func, attrtuple);

            nlr_pop();
        } else {
            mp_printf(&mp_plat_print, "Uncaught exception in callback handler");
            if (nlr.ret_val != MP_OBJ_NULL) {
                mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
            }
        }
    }
}

void wifi_event_scan_done_hdl (char *buf, int buf_len, int flags,
        void *userfunc) {
    mp_obj_t func = MP_OBJ_FROM_PTR(userfunc);

    //TODO Should gc_lock ?
    
    if (func != mp_const_none) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            mp_call_function_0(func);
            nlr_pop();
        } else {
            mp_printf(&mp_plat_print, "Uncaught exception in callback handler");
            if (nlr.ret_val != MP_OBJ_NULL) {
                mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
            }
        }
    }
}

void wifi_event_beacon_after_dhcp_hdl (char *buf, int buf_len, int flags,
        void *userfunc) {

    mp_obj_t func = MP_OBJ_FROM_PTR(userfunc);

    if (func != mp_const_none) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {

            //TODO Should gc_lock ?
            
            mp_call_function_0(func);

            nlr_pop();
        } else {
            mp_printf(&mp_plat_print, "Uncaught exception in callback handler");
            if (nlr.ret_val != MP_OBJ_NULL) {
                mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
            }
        }
    }
}

void wifi_event_connect_hdl (char *buf, int buf_len, int flags,
        void *userfunc) {

    mp_obj_t func = MP_OBJ_FROM_PTR(userfunc);

    if (func != mp_const_none) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {

            //TODO Should gc_lock ?
            
            mp_call_function_0(func);

            nlr_pop();
        } else {
            mp_printf(&mp_plat_print, "Uncaught exception in callback handler");
            if (nlr.ret_val != MP_OBJ_NULL) {
                mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
            }
        }
    }
    
    if(xSTAConnectAPSema != NULL) {
        xSemaphoreGive(xSTAConnectAPSema);
    }

}

void wifi_event_disconnect_hdl (char *buf, int buf_len, int flags,
        void *userfunc) {

    mp_obj_t func = MP_OBJ_FROM_PTR(userfunc);

    if (func != mp_const_none) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {

            //TODO Should gc_lock ?
            
            mp_call_function_0(func);

            nlr_pop();
        } else {
            mp_printf(&mp_plat_print, "Uncaught exception in callback handler");
            if (nlr.ret_val != MP_OBJ_NULL) {
                mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
            }
        }
    }

    if(xSTAConnectAPSema != NULL) {
        xSemaphoreGive(xSTAConnectAPSema);
    }
}

void wifi_event_no_network_hdl (char *buf, int buf_len, int flags,
        void *userfunc) {

    mp_obj_t func = MP_OBJ_FROM_PTR(userfunc);

    if (func != mp_const_none) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {

            //TODO Should gc_lock ?
            
            mp_call_function_0(func);

            nlr_pop();
        } else {
            mp_printf(&mp_plat_print, "Uncaught exception in callback handler");
            if (nlr.ret_val != MP_OBJ_NULL) {
                mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
            }
        }
    }
}

void wifi_event_fourway_handshake_done_hdl (char *buf, int buf_len, int flags,
        void *userfunc) {

    mp_obj_t func = MP_OBJ_FROM_PTR(userfunc);

    if (func != mp_const_none) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {

            //TODO Should gc_lock ?
            
            mp_call_function_0(func);

            nlr_pop();
        } else {
            mp_printf(&mp_plat_print, "Uncaught exception in callback handler");
            if (nlr.ret_val != MP_OBJ_NULL) {
                mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
            }
        }
    }
}

void wifi_event_sta_assoc_hdl (char *buf, int buf_len, int flags,
        void *userfunc) {
    mp_obj_t func = MP_OBJ_FROM_PTR(userfunc);

    //TODO Should gc_lock ?
    
    if (func != mp_const_none) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            mp_call_function_0(func);
            nlr_pop();
        } else {
            mp_printf(&mp_plat_print, "Uncaught exception in callback handler");
            if (nlr.ret_val != MP_OBJ_NULL) {
                mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
            }
        }
    }
}

void wifi_event_sta_disassoc_hdl (char *buf, int buf_len, int flags,
        void *userfunc) {
    mp_obj_t func = MP_OBJ_FROM_PTR(userfunc);

    //TODO Should gc_lock ?
    
    if (func != mp_const_none) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            mp_call_function_0(func);
            nlr_pop();
        } else {
            mp_printf(&mp_plat_print, "Uncaught exception in callback handler");
            if (nlr.ret_val != MP_OBJ_NULL) {
                mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
            }
        }
    }
}

STATIC mp_obj_t wlan_event_handler(mp_obj_t self_in, mp_obj_t event_in,
        mp_obj_t callback_in) {

    mp_uint_t event_id = mp_obj_get_int(event_in);
   
    switch(event_id) {
        case WIFI_EVENT_SCAN_RESULT_REPORT:
            if (callback_in == mp_const_none) 
                wifi_unreg_event_handler(event_id, wifi_event_scan_result_report_hdl);
            else
                wifi_reg_event_handler(event_id, wifi_event_scan_result_report_hdl,
                        MP_OBJ_TO_PTR(callback_in));
            break;
        case WIFI_EVENT_SCAN_DONE:
            if (callback_in == mp_const_none) 
                wifi_unreg_event_handler(event_id, wifi_event_scan_done_hdl);
            else
                wifi_reg_event_handler(event_id, wifi_event_scan_done_hdl,
                        MP_OBJ_TO_PTR(callback_in));
            break;
        case WIFI_EVENT_BEACON_AFTER_DHCP:
            if (callback_in == mp_const_none)
                wifi_unreg_event_handler(event_id, wifi_event_beacon_after_dhcp_hdl);
            else
                wifi_reg_event_handler(event_id, wifi_event_beacon_after_dhcp_hdl,
                        MP_OBJ_TO_PTR(callback_in));
            break;
        case WIFI_EVENT_CONNECT:
            if (callback_in == mp_const_none)
                wifi_unreg_event_handler(event_id, wifi_event_connect_hdl);
            else
                wifi_reg_event_handler(event_id, wifi_event_connect_hdl,
                        MP_OBJ_TO_PTR(callback_in));
            break;
        case WIFI_EVENT_DISCONNECT:
            if (callback_in == mp_const_none)
                wifi_unreg_event_handler(event_id, wifi_event_disconnect_hdl);
            else
                wifi_reg_event_handler(event_id, wifi_event_disconnect_hdl,
                        MP_OBJ_TO_PTR(callback_in));
            break;
        case WIFI_EVENT_NO_NETWORK:
            if (callback_in == mp_const_none)
                wifi_unreg_event_handler(event_id, wifi_event_no_network_hdl);
            else
                wifi_reg_event_handler(event_id, wifi_event_no_network_hdl,
                        MP_OBJ_TO_PTR(callback_in));
            break;
        case WIFI_EVENT_FOURWAY_HANDSHAKE_DONE:
            if (callback_in == mp_const_none)
                wifi_unreg_event_handler(event_id, wifi_event_fourway_handshake_done_hdl);
            else
                wifi_reg_event_handler(event_id, wifi_event_fourway_handshake_done_hdl,
                        MP_OBJ_TO_PTR(callback_in));
            break;
        case WIFI_EVENT_STA_ASSOC:
            if (callback_in == mp_const_none)
                wifi_unreg_event_handler(event_id, wifi_event_sta_assoc_hdl);
            else
                wifi_reg_event_handler(event_id, wifi_event_sta_assoc_hdl,
                        MP_OBJ_TO_PTR(callback_in));
            break;
        case WIFI_EVENT_STA_DISASSOC:
            if (callback_in == mp_const_none)
                wifi_unreg_event_handler(event_id, wifi_event_sta_disassoc_hdl);
            else
                wifi_reg_event_handler(event_id, wifi_event_sta_disassoc_hdl,
                        MP_OBJ_TO_PTR(callback_in));
            break;
        default:
            mp_raise_ValueError("Invalid WiFi event");
            break;
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(wlan_event_handler_obj, wlan_event_handler);

STATIC void wlan_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    wlan_obj_t *self = self_in;
    qstr wlan_qstr;
    qstr security_qstr;
    switch (self->mode) {
        case RTW_MODE_AP:
            wlan_qstr = MP_QSTR_AP;
            break;
        case RTW_MODE_STA:
            wlan_qstr = MP_QSTR_STA;
            break;
        case RTW_MODE_STA_AP:
            wlan_qstr = MP_QSTR_STA_AP;
            break;
    }

    switch(self->security_type) {
        case RTW_SECURITY_OPEN:
            security_qstr = MP_QSTR_OPEN;
            break;
        case RTW_SECURITY_WEP_PSK:
            security_qstr = MP_QSTR_WEP_PSK;
            break;
        case RTW_SECURITY_WEP_SHARED:
            security_qstr = MP_QSTR_WEP_SHARED;
            break;
        case RTW_SECURITY_WPS_OPEN:
            security_qstr = MP_QSTR_WPS_OPEN;
            break;
        case RTW_SECURITY_WPS_SECURE:
            security_qstr = MP_QSTR_WPS_SECURE;
            break;
        case RTW_SECURITY_WPA_AES_PSK:
            security_qstr = MP_QSTR_WPA_AES_PSK;
            break;
        case RTW_SECURITY_WPA2_AES_PSK:
            security_qstr = MP_QSTR_WPA2_AES_PSK;
            break;
        case RTW_SECURITY_WPA_TKIP_PSK:
            security_qstr = MP_QSTR_WPA_TKIP_PSK;
            break;
        case RTW_SECURITY_WPA2_TKIP_PSK:
            security_qstr = MP_QSTR_WPA2_TKIP_PSK;
            break;
        case RTW_SECURITY_WPA2_MIXED_PSK:
            security_qstr = MP_QSTR_WPA2_MIXED_PSK;
            break;
        case RTW_SECURITY_WPA_WPA2_MIXED:
            security_qstr = MP_QSTR_WPA_WPA2_MIXED;
            break;
    }
    mp_printf(print, "mode=%q", wlan_qstr);
    mp_printf(print, ", ssid=%s", self->ssid); 
    mp_printf(print, ", security_type=%q", security_qstr);
    mp_printf(print, ", channel=%d)", self->channel);
}

STATIC mp_obj_t wlan_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *all_args) {
    enum {ARG_mode};
    STATIC const mp_arg_t wlan_init_args[] = {
        { MP_QSTR_mode,   MP_ARG_REQUIRED | MP_ARG_INT },
    };

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(wlan_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args),
            wlan_init_args, args);

    wlan_obj_t *self = &wlan_obj;

    self->mode = args[ARG_mode].u_int;

    validate_wlan_mode(self->mode);

    memset(self->ssid, 0, sizeof(self->ssid));

    wifi_mode = self->mode;

    return (mp_obj_t)self;
}

STATIC const mp_map_elem_t wlan_locals_dict_table[] = {
    // instance methods
    { MP_OBJ_NEW_QSTR(MP_QSTR_scan),             MP_OBJ_FROM_PTR(&wlan_scan_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_start_ap),         MP_OBJ_FROM_PTR(&wlan_start_ap_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_rssi),             MP_OBJ_FROM_PTR(&wlan_rssi_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_mac),              MP_OBJ_FROM_PTR(&wlan_mac_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_rf),               MP_OBJ_FROM_PTR(&wlan_rf_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_channel),          MP_OBJ_FROM_PTR(&wlan_channel_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_connect),          MP_OBJ_FROM_PTR(&wlan_connect_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_disconnect),       MP_OBJ_FROM_PTR(&wlan_disconnect_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on),               MP_OBJ_FROM_PTR(&wlan_on_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_off),              MP_OBJ_FROM_PTR(&wlan_off_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_wifi_is_running),  MP_OBJ_FROM_PTR(&wlan_wifi_is_running_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_interface),        MP_OBJ_FROM_PTR(&wlan_get_interface_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_connect_to_ap), MP_OBJ_FROM_PTR(&wlan_is_connect_to_ap_obj) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_event_handler),    MP_OBJ_FROM_PTR(&wlan_event_handler_obj) },

    // class constants
    
    // WLAN mode 
    { MP_OBJ_NEW_QSTR(MP_QSTR_STA),              MP_OBJ_NEW_SMALL_INT(RTW_MODE_STA) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_AP),               MP_OBJ_NEW_SMALL_INT(RTW_MODE_AP) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_STA_AP),           MP_OBJ_NEW_SMALL_INT(RTW_MODE_STA_AP) },

    // SECURITY MODE
    { MP_OBJ_NEW_QSTR(MP_QSTR_OPEN),             MP_OBJ_NEW_SMALL_INT(RTW_SECURITY_OPEN) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WEP_PSK),          MP_OBJ_NEW_SMALL_INT(RTW_SECURITY_WEP_PSK) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WEP_SHARED),       MP_OBJ_NEW_SMALL_INT(RTW_SECURITY_WEP_SHARED) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WPA_TKIP_PSK),     MP_OBJ_NEW_SMALL_INT(RTW_SECURITY_WPA_TKIP_PSK) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WPA_AES_PSK),      MP_OBJ_NEW_SMALL_INT(RTW_SECURITY_WPA_AES_PSK) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WPA2_TKIP_PSK),    MP_OBJ_NEW_SMALL_INT(RTW_SECURITY_WPA2_TKIP_PSK) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WPA2_AES_PSK),     MP_OBJ_NEW_SMALL_INT(RTW_SECURITY_WPA2_AES_PSK) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WPA2_MIXED_PSK),   MP_OBJ_NEW_SMALL_INT(RTW_SECURITY_WPA2_MIXED_PSK) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WPA_WPA2_MIXED),   MP_OBJ_NEW_SMALL_INT(RTW_SECURITY_WPA_WPA2_MIXED) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_CONNECT),                 MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_CONNECT) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_DISCONNECT),              MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_DISCONNECT) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_FOURWAY_HANDSHAKE_DONE),  MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_SCAN_RESULT_REPORT),      MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_SCAN_RESULT_REPORT) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_SCAN_DONE),               MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_SCAN_DONE) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_RECONNECTION_FAIL),       MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_RECONNECTION_FAIL) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_SEND_ACTION_DONE),        MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_SEND_ACTION_DONE) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_RX_MGNT),                 MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_RX_MGNT) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_STA_ASSOC),               MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_STA_ASSOC) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_STA_DISASSOC),            MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_STA_DISASSOC) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_WPS_FINISH),              MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_WPS_FINISH) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_EAPOL_RECVD),             MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_EAPOL_RECVD) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_NO_NETWORK),              MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_NO_NETWORK) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_EVENT_BEACON_AFTER_DHCP),       MP_OBJ_NEW_SMALL_INT(WIFI_EVENT_BEACON_AFTER_DHCP) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_SCAN_TYPE_ACTIVE),              MP_OBJ_NEW_SMALL_INT(RTW_SCAN_TYPE_ACTIVE) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SCAN_TYPE_PASSIVE),             MP_OBJ_NEW_SMALL_INT(RTW_SCAN_TYPE_PASSIVE) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SCAN_TYPE_PROHIBITED_CHANNELS), MP_OBJ_NEW_SMALL_INT(RTW_SCAN_TYPE_PROHIBITED_CHANNELS) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_BSS_TYPE_INFRASTRUCTURE),       MP_OBJ_NEW_SMALL_INT(RTW_BSS_TYPE_INFRASTRUCTURE) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_BSS_TYPE_ADHOC),                MP_OBJ_NEW_SMALL_INT(RTW_BSS_TYPE_ADHOC) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_BSS_TYPE_ANY),                  MP_OBJ_NEW_SMALL_INT(RTW_BSS_TYPE_ANY) },
};
STATIC MP_DEFINE_CONST_DICT(wlan_locals_dict, wlan_locals_dict_table);

const mp_obj_type_t wlan_type = {
    { &mp_type_type },
    .name        = MP_QSTR_WLAN,
    .print       = wlan_print,
    .make_new    = wlan_make_new,
    .locals_dict = (mp_obj_t)&wlan_locals_dict,
};
