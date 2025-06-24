#ifndef HUB_FIRMWARE_H
#define HUB_FIRMWARE_H

/*--ASSEGNAZIONE DEI PIN PER VERSIONE 1.0 DELL'HUB--*/

#include <Arduino.h>
#include <SimpleCLI.h>          // Include per includere interfaccia a linea di comando
#include <ArduinoJson.h>        // Include per la creazione di json
#include <SPI.h>

#define SERIAL_SPEED    115200 // baud rate seriale

#define RST_GPIO            5   // Pin di reset verso W5500
#define INT_GPIO            6   // Pin di interrupt del W5500
#define MISO_GPIO           8   // Pin MISO SPI
#define MOSI_GPIO           7   // Pin MOSI SPI
#define SCK_GPIO            9   // Pin SCK SPI
#define CS_GPIO             10  // Pin CS SPI

#define TX_485              2   // Pin di tx per 485
#define RW_485              3   // Pin lettura/scrittura per 485 (LOW->READ) (HIGH->WRITE)
#define RX_485              4   // Pin di rx per 485

#define LED_DEBUG_RED       37  // RED      // Pin di controllo del led di debug sulla scheda 
#define LED_DEBUG_GREEN     36  // GREEN    // Pin di controllo del led di debug sulla scheda
#define BUZZER_DEBUG        35  // Pin di controllo del buzzer di debug sulla scheda

#define RST_SWITCH          38  // Pin per il reset dello switch di rete (Attivo basso)
#define PWM_LIGHT           39  // Pin PWM per il controllo dei LED del ROV (Luci di posizione)

#define MULTISENSE_24V      12  // Pin lettura analogica sensing (tensione, corrente, temperatura in funzione del mux)
#define ENABLE_0_24V        13  // Pin attivazione canale 0 24V
#define ENABLE_1_24V        17  // Pin attivazione canale 1 24V
#define ENABLE_SENS_24V     14  // Pin attivazione sensing VND70 24V [Active HIGH]
#define SEL_0_24V           15  // Sense MUX 0 24V [Active HIGH]
#define SEL_1_24V           16  // Sense MUX 1 24V [Active HIGH]
#define FLASH_1_CHANNEL     0   // Canale utilizzato per il flash 1
#define FLASH_2_CHANNEL     1   // Canale utilizzato per il flash 2

#define MULTISENSE_12V      18  // Pin lettura analogica sensing (tensione, corrente, temperatura in funzione del mux)
#define ENABLE_0_12V        21  // Pin attivazione canale 0 12V
#define ENABLE_1_12V        34  // Pin attivazione canale 1 12V
#define ENABLE_SENS_12V     48  // Pin attivazione sensing VND70 12V [Active HIGH]
#define SEL_0_12V           47  // Sense MUX 0 12V [Active HIGH]
#define SEL_1_12V           33  // Sense MUX 1 12V [Active HIGH]
#define IPCAM_CHANNEL       0   // Canale utilizzato per l'IPCam
#define BD3D_CHANNEL        1   // Canale utilizzato per la BlueDepth

#define _ASYNC_WEBSERVER_LOGLEVEL_       2

// Array per la dichiarazione degli output (tutti questi controlli sono attivi alti)
const uint8_t OUTPUT_ARRAY[]={RW_485, LED_DEBUG_RED, LED_DEBUG_GREEN, BUZZER_DEBUG, RST_SWITCH, PWM_LIGHT};

// Array per la dichiarazione degli input
const uint8_t INPUT_ARRAY[]={};

