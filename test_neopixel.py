import serial
import time

def test_neopixel_led():
    print("==========================================================")
    print("  TESTANDO SE A SUA PLACA POSSUI LED RGB (NEOPIXEL / WS2812)")
    print("==========================================================")
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.5)

    ser.write(b'\x03\r\n')
    time.sleep(0.5)
    ser.read_all()

    # Várias placas modernas de ESP32 (S2, S3, SuperMini, Stamp) usam LED RGB Neopixel
    neopixel_code = """import machine, time
try:
    import neopixel
    print("Biblioteca neopixel disponível!")
    rgb_pins = [48, 38, 27, 18, 8, 4, 2, 5, 21]
    for pin_num in rgb_pins:
        try:
            print(f"--> Testando LED RGB Neopixel no GPIO {pin_num} (AZUL)...")
            np = neopixel.NeoPixel(machine.Pin(pin_num), 1)
            np[0] = (0, 0, 255) # Azul
            np.write()
            time.sleep(1.5)
            np[0] = (0, 0, 0)   # Desliga
            np.write()
        except Exception as e:
            pass
except Exception as err:
    print("Erro neopixel:", err)

print("=== TESTE NEOPIXEL CONCLUÍDO ===")
"""

    ser.write(b'\x01')
    time.sleep(0.3)
    ser.write(neopixel_code.encode('utf-8'))
    ser.write(b'\x04')
    time.sleep(0.5)

    start = time.time()
    while time.time() - start < 20:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            print(line.strip())

    ser.write(b'\x02')
    ser.close()

if __name__ == '__main__':
    test_neopixel_led()
