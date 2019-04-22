/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 * and Mnemote Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 "Song Yang" <onionys@gmail.com>
 * Copyright (c) 2019 "Tom Lin" <tomlin.ntust@gmail.com>
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
#include <string.h>

#include "py/nlr.h"
#include "py/objlist.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"

#include "wifi_api.h"
#include "wifi_lwip_helper.h"
#include "lwip/ip4_addr.h"
#include "lwip/inet.h"
#include "ethernetif.h"
#include "hal_gpt.h"
#define WL_NETWORKS_LIST_MAXNUM (16)
typedef struct _wlan_if_obj_t {
    mp_obj_base_t base;
    int if_id;
} wlan_if_obj_t;

enum {
    STAT_DISCONNECT,
    STAT_IDLE,
    STAT_CONNECT,
};

const mp_obj_type_t wlan_if_type;
STATIC const wlan_if_obj_t wlan_sta_obj = {{&wlan_if_type}, WIFI_MODE_STA_ONLY};
STATIC const wlan_if_obj_t wlan_ap_obj = {{&wlan_if_type}, WIFI_MODE_AP_ONLY};

// Set to "true" if esp_wifi_start() was called
static bool wifi_started = false;

static volatile int _wifi_ready = 0;
static wifi_scan_list_item_t _ap_list[WL_NETWORKS_LIST_MAXNUM];
static bool wifi_get_scan = false;

static void _set_wifi_ready() {
    _wifi_ready = 1;
}

bool wifi_ready() {
    return (_wifi_ready > 0);
}

static int32_t _wifi_event_handler(wifi_event_t event,
                                   uint8_t *payload,
                                   uint32_t length) {
    if (event == WIFI_EVENT_IOT_INIT_COMPLETE) {
        _set_wifi_ready();
    } else if (event == WIFI_EVENT_IOT_SCAN_COMPLETE) {
        wifi_get_scan = true;
    }
    return mp_const_none;
}



STATIC mp_obj_t mtk_active(size_t n_args, const mp_obj_t *args) {
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    uint8_t mode;

    if (!wifi_started) {
        mode = 0;
    } else {
        wifi_config_get_opmode(mode);
    }

    int bit = (self->if_id == WIFI_MODE_STA_ONLY) ? WIFI_MODE_STA_ONLY : WIFI_MODE_AP_ONLY;

    if (n_args > 1) {
        bool active = mp_obj_is_true(args[1]);
        mode = active ? (mode | bit) : (mode & ~bit);
        if (mode == 0) {
            if (wifi_started) {
                lwip_net_stop(self->if_id);
                wifi_started = false;
            }
        } else {
            if (!wifi_started) {
                wifi_config_get_opmode(mode);
                lwip_net_start(mode);
                wifi_started = true;
            }
        }
    }

    return (mode & bit) ? mp_const_true : mp_const_false;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mtk_active_obj, 1, 2, mtk_active);


STATIC mp_obj_t mtk_connect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_ssid, ARG_password, ARG_bssid };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_bssid, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // configure any parameters that are given
    if (n_args > 1) {
        mp_uint_t len;
        const char *para;
        if (args[ARG_ssid].u_obj != mp_const_none) {
            para = mp_obj_str_get_data(args[ARG_ssid].u_obj, &len);
            wifi_config_set_ssid(WIFI_PORT_STA, (uint8_t*)para, len);
        }
        if (args[ARG_password].u_obj != mp_const_none) {
            para = mp_obj_str_get_data(args[ARG_password].u_obj, &len);
            wifi_config_set_wpa_psk_key(WIFI_PORT_STA, (uint8_t*)para, len);
        }

        wifi_auth_mode_t auth = WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK;
        wifi_encrypt_type_t encrypt = WIFI_ENCRYPT_TYPE_TKIP_AES_MIX;
        wifi_config_set_security_mode(WIFI_PORT_STA, auth, encrypt);

        if (args[ARG_bssid].u_obj != mp_const_none) {
            para = mp_obj_str_get_data(args[ARG_bssid].u_obj, &len);
            if (len != 6) {
                mp_raise_ValueError(NULL);
            }
            wifi_config_set_bssid((uint8_t*)para);
        }

        if (wifi_config_reload_setting() < 0) {
            mp_raise_msg(&mp_type_OSError, "wifi connect reload setting error\r\n");
            goto err;
        }
    }

    MP_THREAD_GIL_EXIT();
    lwip_net_start(WIFI_MODE_STA_ONLY);
    MP_THREAD_GIL_ENTER();
    
