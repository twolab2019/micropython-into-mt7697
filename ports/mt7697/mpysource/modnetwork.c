/*
 * This file is part of the MicroPython porting to linkit 7697 project
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 "Song Yang" <onionys@gmail.com>
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

#include "FreeRTOS.h"
#include "task.h"

#include "hal.h"
#include "wifi_api.h"
#include "lwip/ip4_addr.h"
#include "lwip/inet.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/sockets.h"
#include "ethernetif.h"
#include "wifi_lwip_helper.h"
#include "netif/etharp.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "modnetwork.h"
// #include "mydebug.h"

/* WLAN obj definition */

typedef struct _machine_wlan_obj_t {
	mp_obj_base_t base;
	uint8_t opmode;
}machine_wlan_obj_t;

const mp_obj_type_t machine_wlan_type;
STATIC machine_wlan_obj_t machine_wlan_obj = {{&machine_wlan_type}, WIFI_MODE_STA_ONLY};

/* utility function definition */
static int _get_port_by_current_opmode(void);

/* mtk wifi event handler */
int32_t _wifi_event_handler(wifi_event_t evt, uint8_t *payload, uint32_t length){
    struct netif *sta_if;
    // printf("-->[wifi event handler]: %d\r\n", evt);
    switch(evt){
        case WIFI_EVENT_IOT_INIT_COMPLETE:
            // printf("\t[WIFI_EVENT_IOT_INIT_COMPLETE]\r\n");
            break;
        case WIFI_EVENT_IOT_PORT_SECURE:
            // printf("\t[WIFI_EVENT_IOT_PORT_SECURE]\r\n");
            break;
        case WIFI_EVENT_IOT_DISCONNECTED:
            // printf("\t[WIFI_EVENT_IOT_DISCONNECTED]\r\n");
            break;
        case WIFI_EVENT_IOT_CONNECTED:
            // printf("\t[WIFI_EVENT_IOT_CONNECTED]\r\n");
            break;
		case WIFI_EVENT_IOT_REPORT_BEACON_PROBE_RESPONSE:
            // printf("\t[WIFI_EVENT_IOT_REPORT_BEACON_PROBE_RESPONSE]\r\n");
			break;
    }
    // printf("<--[wifi event handler]: %d\r\n", evt);
    return 1;
}

/* WLAN obj method init
 * wlan.init()
 * */
STATIC mp_obj_t machine_wlan_init(mp_obj_t self_in){
	if(wifi_config_reload_setting() < 0){
		// printf("[wifi config reload failed!]\r\n");
	}else{
		// printf("[wifi config reload]\r\n");
	}
	// printf("[lwip stop]\r\n");
	lwip_net_stop(machine_wlan_obj.opmode);
	// printf("[lwip start]\r\n");
	lwip_net_start(machine_wlan_obj.opmode);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wlan_init_obj, machine_wlan_init);

/* WLAN obj method ssid 
 * wlan.ssid()
 * wlan.ssid("my_ap_ssid")
 * */