/* ----- MAIN PAGE WEB SERVER ------ */
// HTML welcome page with JS
const char MAIN_page[8000] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>HUB MPE</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>

    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      display: flex;
      color: #fff;
      text-align: center;
      justify-content: center;
      align-items: center;
      margin: 20px;
      padding: 20px;
    }

    .container {
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    .card {
      background-color: rgba(0, 0, 0, 0.7);
      flex-direction: column;
      padding: 20px;
      margin: 15px 15px;
      border-radius: 12px;
      box-shadow: 0 0 15px rgba(0, 0, 0, 0.3);
      color: white;
      width: 300px;
      text-align: center;
      justify-content: center;
      align-items: center;
    }

    h2 {
      color: #333;
      margin-bottom: 20px;
    }
  
    .data {
      font-size: 1.5em;
      margin: 10px 0;
    }

    .button-row {
      display: flex;
      flex-direction: column;
      justify-content: center;
      gap: 10px;
    }

    button {
      padding: 10px 20px;
      margin: 5px;
      font-size: 16px;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      background-color: #007bff;
      color: white;
      transition: 0.3s;
    }
    
    button:hover {
      background-color: #0056b3;
      color: white;
    }

    .switch-container {
      font-size: 20px;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 30px;
      padding: 10px;
    }

    .switch {
    position: relative;
    display: inline-block;
    width: 60px;
    height: 34px;
    }

    .switch input { 
    opacity: 0;
    width: 0;
    height: 0;
    }

    .slider {
    position: absolute;
    cursor: pointer;
    top: 0; left: 0;
    right: 0; bottom: 0;
    background-color: #ccc;
    transition: .4s;
    border-radius: 34px;
    }

    .slider:before {
    position: absolute;
    content: "";
    height: 26px; width: 26px;
    left: 4px; bottom: 4px;
    background-color: white;
    transition: .4s;
    border-radius: 50%;
    }

    input:checked + .slider {
    background-color: #4CAF50;
    }

    input:checked + .slider:before {
    transform: translateX(26px);
    }
  </style>

  <script>
    function fetchData() {
        fetch("/sensor")
            .then(response => response.json())
            .then(data => {

            document.getElementById("Current_Flash_1").innerHTML = data.Current_Flash_1.toFixed(2) + " A";
            document.getElementById("Current_Flash_2").innerHTML = data.Current_Flash_2.toFixed(2) + " A";
            document.getElementById("Voltage_24V").innerHTML = data.Voltage_24V.toFixed(2) + " V";
            document.getElementById("Chip_Temp_24V").innerHTML = data.Chip_Temp_24V.toFixed(2) + " C";
            document.getElementById("Current_BD3D").innerHTML = data.Current_BD3D.toFixed(2) + " A";
            document.getElementById("Current_IPCam").innerHTML = data.Current_IPCam.toFixed(2) + " A";
            document.getElementById("Voltage_12V").innerHTML = data.Voltage_12V.toFixed(2) + " V";
            document.getElementById("Chip_Temp_12V").innerHTML = data.Chip_Temp_12V.toFixed(2) + " C";

            refreshSwitch("12V_0", data["12V_0_State"]);
            refreshSwitch("12V_1", data["12V_1_State"]);
            refreshSwitch("24V_0", data["24V_0_State"]);
            refreshSwitch("24V_1", data["24V_1_State"]);
            })
            .catch(err => console.error("Errore nel fetch:", err));
    }

    function refreshSwitch(device, state) {
        const checkbox = document.getElementById(`switch_${device}`);
        const label = document.getElementById(`${device}`);
        if (!checkbox || !label) return;

        checkbox.checked = state;
        label.textContent = `${device} ${state ? 'ON' : 'OFF'}`;
    }

    function toggleState(checkbox, deviceName) {
        const container = checkbox.closest('.switch-container');
        const label = container.querySelector('.switch-label');
        const stateWord = checkbox.checked ? 'ON' : 'OFF';
        label.textContent = `${deviceName.charAt(0).toUpperCase() + deviceName.slice(1)} ${stateWord}`;
        sendState(deviceName, stateWord);
    }

    function sendState(device, state) {
        const url = `/${device}?state=${state}`;
        console.log(`Invio richiesta a: ${url}`);
        fetch(url)
            .then(res => res.text())
            .then(data => console.log("Risposta dal server:", data))
            .catch(err => console.error("Errore:", err));
    }

    function sendReset() {
        fetch("/reset");
    }

    setInterval(fetchData, 2000);
  </script>

</head>
<body>
  <div class="container">
    <h2>HUB MPE</h2>

    <div class="card">
        <div class="data">Current Flash 1: <span id="Current_Flash_1">--</span></div>
        <div class="data">Current Flash 2: <span id="Current_Flash_2">--</span></div>
        <div class="data">Voltage 24V: <span id="Voltage_24V">--</span></div>
        <div class="data">Chip Temp 24V: <span id="Chip_Temp_24V">--</span></div>
        <div class="data">Current BD3D: <span id="Current_BD3D">--</span></div>
        <div class="data">Current IPCam: <span id="Current_IPCam">--</span></div>
        <div class="data">Voltage 12V: <span id="Voltage_12V">--</span></div>
        <div class="data">Chip Temp 12V: <span id="Chip_Temp_12V">--</span></div>
    </div>

    <div class="card">
        <div class="switch-container">
            <span class="switch-label" id="12V_1">BlueDepth OFF</span>
            <label class="switch">
            <input type="checkbox" id="switch_12V_1" onchange="toggleState(this, 'BD3D')">
            <span class="slider"></span>
            </label>
        </div>
        <div class="switch-container">
            <span class="switch-label" id="12V_0">IPCam OFF</span>
            <label class="switch">
            <input type="checkbox" id="switch_12V_0" onchange="toggleState(this, 'IPCam')">
            <span class="slider"></span>
            </label>
        </div>
        <div class="switch-container">
            <span class="switch-label" id="24V_1">Lamp OFF</span>
            <label class="switch">
            <input type="checkbox" id="switch_24V_1" onchange="toggleState(this, 'lamp')">
            <span class="slider"></span>
            </label>
        </div>
    </div>

    <div class="card">
        <div class="button-row">
            <button onclick="sendReset()">RESET</button>
        </div>
    </div>

  </div>
</body>
</html>
)rawliteral";