err:
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mtk_connect_obj, 1, mtk_connect);

STATIC mp_obj_t mtk_disconnect(mp_obj_t self_in) {
    uint8_t mode;
    _wifi_ready = 0;
    if(wifi_config_get_opmode(&mode) > 0) {
        if (mode == WIFI_MODE_STA_ONLY)
            wifi_connection_disconnect_ap();
        lwip_net_stop(mode);
    }
    
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mtk_disconnect_obj, mtk_disconnect);

STATIC mp_obj_t mtk_status(size_t n_args, const mp_obj_t *args) {
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        if (self->if_id == WIFI_MODE_STA_ONLY) {
            uint8_t link_status;
            wifi_connection_get_link_status(link_status);
            if (!wifi_ready()) {
                return MP_OBJ_NEW_SMALL_INT(STAT_DISCONNECT);
            } else if (link_status == 	WIFI_STATUS_LINK_CONNECTED) {
                return MP_OBJ_NEW_SMALL_INT(STAT_CONNECT);
            } else
                return MP_OBJ_NEW_SMALL_INT(STAT_IDLE);
        }
        return mp_const_none;
    }

    switch ((uintptr_t)args[1]) {
        case (uintptr_t)MP_OBJ_NEW_QSTR(MP_QSTR_stations):
            if (self->if_id == WIFI_MODE_AP_ONLY) {
                wifi_sta_list_t sta_list[WIFI_MAX_NUMBER_OF_STA];
                uint8_t size = 0;
                if (wifi_connection_get_sta_list(&size, sta_list) < 0)
                    mp_raise_msg(&mp_type_OSError, "wifi get sta list  error\r\n");
                else {
                    mp_obj_t list = mp_obj_new_list(0, NULL);
                    for (int i = 0; i < size ; i++) {
                        mp_obj_tuple_t *t =  mp_obj_new_tuple(1,NULL);
                        t->items[0] = mp_obj_new_bytes(sta_list[i].mac_address, sizeof(sta_list[i].mac_address));
                        mp_obj_list_append(list, t);
                    }
                    return list;
                }
            }
        break;
        default:
            mp_raise_ValueError("unknown status param");
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mtk_status_obj, 1, 2, mtk_status);


STATIC mp_obj_t mtk_scan(mp_obj_t self_in) {
    // check that STA mode is active
    uint8_t mode;
    wifi_config_get_opmode(&mode);
    if (mode != WIFI_MODE_STA_ONLY) {
        mp_raise_msg(&mp_type_OSError, "STA must be active\r\n");
    }

    uint8_t attemps = 10;
    uint8_t numOfNetWorks = 0;
    int8_t status = 0;
    int i;

     wifi_get_scan = false;
    for (i = 0; i < WL_NETWORKS_LIST_MAXNUM; i++) {
       memset(&(_ap_list[i]), 0, sizeof(wifi_scan_list_item_t)); 
    }
    if (wifi_connection_scan_init(_ap_list, WL_NETWORKS_LIST_MAXNUM) < 0){
        mp_raise_msg(&mp_type_OSError, "WiFi scan list fail");
    }
    if ((status = wifi_connection_register_event_notifier(WIFI_EVENT_IOT_SCAN_COMPLETE, _wifi_event_handler)) < 0) {
        mp_raise_msg(&mp_type_OSError, "WiFi get scan list register fail ");
    }
    if ((status = wifi_connection_start_scan(NULL, 0, NULL, 0, 0)) < 0) {
        mp_raise_msg(&mp_type_OSError, "WiFi start scan fail");
    } else {
        wifi_get_scan = true;
    }

    while (wifi_get_scan && --attemps > 0) {
        hal_gpt_delay_ms(500);
    }
    wifi_connection_unregister_event_notifier(WIFI_EVENT_IOT_SCAN_COMPLETE, _wifi_event_handler);
    wifi_connection_stop_scan();
    wifi_connection_scan_deinit();
    
    mp_obj_t list = mp_obj_new_list(0, NULL);
    if (status >= 0) {
        for (i = 0; i < WL_NETWORKS_LIST_MAXNUM; i++){
            if (strlen((char*) _ap_list[i].ssid) != 0) {
                mp_obj_t _ssid = mp_obj_new_str((char*)_ap_list[i].ssid, _ap_list[i].ssid_length);
                mp_obj_t _bssid = mp_obj_new_bytes((char*)_ap_list[i].bssid, WIFI_MAC_ADDRESS_LENGTH);
                char *_auth_str =  NULL;
                switch (_ap_list[i].auth_mode) {
                    case WIFI_AUTH_MODE_OPEN:
                        _auth_str = "OPEN";
                        break;
                    case WIFI_AUTH_MODE_SHARED:
                        _auth_str = "SHARED";
                        break;
                    case WIFI_AUTH_MODE_AUTO_WEP:
                        _auth_str = "AUTO_WEP";
                        break;
                    case WIFI_AUTH_MODE_WPA:
                        _auth_str = "WPA";
                        break;
                    case WIFI_AUTH_MODE_WPA_PSK:
                        _auth_str = "WPA_PSK";
                        break;
                    case WIFI_AUTH_MODE_WPA_None:
                        _auth_str = "WPA_None";
                        break;
                    case WIFI_AUTH_MODE_WPA2:
                        _auth_str = "WPA2";
                        break;
                    case WIFI_AUTH_MODE_WPA2_PSK:
                        _auth_str = "WPA2_PSK";
                        break;
                    case WIFI_AUTH_MODE_WPA_WPA2:
                        _auth_str = "WPA_WPA2";
                        break;
                    case WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK:
                        _auth_str = "WPA-PSK/WPA2-PSK";
                        break;
                }
                mp_obj_t _auth_mode = mp_obj_new_str(_auth_str, strlen(_auth_str));
                mp_obj_t _channel = mp_obj_new_int(_ap_list[i].channel);
                mp_obj_t _rssi = mp_obj_new_int(_ap_list[i].rssi);
                mp_obj_t _item[] = {_ssid, _bssid, _auth_mode, _channel, _rssi};
                mp_obj_list_append(list, MP_OBJ_FROM_PTR(mp_obj_new_tuple(5, _item)));
            }
        }    
    }
    return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mtk_scan_obj, mtk_scan);