STATIC mp_obj_t machine_wlan_ssid(size_t n_args, const mp_obj_t * pos_args, mp_map_t *kw_args){
	enum {ARG_ssid};
	static const mp_arg_t allowed_args[] = {
		{MP_QSTR_ssid,     MP_ARG_OBJ, {.u_obj = mp_const_none} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	int _port = _get_port_by_current_opmode();
	if(_port < 0){
		// printf("[get port error:%d]\r\n", _port);
		return mp_const_none;
	}

	if(args[ARG_ssid].u_obj == mp_const_none){
		// printf("[get ssid]\r\n");
		char buff[32] = {0};
		uint8_t len = 0;
		wifi_config_get_ssid(_port,(uint8_t*)buff,&len);
		return mp_obj_new_str(buff,len);
	}
	// SET SSID
	const char * ssid_str = NULL;
	mp_uint_t ssid_str_len = 0;
	ssid_str = mp_obj_str_get_data(args[ARG_ssid].u_obj, &ssid_str_len);
	if(wifi_config_set_ssid(_port,ssid_str, ssid_str_len)<0){
		// printf("[set ssid error]\r\n");
	}
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_wlan_ssid_obj, 1, machine_wlan_ssid);

/*
 * WLAN obj method channel
 * */
static mp_obj_t machine_wlan_channel(size_t n_args, const mp_obj_t * pos_args, mp_map_t *kw_args){
	enum {ARG_channel};
	static const mp_arg_t allowed_args[] = {
		{MP_QSTR_channel,     MP_ARG_OBJ, {.u_obj = mp_const_none} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	int _port = _get_port_by_current_opmode();
	if(_port < 0){
		mp_raise_msg(&mp_type_OSError, "WiFi get port Error");
		return mp_const_none;
	}

	if(args[ARG_channel].u_obj == mp_const_none){
		uint8_t _ch;
		if(wifi_config_get_channel(_port,&_ch)<0){
			mp_raise_msg(&mp_type_OSError, "WiFi Get Channel Error");
		}
		return mp_obj_new_int((mp_int_t)_ch);
	}

	mp_int_t _ch = mp_obj_get_int(args[ARG_channel].u_obj);
	if(wifi_config_set_channel(_port,(uint8_t)_ch) < 0){
		mp_raise_msg(&mp_type_OSError, "invaled WiFi Channel");
	}
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_wlan_channel_obj, 1,machine_wlan_channel);

/* WLAN obj method auth 
 * wlan.auth()
 * wlan.auth((WLAN.WPA, "my_ap_password"))
 * */

STATIC mp_obj_t machine_wlan_auth(size_t n_args, const mp_obj_t *args){
	// mp_obj_t self = args[0];
	
	int port = _get_port_by_current_opmode();
	if(n_args == 1){
	    /* GET */
	    // get security mode 
		wifi_auth_mode_t auth_mode;
		wifi_encrypt_type_t encrypt_mode;
		if(port < 0){
			return mp_const_none;
		}
		if(wifi_config_get_security_mode(port, &auth_mode, &encrypt_mode) < 0){
			// printf("[wifi get security mode error]\r\n");
			return mp_const_none;
		}
		// printf("[encrypt mode:%u]\r\n", encrypt_mode);
		/* GET Password */ 
		char * auth_mode_str = "";
		uint8_t _buff[64] = {0};
		uint8_t * password_ptr = _buff;
		uint8_t password_len = 0;
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

		if(auth_mode == WIFI_AUTH_MODE_OPEN){
			// printf("[OPEN MODE]\r\n");
		}else if (encrypt_mode == WIFI_ENCRYPT_TYPE_WEP_ENABLED || encrypt_mode == WIFI_ENCRYPT_TYPE_ENCRYPT1_ENABLED){
			// printf("[get WEP KEY]\r\n");
			/* GET WEP KEY */ 
			wifi_wep_key_t wep_key;
			if(wifi_config_get_wep_key(port, &wep_key)){
				// printf("[get wep key error]\r\n");
				return mp_const_none;
			}
			password_ptr = wep_key.wep_key[wep_key.wep_tx_key_index];
			password_len = wep_key.wep_key_length[wep_key.wep_tx_key_index];

		}else if (encrypt_mode == WIFI_ENCRYPT_TYPE_TKIP_ENABLED || 
				encrypt_mode == WIFI_ENCRYPT_TYPE_ENCRYPT2_ENABLED || 
				encrypt_mode == WIFI_ENCRYPT_TYPE_AES_ENABLED || 
				encrypt_mode == WIFI_ENCRYPT_TYPE_ENCRYPT3_ENABLED ||
				encrypt_mode == WIFI_ENCRYPT_TYPE_TKIP_AES_MIX ||
				encrypt_mode == WIFI_ENCRYPT_TYPE_ENCRYPT4_ENABLED ){
			// GET WPA KEY
			// printf("[get WPA KEY]\r\n");
			int res = wifi_config_get_wpa_psk_key(port, password_ptr, &password_len);
			if( res < 0){
				// printf("[get wpa key error][%d]\r\n", res);
				return mp_const_none;
			}
		}else {
			// printf("[encrypt unsupport][%u]\r\n", encrypt_mode);
			return mp_const_none;
		}

		mp_obj_t auth_tuple[] = {
			mp_obj_new_str(auth_mode_str,strlen(auth_mode_str)),
			mp_obj_new_bytes(password_ptr, password_len)
		};
		return mp_obj_new_tuple(MP_ARRAY_SIZE(auth_tuple), auth_tuple);
	}else if (n_args == 2){

		/* TODO SET auth */

		// printf("[set auth]\r\n");
		size_t len;
		mp_obj_t * elem;
		mp_obj_get_array(args[1],&len,&elem);
		if(len == 2){
			mp_int_t _auth_mode = mp_obj_get_int(elem[0]);
			uint8_t len;
			const char * _pw = mp_obj_str_get_data(elem[1], &len);
			if(_auth_mode == WIFI_AUTH_MODE_OPEN){
				wifi_config_set_security_mode(port, _auth_mode, WIFI_ENCRYPT_TYPE_WEP_DISABLED);
			}else if (_auth_mode == WIFI_AUTH_MODE_WPA_PSK){
				wifi_config_set_wpa_psk_key(port, _pw, len);
				wifi_config_set_security_mode(port, _auth_mode, WIFI_ENCRYPT_TYPE_AES_ENABLED);
			}else if (_auth_mode == WIFI_AUTH_MODE_WPA2_PSK){
				wifi_config_set_wpa_psk_key(port, _pw, len);
				wifi_config_set_security_mode(port, _auth_mode, WIFI_ENCRYPT_TYPE_AES_ENABLED);
			}else if (_auth_mode == WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK){
				wifi_config_set_wpa_psk_key(port, _pw, len);
				wifi_config_set_security_mode(port, _auth_mode, WIFI_ENCRYPT_TYPE_AES_ENABLED);
			}else{
				mp_raise_ValueError("invalid auth mode!");
			}
		}else{
			mp_raise_ValueError("invalid args!");
		}
	}else {
		mp_raise_ValueError("invalid args num!");
	}
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_wlan_auth_obj, 1, 2, machine_wlan_auth);



/* WLAN obj method connect 
 * wlan.connect(ssid = 'ssid',password = 'xxxxxx')
 * */

STATIC mp_obj_t machine_wlan_connect(size_t n_args, const mp_obj_t * pos_args, mp_map_t *kw_args){
	enum {ARG_ssid, ARG_password};
	static const mp_arg_t allowed_args[] = {
		{MP_QSTR_ssid,     MP_ARG_OBJ, {.u_obj = mp_const_none} },
		{MP_QSTR_password, MP_ARG_OBJ, {.u_obj = mp_const_none} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	// printf("[wlan connect]\r\n");
	if(n_args > 1){

		const char *ssid_str=NULL;
		mp_uint_t ssid_str_len=0;

		char *password_str=NULL;
		mp_uint_t password_str_len=0;

		uint8_t _port = 0;

		if(args[ARG_ssid].u_obj != mp_const_none){
			ssid_str = mp_obj_str_get_data(args[ARG_ssid].u_obj, &ssid_str_len);
			// printf("[ssid][%s]\r\n",ssid_str);
		}else{
			mp_raise_msg(&mp_type_OSError, "invaled WiFi SSID");
		}

		if(args[ARG_password].u_obj != mp_const_none){
			password_str = mp_obj_str_get_data(args[ARG_password].u_obj, &password_str_len);
			// printf("[password][%s]\r\n",password_str);
		}else{
			mp_raise_ValueError("invalid value password");
		}

		if(machine_wlan_obj.opmode == WIFI_MODE_STA_ONLY) { 
			_port = WIFI_PORT_STA;
		}else if(machine_wlan_obj.opmode == WIFI_MODE_AP_ONLY) { 
			_port = WIFI_PORT_AP;
		}else{
			// -- TODO -- REPEATER ? 
		}

		if(wifi_config_set_ssid(_port, (uint8_t*)ssid_str, ssid_str_len) < 0){
			// printf("[ssid][%s:%u]\r\n",ssid_str,ssid_str_len);
			mp_raise_msg(&mp_type_OSError, "invalid WiFi SSID");
		}
		if(wifi_config_set_wpa_psk_key(_port, password_str, password_str_len) < 0){
			// printf("[wpa psk key][%s:%u]\r\n", password_str, password_str_len);
			mp_raise_msg(&mp_type_OSError, "invalid WiFi Key");
		}
		if(wifi_connection_disconnect_ap()<0){
			mp_raise_msg(&mp_type_OSError, "WiFi disconnect AP Error!");
			return mp_const_none;
		}
	}

	if(wifi_config_reload_setting() < 0){
		// printf("[wifi config reload failed!]\r\n");
		mp_raise_msg(&mp_type_OSError, "WiFi reload config Error!");
	}else{
		// printf("[wifi config reload]\r\n");
	}

	// printf("[lwip stop]\r\n");
	lwip_net_stop(machine_wlan_obj.opmode);
	// printf("[lwip start]\r\n");
	lwip_net_start(machine_wlan_obj.opmode);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_wlan_connect_obj, 1, machine_wlan_connect);

/* WLAN obj method disconnect */

STATIC mp_obj_t machine_wlan_disconnect(mp_obj_t self_in){
	if(wifi_connection_disconnect_ap() < 0){
		mp_raise_msg(&mp_type_OSError, "WiFi disconnect AP Error!");
	}
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wlan_disconnect_obj, machine_wlan_disconnect);

/* WLAN obj method scan 
 * return tuple consists of ssid, bssid, sec, channel, rssi
 * */

STATIC mp_obj_t machine_wlan_scan(mp_obj_t self_in){
	uint8_t size = 10;
	wifi_scan_list_item_t scan_ap_list[size];
	memset(scan_ap_list, 0, sizeof(wifi_scan_list_item_t) * size);
	// printf("[init]\r\n");
	wifi_connection_scan_init(scan_ap_list,size);
	// printf("[start]\r\n");
	wifi_connection_start_scan(NULL, 0, NULL, 0, 0);
	vTaskDelay(1000/portTICK_PERIOD_MS);
	// printf("[stop]\r\n");
	wifi_connection_stop_scan();
	// printf("[deinit]\r\n");
	wifi_connection_scan_deinit();
	int i;
	mp_obj_tuple_t * tuple_obj = mp_obj_new_tuple(size, NULL);
	for(i=0;i<size;i++){
		mp_obj_t _ssid = mp_obj_new_str((char*)scan_ap_list[i].ssid, scan_ap_list[i].ssid_length);
		mp_obj_t _bssid = mp_obj_new_bytes((char*)scan_ap_list[i].bssid, WIFI_MAC_ADDRESS_LENGTH);
		char * _sec_str = NULL;
		switch(scan_ap_list[i].auth_mode){
			case WIFI_AUTH_MODE_OPEN:
				_sec_str = "OPEN";
				break;
			case WIFI_AUTH_MODE_SHARED:
				_sec_str = "SHARED";
				break;
			case WIFI_AUTH_MODE_AUTO_WEP:
				_sec_str = "WEP";
				break;
			case WIFI_AUTH_MODE_WPA:
				_sec_str = "WPA";
				break;
			case WIFI_AUTH_MODE_WPA_PSK:
				_sec_str = "WPA_PSK";
				break;
			case WIFI_AUTH_MODE_WPA_None:
				_sec_str = "WPA_None";
				break;
			case WIFI_AUTH_MODE_WPA2:
				_sec_str = "WPA2";
				break;
			case WIFI_AUTH_MODE_WPA2_PSK:
				_sec_str = "WPA2_PSK";
				break;
			case WIFI_AUTH_MODE_WPA_WPA2:
				_sec_str = "WPA/WPA2";
				break;
			case WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK:
				_sec_str = "WPA2_PSK";
				break;
			default:
				_sec_str = "UNKNOWN";
				break;
		}
		mp_obj_t _sec = mp_obj_new_str(_sec_str,strlen(_sec_str));
		mp_obj_t _channel = mp_obj_new_int(scan_ap_list[i].channel);
		mp_obj_t _rssi = mp_obj_new_int(scan_ap_list[i].rssi);
		mp_obj_t _item[] = {
			_ssid,
			_bssid,
			_sec,
			_channel,
			_rssi
		};
		tuple_obj->items[i] = mp_obj_new_tuple(5,_item);
	}
	return tuple_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wlan_scan_obj, machine_wlan_scan);

/* WLAN obj method isconnected */

STATIC mp_obj_t machine_wlan_isconnected(mp_obj_t self_in){
	return (check_is_ip_ready())?(mp_const_true):(mp_const_none);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wlan_isconnected_obj, machine_wlan_isconnected);

/* WLAN obj method ifconfig 
 * if at mode AP return ('<ip>','<mask>','<gateway>','<dns>')
 * */
STATIC mp_obj_t machine_wlan_ifconfig(size_t n_args, const mp_obj_t * args){
	struct netif * net_if;
	ip4_addr_t * _ip_addr;
	char * _ip_addr_str;
	uint8_t opmode = 0;

	wifi_config_get_opmode(&opmode);

	/* obtain net interface object */
	if(opmode == WIFI_MODE_AP_ONLY){
		net_if = netif_find_by_type(NETIF_TYPE_AP);
	}else if (opmode == WIFI_MODE_STA_ONLY){
		net_if = netif_find_by_type(NETIF_TYPE_STA);
	}
	mp_obj_tuple_t * res_tuple = mp_obj_new_tuple(3,NULL);
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

	return res_tuple;
	// return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_wlan_ifconfig_obj, 1, 2, machine_wlan_ifconfig);

/* 
 * class obj WLAN interface 
 * */

STATIC const mp_rom_map_elem_t wlan_if_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_init),        MP_ROM_PTR(&machine_wlan_init_obj)},
	{ MP_ROM_QSTR(MP_QSTR_ssid),        MP_ROM_PTR(&machine_wlan_ssid_obj)},
	{ MP_ROM_QSTR(MP_QSTR_auth),        MP_ROM_PTR(&machine_wlan_auth_obj)},
	{ MP_ROM_QSTR(MP_QSTR_channel),     MP_ROM_PTR(&machine_wlan_channel_obj)},
	{ MP_ROM_QSTR(MP_QSTR_connect),     MP_ROM_PTR(&machine_wlan_connect_obj)},
	{ MP_ROM_QSTR(MP_QSTR_disconnect),  MP_ROM_PTR(&machine_wlan_disconnect_obj)},
	{ MP_ROM_QSTR(MP_QSTR_scan),        MP_ROM_PTR(&machine_wlan_scan_obj)},
	{ MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&machine_wlan_isconnected_obj)},
	{ MP_ROM_QSTR(MP_QSTR_ifconfig),    MP_ROM_PTR(&machine_wlan_ifconfig_obj)},
};

STATIC MP_DEFINE_CONST_DICT(wlan_if_locals_dict, wlan_if_locals_dict_table);

const mp_obj_type_t machine_wlan_type = {
	{ &mp_type_type },
	.name = MP_QSTR_WLAN,
	.locals_dict = (mp_obj_t)&wlan_if_locals_dict,
};


/* module network method get wlan 
 *     args[0] : WIFI_MODE_STA_ONLY or WIFI_MODE_AP_ONLY or ...
 * */

STATIC mp_obj_t machine_get_wlan(size_t n_args, const mp_obj_t *args) {
	if(n_args > 0){
		mp_int_t opmode = mp_obj_get_int(args[0]);
		wifi_set_opmode(opmode);
		// wifi_config_set_opmode(opmode);
		machine_wlan_obj.opmode = opmode;
	}
	return &machine_wlan_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_get_wlan_obj, 0, 1, machine_get_wlan);

/* module network method network init */

STATIC mp_obj_t machine_network_init() {
	// printf("[network init]\r\n");
	wifi_config_t wifi_config = {0};
    wifi_config.opmode = WIFI_MODE_STA_ONLY;
    wifi_init(&wifi_config,NULL);
    lwip_network_init(wifi_config.opmode);
	// printf("[network init--done]\r\n");

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_network_init_obj, machine_network_init);

/* module network method network mode */
STATIC mp_obj_t machine_mode(size_t n_args, const mp_obj_t * pos_args, mp_map_t *kw_args){
	enum {ARG_mode};
	static const mp_arg_t allowed_args[] = {
		{MP_QSTR_mode,     MP_ARG_OBJ, {.u_obj = mp_const_none} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	if(args[ARG_mode].u_obj == mp_const_none){
		char * mode_str = "";
		int _port = _get_port_by_current_opmode();
		if(_port == WIFI_PORT_STA){
			mode_str = "STA";
		}else if (_port == WIFI_PORT_AP){
			mode_str = "AP";
		}else {
			mode_str = "ERROR";
		}
		return mp_obj_new_str(mode_str,strlen(mode_str));
	}
	mp_int_t _mode = mp_obj_get_int(args[ARG_mode].u_obj);
	if(_mode == WIFI_MODE_STA_ONLY){
		wifi_config_set_opmode(WIFI_MODE_STA_ONLY);
	}else if (_mode == WIFI_MODE_AP_ONLY){
		wifi_config_set_opmode(WIFI_MODE_AP_ONLY);
	}else{
		// printf("[error set mode]\r\n");
	}
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_mode_obj,1,machine_mode);

/* 
 * module network 
 * */

STATIC const mp_rom_map_elem_t machine_module_network_globals_table[]={
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_network) },
	{ MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&machine_network_init_obj) },
	{ MP_ROM_QSTR(MP_QSTR_WLAN),     MP_ROM_PTR(&machine_get_wlan_obj) },
	{ MP_ROM_QSTR(MP_QSTR_mode),     MP_ROM_PTR(&machine_mode_obj) },
	// { MP_ROM_QSTR(MP_QSTR_LAN),      MP_ROM_PTR(&machine_get_lan_obj) },

    /* constant */
	{ MP_ROM_QSTR(MP_QSTR_STA_IF),   MP_ROM_INT(WIFI_MODE_STA_ONLY) },
	{ MP_ROM_QSTR(MP_QSTR_AP_IF),    MP_ROM_INT(WIFI_MODE_AP_ONLY) },
	/* security mode constant */
	{ MP_ROM_QSTR(MP_QSTR_OPEN),     MP_ROM_INT(WIFI_AUTH_MODE_OPEN) },
	{ MP_ROM_QSTR(MP_QSTR_WPA),      MP_ROM_INT(WIFI_AUTH_MODE_WPA_PSK) },
	{ MP_ROM_QSTR(MP_QSTR_WPA2),     MP_ROM_INT(WIFI_AUTH_MODE_WPA2_PSK) },
	{ MP_ROM_QSTR(MP_QSTR_WPA_WPA2), MP_ROM_INT(WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK) },
};
STATIC MP_DEFINE_CONST_DICT(machine_module_network_globals, machine_module_network_globals_table);

const mp_obj_module_t machine_module_network = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&machine_module_network_globals,
};


void mod_network_wifi_init(wifi_config_t * wifi_config){
	wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE, _wifi_event_handler);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_CONNECTED, _wifi_event_handler);
	wifi_connection_register_event_handler(WIFI_EVENT_IOT_REPORT_BEACON_PROBE_RESPONSE, _wifi_event_handler);
    // wifi_connection_register_event_handler(WIFI_EVENT_IOT_PORT_SECURE, _wifi_event_handler);
    // wifi_connection_register_event_handler(WIFI_EVENT_IOT_DISCONNECTED, _wifi_event_handler);
	
    // wifi_config_t config = {0};
    // config.opmode = WIFI_MODE_STA_ONLY;
	// get_default_wifi_config(&config);
    // wifi_init(&config, NULL);
	if(wifi_config->opmode == 0){
		wifi_config->opmode = WIFI_MODE_STA_ONLY;
	}
    wifi_init(wifi_config, NULL);
    // printf("[wifi init]\r\n");
    // lwip_tcpip_config_t tcpip_config = {{0},{0},{0},{0},{0},{0}};
    // lwip_tcpip_init(&tcpip_config, WIFI_MODE_STA_ONLY);
	lwip_network_init(wifi_config->opmode);
	lwip_net_start(wifi_config->opmode);
    // printf("[lwip init]\r\n");
}

/* utility function */
static int _get_port_by_current_opmode(void){
	uint8_t opmode = 0;
	int port = -1;
	int res = wifi_config_get_opmode(&opmode);
	if(res < 0) {
		// printf("[wifi get opmode error][%d]\r\n", res);
	}else if(opmode == WIFI_MODE_STA_ONLY){
		port = WIFI_PORT_STA;
	}else if (opmode == WIFI_MODE_AP_ONLY){
		port = WIFI_PORT_AP;
	}else{
		// printf("[wifi uncompleted mode][%u]\r\n", opmode);
	}
	return port;
}

