import serial
import time

def run_micropython_test():
    ser = serial.Serial('COM14', 115200, timeout=2)
    time.sleep(1)
    
    # Interrupt any running program and enter REPL
    ser.write(b'\r\n\x03\x03')
    time.sleep(0.5)
    print("REPL Response:")
    print(ser.read_all().decode('utf-8', errors='ignore'))

    commands = [
        "import network, time",
        "wlan = network.WLAN(network.STA_IF)",
        "wlan.active(True)",
        "print('=== ESCANEANDO REDES WI-FI DISPONÍVEIS ===')",
        "scan_results = wlan.scan()",
        "for net in scan_results:",
        "    print('SSID:', net[0].decode('utf-8', 'ignore'), '| Ch:', net[2], '| RSSI:', net[3])",
        "print('=== TENTANDO CONECTAR EM AP104-2.4G ===')",
        "wlan.connect('AP104-2.4G', 'papagaio')",
        "for i in range(25):",
        "    status = wlan.status()",
        "    connected = wlan.isconnected()",
        "    print(f'Tentativa {i+1}/25 - Status: {status} | Conectado: {connected}')",
        "    if connected: break",
        "    time.sleep(1)",
        "if wlan.isconnected():",
        "    print('🎉 CONECTADO COM SUCESSO!')",
        "    print('Configuração IP:', wlan.ifconfig())",
        "    print('=== TESTANDO REQUISIÇÃO HTTP NA VERCEL API ===')",
        "    import urequests",
        "    r = urequests.get('https://projeto-esp32-irrigacao.vercel.app/api/esp32')",
        "    print('Status HTTP Vercel:', r.status_code)",
        "    print('Resposta da API:', r.text)",
        "    r.close()",
        "else:",
        "    print('❌ Falha na conexão. Status final:', wlan.status())"
    ]

    print("\nEnviando comandos Python para o ESP32...")
    for cmd in commands:
        ser.write((cmd + '\r\n').encode('utf-8'))
        time.sleep(0.3)
    
    time.sleep(10)
    output = ser.read_all().decode('utf-8', errors='ignore')
    print("\n--- OUTPUT DO ESP32 (MICROPYTHON) ---")
    print(output)
    ser.close()

if __name__ == '__main__':
    run_micropython_test()