STATIC mp_obj_t mtk_isconnected(mp_obj_t self_in) {
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->if_id == WIFI_MODE_STA_ONLY) {
        uint8_t link_status;
        if (wifi_connection_get_link_status(&link_status) < 0)
		    mp_raise_msg(&mp_type_OSError, "WiFi disconnect AP Error!");
        return mp_obj_new_bool(link_status == WIFI_STATUS_LINK_CONNECTED);
    } else {
        wifi_sta_list_t sta_list;
        uint8_t size = 0;
        if (wifi_connection_get_sta_list(&size, &sta_list) < 0)
            mp_raise_msg(&mp_type_OSError, "wifi get sta list  error\r\n");
        return mp_obj_new_bool(size > 0);

    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mtk_isconnected_obj, mtk_isconnected);


STATIC mp_obj_t mtk_config(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    if (n_args != 1 && kwargs->used != 0) {
        mp_raise_TypeError("either pos or kw args are allowed");
    }
    wlan_if_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    // Get config

    if (n_args != 2) {
        mp_raise_TypeError("can query only one param");
    }

    int req_if = -1;
    mp_obj_t val;

    #define QS(x) (uintptr_t)MP_OBJ_NEW_QSTR(x)
    switch ((uintptr_t)args[1]) {
        case QS(MP_QSTR_mac): {
            uint8_t mac[6];
            if (self->if_id == WIFI_MODE_STA_ONLY)
                wifi_config_get_mac_address(WIFI_PORT_STA, mac);
            return mp_obj_new_bytes(mac, sizeof(mac));
          }
        case QS(MP_QSTR_essid): {
            char ssid[WIFI_MAX_LENGTH_OF_SSID];
            uint8_t len;
            int _port = 0;
            if (self->if_id == WIFI_MODE_STA_ONLY) {
                _port = WIFI_PORT_STA;
            } else {
                _port = WIFI_PORT_AP;
            }
            wifi_config_get_ssid(_port, (uint8_t *)ssid, (uint8_t *)&len); 
            return mp_obj_new_str(ssid,len);
          }
        case QS(MP_QSTR_hidden):
            if (self->if_id == WIFI_MODE_AP_ONLY) {
                uint8_t flag = 0;
                if (wifi_config_get_radio(&flag) < 0)
                    mp_raise_msg(&mp_type_OSError, "get hidden status fail");
                return mp_obj_new_bool(flag);
            } else
                mp_raise_msg(&mp_type_OSError, "AP required");
            break;
        case QS(MP_QSTR_authmode):
            if (self->if_id == WIFI_MODE_AP_ONLY) {
                wifi_auth_mode_t auth_mode;
                wifi_encrypt_type_t encrypt_mode;
		        if(wifi_config_get_security_mode(WIFI_PORT_AP, &auth_mode, &encrypt_mode) < 0)
                    mp_raise_msg(&mp_type_OSError, "get autho mode fail");
                
		        char * auth_mode_str = "";
		        if(auth_mode == WIFI_AUTH_MODE_OPEN){
		        	auth_mode_str = "OPEN";
		        }else if (auth_mode == WIFI_AUTH_MODE_WPA_PSK){
		        	auth_mode_str = "WAP";
		        }else if (auth_mode == WIFI_AUTH_MODE_WPA2_PSK){
		        	auth_mode_str = "WAP2";
		        }else if (auth_mode == WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK){
		        	auth_mode_str = "WPA/WPA2";
		        }else {
		        	auth_mode_str = "UNKNOWN";
                }

                return mp_obj_new_str(auth_mode_str, strlen(auth_mode_str));		
            } else
                mp_raise_msg(&mp_type_OSError, "AP required");
            break;
        case QS(MP_QSTR_channel):
            if (self->if_id == WIFI_MODE_AP_ONLY) {
                uint8_t _ch;
                if (wifi_config_get_channel(WIFI_PORT_AP, &_ch) < 0) {
                    mp_raise_msg(&mp_type_OSError, "Wifi get channel error");
                } else
		            return mp_obj_new_int((mp_int_t)_ch);
            } else
                mp_raise_msg(&mp_type_OSError, "AP required");
            break;
        default:
            mp_raise_ValueError("unknown config param");
    }
    #undef QS
    return val;

}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mtk_config_obj, 1, mtk_config);

