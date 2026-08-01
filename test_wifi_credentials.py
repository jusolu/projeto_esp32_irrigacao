import subprocess
import re

def list_nearby_wifis():
    print("==========================================================")
    print("  LISTA DE TODAS AS REDES WI-FI VISÍVEIS (2.4GHz e 5GHz)")
    print("==========================================================")
    
    try:
        nets_out = subprocess.check_output("netsh wlan show networks mode=bssid", shell=True, text=True, errors='ignore')
        
        current_ssid = ""
        for line in nets_out.splitlines():
            line_str = line.strip()
            if line_str.startswith("SSID"):
                current_ssid = line_str.split(":", 1)[-1].strip()
                print(f"\n[SSID]: '{current_ssid}'")
            elif "Tipo de rede" in line_str or "Network type" in line_str:
                print(f"  Tipo: {line_str}")
            elif "Autentica" in line_str or "Authentication" in line_str:
                print(f"  Segurança: {line_str}")
            elif "Sinal" in line_str or "Signal" in line_str or "BSSID" in line_str:
                print(f"  {line_str}")

    except Exception as e:
        print("Erro ao listar redes Wi-Fi:", e)

if __name__ == '__main__':
    list_nearby_wifis()
