import serial
import time

def upload_main():
    print("=== GRAVANDO MAIN.PY NO ESP32 VIA RAW REPL ===")
    try:
        ser = serial.Serial('COM14', 115200, timeout=3)
    except Exception as e:
        print("Erro ao abrir COM14:", e)
        return

    time.sleep(1)
    
    # Interrompe qualquer script e entra no Raw REPL (Ctrl+C, depois Ctrl+A)
    ser.write(b'\r\n\x03\x03')
    time.sleep(0.5)
    ser.write(b'\x01')
    time.sleep(0.5)
    
    ser.read_all()
    
    main_code = """import network, time, urequests, machine

# LED Nativo do ESP32 (GPIO 2)
led = machine.Pin(2, machine.Pin.OUT)
led.value(0)

print("=== ESP32 STANDALONE MAIN.PY INICIADO ===")
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect('AP104-2.4G', 'papagaio')

# Pisca rápido enquanto conecta
while not wlan.isconnected():
    led.value(1)
    time.sleep(0.15)
    led.value(0)
    time.sleep(0.15)

print("🎉 WI-FI CONECTADO! IP:", wlan.ifconfig()[0])

# 3 Piscas de confirmação de conexão
for _ in range(3):
    led.value(1); time.sleep(0.3); led.value(0); time.sleep(0.2)

url = 'https://projeto-esp32-irrigacao.vercel.app/api/esp32'

while True:
    try:
        # Pulso do LED a cada requisição (Heartbeat)
        led.value(1); time.sleep(0.08); led.value(0)
        
        res = urequests.post(url, json={'soilMoisture': 52, 'temperature': 24.8, 'humidity': 58, 'batteryVoltage': 4.1})
        data = res.json()
        res.close()
        
        if data.get('waterRequested', False):
            dur = data.get('durationSec', 10)
            print('💧 REGA SOLICITADA! LIGANDO LED AZUL POR', dur, 'SEGUNDOS...')
            led.value(1) # LIGA LED FIXO
            time.sleep(dur)
            led.value(0) # DESLIGA LED
            print('✅ REGA CONCLUÍDA!')
            
            # Limpa o pedido na Vercel
            r2 = urequests.post(url, json={'waterCompleted': True})
            r2.close()
    except Exception as e:
        print('Erro no loop:', e)
    
    time.sleep(2)
"""

    # Código em python para escrever o arquivo no ESP32
    cmd = f"with open('main.py', 'w') as f:\n    f.write({repr(main_code)})\n"
    ser.write(cmd.encode('utf-8'))
    time.sleep(0.5)
    ser.write(b'\x04') # Executa no Raw REPL
    time.sleep(2)
    
    res = ser.read_all().decode('utf-8', errors='ignore')
    print("Resultado da gravação de main.py:\n", res)
    
    # Sai do Raw REPL (Ctrl+B) e dá Soft Reset (Ctrl+D)
    print("\nReiniciando o ESP32 para rodar o main.py autônomo...")
    ser.write(b'\x02\x04')
    time.sleep(3)
    
    boot_log = ser.read_all().decode('utf-8', errors='ignore')
    print("=== LOG DE BOOT DO ESP32 ===")
    print(boot_log)
    ser.close()

if __name__ == '__main__':
    upload_main()