STATIC mp_obj_t mtk_ifconfig(size_t n_args, const mp_obj_t *args) {
    struct netif *net_if;
	ip4_addr_t * _ip_addr;
	char * _ip_addr_str;
	uint8_t opmode = 0;
	wifi_config_get_opmode(&opmode);
    if ( n_args == 1) { 
        // get
	    /* obtain net interface object */
	    if(opmode == WIFI_MODE_AP_ONLY){
	    	net_if = netif_find_by_type(NETIF_TYPE_AP);
	    }else if (opmode == WIFI_MODE_STA_ONLY){
	    	net_if = netif_find_by_type(NETIF_TYPE_STA);
	    }
	    mp_obj_tuple_t * res_tuple = mp_obj_new_tuple(4,NULL);
	    /* get ip addr */
	    _ip_addr = netif_ip4_addr(net_if);
	    _ip_addr_str = ip4addr_ntoa(_ip_addr);
	    res_tuple->items[0] = mp_obj_new_str(_ip_addr_str, strlen(_ip_addr_str));
	    /* get netmask */
	    _ip_addr = netif_ip4_netmask(net_if);
	    _ip_addr_str = ip4addr_ntoa(_ip_addr);
	    res_tuple->items[1] = mp_obj_new_str(_ip_addr_str, strlen(_ip_addr_str));
	    /* get gateway */
	    _ip_addr = netif_ip4_gw(net_if);
	    _ip_addr_str = ip4addr_ntoa(_ip_addr);
	    res_tuple->items[2] = mp_obj_new_str(_ip_addr_str, strlen(_ip_addr_str));
        /* DNS add */
        _ip_addr = dns_getserver(0);
	    _ip_addr_str = ip4addr_ntoa(_ip_addr);
	    res_tuple->items[3] = mp_obj_new_str(_ip_addr_str, strlen(_ip_addr_str));
	    return res_tuple;
    } else {
        // set

    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mtk_ifconfig_obj, 1, 2, mtk_ifconfig);

STATIC const mp_rom_map_elem_t wlan_if_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_active), MP_ROM_PTR(&mtk_active_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&mtk_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&mtk_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&mtk_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&mtk_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&mtk_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_config), MP_ROM_PTR(&mtk_config_obj) },
    { MP_ROM_QSTR(MP_QSTR_ifconfig), MP_ROM_PTR(&mtk_ifconfig_obj) },
};

STATIC MP_DEFINE_CONST_DICT(wlan_if_locals_dict, wlan_if_locals_dict_table);

const mp_obj_type_t wlan_if_type = {
    { &mp_type_type },
    .name = MP_QSTR_WLAN,
    .locals_dict = (mp_obj_t)&wlan_if_locals_dict,
};

