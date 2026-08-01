# =============================================================================
# SIMULADOR DO CLIENTE ESP32 RODANDO DIRETAMENTE NO WINDOWS
# =============================================================================
# Este script roda no seu Windows PC, faz o polling no servidor Vercel a cada 2s
# e simula a ação do LED/Rega quando você clica em "REGAR AGORA" no Dashboard!
# =============================================================================

import urllib.request
import json
import time
import sys

VERCEL_API_URL = 'https://projeto-esp32-irrigacao.vercel.app/api/esp32'
HEADERS = {'Content-Type': 'application/json', 'User-Agent': 'Mozilla/5.0'}

def print_header():
    print("=" * 65)
    print("  SIMULADOR DO CLIENTE RODANDO NATIVAMENTE NO WINDOWS")
    print("  Conectado a Vercel: https://projeto-esp32-irrigacao.vercel.app")
    print("=" * 65)
    print("Aguardando comandos do Dashboard Vercel...\n")

def run_windows_client():
    print_header()
    
    cycle = 0
    while True:
        cycle += 1
        try:
            payload = {'soilMoisture': 50, 'temperature': 25.0, 'humidity': 60.0, 'batteryVoltage': 4.1}
            req = urllib.request.Request(VERCEL_API_URL, data=json.dumps(payload).encode(), headers=HEADERS, method='POST')
            res = urllib.request.urlopen(req, timeout=10)
            data = json.loads(res.read().decode())
            res.close()

            water = data.get('waterRequested', False)
            dur = data.get('durationSec', 10)

            ts = time.strftime('%H:%M:%S')

            if water:
                print(f"\n[{ts}] >>> REGAR AGORA CLICADO NO DASHBOARD VERCEL! <<<")
                print(f"[{ts}]     SIMULANDO: LED AZUL LIGADO FIXO POR {dur} SEGUNDOS...")
                
                for s in range(dur, 0, -1):
                    sys.stdout.write(f"\r[{time.strftime('%H:%M:%S')}]     [LED AZUL ATIVO] Restam {s} segundos de rega...")
                    sys.stdout.flush()
                    time.sleep(1)
                
                print(f"\n[{time.strftime('%H:%M:%S')}]     REGA CONCLUIDA! Desligando LED azul e notificando Vercel...")

                r2_req = urllib.request.Request(VERCEL_API_URL, data=json.dumps({'waterCompleted': True}).encode(), headers=HEADERS, method='POST')
                r2_res = urllib.request.urlopen(r2_req, timeout=10)
                r2_res.close()
                print(f"[{time.strftime('%H:%M:%S')}]     Vercel notificada com sucesso!\n")
            else:
                sys.stdout.write(f"\r[{ts}] [Polling #{cycle}] Vercel OK | waterRequested: False (Aguardando clique no Dashboard)...")
                sys.stdout.flush()

        except Exception as e:
            print(f"\n[{time.strftime('%H:%M:%S')}] Erro de conexao com Vercel: {e}")

        time.sleep(2)

if __name__ == '__main__':
    run_windows_client()
