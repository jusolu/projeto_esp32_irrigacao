import serial
import time

def monitor():
    ser = serial.Serial('COM14', 115200, timeout=1)
    ser.dtr = False
    ser.rts = False
    print("=== MONITORANDO ESP32 AO VIVO - CLIQUE EM REGAR AGORA ===")
    start = time.time()
    while time.time() - start < 60:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            ts = time.strftime('%H:%M:%S')
            print(f"[{ts}] {line.strip()}")
    ser.close()

if __name__ == '__main__':
    monitor()
