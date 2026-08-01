import serial
import time

def reset_normal():
    print("=== REINICIANDO ESP32 EM MODO DE EXECUÇÃO NORMAL (SPI_FAST_FLASH_BOOT) ===")
    ser = serial.Serial('COM14', 115200, timeout=1)
    
    # Pulsa EN (RTS) sem segurar IO0 (DTR) para sair do modo bootloader
    ser.dtr = False
    ser.rts = True
    time.sleep(0.2)
    ser.rts = False
    time.sleep(0.5)

    print("\n--- LENDO LOGS DO BOOT NORMAL DO ESP32 ---")
    start = time.time()
    while time.time() - start < 10:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            print(f"  [SERIAL] {line.encode('ascii', 'ignore').decode()}")

    ser.close()

if __name__ == '__main__':
    reset_normal()
