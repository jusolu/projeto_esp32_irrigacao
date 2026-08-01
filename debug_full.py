import serial
import time
import urllib.request
import json
import threading
import urllib.parse

HEADERS = {'Content-Type': 'application/json', 'User-Agent': 'Mozilla/5.0'}
WATER_URL = 'https://projeto-esp32-irrigacao.vercel.app/api/water'
ESP32_URL = 'https://projeto-esp32-irrigacao.vercel.app/api/esp32'
REDIS_URL = 'https://on-phoenix-191786.upstash.io'
REDIS_TOKEN = 'gQAAAAAAAu0qAAIgcDI3YTIyODQzNTQ2NWE0MjdhODExMDQ4NDVhYWQyNmExYw'

def api_post(url, payload):
    data = json.dumps(payload).encode()
    req = urllib.request.Request(url, data=data, headers=HEADERS, method='POST')
    res = urllib.request.urlopen(req, timeout=15)
    return json.loads(res.read().decode())

def api_get(url):
    req = urllib.request.Request(url, headers=HEADERS, method='GET')
    res = urllib.request.urlopen(req, timeout=15)
    return json.loads(res.read().decode())

def redis_get(key):
    req = urllib.request.Request(f'{REDIS_URL}/get/{key}',
        headers={'Authorization': f'Bearer {REDIS_TOKEN}'})
    res = urllib.request.urlopen(req, timeout=10)
    data = json.loads(res.read().decode())
    return json.loads(data['result']) if data.get('result') else None

def ts():
    return time.strftime('%H:%M:%S')

def run():
    print("=" * 65)
    print("  TESTE COMPLETO: VERCEL REDIS + ESP32 LED")
    print("=" * 65)

    # Abre serial
    ser = serial.Serial('COM14', 115200, timeout=1)
    ser.dtr = False
    ser.rts = False

    # Thread que lê o ESP32 em paralelo
    logs = []
    stop_flag = [False]

    def read_serial():
        while not stop_flag[0]:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                msg = f"  [ESP32 {ts()}] {line}"
                print(msg)
                logs.append(msg)

    t = threading.Thread(target=read_serial, daemon=True)
    t.start()

    print(f"\n[{ts()}] PASSO 1: Estado atual no Redis...")
    state_redis = redis_get('esp32:state')
    print(f"  Redis waterRequested: {state_redis.get('waterRequested') if state_redis else 'NULO (ainda não inicializado)'}")

    print(f"\n[{ts()}] PASSO 2: Lendo 3 ciclos do ESP32 antes do acionamento...")
    time.sleep(8)

    print(f"\n[{ts()}] PASSO 3: DISPARANDO /api/water REGAR AGORA...")
    try:
        water_resp = api_post(WATER_URL, {'action': 'start', 'durationSec': 10})
        print(f"  /api/water -> success: {water_resp.get('success')}")
        print(f"  /api/water -> waterRequested: {water_resp.get('state', {}).get('waterRequested')}")
    except Exception as e:
        print(f"  ERRO /api/water: {e}")

    print(f"\n[{ts()}] PASSO 4: Verificando Redis após acionamento...")
    time.sleep(1)
    state_after = redis_get('esp32:state')
    print(f"  Redis waterRequested APÓS: {state_after.get('waterRequested') if state_after else 'ERRO'}")

    print(f"\n[{ts()}] PASSO 5: Simulando chamada ESP32 a /api/esp32...")
    try:
        esp32_resp = api_post(ESP32_URL, {'soilMoisture': 50, 'temperature': 25.0, 'humidity': 60, 'batteryVoltage': 4.1})
        print(f"  /api/esp32 -> waterRequested: {esp32_resp.get('waterRequested')}")
        print(f"  /api/esp32 -> durationSec: {esp32_resp.get('durationSec')}")
    except Exception as e:
        print(f"  ERRO /api/esp32: {e}")

    print(f"\n[{ts()}] PASSO 6: Aguardando ESP32 físico responder (25s)...")
    time.sleep(25)

    stop_flag[0] = True
    ser.close()

    print("\n" + "=" * 65)
    print("  RESULTADO FINAL")
    print("=" * 65)
    water_lines = [l for l in logs if 'waterRequested' in l or 'LED' in l or 'REGA' in l or 'ERRO' in l]
    if water_lines:
        for l in water_lines:
            print(l)
    else:
        print("  Nenhum log de rega/LED capturado do ESP32.")
    print("=" * 65)

if __name__ == '__main__':
    run()
