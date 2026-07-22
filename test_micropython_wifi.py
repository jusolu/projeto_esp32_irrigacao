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
        "print('Escaneando redes via MicroPython...')",
        "scan_results = wlan.scan()",
        "for net in scan_results:",
        "    print('SSID:', net[0].decode(), 'Channel:', net[2], 'RSSI:', net[3])",
        "print('Tentando conectar em ACERJS 9417...')",
        "wlan.connect('ACERJS 9417', '149oF8@3')",
        "for i in range(20):",
        "    print('Status:', wlan.status(), 'Connected:', wlan.isconnected())",
        "    if wlan.isconnected(): break",
        "    time.sleep(1)",
        "print('IFCONFIG:', wlan.ifconfig() if wlan.isconnected() else 'NAO CONECTOU')"
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
