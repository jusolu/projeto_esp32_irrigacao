import serial
import time

def find_led():
    print("==========================================================")
    print("  BUSCANDO O PINO EXATO DO LED NATIVO NA SUA PLACA ESP32")
    print("==========================================================")
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.5)

    ser.write(b'\x03\r\n')
    time.sleep(0.5)
    ser.read_all()

    # Testando GPIO 33, 16, 22, 5, 4, 13, 12, 15, 0, 1 (os pinos de LED mais usados além do 2)
    code = """import machine, time

candidate_pins = [33, 16, 22, 5, 4, 13, 12, 15, 0, 1]

for pin_num in candidate_pins:
    try:
        p = machine.Pin(pin_num, machine.Pin.OUT)
        print(f"\\n---> AGORA TESTANDO GPIO {pin_num} (PISCANDO 4 VEZES)...")
        for _ in range(4):
            p.value(1)
            time.sleep(0.3)
            p.value(0)
            time.sleep(0.3)
        time.sleep(1)
    except Exception as e:
        print(f"Erro pino {pin_num}:", e)

print("\\n=== BUSCA FINALIZADA ===")
"""

    ser.write(b'\x01')
    time.sleep(0.3)
    ser.write(code.encode('utf-8'))
    ser.write(b'\x04')
    time.sleep(0.5)

    start = time.time()
    while time.time() - start < 35:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            print(line.strip())

    ser.write(b'\x02')
    ser.close()

if __name__ == '__main__':
    find_led()
