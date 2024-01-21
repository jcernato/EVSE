from machine import UART, Pin
from time import sleep_ms, ticks_ms
from struct import unpack
import json

class Wallbox:
    serial: UART
    send_request: bool = False
    repaint: bool
    TOPIC_RECEIVE = b'cmd/wallbox'
    TOPIC_SEND = b'tele/wallbox/state'
    data = {"state": "no data available"}

    def send(self, cmd: bytes, value: int = 0):
        # send magic number + value in 2 bytes + checksum
        msg = cmd + value.to_bytes(2, 'big')
        cs = (sum(msg) & 0xff).to_bytes(1, 'big')
        self.serial.write(msg + cs)
        print("sent: ", end=' ')
        print(msg + cs)

    def read_serial(self):
        if not self.serial.any(): return

        # wait - allow transfer to complete
        sleep_ms(50)
        data = self.serial.read(1)
        if not data == b'$':
            # dbg message
            try:
                data += self.serial.readline() # type: ignore
                print(data.decode(), end='')
            except:
                print("encoding error")
                print(data)
        else:
            # data package
            print("Packet received")
            if self.serial.any() < 15:
                print("string too short")
                print(self.serial.read(self.serial.any()))
                return
            try:
                data += self.serial.read(15)
                cs = sum(data[:15]) & 0xff
                if not cs == data[15]:
                    raise Exception("Error checksum")
                else:
                    self.decode(data)
            except Exception as e:
                print(e)
                print(data)

            # flush rx buffer
            if self.serial.any(): self.serial.read(self.serial.any())
        
    def decode(self, data: bytes):
        self.timestamp = ticks_ms()
        self.repaint = True
        self.data = {
            "state": chr(data[1]),
            "mode": chr(data[2]),
            "pwm_active": str(data[3]==1),
            "leistung": (data[4] << 8) + data[5],
            "cp_high": unpack('f', data[6:10])[0],
            "cp_low": unpack('f', data[10:14])[0],
            "error": data[14]
        }

    def parse_mqtt(self, msg):
        data = json.loads(msg.decode())
        command = data["command"]
        if command == 'get':
            self.send(b'S')
            sleep_ms(200)
            self.read_serial()
            self.send_request = True
            return
        elif command == 'reset':
            self.send(b'R')
            return
        leistung = data["power"]
        if leistung < 0 or leistung > 3600:
            print("Error: power out of range")
            return
        if command == 'set':
            self.send(b'L', leistung)
        elif command == 'force':
            self.send(b'F', leistung)
        else:
            print("Unknown command")

class Counter:
    TOPIC_RECEIVE = b'cmd/S0'
    TOPIC_SEND = b'tele/S0/state'
    def __init__(self):
        self.S0 = Pin(21, Pin.IN, Pin.PULL_UP)
        self.S0.irq(trigger=Pin.IRQ_RISING, handler=self.increment)
        self.Led = Pin(6, Pin.OUT)
        self.data = { "count": 0 }
        self.send_request = False

    def increment(self, pin):
        self.data["count"] += 1
        print(self.data["count"])
        self.Led.toggle()

    def parse_mqtt(self, msg):
        data = json.loads(msg.decode())
        command = data["command"]
        count = data["count"]
        if command == 'set':
            self.data["count"] = count
        elif command == 'get':
            self.send_request = True
        else:
            print("Unknown command")