import serial
import time

def test_active_low_pins():
    print("\n=======================================================")
    print("  TESTANDO PINOS EM NÍVEL BAIXO (ACTIVE LOW: value(0))")
    print("  Muitas placas ESP32 ligam o LED nativo enviando 0 (LOW)!")
    print("=======================================================\n")
    
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.5)

    ser.write(b'\x03\r\n')
    time.sleep(0.5)
    ser.read_all()

    scan_script = """import machine, time

# Pinos mais comuns em placas com LED Active-LOW (incluindo GPIO 2, 5, 1, 3, 16, 22)
pins_to_test = [2, 1, 3, 5, 16, 17, 22, 23]

# Inicializa todos em HIGH (1 = desligado em Active-LOW)
objs = {}
for p in pins_to_test:
    try:
        objs[p] = machine.Pin(p, machine.Pin.OUT)
        objs[p].value(1)
    except: pass

print("\\n>>> COMEÇANDO TESTE ACTIVE-LOW (LIGANDO ENVIANDO 0) <<\\n")

for pin_num in pins_to_test:
    if pin_num in objs:
        try:
            print(f"[{time.ticks_ms()//1000}s] 💡 TESTANDO GPIO {pin_num} -> ENVIANDO 0 (LOW) POR 3 SEGUNDOS...")
            objs[pin_num].value(0) # TENTA LIGAR COM LOW (0)
            time.sleep(3.0)
            objs[pin_num].value(1) # APAGA COM HIGH (1)
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
    while time.time() - start < 35:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            print(line.strip())

    ser.write(b'\x02')
    ser.close()

if __name__ == '__main__':
    test_active_low_pins()
