import serial
import time

def diagnose():
    ser = serial.Serial('COM14', 115200, timeout=2)
    ser.dtr = False
    ser.rts = False
    time.sleep(1)
    
    # Envia Ctrl+C 3x para interromper qualquer loop
    for _ in range(3):
        ser.write(b'\x03')
        time.sleep(0.2)
    ser.write(b'\r\n')
    time.sleep(0.5)

    # Testa REPL
    ser.write(b'print("REPL OK")\r\n')
    time.sleep(0.5)
    out1 = ser.read_all().decode('utf-8', errors='ignore')
    print("REPL TEST:", repr(out1))

    # Le o main.py
    ser.write(b"f=open('main.py'); c=f.read(); f.close(); print(c)\r\n")
    time.sleep(2)
    out2 = ser.read_all().decode('utf-8', errors='ignore')
    print("MAIN.PY:")
    print(out2)

    ser.close()

if __name__ == '__main__':
    diagnose()
