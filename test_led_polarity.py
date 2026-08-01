import serial
import time

def test_polarity():
    print("=== TESTANDO POLARIDADE DO LED NATIVO (GPIO 2) ===")
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.5)

    ser.write(b'\x03\r\n')
    time.sleep(0.5)
    ser.read_all()

    # Script simples em Python para testar valor 1 vs 0 no GPIO 2
    test_code = """import machine, time
led = machine.Pin(2, machine.Pin.OUT)

print('\\n[TESTE 1] Enviando led.value(1)... (Verifique se o LED AZUL ACENDEU)')
led.value(1)
time.sleep(4)

print('\\n[TESTE 2] Enviando led.value(0)... (Verifique se o LED AZUL APAGOU)')
led.value(0)
time.sleep(4)

print('\\n[TESTE CONCLUIDO]')
"""

    ser.write(b'\x01') # Raw REPL
    time.sleep(0.3)
    ser.write(test_code.encode('utf-8'))
    ser.write(b'\x04')
    time.sleep(0.5)

    start = time.time()
    while time.time() - start < 10:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            print(line.strip())

    ser.write(b'\x02')
    ser.close()

if __name__ == '__main__':
    test_polarity()
