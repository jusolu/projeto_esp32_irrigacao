import serial
import time

MAIN_PY = """import network, time, urequests, machine

# No ESP32 DevKit V1, o LED azul nativo é o GPIO 2 (const int LED_BUILTIN = 2)
LED_BUILTIN = 2
led = machine.Pin(LED_BUILTIN, machine.Pin.OUT)
led.value(0) # Inicia DESLIGADO

print("=== ESP32 DevKit V1 (LED_BUILTIN = 2) INICIADO ===")
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect('AP104-2.4G', 'papagaio')

while not wlan.isconnected():
    time.sleep(0.2)

print("🎉 WI-FI CONECTADO! IP:", wlan.ifconfig()[0])
led.value(0)

url = 'https://projeto-esp32-irrigacao.vercel.app/api/esp32'

while True:
    try:
        payload = {'soilMoisture': 50, 'temperature': 25.0, 'humidity': 60.0, 'batteryVoltage': 4.1}
        res = urequests.post(url, json=payload)
        data = res.json()
        res.close()
        
        water = data.get('waterRequested', False)
        dur = data.get('durationSec', 10)
        
        if water:
            print("💧 REGA SOLICITADA NO DASHBOARD! LIGANDO LED AZUL (GPIO 2) POR", dur, "s")
            led.value(1) # LIGA LED AZUL EMBUTIDO (GPIO 2)
            time.sleep(dur) # FICA LIGADO FIXO DURANTE A REGA
            led.value(0) # DESLIGA LED AZUL
            print("✅ REGA CONCLUIDA! Notificando Vercel...")
            
            r2 = urequests.post(url, json={'waterCompleted': True})
            r2.close()
        else:
            led.value(0)
    except Exception as e:
        print("Erro loop:", e)
        
    time.sleep(2)
"""

def upload_devkit_v1_main():
    print("=== GRAVANDO MAIN.PY COM LED_BUILTIN = 2 NO ESP32 DevKit V1 ===")
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.5)

    for _ in range(3):
        ser.write(b'\x03')
        time.sleep(0.2)
    ser.write(b'\r\n')
    time.sleep(0.5)
    ser.read_all()

    ser.write(b'\x01') # Raw REPL
    time.sleep(0.3)

    code = f"f=open('main.py','w')\nf.write({repr(MAIN_PY)})\nf.close()\nprint('GRAVACAO OK')\n"
    ser.write(code.encode('utf-8'))
    ser.write(b'\x04')
    time.sleep(2.5)

    out = ser.read_all().decode('utf-8', errors='ignore')
    print("Resultado da gravação:", "GRAVACAO OK" if "GRAVACAO OK" in out else out)

    ser.write(b'\x02')
    time.sleep(0.3)
    ser.write(b'import machine; machine.reset()\r\n')
    time.sleep(1)
    ser.close()
    print("\nESP32 DevKit V1 reiniciado com LED_BUILTIN = 2 configurado!")

if __name__ == '__main__':
    upload_devkit_v1_main()
