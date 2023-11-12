import time
from umqtt import MQTTClient
import network
from machine import UART, Pin
import json
import binascii
from machine import WDT


indicator = Pin("LED", Pin.OUT)
led_red=Pin(6, Pin.OUT)
led_orange=Pin(5, Pin.OUT)

wlan = network.WLAN(network.STA_IF)
wlan.active(True)

SSID ='FRITZ!Box 7530 LQ'
PSK = '04677226468126224463'
TOPIC_SMARTMETER = b'tele/smartmeter/state'
TOPIC_PHOTOVOLTAIK = b'tele/photovoltaik/state'
TOPIC_CMND = b'cmnd/evse/'
TOPIC_CMND_WILDCARD = b'cmnd/evse/+'


#TOPIC = b'test'

start_ladeleistung = 1200
min_ueberschuss = -600
max_bezug = 800
min_solar_power = 600
force_solar_power = 1500



wlan.connect(SSID, PSK)
i = 1
while not wlan.isconnected():
    time.sleep(0.5)
    indicator.toggle()
    print(".", end='')
    if i%5 == 0:
        print(" retry")
        wlan.active(False)
        time.sleep(0.5)
        wlan.active(True)
        time.sleep(0.2)
        wlan.connect(SSID, PSK)
    if i == 50:
        print(" reset")
        machine.reset()
    i += 1

print("Wlan ready")
print(wlan.ifconfig())
indicator.on()
#wdt = WDT(timeout=5000)
wdt=None

wallbox = UART(0, 9600, rx=Pin(17), tx=Pin(16))

counter = 0
ladeleistung = 1000
evse_status='unknown'
evse_rawhex=None
solar_power=0
last_solar_power_update=time.ticks_ms()
evse_state_char='S'
evse_state_int=0
evse_mode='M'
evse_ladeleistung=0
sm_power=0


def send_wallbox(cmd: bytes, value: int = 0):
    # send magic number + value in 2 bytes + checksum
    msg = cmd + value.to_bytes(2, 'big')
    cs = (sum(msg) & 0xff).to_bytes(1, 'big')
    wallbox.write(msg + cs)

def request_status():
    global evse_state_char
    global evse_state_int
    global evse_mode
    global evse_ladeleistung
    global evse_rawhex

    send_wallbox(b'S')
    
    time.sleep_ms(100)
    if wallbox.any(): 
        data = wallbox.read(wallbox.any())
    else: 
        return 'Error'
    if len(data) == 6:
        cs = sum(data[:5]) & 0xff


        if cs == data[5]:
            evse_state_char=chr(data[1])
            if evse_state_char=='A': evse_state_int=0
            elif evse_state_char=='B': evse_state_int=1
            elif evse_state_char=='C': evse_state_int=2
            elif evse_state_char=='F': evse_state_int=-1
            else: evse_state_int=-2

            evse_mode=chr(data[2])
            evse_ladeleistung= ( data[3] << 8 ) + data[4]
            evse_rawhex=binascii.hexlify(data).decode()

            if chr(data[2]) == 'M':
                print("Manueller Modus")
                
            return chr(data[1])
        else:
            print("Error checksum")
            print(data)
    else:
        print("wrong data len")

#    evse_state_char='A'
#    evse_state_int=-3
#    evse_mode='M'
#    evse_ladeleistung=0
        
    return "Error"

s0_counter=0
def s0_trigger(pin):
    global s0_counter
    global led_red
    print("s0")
    s0_counter+=1
    led_red.toggle()
    
    
def send_mqtt_status():
    global c
    global s0_counter
    global ladeleistung
    global evse_status
    

# Received messages from subscriptions will be delivered to this callback
def sub_mqtt_cb(topic, msg):
    global led_orange
    led_orange.on()
    if topic==TOPIC_PHOTOVOLTAIK:
        sub_mqtt_solar_cb(topic, msg)
    elif topic==TOPIC_SMARTMETER:
        sub_mqtt_smartmeter_cb(topic, msg)
    else:
        sub_mqtt_command(topic, msg)


def sub_mqtt_solar_cb(topic, msg):
    global solar_power
    global last_solar_power_update
    try:
        data = json.loads(msg.decode())
        power = int(data["power"])
        solar_power=power
        last_solar_power_update=time.ticks_ms()
    except Exception as ex:
        print(ex)
        print(msg)
        print("unable to decode mqtt data")
        
        

