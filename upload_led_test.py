import serial
import time

# Script exclusivo para testar APENAS o LED azul nativo (GPIO 2) do ESP32 DevKit v1
MAIN_TEST_LED = """import machine, time

led = machine.Pin(2, machine.Pin.OUT)

print("=== TESTE DEDICADO DO LED AZUL EMBUTIDO DO ESP32 DEVKIT V1 ===")
print("Piscando: 1 segundo LIGADO, 1 segundo DESLIGADO...")

while True:
    led.value(1) # LIGA O LED AZUL
    time.sleep(1)
    led.value(0) # DESLIGA O LED AZUL
    time.sleep(1)
"""

def upload_led_test():
    print("=== GRAVANDO TESTE EXCLUSIVO DO LED AZUL (GPIO 2) NO ESP32 ===")
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.5)

    ser.write(b'\x03\r\n')
    time.sleep(0.5)
    ser.read_all()

    ser.write(b'\x01') # Raw REPL
    time.sleep(0.3)

    code = f"f=open('main.py','w')\nf.write({repr(MAIN_TEST_LED)})\nf.close()\nprint('GRAVACAO OK')\n"
    ser.write(code.encode('utf-8'))
    ser.write(b'\x04')
    time.sleep(2)

    out = ser.read_all().decode('utf-8', errors='ignore')
    print("Resultado da gravação:", "GRAVACAO OK" if "GRAVACAO OK" in out else out)

    ser.write(b'\x02')
    time.sleep(0.3)
    ser.write(b'import machine; machine.reset()\r\n')
    time.sleep(1)
    ser.close()
    print("\nESP32 reiniciado! Agora o LED azul no GPIO 2 deve piscar a cada 1 segundo!")

if __name__ == '__main__':
    upload_led_test()
