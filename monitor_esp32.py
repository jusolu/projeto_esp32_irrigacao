import serial
import time
import sys

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

def monitor():
    ser = serial.Serial('COM14', 115200, timeout=1)
    
    # Pulso rápido de reset para iniciar o ciclo do zero
    ser.dtr = False
    ser.rts = True
    time.sleep(0.2)
    ser.rts = False
    time.sleep(0.3)

    print("=== MONITORANDO CICLO HIDRÁULICO AO VIVO NA COM14 ===")
    start = time.time()
    while time.time() - start < 65:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            ts = time.strftime('%H:%M:%S')
            print(f"[{ts}] {line.strip()}")
    ser.close()
    print("=== FIM DO MONITORAMENTO ===")

if __name__ == '__main__':
    monitor()
