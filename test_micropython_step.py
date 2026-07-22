import serial
import time

def send_cmd(ser, cmd, delay=1.0):
    ser.write((cmd + '\r\n').encode('utf-8'))
    time.sleep(delay)
    output = ser.read_all().decode('utf-8', errors='ignore')
    return output

def run():
    ser = serial.Serial('COM14', 115200, timeout=2)
    
    # Soft reset
    ser.write(b'\x03\x04')
    time.sleep(1.5)
    print("=== BOOT LOG ===")
    print(ser.read_all().decode('utf-8', errors='ignore'))
    
    print("\n--- 1. Ativando Wi-Fi no MicroPython ---")
    print(send_cmd(ser, "import network, time"))
    print(send_cmd(ser, "wlan = network.WLAN(network.STA_IF)"))
    print(send_cmd(ser, "wlan.active(True)", delay=1.5))
    
    print("\n--- 2. Escaneando redes ---")
    print(send_cmd(ser, "nets = wlan.scan()", delay=3.0))
    print(send_cmd(ser, "[print(n[0].decode(), 'Ch:', n[2], 'RSSI:', n[3]) for n in nets]"))
    
    print("\n--- 3. Tentando conectar em 'ACERJS 9417' ---")
    print(send_cmd(ser, "wlan.connect('ACERJS 9417', '149oF8@3')", delay=1.0))
    
    print("\n--- 4. Monitorando status da conexão ---")
    for i in range(12):
        res = send_cmd(ser, "print('t=', " + str(i) + ", 'Status:', wlan.status(), 'IsConnected:', wlan.isconnected())", delay=1.0)
        print(res.strip())
        if "IsConnected: True" in res or "STAT_GOT_IP" in res:
            break
            
    print("\n--- 5. Dados da Interface IP ---")
    print(send_cmd(ser, "print('IFCONFIG:', wlan.ifconfig())"))
    
    ser.close()

if __name__ == '__main__':
    run()
