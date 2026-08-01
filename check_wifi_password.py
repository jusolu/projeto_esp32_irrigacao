import subprocess

profiles = ['AP104-2.4G', 'AP104-5G']

for profile in profiles:
    print(f"\n{'='*55}")
    print(f"  PERFIL: {profile}")
    print(f"{'='*55}")
    try:
        cmd = f'netsh wlan show profile name="{profile}" key=clear'
        out = subprocess.run(cmd, shell=True, capture_output=True, text=True, errors='ignore')
        
        if out.returncode != 0:
            print("  Erro ao ler perfil:", out.stderr.strip())
            continue
            
        for line in out.stdout.splitlines():
            s = line.strip()
            if s:
                print(" ", s)
    except Exception as e:
        print("  Erro:", e)
