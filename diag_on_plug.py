import serial.tools.list_ports, serial, time, sys

print("=" * 60)
print("::: AGUARDANDO ESP32 SER CONECTADO NO CABO USB :::")
print("=" * 60)

known_ports = {'COM4', 'COM5', 'COM6', 'COM7'}
detected_port = None
start = time.time()

while time.time() - start < 90:
    current = [p.device for p in serial.tools.list_ports.comports()]
    new_ports = [p for p in current if p not in known_ports]
    if new_ports:
        detected_port = new_ports[0]
        break
    time.sleep(0.5)

if not detected_port:
    print("[-] Nenhuma nova porta COM detectada no tempo limite.")
    sys.exit(1)

print(f"\n[+] ESP32 DETECTADO NA PORTA: {detected_port}!")
print(">>> Lendo mensagens de inicializacao do ESP32...\n")

time.sleep(0.5)
try:
    ser = serial.Serial(detected_port, 115200, timeout=1)
    ser.rts = True
    time.sleep(0.1)
    ser.rts = False
    time.sleep(0.2)

    read_start = time.time()
    while time.time() - read_start < 15:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            print(line.encode('ascii', 'ignore').decode())
    ser.close()
except Exception as e:
    print("[-] Erro ao abrir serial:", e)
