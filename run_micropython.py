import serial
import time

def run():
    ser = serial.Serial('COM14', 115200, timeout=3)
    
    # Send Ctrl+C then Ctrl+D (MicroPython soft reboot)
    ser.write(b'\x03\x04')
    time.sleep(2)
    print("--- REPL BOOT OUTPUT ---")
    boot_log = ser.read_all().decode('utf-8', errors='ignore')
    print(boot_log)
    
    # Enter raw REPL or normal REPL to execute python code
    commands = [
        "import network, time\r\n",
        "wlan = network.WLAN(network.STA_IF)\r\n",
        "wlan.active(True)\r\n",
        "print('=== ESCANEANDO REDES ===')\r\n",
        "for net in wlan.scan(): print(net[0].decode(), net[2], net[3])\r\n",
        "print('=== CONECTANDO EM ACERJS 9417 ===')\r\n",
        "wlan.connect('ACERJS 9417', '149oF8@3')\r\n",
        "for i in range(15):\r\n",
        "    time.sleep(1)\r\n",
        "    print('Status:', wlan.status(), 'IsConnected:', wlan.isconnected())\r\n",
        "    if wlan.isconnected(): break\r\n",
        "print('IP FINAL:', wlan.ifconfig())\r\n"
    ]
    
    for cmd in commands:
        ser.write(cmd.encode('utf-8'))
        time.sleep(0.4)
        
    time.sleep(10)
    res = ser.read_all().decode('utf-8', errors='ignore')
    print("--- RESULTADO COMPLETO DO REPL ---")
    print(res)
    ser.close()

if __name__ == '__main__':
    run()
