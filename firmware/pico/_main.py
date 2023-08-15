import time
import uasyncio as asyncio
import struct
from umqtt import MQTTClient
import network
from machine import UART, Pin, reset, WDT
import json

indicator = Pin("LED", Pin.OUT)
wlan = network.WLAN(network.STA_IF)
wlan.active(True)

SSID = 'FBI_surveillance_module_0xc17'
PSK = 'freesnowden!'
#SSID ='FRITZ!Box 7530 LQ'
#PSK = '04677226468126224463'
#TOPIC = b'tele/smartmeter/state'
TOPIC_RECEIVE = b'tele/wallbox/set'
TOPIC_SEND = b'tele/wallbox/state'

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
        reset()
    i += 1

print("Wlan ready")
indicator.on()

class wallbox:
    def __init__(self):
        self.serial = UART(0, 9600, rx=Pin(17), tx=Pin(16))

    def send_wallbox(self, cmd: bytes, value: int = 0):
        # send magic number + value in 2 bytes + checksum
        msg = cmd + value.to_bytes(2, 'big')
        cs = (sum(msg) & 0xff).to_bytes(1, 'big')
        self.serial.write(msg + cs)
        print("sent: ", end=' ')
        print(msg + cs)

    def read_serial(self):
        if self.serial.any():
            # wait - allow transfer to complete
            time.sleep_ms(50)
            data = self.serial.read(1)
            if not data == b'$':
                # dbg message
                try:
                    data += self.serial.readline() # type: ignore
                    print(data.decode(), end='')
                except:
                    print("encoding error")
                    print(data)
                return None
            else:
                # data package
                print("Packet received")
                if not self.serial.any() >= 15:
                    print("string too short")
                    print(self.serial.read(self.serial.any()))
                try:
                    data += self.serial.read(15)
                except:
                    print("encoding error2")
                    print(data)

                # flush rx buffer
                if self.serial.any(): self.serial.read(self.serial.any())

                return data
        
    def decode(self, data: bytes):
        msg = {
            "state": chr(data[1]),
            "mode": chr(data[2]),
            "pwm_active": str(data[3]==1),
            "leistung:": (data[4] << 8) + data[5],
            "cp_high": struct.unpack('f', data[6:10]),
            "cp_low": struct.unpack('f', data[10:14]),
            "error": data[14]
        }
        return msg


def send_mqtt(data: bytes):
    msg = wb.decode(data)
    j=json.dumps(msg)
    try:
        c.publish(TOPIC_SEND, j.encode())
    except:
        print("mqtt send failed")


async def read():
    print("read initialzied")
    while True:
        await asyncio.sleep_ms(100)
        data = wb.read_serial() 
        if not data == None:
            try:
                cs = sum(data[:15]) & 0xff
                if not cs == data[15]:
                    print("Error checksum: ", end=' ')
                    print(data)
                else:
                    send_mqtt(data)
            except:
                print("Data corrupt")

def sub_cb(topic, msg):
    print((topic, msg))
    data = json.loads(msg.decode())
    command = data["command"]
    if command == 'get':
        wb.send_wallbox(b'S')
        return
    elif command == 'reset':
        wb.send_wallbox(b'R')
        return
    leistung = data["power"]
    if leistung < 0 or leistung > 3600:
        print("Error: power out of range")
        return
    if command == 'set':
        wb.send_wallbox(b'L', leistung)
    elif command == 'force':
        wb.send_wallbox(b'F', leistung)

wdt = WDT(timeout=5000)
wb = wallbox()
async def main():
    print("main")
    asyncio.create_task(read())
    while True:
        await asyncio.sleep_ms(300)
        c.check_msg()
        wdt.feed()

# c = MQTTClient("wallbox", '10.0.0.1', user='solar', password='fronius!')
c = MQTTClient("wallbox", '10.0.0.22')
c.set_callback(sub_cb)
c.connect()
print("MQTT connected")
c.subscribe(TOPIC_RECEIVE)
asyncio.run(main())

c.disconnect()


