import serial
import time
import urllib.request
import json
import threading

HEADERS = {'Content-Type': 'application/json', 'User-Agent': 'Mozilla/5.0'}
WATER_URL = 'https://projeto-esp32-irrigacao.vercel.app/api/water'
ESP32_URL = 'https://projeto-esp32-irrigacao.vercel.app/api/esp32'
NPOINT_URL = 'https://api.npoint.io/512b9bf227ca7ffaa3c3'

def api_call(url, payload=None, method='POST'):
    data = json.dumps(payload).encode() if payload else None
    req = urllib.request.Request(url, data=data, headers=HEADERS, method=method)
    res = urllib.request.urlopen(req, timeout=10)
    return json.loads(res.read().decode())

def monitor_serial(ser, duration=30):
    """Lê a serial do ESP32 em tempo real"""
    start = time.time()
    while time.time() - start < duration:
        line = ser.readline().decode('utf-8', errors='ignore')
        if line.strip():
            ts = time.strftime('%H:%M:%S')
            print(f"  [ESP32 {ts}] {line.strip()}")

def run_debug():
    print("=" * 60)
    print("DEBUG COMPLETO: DASHBOARD -> VERCEL -> ESP32")
    print("=" * 60)

    # 1. Verifica estado atual da Vercel
    print("\n[1] Verificando /api/status...")
    status = api_call('https://projeto-esp32-irrigacao.vercel.app/api/status', method='GET')
    print(f"   waterRequested atual: {status.get('waterRequested')}")
    print(f"   durationSec atual: {status.get('durationSec')}")

    # 2. Verifica npoint.io diretamente
    print("\n[2] Verificando npoint.io (armazenamento persistente)...")
    npoint_data = api_call(NPOINT_URL, method='GET')
    print(f"   npoint waterRequested: {npoint_data.get('waterRequested')}")

    # 3. Abre serial para monitorar ESP32
    print("\n[3] Conectando ao ESP32 serial (COM14)...")
    ser = serial.Serial('COM14', 115200, timeout=1)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.3)

    # Lê alguns ciclos do ESP32 ANTES de acionar
    print("\n[4] Lendo 2 ciclos do ESP32 ANTES do acionamento:")
    monitor_serial(ser, duration=6)

    # 4. Dispara o comando REGAR via /api/water
    print("\n[5] DISPARANDO REGAR AGORA via /api/water...")
    water_resp = api_call(WATER_URL, payload={'action': 'start', 'durationSec': 10})
    print(f"   Vercel /api/water -> waterRequested: {water_resp.get('state', {}).get('waterRequested')}")

    # 5. Verifica npoint imediatamente após
    print("\n[6] Verificando npoint.io APÓS o acionamento...")
    time.sleep(1)
    npoint_after = api_call(NPOINT_URL, method='GET')
    print(f"   npoint waterRequested APÓS: {npoint_after.get('waterRequested')}")

    # 6. Simula o que o ESP32 faria ao chamar /api/esp32
    print("\n[7] Simulando chamada do ESP32 para /api/esp32...")
    esp32_resp = api_call(ESP32_URL, payload={'soilMoisture': 50, 'temperature': 25, 'humidity': 60, 'batteryVoltage': 4.1})
    print(f"   /api/esp32 retorna waterRequested: {esp32_resp.get('waterRequested')}")

    # 7. Monitora o ESP32 por 25s esperando ele receber o comando
    print("\n[8] Monitorando ESP32 serial por 25s (aguardando resposta do LED):")
    monitor_serial(ser, duration=25)

    ser.close()
    print("\n" + "=" * 60)
    print("DEBUG CONCLUIDO")

if __name__ == '__main__':
    run_debug()
