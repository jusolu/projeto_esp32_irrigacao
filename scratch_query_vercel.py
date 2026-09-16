import urllib.request
import json

url = 'https://projeto-esp32-irrigacao.vercel.app/api/status'
try:
    req = urllib.request.urlopen(url)
    data = json.loads(req.read().decode('utf-8'))
    history = data.get('history', [])
    print(f"Total de registros na Vercel: {len(history)}")
    for i, item in enumerate(history):
        print(f"[{i+1}] RTC: {item.get('rtcTime')} | Rega: {item.get('durationSec')}s | Bateria: {item.get('batteryVoltage')}V ({item.get('batteryPct')}%) | Origem: {item.get('source')}")
except Exception as e:
    print("Erro ao acessar Vercel:", e)
