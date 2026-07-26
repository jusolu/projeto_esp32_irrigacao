import serial
import time

# main.py que vai rodar no ESP32
# NOTA: este arquivo é gravado linha a linha no ESP32 via serial

MAIN_PY = """import network, time, urequests, machine

led = machine.Pin(2, machine.Pin.OUT)
led.value(0)

print("ESP32 MAIN.PY INICIADO")
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect('AP104-2.4G', 'papagaio')

while not wlan.isconnected():
    led.value(1)
    time.sleep(0.1)
    led.value(0)
    time.sleep(0.1)

ip = wlan.ifconfig()[0]
print("CONECTADO! IP:", ip)

for _ in range(5):
    led.value(1)
    time.sleep(0.1)
    led.value(0)
    time.sleep(0.1)

url = 'https://projeto-esp32-irrigacao.vercel.app/api/esp32'

while True:
    try:
        led.value(1)
        time.sleep(0.06)
        led.value(0)
        payload = {'soilMoisture': 50, 'temperature': 25, 'humidity': 60, 'batteryVoltage': 4.1}
        res = urequests.post(url, json=payload)
        data = res.json()
        res.close()
        water = data.get('waterRequested', False)
        dur = data.get('durationSec', 10)
        print("waterRequested:", water)
        if water:
            print("LIGANDO LED POR", dur, "s")
            led.value(1)
            time.sleep(dur)
            led.value(0)
            print("REGA CONCLUIDA")
            r2 = urequests.post(url, json={'waterCompleted': True})
            r2.close()
    except Exception as e:
        print("ERRO:", e)
    time.sleep(2)
"""

def save_main():
    print("=== GRAVANDO main.py CORRIGIDO NO ESP32 ===")
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

    # Grava usando Raw REPL (modo mais confiável para arquivos multi-linha)
    ser.write(b'\x01')  # Ctrl+A = Raw REPL
    time.sleep(0.3)

    code = f"f=open('main.py','w')\nf.write({repr(MAIN_PY)})\nf.close()\nprint('GRAVADO OK')\n"
    ser.write(code.encode('utf-8'))
    ser.write(b'\x04')  # Ctrl+D = executa
    time.sleep(3)

    out = ser.read_all().decode('utf-8', errors='ignore')
    print("RESULTADO GRAVAÇÃO:", out)

    # Sai do Raw REPL e reinicia
    ser.write(b'\x02')  # Ctrl+B = Normal REPL
    time.sleep(0.3)
    ser.write(b'import machine; machine.reset()\r\n')
    time.sleep(2)

    print("\n=== LENDO OUTPUT DO BOOT ===")
    start = time.time()
    while time.time() - start < 12:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            print(line.strip())

    ser.close()

if __name__ == '__main__':
    save_main()