STATIC mp_obj_t mtk_initialize() {
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mtk_initialize_obj, mtk_initialize);

STATIC mp_obj_t get_wlan(size_t n_args, const mp_obj_t *args) {
    int opmode = (n_args > 0) ? mp_obj_get_int(args[0]) : WIFI_MODE_STA_ONLY;
    static int initialized = 0;
    if (!initialized) {
        wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE , _wifi_event_handler);
        wifi_config_t config;
        memset(&config , 0, sizeof(config));
        config.opmode = opmode;
        strcpy((char *)config.sta_config.ssid, (const char *)" ");
        config.sta_config.ssid_length = strlen((const char *)config.sta_config.ssid);

        wifi_config_ext_t ex_config;
        memset(&ex_config, 0, sizeof(ex_config));
        ex_config.sta_auto_connect_present = 1; // validate "sta_auto_connect" config
        ex_config.sta_auto_connect = 0;         // don't auto-connect - we just want to initialize

        wifi_init(&config, &ex_config);
        initialized = 1;
    }
    lwip_network_init(opmode);

    // block until wifi is ready
    while (!wifi_ready()) {
        hal_gpt_delay_ms(5000);
            break; 
    }

    if (opmode == WIFI_MODE_STA_ONLY) {
        return MP_OBJ_FROM_PTR(&wlan_sta_obj);
    } else if (opmode == WIFI_MODE_AP_ONLY) {
        return MP_OBJ_FROM_PTR(&wlan_ap_obj);
    } else {
        mp_raise_ValueError("invalid WLAN interface identifier");
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(get_wlan_obj, 0, 1, get_wlan);

STATIC mp_obj_t mtk_phy_mode(size_t n_args, const mp_obj_t * pos_args, mp_map_t *kw_args){
    enum { ARG_mode }; 
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_KW_ONLY | MP_ARG_OBJ},
    }; 
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int phy_mode = mp_obj_get_int(args[ARG_mode].u_obj);
    if (phy_mode == WIFI_PHY_11B || phy_mode == WIFI_PHY_11BG_MIXED || phy_mode == WIFI_PHY_11BGN_MIXED) {
        uint8_t port = WIFI_PORT_STA;
        uint8_t opmode;
        wifi_config_get_opmode(&opmode);
        if (opmode == WIFI_MODE_AP_ONLY)
            port = WIFI_PORT_AP;
        if (wifi_config_set_wireless_mode(port, opmode) < 0) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "When the wireless mode was set another mode is fail."));
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mtk_phy_mode_obj, 1, mtk_phy_mode);

STATIC const mp_rom_map_elem_t mp_module_network_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_network) },
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&mtk_initialize_obj) },
    { MP_ROM_QSTR(MP_QSTR_WLAN), MP_ROM_PTR(&get_wlan_obj) },
    { MP_ROM_QSTR(MP_QSTR_phy_mode), MP_ROM_PTR(&mtk_phy_mode_obj) },

    { MP_ROM_QSTR(MP_QSTR_STA_IF), MP_ROM_INT(WIFI_MODE_STA_ONLY) },
    { MP_ROM_QSTR(MP_QSTR_AP_IF), MP_ROM_INT(WIFI_MODE_AP_ONLY) },

    { MP_ROM_QSTR(MP_QSTR_MODE_11B), MP_ROM_INT(WIFI_PHY_11B) },
    { MP_ROM_QSTR(MP_QSTR_MODE_11G), MP_ROM_INT(WIFI_PHY_11BG_MIXED) },
    { MP_ROM_QSTR(MP_QSTR_MODE_11N), MP_ROM_INT(WIFI_PHY_11BGN_MIXED) },

    { MP_ROM_QSTR(MP_QSTR_AUTH_OPEN), MP_ROM_INT(WIFI_AUTH_MODE_OPEN) },
    { MP_ROM_QSTR(MP_QSTR_AUTH_WPA_PSK), MP_ROM_INT(WIFI_AUTH_MODE_WPA_PSK) },
    { MP_ROM_QSTR(MP_QSTR_AUTH_WPA2_PSK), MP_ROM_INT(WIFI_AUTH_MODE_WPA2_PSK) },
    { MP_ROM_QSTR(MP_QSTR_AUTH_WPA_WPA2_PSK), MP_ROM_INT(WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_network_globals, mp_module_network_globals_table);

const mp_obj_module_t mp_module_network = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_network_globals,
};
