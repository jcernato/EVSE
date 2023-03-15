from machine import Pin, UART
import time

wallbox = UART(0, 9600, rx=Pin(17), tx=Pin(16))

ge = Pin(5, Pin.OUT)
ro = Pin(6, Pin.OUT, value=1)


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
            print(data[:3].decode(), end=' ')
            print((data[3] << 8) + data[4])
            return (chr(data[1]))
        else:
            print("Error checksum")
            print(data)
            return 'Error'

def read_serial():
    if wallbox.any():
        data = wallbox.read(wallbox.any())
        try: print(data.decode())
        except: print(data)

dl = 0.1
while True:
    time.sleep(dl)
    ge.toggle()
    ro.toggle()

    request_status()

    read_serial()
    send_wallbox(b'V')
    time.sleep(dl)
    read_serial()
