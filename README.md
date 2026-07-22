# 🪴 Servidor de Irrigação Automatizada (Vercel + ESP32)

Este repositório contém o código completo para um sistema de irrigação inteligente controlado pela Web, implantado no Vercel (Next.js) e conectado a um microcontrolador ESP32.

---

## 📁 Estrutura do Projeto

```
c:/server_api_esp32/
├── app/
│   ├── api/
│   │   ├── esp32/route.js    # Endpoint REST para o ESP32 (GET/POST)
│   │   ├── water/route.js    # Endpoint para acionar/cancelar rega
│   │   └── status/route.js   # Endpoint para leitura do estado atual
│   ├── globals.css           # Design System & Estilos em Vanilla CSS
│   ├── layout.js             # Layout raiz do Next.js
│   └── page.js               # Dashboard Web Interativo
├── lib/
│   └── state.js              # Gerenciador do estado em memória
├── esp32/
│   └── esp32_irrigation.ino  # Código C++ para Arduino IDE / PlatformIO
├── package.json
└── README.md
```

---

## ⚡ Passo 1: Testando Localmente

1. No terminal da pasta do projeto (`c:\server_api_esp32`):
   ```bash
   npm install
   npm run dev
   ```
2. Abra no navegador: [http://localhost:3000](http://localhost:3000)

---

## ☁️ Passo 2: Fazer Deploy Gratuito no Vercel

### Opção A: Usando a CLI do Vercel (Mais Rápido)
No terminal da pasta do projeto, execute:
```bash
npx vercel
```
Siga as instruções na tela (pressione Enter para as opções padrão). A CLI vai gerar uma URL pública HTTPS (exemplo: `https://server-api-esp32.vercel.app`).

### Opção B: Enviar para o GitHub e conectar ao Vercel
1. Suba esta pasta para o seu repositório no GitHub.
2. Acesse [vercel.com](https://vercel.com) e clique em **Add New Project**.
3. Importe o repositório e clique em **Deploy**.

---

## 🧠 Passo 3: Configurar e gravar o ESP32

1. Abra a [IDE do Arduino](https://www.arduino.cc/en/software).
2. Abra o arquivo [`esp32/esp32_irrigation.ino`](file:///c:/server_api_esp32/esp32/esp32_irrigation.ino).
3. Instale as bibliotecas necessárias via **Tools -> Manage Libraries**:
   - `ArduinoJson` (por Benoit Blanchon)
   - `DHT sensor library` (por Adafruit)
   - `RTClib` (por Adafruit)
4. No topo do arquivo `esp32_irrigation.ino`, edite:
   - `WIFI_SSID`: Nome da sua rede Wi-Fi.
   - `WIFI_PASSWORD`: Senha da sua rede Wi-Fi.
   - `VERCEL_API_URL`: Cole a URL gerada pelo Vercel + `/api/esp32` (ex: `https://seu-projeto.vercel.app/api/esp32`).
5. Conecte o ESP32 na porta USB, selecione a placa **ESP32 Dev Module** e clique em **Upload**!
