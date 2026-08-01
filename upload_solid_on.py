import serial
import time

# Script limpo que ativa o LED azul (GPIO 2) e o mantém LIGADO FIXO
SOLID_ON_PY = """import machine

# Configura o LED azul nativo no GPIO 2 e liga fixo
led = machine.Pin(2, machine.Pin.OUT)
led.value(1)

print("=== LED AZUL NATIVO (GPIO 2) LIGADO FIXO! ===")
"""

def upload_solid_on():
    print("=== GRAVANDO SCRIPT LIMPO: LED AZUL LIGADO FIXO NO ESP32 ===")
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.5)

    ser.write(b'\x03\r\n')
    time.sleep(0.5)
    ser.read_all()

    ser.write(b'\x01') # Raw REPL
    time.sleep(0.3)

    code = f"f=open('main.py','w')\nf.write({repr(SOLID_ON_PY)})\nf.close()\nprint('GRAVACAO OK')\n"
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
    print("\nESP32 reiniciado! O LED azul no GPIO 2 está LIGADO FIXO agora!")

if __name__ == '__main__':
    upload_solid_on()