def sub_mqtt_smartmeter_cb(topic, msg):
    global ladeleistung
    global counter
    global evse_status
    global evse_rawhex
    global solar_power
    global last_solar_power_update
    global sm_power
    global start_ladeleistung
    global min_ueberschuss
    global max_bezug
    global min_solar_power
    global force_solar_power
  #  print((topic, msg))
    try:
        data = json.loads(msg.decode())
        sm_power = int(data["power"])
    except:
        print("unable to decode mqtt data")
        return

    evse = request_status()

    if evse == 'A':
        print('disconnected')
        ladeleistung = 1000
        evse_status="disconnected"
    elif evse == 'B' or evse == 'C':
        print("connected")
        evse_status="connected"
        if sm_power < min_ueberschuss and ladeleistung == 0:
            ladeleistung = start_ladeleistung
            counter = 0
        elif sm_power < -100 and ladeleistung < 3400:
            if ladeleistung<solar_power:
                ladeleistung += 100
                counter = 0
            else:
                pass # nicht genuegend Leistung
            
        if sm_power > 0:
            if ladeleistung > start_ladeleistung:
                ladeleistung -= sm_power
            else:
                pass # ladeleistung schon auf minimum reduziert

        if sm_power > max_bezug:
            ladeleistung = start_ladeleistung
            counter += 1
        elif solar_power < min_solar_power:
            ladeleistung = start_ladeleistung
            counter += 1

        curtime=time.ticks_ms()
        if time.ticks_diff(curtime, last_solar_power_update) < 120000:   # daten < 2 Minuten alt
            if solar_power > force_solar_power:
                counter=0
                if ladeleistung < start_ladeleistung:
                    ladeleistung=start_ladeleistung
        else:
            print("no solar power data")

        if counter > 6:
            print("Ladung stoppen")
            ladeleistung=0
    
        # if ladeleistung < 1100 and counter > 6:
            # ladeleistung=0
    elif evse == 'F':
        print("Wallbox error")
        evse_status="error"
        ladeleistung = 0
    
    else:
        print("Error")
        counter += 1
    if counter > 6:
        ladeleistung = 0
    send_wallbox(b'L', ladeleistung)


def sub_mqtt_command(topic, msg):
    global start_ladeleistung
    global min_ueberschuss
    global max_bezug
    global min_solar_power
    global force_solar_power

    
    if topic==TOPIC_CMND+b"START_LADELEISTUNG":
        start_ladeleistung=int(msg.decode())
    elif topic==TOPIC_CMND+b"MIN_UEBERSCHUSS":
        min_ueberschuss=int(msg.decode())
    elif topic==TOPIC_CMND+b"MAX_BEZUG":
        max_bezug=int(msg.decode())
    elif topic==TOPIC_CMND+b"MIN_SOLAR_POWER":
        min_solar_power=int(msg.decode())
    elif topic==TOPIC_CMND+b"FORCE_SOLAR_POWER":
        force_solar_power=int(msg.decode())
    elif topic==TOPIC_CMND+b"Ladeleistung":
        send_wallbox(b'L', int(msg.decode()))
    elif topic==TOPIC_CMND+b"Power": 
        if msg.decode()=="OFF":
            send_wallbox(b'F', 0)
        elif msg.decode()=="ON":
            send_wallbox(b'L', start_ladeleistung)
        else:
            print("wrong msg: " + msg.decode())
        
    else:
        print("unknown topic")
        print(topic)



def send_mqtt_status():
    global c
    global s0_counter
    global ladeleistung
    global evse_status
    global evse_rawhex
    global evse_state_char
    global evse_state_int
    global evse_mode
    global evse_ladeleistung
    global solar_power
    global last_solar_power_update
    global start_ladeleistung
    global min_ueberschuss
    global max_bezug
    global min_solar_power
    global force_solar_power
    
    data={
        "s0_counter": s0_counter,
        "ladeleistung": ladeleistung,
        "evse_status": evse_status,
        "evse_hex": evse_rawhex,
        "evse_state_char": evse_state_char,
        "evse_state_int": evse_state_int,
        "evse_mode": evse_mode,
        "evse_ladeleistung": evse_ladeleistung,
        "solar_power": solar_power,
        "last_solar_power_update": last_solar_power_update,
        "sm_power": sm_power,
        "START_LADELEISTUNG": start_ladeleistung,
        "MIN_UEBERSCHUSS": min_ueberschuss,
        "MAX_BEZUG": max_bezug,
        "MIN_SOLAR_POWER": min_solar_power,
        "FORCE_SOLAR_POWER": force_solar_power,
        "uptime": time.ticks_ms(),
        }
    
    j=json.dumps(data)
    print("send_mqtt_status:"+j) 
    c.publish(b'tele/evse/state',j.encode())

print("connect to mqtt")
c = MQTTClient("umqtt_client", '10.0.0.1', user='solar', password='fronius!')
c.set_callback(sub_mqtt_cb)
c.connect()
c.subscribe(TOPIC_SMARTMETER)
c.subscribe(TOPIC_PHOTOVOLTAIK)
c.subscribe(TOPIC_CMND_WILDCARD)

btn = Pin(21, Pin.IN, Pin.PULL_UP)
btn.irq(trigger=Pin.IRQ_RISING, handler=s0_trigger)

last_mqtt_status=time.ticks_ms()

led7 = Pin(7, Pin.OUT)
led9 =  Pin(9, Pin.OUT)

print("begin mainloop")
while True:
        # Non-blocking wait for message
        indicator.on()
        
        
        led7.toggle()
        led9.toggle()
        if wdt:
            wdt.feed()
        c.check_msg()

        time.sleep(0.2)
        if wallbox.any():
            data = wallbox.read(wallbox.any())
            try: print(data.decode())
            except: print(data)
            
        curtime=time.ticks_ms()
        if time.ticks_diff(curtime, last_mqtt_status) > 30000:
            last_mqtt_status=curtime
            send_mqtt_status()
              
        led_orange.off()
        led_red.off()

c.disconnect()






