import gc, uos
import network, utime
import usocket as socket

wlan = None

def ap_init():
    global wlan
    import ap
    if ap.mode == 'ap':
        ssid = ap.ssid
        pw = ap.pw
        print('start ap mode...[ip:10.10.10.1]')
        print('ssid: ' + ssid)
        print('pw: ' + pw)
        wlan = network.WLAN(network.AP_IF)
        wlan.ssid(ssid)
        wlan.auth((network.WPA2,pw))
        wlan.connect()
        return wlan
    sta_init()

def sta_init():
    global wlan
    import ap
    ssid = ap.ssid
    pw = ap.pw
    print('start sta mode...')
    print('connect to ap:',ssid)
    wlan = network.WLAN()
    utime.sleep(3)
    wlan.connect(ssid,pw)
    print('connecting.',end='')
    for i in range(10):
        if wlan.isconnected():
            utime.sleep(3)
            print('connected!')
            return wlan
        print('.',end='')
        utime.sleep(1)
    return wlan

def start():
    try:
        f = open('ap.py')
        f.close()
    except:
        print('new ap.py')
        mode('ap','TWOLAB_UPY_AP','12345678')

    import ap
    if ap:
        mode = ap.mode
    else:
        raise(Exception('[ERROR] ap.mode not found. "ap" or "sta"'))
    
    if mode == "ap":
        ap_init()
    elif mode == "sta":
        sta_init()


def mode(mode,ssid,pw):
    try:
        uos.remove('ap.py')
    except:
        pass
    with open('ap.py','w') as f:
        f.write('\nmode="' + mode + '"\n')
        f.write('ssid="' +  ssid + '"\n')
        f.write('pw="' + pw + '"\n')

def ap(ssid,pw):
    mode('ap',ssid,pw)
        
def sta(ssid,pw):
    mode('sta',ssid,pw)

