import serial
import time

def live_log():
    print("==========================================================")
    print("  MONITORANDO SERIAL COM14 AO VIVO (60 SEGUNDOS)")
    print("  Por favor, clique no botão REGAR AGORA no Dashboard!")
    print("==========================================================")
    
    try:
        ser = serial.Serial('COM14', 115200, timeout=0.5)
        ser.dtr = False
        ser.rts = False
    except Exception as e:
        print("Erro ao abrir serial:", e)
        return

    start = time.time()
    while time.time() - start < 60:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"[{time.strftime('%H:%M:%S')}] {line.encode('ascii', 'ignore').decode()}")
        time.sleep(0.05)

    ser.close()

if __name__ == '__main__':
    live_log()
