import serial
import time
import urllib.request
import json
import threading

HEADERS = {'Content-Type': 'application/json', 'User-Agent': 'Mozilla/5.0'}
WATER_URL = 'https://projeto-esp32-irrigacao.vercel.app/api/water'

def trigger_water():
    print("\n[PYTHON] Disparando REGAR AGORA na API Vercel...")
    req = urllib.request.Request(WATER_URL, data=json.dumps({'action': 'start', 'durationSec': 10}).encode(), headers=HEADERS, method='POST')
    try:
        res = urllib.request.urlopen(req, timeout=10)
        print("[PYTHON] Resposta da API /api/water:", res.read().decode())
    except Exception as e:
        print("[PYTHON] Erro /api/water:", e)

def run_deep_diag():
    print("==========================================================")
    print("  DIAGNÓSTICO PROFUNDO DA COMUNICAÇÃO C++ <-> VERCEL")
    print("==========================================================")

    ser = serial.Serial('COM14', 115200, timeout=1)
    ser.dtr = False
    ser.rts = True
    time.sleep(0.1)
    ser.rts = False

    print("\n[1] Lendo boot e logs da serial por 10 segundos...")
    start = time.time()
    while time.time() - start < 10:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            print(f"  [ESP32] {line.encode('ascii', 'ignore').decode()}")

    print("\n[2] Agora disparando o comando de rega na API Vercel...")
    t = threading.Thread(target=trigger_water)
    t.start()

    print("\n[3] Lendo respostas do ESP32 pelos próximos 20 segundos...")
    start = time.time()
    while time.time() - start < 20:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            print(f"  [ESP32] {line.encode('ascii', 'ignore').decode()}")

    ser.close()
    print("\n==========================================================")
    print("  DIAGNÓSTICO FINALIZADO")

if __name__ == '__main__':
    run_deep_diag()