/*--------------FUNZIONI-------------*/

/*
* FUNZIONE PER INIZIALIZZARE I PIN CONTENUTI IN UN ARRAY
* @param uint8_t array contenete i pin da inizializzare
* @param uint8_t dimensione dell'array [sizeof(array_da_inizializzare)]
* @param byte tipo del pin (INPUT, OUTPUT)
*/
void declaration_function(const uint8_t array[], uint8_t size, byte type);

/*
* FUNZIONE PER IMPOSTARE IL VALORE DEI PIN CONTENUTI IN UN ARRAY
* @param uint8_t array contenete i pin da impostare
* @param uint8_t dimensione dell'array [sizeof(array_da_impostare)]
* @param byte valore del pin (HIGH, LOW)
*/
void set_pin_function(const uint8_t array[], uint8_t size, byte value);

/*
* FUNZIONE PER L'INIZIALIZZAZIONE DI TUTTE LE ISTANZE
*
*/
void initialize();

/* 
* FUNZIONE CHE STAMPA IL TESTO SUL 485 (bisogna esplicitare '\n')
* @param String il testo da stampare
*/
void write485(String text);

/*---------------TELNET-------------*/

bool isConnected();

bool connectToWiFi(const char* ssid, const char* password, int max_tries, int pause);

void errorMsg(String error, bool restart);

// (optional) callback functions for telnet events
void onTelnetConnect(String ip);

void onTelnetDisconnect(String ip);

void onTelnetReconnect(String ip);

void onTelnetConnectionAttempt(String ip);

void onTelnetInput(String str);

void setupTelnet();

/*
* FUNZIONE CON IL LOOP DI TELNET E VERIFICA SE CI SONO VALORI DA LEGGERE
*/
String loopTelnet();

/* 
* FUNZIONE CHE SCRIVE SU TELNET UNA STRINGA (VA A CAPO)
* @param String testo da stampare
*/
void writeTelnet(String text);

#endif