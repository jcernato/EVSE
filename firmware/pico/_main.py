import time
import uasyncio as asyncio
import struct
from umqtt import MQTTClient
import network
from machine import UART, Pin
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
        machine.reset()
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
            data = self.serial.read(1)
            time.sleep_ms(10)
            if not data == b'$':
                try:
                    data += self.serial.readline()
                    print(data.decode(), end='')
                except:
                    print("error0")
                    print(data)
                return None
            else:
                print("Packet received")
                if not self.serial.any() >= 14:
                    print("string too short")
                    print(self.serial.read(self.serial.any()))
                try:
                    data += self.serial.read(14)
                except:
                    print("error1")
                    print(data)
                # while True:
                #     byte = self.serial.read(1)
                #     if byte == b'#':
                #         break
                #     else:
                #         try:
                #             data += byte
                #         except TypeError:
                #             print("type error", type(byte), byte)
                #             return None
                if self.serial.any(): self.serial.read(self.serial.any())
                return data

wb = wallbox()
def decode(data: bytes):
    print("command: " + chr(data[0]))
    print("state: " + chr(data[1]))
    print("mode: " + chr(data[2]))
    print("pwm active: " + str(data[3]==1))
    print("leistung: " + str((data[4] << 8) + data[5]))
    print("high pegel: %.2f" %(struct.unpack('f', data[6:10])))
    print("low pegel: %.2f" %(struct.unpack('f', data[10:14])))

def send_mqtt(data: bytes):
    msg = {
        "state": chr(data[1]),
        "mode": chr(data[2]),
        "pwm_active": str(data[3]==1),
        "leistung:": (data[4] << 8) + data[5],
        "cp_high": struct.unpack('f', data[6:10]),
        "cp_low": struct.unpack('f', data[10:14])
    }
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
                cs = sum(data[:14]) & 0xff
                if not cs == data[14]:
                    print("Error checksum: ", end=' ')
                    print(data)
                else:
                    # decode(data)
                    send_mqtt(data)
            except:
                print("Data corrupt")


async def main():
    print("main")
    n=0
    asyncio.create_task(read())
    while True:
        await asyncio.sleep_ms(300)
        c.check_msg()
        # print("n: " + str(n))
        # wb.send_wallbox(b'S', n)
        # n += 1


def sub_cb(topic, msg):
    print((topic, msg))
    data = json.loads(msg.decode())
    command = data["command"]
    if command == 'get':
        wb.send_wallbox(b'S')
        return
    leistung = data["power"]
    if leistung < 0 or leistung > 3600:
        print("Error: power out of range")
        return
    if command == 'set':
        wb.send_wallbox(b'L', leistung)
    elif command == 'force':
        wb.send_wallbox(b'F', leistung)



# c = MQTTClient("wallbox", '10.0.0.1', user='solar', password='fronius!')
c = MQTTClient("wallbox", '10.0.0.22')
c.set_callback(sub_cb)
c.connect()
print("MQTT connected")
c.subscribe(TOPIC_RECEIVE)
asyncio.run(main())

c.disconnect()


