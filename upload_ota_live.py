import socket, time, subprocess, sys, concurrent.futures

ESPOTA_PATH = r"C:\Users\ACER\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.17\tools\espota.py"
BIN_PATH = r"c:\server_api_esp32\esp32_rega_simples\build\esp32.esp32.esp32\esp32_rega_simples.ino.bin"
SUBNET = "192.168.18"

print("=" * 60)
print("::: AGUARDANDO ESP32 CONECTAR AO WI-FI PARA GRAVACAO OTA :::")
print("=" * 60)

def check_ip(ip):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(0.15)
        res = sock.connect_ex((ip, 3232))
        sock.close()
        if res == 0:
            return ip
    except:
        pass
    return None

start_time = time.time()
found_ip = None

while time.time() - start_time < 90:
    ips_to_test = [f"{SUBNET}.{i}" for i in range(1, 255)]
    with concurrent.futures.ThreadPoolExecutor(max_workers=60) as executor:
        results = list(executor.map(check_ip, ips_to_test))
    
    active = [ip for ip in results if ip]
    if active:
        found_ip = active[0]
        break
    
    print(f"[{int(time.time() - start_time)}s] Escutando na rede {SUBNET}.0/24...")
    time.sleep(1)

if not found_ip:
    print("[-] Tempo limite esgotado. ESP32 nao encontrado na porta 3232.")
    sys.exit(1)

print(f"\n[+] ESP32 ENCONTRADO NO IP: {found_ip}!")
print(">>> Enviando novo firmware sem fio via ArduinoOTA...")

cmd = [
    sys.executable,
    ESPOTA_PATH,
    "-i", found_ip,
    "-p", "3232",
    "-f", BIN_PATH
]

res = subprocess.run(cmd)
if res.returncode == 0:
    print("\n[OK] GRAVACAO OTA CONCLUIDA COM SUCESSO 100% SEM FIO!")
else:
    print("\n[-] Erro durante o envio do arquivo OTA.")
