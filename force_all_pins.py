import serial
import time

def force_all_pins():
    print("==========================================================")
    print("  FORÇANDO TODOS OS PINOS DO ESP32 (LIGANDO TUDO SIMULTANEAMENTE)")
    print("==========================================================")
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.5)

    ser.write(b'\x03\r\n')
    time.sleep(0.5)
    ser.read_all()

    # Liga TODOS os pinos possíveis em HIGH (1) primeiro
    code_high = """import machine, time
pins = [2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 0, 1, 3]
p_objs = []
for p in pins:
    try:
        obj = machine.Pin(p, machine.Pin.OUT)
        obj.value(1) # LIGA NÍVEL ALTO
        p_objs.append(obj)
    except: pass

print("\\n[FASE 1] TODOS OS PINOS LIGADOS EM NÍVEL ALTO (HIGH - 1) POR 6 SEGUNDOS...")
time.sleep(6)

print("\\n[FASE 2] TODOS OS PINOS MUDANDO PARA NÍVEL BAIXO (LOW - 0) POR 6 SEGUNDOS...")
for obj in p_objs:
    try: obj.value(0) # LIGA NÍVEL BAIXO
    except: pass
time.sleep(6)

print("\\n[TESTE DIRETO FINALIZADO]")
"""

    ser.write(b'\x01')
    time.sleep(0.3)
    ser.write(code_high.encode('utf-8'))
    ser.write(b'\x04')
    time.sleep(0.5)

    start = time.time()
    while time.time() - start < 15:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            print(line.strip())

    ser.write(b'\x02')
    ser.close()

if __name__ == '__main__':
    force_all_pins()
