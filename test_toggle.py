import serial
import time

def test_toggle():
    print("==========================================================")
    print("  TESTE DE LÓGICA DO LED (ACTIVE HIGH vs ACTIVE LOW)")
    print("==========================================================")
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.5)

    ser.write(b'\x03\r\n')
    time.sleep(0.5)
    ser.read_all()

    toggle_code = """import machine, time
led = machine.Pin(2, machine.Pin.OUT)

print("\\n[ETAPA A] Enviando led.value(1)... (Fique olhando para a placa)")
led.value(1)
time.sleep(5)

print("\\n[ETAPA B] Enviando led.value(0)... (Fique olhando para a placa)")
led.value(0)
time.sleep(5)

print("\\n[TESTE CONCLUIDO]")
"""

    ser.write(b'\x01')
    time.sleep(0.3)
    ser.write(toggle_code.encode('utf-8'))
    ser.write(b'\x04')
    time.sleep(0.5)

    start = time.time()
    while time.time() - start < 14:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            print(line.strip())

    ser.write(b'\x02')
    ser.close()

if __name__ == '__main__':
    test_toggle()
