
import time
import serial

wallbox = serial.Serial('/dev/ttyACM0', 9600)

def send_wallbox(cmd: bytes, value: int = 0):
    # send magic number + value in 2 bytes + checksum
    msg = cmd + value.to_bytes(2, 'big')
    cs = (sum(msg) & 0xff).to_bytes(1, 'big')
    wallbox.write(msg + cs)

def request_status():
    send_wallbox(b'S')
    time.sleep(0.1)
    if wallbox.inWaiting(): data = wallbox.read(wallbox.inWaiting())
    else:
        print("Error, keine Daten")
        return 'Error'
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
    else: return data