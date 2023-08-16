import uasyncio as asyncio
from verbindung import *
from wallbox import Wallbox
from machine import UART, Pin, WDT

print("Wallbox MQTT Gateway")

uart = UART(0, 9600, rx=Pin(17), tx=Pin(16))
wb = Wallbox()
wb.serial = uart

print("Subscription topic: " + wb.TOPIC_RECEIVE.decode())
print("Publication topic: " + wb.TOPIC_SEND.decode())

d=[wb]
network = Verbindung(d)

async def read_serial_wallbox():
    print("read serial wallbox started")
    while True:
        await asyncio.sleep_ms(50)
        wb.read_serial() 

wdt = WDT(timeout=5000)
async def main():
    print("main started")
    asyncio.create_task(read_serial_wallbox())
    print("Ready!")
    while True:
        await asyncio.sleep_ms(500)
        network.loop()
        wdt.feed()

asyncio.run(main())