import serial
import time

def run_blink_dashboard_integration():
    print("=== ESP32: PISCAR LED NATIVO (GPIO 2) + INTEGRAÇÃO VERCEL DASHBOARD ===")
    
    # Mata tarefas anteriores se a porta estivesse em uso
    try:
        ser = serial.Serial('COM14', 115200, timeout=2)
    except Exception as e:
        print("Erro ao abrir serial COM14:", e)
        return

    time.sleep(1)
    ser.write(b'\r\n\x03\x03')
    time.sleep(0.5)
    ser.read_all()

    # Código MicroPython que controla o LED Nativo (GPIO 2) e conecta no Vercel
    commands = [
        "import network, time, urequests, machine",
        "led = machine.Pin(2, machine.Pin.OUT)",
        "led.value(0)", # Inicia desligado
        "wlan = network.WLAN(network.STA_IF)",
        "wlan.active(True)",
        "wlan.connect('AP104-2.4G', 'papagaio')",
        "print('Conectando ao Wi-Fi...')",
        "while not wlan.isconnected():",
        "    led.value(1)",
        "    time.sleep(0.1)",
        "    led.value(0)",
        "    time.sleep(0.1)",
        "print('🎉 ESP32 ONLINE! IP:', wlan.ifconfig()[0])",
        "print('==================================================')",
        "print('💡 LED NATIVO (GPIO 2) PRONTO PARA SINALIZAR REGA')",
        "print('==================================================')",
        "while True:",
        "    try:",
        "        # Pisca rápido 2x indicando ciclo de consulta ao servidor",
        "        for _ in range(2): led.value(1); time.sleep(0.05); led.value(0); time.sleep(0.05)",
        "        res = urequests.post('https://projeto-esp32-irrigacao.vercel.app/api/esp32', json={'soilMoisture': 50, 'temperature': 25.0, 'humidity': 60, 'batteryVoltage': 4.1})",
        "        data = res.json()",
        "        res.close()",
        "        water_req = data.get('waterRequested', False)",
        "        dur = data.get('durationSec', 10)",
        "        if water_req:",
        "            print(f'\\n💧 🚨 COMANDO REGAR RECEBIDO! LIGANDO LED NATIVO POR {dur} SEGUNDOS...')",
        "            led.value(1)", # LIGA LED NATIVO
        "            time.sleep(dur)", # Fica ligado o tempo da rega
        "            led.value(0)", # DESLIGA LED NATIVO
        "            print('   ✅ Rega concluída! Desligando LED e notificando Vercel...')",
        "            res_done = urequests.post('https://projeto-esp32-irrigacao.vercel.app/api/esp32', json={'waterCompleted': True, 'soilMoisture': 70})",
        "            res_done.close()",
        "        else:",
        "            print('.', end='')",
        "    except Exception as e:",
        "        print('\\n❌ Erro de comunicação:', e)",
        "    time.sleep(2)"
    ]

    print("\nEnviando código do LED Blink para o ESP32...")
    for cmd in commands:
        ser.write((cmd + '\r\n').encode('utf-8'))
        time.sleep(0.3)
    
    print("\n[MONITORANDO LOGS DO ESP32] Pressione no Dashboard para acionar o LED!")
    start = time.time()
    while time.time() - start < 120:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            print(line.strip())
            
    ser.close()

if __name__ == '__main__':
    run_blink_dashboard_integration()
