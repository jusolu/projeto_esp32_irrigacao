import serial
import time

def scan_pins_slowly():
    print("\n=======================================================")
    print("  INICIANDO VARREDURA DE PINOS DE LED NO ESP32")
    print("  Observe o LED da sua placa enquanto os pinos são testados!")
    print("=======================================================\n")
    
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.5)

    ser.write(b'\x03\r\n')
    time.sleep(0.5)
    ser.read_all()

    scan_script = """import machine, time

pins_to_test = [2, 4, 5, 12, 13, 14, 15, 16, 18, 19, 21, 22, 23, 25, 26, 27]

print("\\n>>> COMEÇANDO O TESTE DOS PINOS <<\\n")

for pin_num in pins_to_test:
    try:
        p = machine.Pin(pin_num, machine.Pin.OUT)
        print(f"[{time.ticks_ms()//1000}s] 💡 TESTANDO GPIO {pin_num} (LIGANDO POR 3 SEGUNDOS)...")
        p.value(1)
        time.sleep(3.0)
        p.value(0)
        time.sleep(0.5)
    except Exception as e:
        print(f"Erro GPIO {pin_num}:", e)

print("\\n>>> TESTE CONCLUÍDO! <<<")
"""

    ser.write(b'\x01') # Raw REPL
    time.sleep(0.3)
    ser.write(scan_script.encode('utf-8'))
    ser.write(b'\x04')
    time.sleep(0.5)

    start = time.time()
    while time.time() - start < 65:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            print(line.strip())

    ser.write(b'\x02')
    ser.close()

if __name__ == '__main__':
    scan_pins_slowly()
