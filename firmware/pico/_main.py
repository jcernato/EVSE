import time
from umqtt import MQTTClient
import network
from machine import UART, Pin
import json

indicator = Pin("LED", Pin.OUT)
wlan = network.WLAN(network.STA_IF)
wlan.active(True)

SSID ='FRITZ!Box 7530 LQ'
PSK = '04677226468126224463'
#TOPIC = b'tele/smartmeter/state'
TOPIC = b'test'

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
indicator.on()

wallbox = UART(0, 9600, rx=Pin(17), tx=Pin(16))

def send_wallbox(cmd: bytes, value: int = 0):
    # send magic number + value in 2 bytes + checksum
    msg = cmd + value.to_bytes(2, 'big')
    cs = (sum(msg) & 0xff).to_bytes(1, 'big')
    wallbox.write(msg + cs)

def request_status():
    send_wallbox(b'S')
    time.sleep_ms(100)
    if wallbox.any(): data = wallbox.read(wallbox.any())
    else: return 'Error'
    if len(data) == 6:
        cs = sum(data[:5]) & 0xff
        if cs == data[5]:
            if chr(data[2]) == 'M':
                print("Manueller Modus")
            return chr(data[1])
        else:
            print("Error checksum")
            print(data)
            return 'Error'


# Received messages from subscriptions will be delivered to this callback
counter = 0
ladeleistung = 1000
def sub_cb(topic, msg):
    global ladeleistung
    global counter
  #  print((topic, msg))
    data = json.loads(msg.decode())
    power = int(data["power"])

    evse = request_status()

    if evse == 'A':
        print('disconnected')
        ladeleistung = 1000
    elif evse == 'B' or evse == 'C':
        print("connected")
        if power < -1200 and ladeleistung == 0:
            ladeleistung = 1100
            counter = 0
        elif power < -100 and ladeleistung < 3400:
            ladeleistung += 100
            counter = 0
            
        if power > 0:
            ladeleistung -= power
        if ladeleistung < 1000:
            ladeleistung = 1000
            counter += 1
    
        # if ladeleistung < 1100 and counter > 6:
            # ladeleistung=0
    elif evse == 'F':
        print("Wallbox error")
        ladeleistung = 0
    
    else:
        print("Error")
        counter += 1
    if counter > 6:
        ladeleistung = 0
    send_wallbox(b'L', ladeleistung)

c = MQTTClient("umqtt_client", '10.0.0.1', user='solar', password='fronius!')
c.set_callback(sub_cb)
c.connect()
c.subscribe(TOPIC)
while True:
        # Non-blocking wait for message
        c.check_msg()

        time.sleep(0.2)
        if wallbox.any():
            data = wallbox.read(wallbox.any())
            try: print(data.decode())
            except: print(data)

c.disconnect()


