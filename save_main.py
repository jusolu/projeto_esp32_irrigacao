import serial
import time

def save_main_to_esp32():
    print("=== GRAVANDO MAIN.PY NO ESP32 (PERSISTENTE NO FIRMWARE) ===")
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.5)

    # Interrompe qualquer script
    ser.write(b'\x03\r\n')
    time.sleep(0.5)

    main_content = """import network, time, urequests, machine

# LED Nativo do ESP32 (GPIO 2)
led = machine.Pin(2, machine.Pin.OUT)
led.value(0)

print("=== ESP32 MAIN.PY PERSISTENTE INICIADO ===")
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect('AP104-2.4G', 'papagaio')

while not wlan.isconnected():
    led.value(1)
    time.sleep(0.1)
    led.value(0)
    time.sleep(0.1)

print("🎉 WI-FI CONECTADO! IP:", wlan.ifconfig()[0])
for _ in range(5):
    led.value(1); time.sleep(0.1); led.value(0); time.sleep(0.1)

url = 'https://projeto-esp32-irrigacao.vercel.app/api/esp32'

while True:
    try:
        # Pisca curto a cada 2s indicando consulta no Vercel (Heartbeat)
        led.value(1); time.sleep(0.06); led.value(0)
        
        res = urequests.post(url, json={'soilMoisture': 50, 'temperature': 25.0, 'humidity': 60, 'batteryVoltage': 4.1})
        data = res.json()
        res.close()
        
        if data.get('waterRequested', False):
            dur = data.get('durationSec', 10)
            print('💧 REGA SOLICITADA! LIGANDO LED AZUL NATIVO POR', dur, 'SEGUNDOS...')
            led.value(1) # LED LIGADO FIXO DURANTE A REGA
            time.sleep(dur)
            led.value(0) # DESLIGA LED
            print('✅ REGA CONCLUÍDA!')
            
            # Notifica a Vercel que o pedido foi concluído
            r2 = urequests.post(url, json={'waterCompleted': True})
            r2.close()
    except Exception as e:
        print('Erro no loop:', e)
    
    time.sleep(2)
"""

    ser.write(b"f = open('main.py', 'w')\r\n")
    time.sleep(0.3)

    for line in main_content.splitlines():
        escaped_line = line.replace('\\', '\\\\').replace("'", "\\'")
        cmd = f"f.write('{escaped_line}\\n')\r\n"
        ser.write(cmd.encode('utf-8'))
        time.sleep(0.04)

    ser.write(b"f.close()\r\n")
    time.sleep(0.5)

    ser.write(b"import os; print('=== ARQUIVOS NO ESP32 ===:', os.listdir())\r\n")
    time.sleep(1)
    print(ser.read_all().decode('utf-8', errors='ignore'))

    print("\n[REINICIANDO ESP32 PARA EXECUTAR O MAIN.PY AUTÔNOMO]")
    ser.write(b"import machine; machine.reset()\r\n")
    time.sleep(1)
    ser.close()

if __name__ == '__main__':
    save_main_to_esp32()
