import serial
import time

def listen():
    print("::: LISTENING TO ESP32 SERIAL ON COM14 :::")
    ser = serial.Serial('COM14', 115200, timeout=1)
    ser.dtr = False
    ser.rts = False
    start = time.time()
    while time.time() - start < 45:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            print(f"[{time.strftime('%H:%M:%S')}] {line}")
    ser.close()

if __name__ == '__main__':
    listen()
