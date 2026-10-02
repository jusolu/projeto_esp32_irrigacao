import serial
import time
import sys

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

def read_live():
    print("=== LENDO STATUS ATUAL DO ESP32 NA COM14 (SEM RESET) ===")
    ser = serial.Serial('COM14', 115200, timeout=1)
    ser.dtr = False
    ser.rts = False
    start = time.time()
    while time.time() - start < 35:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            ts = time.strftime('%H:%M:%S')
            print(f"[{ts}] {line}")
    ser.close()
    print("=== LEITURA FINALIZADA COM SUCESSO ===")

if __name__ == '__main__':
    read_live()
