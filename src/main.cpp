// ============================================================
// POMYSLY NA ROZWINIĘCIE PROJEKTU:
// Funkcja glosowa umyj sie  (Wycieraczki)
// ============================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPI.h>
#include <mcp_can.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
// ============================================================
// 🛠️ PLACEHOLDERY - ENTER YOUR SNIFFER CODES HERE
// ============================================================

// REV AND SPEED GAUGES
#define ID_RPM_SEND      0x0AA
#define ID_SPD_SEND      0x1A6

// COMFORT MODULE (Sunroof, Windows - Often ID 0x290 or similar in E60)
#define ID_BODY_MODULE   0x000 

// AIR CONDITIONING (IHKA - Often ID 0x242 lub 0x246)
#define ID_CLIMATE       0x000

// LIGHTS (LM - Light Module)
#define ID_LIGHTS_CTRL   0x000

// Media
#define ID_MEDIA_CTRL    0x000

// Hardware configuration
#define SPI_CS_PIN  5   // Chip Select for CAN
#define CAN_INT_PIN 4   // CAN Interrupt Pin
#define CAN_SPEED CAN_100KBPS  // E60 K-CAN (For PT-CAN change to 500KBPS)

MCP_CAN CAN0(SPI_CS_PIN);
WebServer server(80);

// ============================================================
// 🧠 DATA STRUCTURES AND GLOBAL VARIABLES
// ============================================================

struct Settings {
  bool needleSweep;
  bool cornering;
  bool essStop;
  bool autoMute;
  bool rearWelcome;
  bool rainClose;
  bool mirrorDip;
} settings;

// --- LOGICAL VARIABLES ---
bool policeModeActive = false;       // Is the fog light on?
unsigned long lastPoliceToggle = 0;  // Blink timer
bool policeState = false;            // Blinking state (left/right)

bool reverseGearActive = false;      // Is reverse gear engaged?
bool muteDone = false;               // muted yet?

// Rear Welcome & Rain
unsigned long rearWelcomeTimer = 0;
bool rearWelcomeActive = false;
unsigned long lastRainCmdTime = 0;

// ESS (F1 Brake)
bool essActive = false;         // Is the F1 system active?
unsigned long lastEssStrobe = 0; // Stop blink timer
bool essState = false;          // Brake light status (on/off)

// System variables
int currentTemp = 0;
float batteryVolt = 0.0;
bool snifferActive = false;
String lastCanMsgHtml = ""; 

// ============================================================
// 💾 MEMORY (EEPROM)
// ============================================================

void loadSettings() {
  EEPROM.begin(512);
  if (EEPROM.read(0) == 123) {
    EEPROM.get(1, settings);
  } else {
    settings = {true, true, true, true, true, true, true};
    EEPROM.write(0, 123);
    EEPROM.put(1, settings);
    EEPROM.commit();
  }
}

void saveSettings() {
  EEPROM.put(1, settings);
  EEPROM.commit();
}

// ============================================================
// 🌐 WEB INTERFACE  
// ============================================================

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="pl">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <title>BMW E60 AI CONTROL</title>
  <style>
    body { background-color: #151515; margin: 0; padding: 0; font-family: "Courier New", monospace; color: #ff8800; user-select: none; -webkit-tap-highlight-color: transparent; }
    .lcd-display { background-color: #100800; color: #ff9900; border: 4px solid #333; border-radius: 4px; margin: 10px; padding: 15px; box-shadow: inset 0 0 10px #000; text-transform: uppercase; position: relative; }
    .lcd-text { font-size: 20px; font-weight: bold; letter-spacing: 2px; text-shadow: 0 0 5px rgba(255, 136, 0, 0.7); min-height: 24px;}
    .status-line { font-size: 12px; border-bottom: 2px dashed #ff8800; padding-bottom: 5px; margin-bottom: 10px; display: flex; justify-content: space-between; }
    .radio-buttons { display: flex; overflow-x: auto; background: #222; padding: 10px 0; border-top: 2px solid #000; border-bottom: 2px solid #000; }
    .radio-btn { background: #1a1a1a; color: #ccc; border: 1px solid #444; padding: 8px 15px; margin: 0 4px; font-size: 14px; font-weight: bold; cursor: pointer; box-shadow: 2px 2px 0 #000; white-space: nowrap; }
    .radio-btn.active { color: #ff8800; border-color: #ff8800; background: #111; text-shadow: 0 0 3px #ff8800; }
    .panel { display: none; padding: 10px; animation: fadeIn 0.3s; }
    .panel.active { display: block; }
    .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top:10px; }
    .ctrl-btn { background: linear-gradient(to bottom, #2a2a2a, #1a1a1a); color: #ff8800; border: 1px solid #000; border-top: 1px solid #444; padding: 15px 5px; font-size: 16px; font-weight: bold; cursor: pointer; text-transform: uppercase; border-radius: 3px; text-align: center;}
    .ctrl-btn:active { background: #111; color: #ffaa33; transform: translateY(2px); box-shadow: inset 0 0 10px #000; }
    .btn-red { color: #ff3300; text-shadow: 0 0 2px red; }
    .wide { grid-column: span 2; }
    .rec-btn { width: 80px; height: 80px; margin: 20px auto; border: 4px solid #444; border-radius: 50%; background: #000; color: #444; font-size: 40px; display: flex; align-items: center; justify-content: center; cursor: pointer; }
    .rec-btn.active { border-color: #ff3300; color: #ff3300; animation: pulse 1s infinite; background: #220000; }
    .switch-row { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #333; padding: 12px 0; font-size: 14px; }
    .switch-input { transform: scale(1.5); accent-color: #ff8800; }
    .ai-chat-box { max-height: 100px; overflow-y: auto; font-size: 12px; margin-top: 10px; border-top: 1px dashed #555; padding-top: 5px; color: #aaa; text-transform: none; }
    .ai-msg { margin-bottom: 4px; border-bottom: 1px solid #222; padding-bottom: 2px;}
    @keyframes fadeIn { from{opacity:0;} to{opacity:1;} }
    @keyframes pulse { 0%{transform:scale(1);} 50%{transform:scale(1.05);} 100%{transform:scale(1);} }
  </style>
</head>
<body>

  <div class="lcd-display">
    <div class="status-line">
      <span>Engine: <span id="eng-temp">--</span>°C</span>
      <span id="time">BMW E60 AI</span>
    </div>
    <div id="main-text" class="lcd-text">SYSTEM READY</div>
    <div id="ai-status" style="font-size:10px; color:#ff00ff; text-align:center;">ONLINE</div>
    <div style="font-size:10px; color:#555; text-align:right; margin-top:5px;">IP: <span id="ip-addr">...</span> | BAT: <span id="volt">--.-</span>V</div>
    <div id="chat-history" class="ai-chat-box"></div>
  </div>

  <div class="radio-buttons">
    <div class="radio-btn active" onclick="tab('windows', this)">WINDOWS</div>
    <div class="radio-btn" onclick="tab('climate', this)">AC</div>
    <div class="radio-btn" onclick="tab('lights', this)">LIGHTS</div>
    <div class="radio-btn" onclick="tab('media', this)">MEDIA</div>
    <div class="radio-btn" onclick="tab('voice', this)">VOICE</div>
    <div class="radio-btn" onclick="tab('service', this)">SERVICE</div>
    <div class="radio-btn" onclick="tab('settings', this)" style="color:#0f0;">SETUP</div>
  </div>

  <div id="windows" class="panel active">
    <div style="margin-top:5px; color:#888; font-size:12px;">LEFT FRONT</div>
    <div class="grid-2" style="margin-top:2px;">
      <div class="ctrl-btn" onclick="cmd('win_fl_down', 'LEFT FRONT DOWN')">⬇️ OPEN</div>
      <div class="ctrl-btn" onclick="cmd('win_fl_up', 'LEFT FRONT UP')">⬆️ CLOSE</div>
    </div>

    <div style="margin-top:10px; color:#888; font-size:12px;">RIGHT FRONT</div>
    <div class="grid-2" style="margin-top:2px;">
      <div class="ctrl-btn" onclick="cmd('win_fr_down', 'RIGHT FRONT DOWN')">⬇️ OPEN</div>
      <div class="ctrl-btn" onclick="cmd('win_fr_up', 'RIGHT FRONT UP')">⬆️ CLOSE</div>
    </div>

    <div style="margin-top:10px; color:#888; font-size:12px;">LEFT REAR</div>
    <div class="grid-2" style="margin-top:2px;">
      <div class="ctrl-btn" onclick="cmd('win_rl_down', 'LEFT REAR DOWN')">⬇️ OPEN</div>
      <div class="ctrl-btn" onclick="cmd('win_rl_up', 'LEFT REAR UP')">⬆️ CLOSE</div>
    </div>
    <div style="margin-top:10px; color:#888; font-size:12px;">RIGHT REAR</div>
    <div class="grid-2" style="margin-top:2px;">
      <div class="ctrl-btn" onclick="cmd('win_rr_down', 'RIGHT REAR DOWN')">⬇️ OPEN</div>
      <div class="ctrl-btn" onclick="cmd('win_rr_up', 'RIGHT REAR UP')">⬆️ CLOSE</div>
    </div>

    <div class="grid-2" style="margin-top:20px; border-top:1px dashed #333; padding-top:10px;">
      <div class="ctrl-btn wide" onclick="cmd('win_all_down', 'ALL DOWN')">⬇️⬇️ ALL DOWN</div>
      <div class="ctrl-btn wide" onclick="cmd('win_all_up', 'ALL UP')">⬆️⬆️ ALL UP</div>
    </div>
  </div>

  <div id="climate" class="panel">
    <div class="grid-2">
      <div class="ctrl-btn wide" onclick="cmd('ac_max', 'MAX COOL')">❄️ MAX COOL</div>
      <div class="ctrl-btn" onclick="cmd('temp_up', 'TEMP +')">🌡️ TEMP +</div>
      <div class="ctrl-btn" onclick="cmd('temp_down', 'TEMP -')">🌡️ TEMP -</div>
      <div class="ctrl-btn" onclick="cmd('fan_up', 'FAN +')">💨 FAN +</div>
      <div class="ctrl-btn" onclick="cmd('fan_down', 'FAN -')">💨 FAN -</div>
      <div class="ctrl-btn wide btn-red" onclick="cmd('ac_off', 'AC OFF')">❌ DISABLE AC</div>
    </div>
  </div>
<div id="lights" class="panel">
    <div class="grid-2">
      <div class="ctrl-btn wide btn-red" onclick="cmd('police_mode_toggle', '🚨 POLICE MODE 🚨')">🚨 POLICE STROBE</div>
      <div class="ctrl-btn" onclick="cmd('light_welcome', 'WELCOME LIGHTS')">✨ WELCOME LIGHTS</div>
      <div class="ctrl-btn" onclick="cmd('light_fog', 'HALOGENS')">🌫️ HALOGENS</div>
      <div class="ctrl-btn" onclick="cmd('light_hazards', 'HAZARDS')">⚠️ HAZARDS</div>
      <div class="ctrl-btn wide" onclick="cmd('light_interior', 'INTERIOR')">💡 INTERIOR LIGHTING</div>
    </div>
  </div>

  <div id="media" class="panel">
    <div class="grid-2">
      <div class="ctrl-btn" onclick="cmd('vol_up', 'VOLUME UP')">VOL +</div>
      <div class="ctrl-btn" onclick="cmd('vol_down', 'VOLUME DOWN')">VOL -</div>
      <div class="ctrl-btn wide" onclick="cmd('test_clocks', 'NEEDLE SWEEP')">🚀 NEEDLE SWEEP</div>
    </div>
  </div>

  <div id="voice" class="panel">
    <div style="text-align:center; color:#888;">PRESS AND SAY</div>
    <div id="mic-btn" class="rec-btn" onclick="toggleMic()">🎙️</div>
  </div>

  <div id="service" class="panel">
    <div class="ctrl-btn wide" onclick="window.location.href='/sniffer'">🕵️ OPEN LIVE SNIFFER</div>
    <div id="console" style="margin-top:10px; color:#555; font-size:10px;">Logs will appear here...</div>
  </div>

  <div id="settings" class="panel">
    <h3 style="border-bottom: 2px solid #ff8800;">AUTOMATION</h3>
    <div class="switch-row">1. Needle Sweep <input type="checkbox" class="switch-input" id="s1" onchange="tgl(1)"></div>
    <div class="switch-row">2. Additional lighting <input type="checkbox" class="switch-input" id="s2" onchange="tgl(2)"></div>
    <div class="switch-row">3. ECC <input type="checkbox" class="switch-input" id="s3" onchange="tgl(3)"></div>
    <div class="switch-row">4. Auto Mute (R) <input type="checkbox" class="switch-input" id="s4" onchange="tgl(4)"></div>
    <div class="switch-row">5. Rear Welcome <input type="checkbox" class="switch-input" id="s5" onchange="tgl(5)"></div>
    <div class="switch-row">6. Rain-Closer <input type="checkbox" class="switch-input" id="s6" onchange="tgl(6)"></div>
    <div class="switch-row">7. Mirror Down <input type="checkbox" class="switch-input" id="s7" onchange="tgl(7)"></div>
    <div class="switch-row" style="justify-content:center; border:none; margin-top: 20px;">
    <button onclick="loginSpotify()" style="background:#1DB954; color:#fff; border:none; padding:15px 30px; border-radius:30px; font-weight:bold; font-size:16px; cursor:pointer; width:100%;">
      LOG IN TO SPOTIFY 🎵
    </button>
  </div>
  </div>

<script>
  // --- API KEYS etc. ---
  const API_KEY = ""; 
  const MODEL_NAME = "gemini-2.5-flash-lite";
  const ELEVEN_KEY = ""; 
  const VOICE_ID = ""; 
  const SPOTIFY_CLIENT_ID = "";

  const REDIRECT_URI = window.location.origin + "/"; 
  const SCOPES = "user-modify-playback-state user-read-playback-state user-read-currently-playing";

  const SYSTEM_PROMPT = `
    You are the virtual assistant/lover of the BMW E60 530i driver. Be brief and to the point.
    Your job is to help the driver control the car's functions using voice commands.
    Typically, respond in 1-2 sentences, but occasionally, if you feel like it, elaborate.
    Speak in short sentences.

    
   IF you need to do something, add the command in parentheses at the very end:
    - Open all windows -> [WIN_DOWN]
    - Close all windows -> [WIN_UP]
    - Open driver's window -> [WIN_FL_DOWN]
    - Close driver's window -> [WIN_FL_UP]
    - Open passenger's window -> [WIN_FR_DOWN]
    - Close passenger's window -> [WIN_FR_UP]
    - Open left rear window -> [WIN_RL_DOWN]
    - Close left rear window -> [WIN_RL_UP]
    - Open right rear window -> [WIN_RR_DOWN]
    - Close right rear window -> [WIN_RR_UP]
    - Open sunroof -> [roof_open]
    - Close sunroof -> [roof_close]
    - Tilt sunroof -> [roof_tilt]
    - Turn off AC -> [AC_OFF]
    - Increase temperature -> [TEMP_UP]
    - Decrease temperature -> [TEMP_DOWN]
    - Increase fan speed -> [FAN_UP]
    - Decrease fan speed -> [FAN_DOWN]
    - Max climate -> [AC_MAX]
    - Fog lights -> [LIGHT_FOG]
    - Hazard lights -> [LIGHT_HAZARDS]
    - Interior lights -> [LIGHT_INTERIOR]
    - Welcome lights -> [LIGHT_WELCOME]
    - Police/Hide -> [POLICE]
    - Test clocks -> [TEST_CLOCKS]
    - Louder -> [VOL_UP]
    - Quiet -> [VOL_DOWN]

    If you need to turn something on once, use the command only once. For example, [WIN_DOWN], [AC_MAX].
    If you need to change something multiple times, e.g., temperature or airflow, repeat the command as many times as necessary and use the format e.g., [TEMP_UP:X] where X is the number of REPETITIONS. For example, "INCREASE temperature by 3 degrees" -> [TEMP_UP:3]. Do the same with any other command that requires multiple use.
  `;
  // ==========================================================
  //  SYSTEM STARTUP AND SPOTIFY LOGIN (FROM MEMORY)
  // ==========================================================

window.onload = function() {
    update(); 


    const storedToken = localStorage.getItem('spotify_token');
    const storedExpiry = localStorage.getItem('spotify_expiry');
    const now = new Date().getTime();

    if (storedToken && storedExpiry && now < storedExpiry) {
        spotifyToken = storedToken;
        document.getElementById('ai-status').innerText = "SPOTIFY: READY";
        document.getElementById('ai-status').style.color = "#0f0"; 
    } else {
        document.getElementById('ai-status').innerText = "SPOTIFY: OFFLINE";
    }

    const hash = window.location.hash.substring(1).split('&').reduce(function (initial, item) {
      if (item) { var parts = item.split('='); initial[parts[0]] = decodeURIComponent(parts[1]); }
      return initial;
    }, {});
    
    if (hash.access_token) {
      spotifyToken = hash.access_token;
      const expiryTime = new Date().getTime() + (parseInt(hash.expires_in || 3600) - 60) * 1000;

      localStorage.setItem('spotify_token', spotifyToken);
      localStorage.setItem('spotify_expiry', expiryTime);

      window.location.hash = '';
      document.getElementById('ai-status').innerText = "SPOTIFY: LOGGED IN";
      document.getElementById('ai-status').style.color = "#0f0";
      
      
      tab('dashboard', document.querySelector('.radio-group button:first-child'));
    }
  }

  function loginSpotify() {
    let url = `https://accounts.spotify.com/authorize?client_id=${SPOTIFY_CLIENT_ID}&response_type=token&redirect_uri=${encodeURIComponent(REDIRECT_URI)}&scope=${encodeURIComponent(SCOPES)}`;
    window.location.href = url;
  }

  // ==========================================================
  // PLAY FUNCTION (SEARCH AND PLAY)
  // ==========================================================
  async function playSpotify(query) {
    if(!spotifyToken) {
       speakEleven("You must first log in to Spotify in the settings.");
       return;
    }
    
    try {
        let searchRes = await fetch(`https://api.spotify.com/v1/search?q=${encodeURIComponent(query)}&type=track&limit=1`, {
            headers: { 'Authorization': 'Bearer ' + spotifyToken }
        });
        let searchData = await searchRes.json();
        
        if(!searchData.tracks || searchData.tracks.items.length === 0) {
            speakEleven("I didn't find anything like that.");
            return;
        }

        let trackUri = searchData.tracks.items[0].uri;
      
        let playRes = await fetch(`https://api.spotify.com/v1/me/player/play`, {
            method: 'PUT',
            headers: { 'Authorization': 'Bearer ' + spotifyToken },
            body: JSON.stringify({ "uris": [trackUri] })
        });

        if(playRes.status === 404) {
             speakEleven("First, turn on Spotify on your phone, because I can't see the device.");
        }

    } catch(e) { console.log("Spotify Err:", e); }
  }

  window.switchesLoaded = false;

  function update() {
    fetch('/data').then(r => r.json()).then(d => {
       document.getElementById('eng-temp').innerText = d.temp || "--";
       document.getElementById('volt').innerText = d.volt || "--";
       document.getElementById('ip-addr').innerText = d.ip || "--";
       
       if(!window.switchesLoaded && d.s1 !== undefined) {
         for(let i=1; i<=7; i++) {
            let el = document.getElementById('s'+i);
            if(el) el.checked = d['s'+i];
         }
         window.switchesLoaded = true;
       }
    }).catch(e => {}); 
  }
  setInterval(update, 2000);
  window.onload = update;

  function tab(id, btn) {
    document.querySelectorAll('.panel').forEach(p => p.classList.remove('active'));
    document.getElementById(id).classList.add('active');
    document.querySelectorAll('.radio-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
  }

  function cmd(act, label) {
    if(label) document.getElementById('main-text').innerText = label;
    fetch('/cmd?act=' + act);
  }
  
  async function repeatCmd(act, count) {
    console.log("Repeat command: " + act + " times: " + count);
    for(let i=0; i<count; i++) {
        fetch('/cmd?act=' + act);
        await new Promise(r => setTimeout(r, 200)); // 
    }
  } 

  function tgl(id) { fetch('/toggle?id=' + id); }
   // ==========================================================
  // 🧠 AI (GEMINI + PROXY)
  // ==========================================================

  async function askGemini(userText) {
    const statusEl = document.getElementById('main-text');
    statusEl.innerText = "Thinking...";

    const googleUrl = `https://generativelanguage.googleapis.com/v1beta/models/${MODEL_NAME}:generateContent?key=${API_KEY}`;
    const proxyUrl = "https://corsproxy.io/?" + encodeURIComponent(googleUrl);

    try {
      let response = await fetch(proxyUrl, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ contents: [{ parts: [{ text: SYSTEM_PROMPT + "\nUser: " + userText }] }] })
      });
      if (!response.ok) throw new Error("Google Error: " + response.status);

      let json = await response.json();
      if (json.candidates) {
        let aiMsg = json.candidates[0].content.parts[0].text;
        let cleanText = aiMsg.replace(/\[.*?\]/g, "").trim();
        
        statusEl.innerText = cleanText;

        // AI Commands
        if(aiMsg.includes("[WIN_DOWN]")) cmd('win_all_down');
        if(aiMsg.includes("[WIN_UP]"))   cmd('win_all_up');
        if(aiMsg.includes("[WIN_FL_DOWN]")) cmd('win_fl_down');
        if(aiMsg.includes("[WIN_FL_UP]"))   cmd('win_fl_up');
        if(aiMsg.includes("[WIN_FR_DOWN]")) cmd('win_fr_down');
        if(aiMsg.includes("[WIN_FR_UP]"))   cmd('win_fr_up');
        if(aiMsg.includes("[WIN_RL_DOWN]")) cmd('win_rl_down');
        if(aiMsg.includes("[WIN_RL_UP]"))   cmd('win_rl_up');
        if(aiMsg.includes("[WIN_RR_DOWN]")) cmd('win_rr_down');
        if(aiMsg.includes("[WIN_RR_UP]"))   cmd('win_rr_up');
        if(aiMsg.includes("[roof_open]"))  cmd('roof_open');
        if(aiMsg.includes("[roof_close]")) cmd('roof_close');
        if(aiMsg.includes("[roof_tilt]"))  cmd('roof_tilt');
        if(aiMsg.includes("[AC_OFF]"))    cmd('ac_off');
        if(aiMsg.includes("[TEMP_UP]"))   cmd('temp_up');
        if(aiMsg.includes("[TEMP_DOWN]")) cmd('temp_down');
        if(aiMsg.includes("[FAN_UP]"))    cmd('fan_up');
        if(aiMsg.includes("[FAN_DOWN]"))  cmd('fan_down');
        if(aiMsg.includes("[AC_MAX]"))   cmd('ac_max');
        if(aiMsg.includes("[LIGHT_FOG]"))     cmd('light_fog');
        if(aiMsg.includes("[LIGHT_HAZARDS]")) cmd('light_hazards');
        if(aiMsg.includes("[LIGHT_INTERIOR]")) cmd('light_interior');
        if(aiMsg.includes("[LIGHT_WELCOME]"))  cmd('light_welcome');
        if(aiMsg.includes("[POLICE]"))   cmd('police_mode_toggle');
        if(aiMsg.includes("[TEST_CLOCKS]")) cmd('test_clocks');
        if(aiMsg.includes("[VOL_UP]"))    cmd('vol_up');
        if(aiMsg.includes("[VOL_DOWN]"))  cmd('vol_down');

        // Ai commands with counts
        let temppUp = aiMsg.match(/\[TEMP_UP:(\d+)\]/);
        if(temppUp) await repeatCmd('temp_up', parseInt(temppUp[1]));

        let temppDown = aiMsg.match(/\[TEMP_DOWN:(\d+)\]/);
        if(temppDown) await repeatCmd('temp_down', parseInt(temppDown[1]));

        let fanUp = aiMsg.match(/\[FAN_UP:(\d+)\]/);
        if(fanUp) await repeatCmd('fan_up', parseInt(fanUp[1]));

        let fanDown = aiMsg.match(/\[FAN_DOWN:(\d+)\]/);
        if(fanDown) await repeatCmd('fan_down', parseInt(fanDown[1]));

        let volUp = aiMsg.match(/\[VOL_UP:(\d+)\]/);  
        if(volUp) await repeatCmd('vol_up', parseInt(volUp[1]));

        let volDown = aiMsg.match(/\[VOL_DOWN:(\d+)\]/);
        if(volDown) await repeatCmd('vol_down', parseInt(volDown[1]));

        // music from Spotify
      //  if(aiMsg = aiMsg.match(/\[DJ:(.+?)\]/)[1].trim();
        ///    if(typeof playSpotify!== 'undefined') {
           //     playSpotify(song);
           // else if(typeof playSpotifySipler !== 'undefined') {
             //   playSpotifySipler(song);
          //  }

        if(cleanText.length > 0) speakEleven(cleanText);
      }
    } catch (e) {
      statusEl.innerText = "AI ERROR";
      console.log(e);
    }
  }
 // ==========================================================
  // 🔊 ELEVEN LABS 
  // ==========================================================
  async function speakEleven(text) {
    const statusEl = document.getElementById('main-text');
    statusEl.innerText += " (saying...)";

    const elUrl = `https://api.elevenlabs.io/v1/text-to-speech/${VOICE_ID}`;
    const proxyUrl = "https://corsproxy.io/?" + encodeURIComponent(elUrl); 

    try {
      let response = await fetch(proxyUrl, {
        method: "POST",
        headers: { 
            "Accept": "audio/mpeg", 
            "Content-Type": "application/json", 
            "xi-api-key": ELEVEN_KEY 
        },
        body: JSON.stringify({
          "text": text,
          "model_id": "eleven_turbo_v2_5",
          "voice_settings": { "stability": 0.35, "similarity_boost": 0.75, "style": "0.5", "use_speaker_boost": true }
        })
      });

      if (!response.ok) throw new Error("Eleven err");

      let blob = await response.blob();
      let audioUrl = URL.createObjectURL(blob);
      let audio = new Audio(audioUrl); 
      
      statusEl.innerText = text;
      audio.play();

    } catch (e) {
      console.log(e);
      // Fallback
      let u = new SpeechSynthesisUtterance(text);
      u.lang = 'en-EN';
      window.speechSynthesis.speak(u);
    }
  }

 // ==========================================================
 // 🎤 MICROPHONE
 // ==========================================================
  var recognition;
  function toggleMic() {
    if (!('webkitSpeechRecognition' in window)) { alert("Use Chrome!"); return; }
    if (!recognition) {
      recognition = new webkitSpeechRecognition();
      recognition.lang = 'en-en';
      recognition.continuous = false; 
      recognition.interimResults = false;
      recognition.onstart = function() { document.getElementById('mic-btn').classList.add('active'); };
      recognition.onend = function() { document.getElementById('mic-btn').classList.remove('active'); };
      recognition.onresult = function(e) { 
         let txt = e.results[0][0].transcript;
         document.getElementById('main-text').innerText = "Ty: " + txt;
         askGemini(txt); 
      };
    }
    recognition.start();
  }
</script>
</body>
</html>
)=====";

const char SNIFFER_page[] PROGMEM = R"=====(
<!DOCTYPE html><html><head><title>CAN SNIFFER</title><meta name="viewport" content="width=device-width, initial-scale=1">
<style>body{background:#000;color:#0f0;font-family:monospace}table{width:100%;border-collapse:collapse}td,th{border:1px solid #333;padding:4px}.btn{padding:10px;background:#333;color:#fff;text-decoration:none;border:1px solid #fff;display:inline-block;margin-bottom:10px}</style>
<script>setInterval(function(){fetch('/sniff_data').then(r=>r.text()).then(t=>{if(t.length>5){var r="<tr>"+t+"</tr>",b=document.getElementById('tb');b.innerHTML=r+b.innerHTML;if(b.rows.length>50)b.deleteRow(50)}})},100);</script>
</head><body><a href="/" class="btn">< BACK</a><h3>LIVE CAN</h3><table><thead><tr><th>TIME</th><th>ID</th><th>LEN</th><th>DATA</th></tr></thead><tbody id="tb"></tbody></table></body></html>
)=====";

// ============================================================
// 🚗 CAN FUNCTIONS
// ============================================================

void sendCan(unsigned long id, byte d0, byte d1, byte d2, byte d3, byte d4, byte d5, byte d6, byte d7) {
    byte data[8] = {d0, d1, d2, d3, d4, d5, d6, d7};
    CAN0.sendMsgBuf(id, 0, 8, data);
}

// ============================================================
// 🚓 FUNCTION LOGIC (NEEDLE SWEEP, POLICE, CORNERING, AUTO MUTE)
// ============================================================

void handlePoliceMode() {
    if (!policeModeActive) return; // If disabled, do nothing

    // Blink every 150ms 
    if (millis() - lastPoliceToggle > 150) {
        lastPoliceToggle = millis();
        policeState = !policeState;

        if (policeState) {
            // PHASE 1: E.g. Left Long + Right Halogen
            Serial.println("STROBO: LEFT");
        } else {
            // PHASE 2: E.g. Right Long + Left Halogen
            Serial.println("STROBO: RIGHT");
        }
    }
}

// ============================================================
// 🧠 SYSTEM BRAIN,  FUNCTIONS
// ============================================================

void handleAutomaticFeatures(long unsigned int id, unsigned char len, unsigned char *buf) {

    // ---------------------------------------------------------
    // 1. REVERSE DETECTION (Gearbox)
    // ---------------------------------------------------------
    if (id == 0x1D2) { // <--- LOOK FOR THE BOX ID HERE (0x1D2 is a common shot for manual)
        // We assume that buf[0] == 0x78 is R (you need to confirm this with a sniffer!)
        bool isReverseNow = (buf[0] == 0x78); 

        if (isReverseNow && !reverseGearActive) {
            reverseGearActive = true;
            
            // A. AUTO MUTE
            if (settings.autoMute && !muteDone) {
                // sendCan(0x1D6, 0xC0, ...); 
                Serial.println(">>> AUTO MUTE: ON (NO sound)");
                muteDone = true;
            }

            // B. MIRROR DIP
            if (settings.mirrorDip) {
                Serial.println(">>> MIRROR: DOWN");
            }
        } 
        else if (!isReverseNow && reverseGearActive) {
            reverseGearActive = false;
            muteDone = false; 
            
            if (settings.autoMute) {
                 Serial.println(">>> AUTO MUTE: OFF (RESTORE sound)");
            }

            if (settings.mirrorDip) {
                Serial.println(">>> MIRROR: UP");
            }
        }
    }

    // ---------------------------------------------------------
    // 2. ESS STOP (F1 STYLE) 🏎️
    // ---------------------------------------------------------
   // We are looking for the DSC pump ID (e.g. 0x1F0). Pressure byte.
    if (settings.essStop && id == 0x000) { // <--- ENTER DSC ID
        int brakePressure = buf[2]; // <--- ENTER PRESSURE BYTE
        
        // Pressure threshold (to be selected experimentally, e.g. 80/255)
        if (brakePressure > 80) {
            essActive = true; // Activate F1 mode
        } else {
            essActive = false; // You brake lightly or not at all - turn off F1
        }
    }

    // ---------------------------------------------------------
    // 3. REAR WELCOME LIGHTS ✨
    // ---------------------------------------------------------
    // Detection of central lock opening (CAS/KGM)
    if (settings.rearWelcome && id == 0x000) { // <--- ENTER PILOT ID
        // E.g. buf[0] changes from 0x00 to 0x01 when unlocked
        bool justUnlocked = (buf[0] == 0x01); 

        if (justUnlocked && !rearWelcomeActive) {
            Serial.println(">>> WELCOME: TURN ON REAR LIGHTS");
            
            // We turn on the same thing as under the "WELCOME" button (Low beam + Positions)
            sendCan(ID_LIGHTS_CTRL, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
            
            rearWelcomeActive = true;
            rearWelcomeTimer = millis(); 
        }
    }

    // ---------------------------------------------------------
    // 4. RAIN CLOSE 
    // ---------------------------------------------------------
    if (settings.rainClose && id == 0x000) { // <--- ENTER RLS ID
        bool isRaining = (buf[0] == 0xFF);
        if (isRaining && (millis() - lastRainCmdTime > 10000)) {
            Serial.println(">>> RAIN: CLOSING WINDOWS");
            sendCan(ID_BODY_MODULE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
            lastRainCmdTime = millis();
        }
    }

    // ---------------------------------------------------------
    // 5. CORNERING (Doświetlanie zakrętów)
    // ---------------------------------------------------------
    if (settings.cornering && id == 0x0C4) { // ID Kąta kierownicy
        // int angle = (buf[0] << 8) | buf[1];  // 
        // if(angle > 500) ...
    }
}

void performNeedleSweep() {
    int max_rpm = 28000; int max_spd = 0x0D50;
    for(int i = 0; i <= 40; i++) {
        float p = i / 40.0;
        int r = (int)(max_rpm * p); int s = (int)(max_spd * p);
        sendCan(ID_RPM_SEND, 0xFE, 0xFE, 0xFE, 0xFE, r&0xFF, (r>>8)&0xFF, 0xFE, 0x98);
        sendCan(ID_SPD_SEND, s&0xFF, (s>>8)&0xFF, 0, 0, 0, 0, 0, 0);
        delay(10);
    }
    delay(100);
    for(int i = 40; i >= 0; i--) {
        float p = i / 40.0;
        int r = (int)(max_rpm * p); int s = (int)(max_spd * p);
        sendCan(ID_RPM_SEND, 0xFE, 0xFE, 0xFE, 0xFE, r&0xFF, (r>>8)&0xFF, 0xFE, 0x98);
        sendCan(ID_SPD_SEND, s&0xFF, (s>>8)&0xFF, 0, 0, 0, 0, 0, 0);
        delay(10);
    }
}

// ============================================================
// 🌐 SERVER DNS (CAPTIVE PORTAL)
// ============================================================

const byte DNS_PORT = 53;

// ============================================================
// ⚙️ SETUP & LOOP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500); 
  
  loadSettings(); 

  // ---- WIFI SETUP ----
  WiFi.mode(WIFI_AP_STA); // Double mode: connect to router + create own hotspot ( For now)
  
  // 1. Connect to your WiFi (if available) - 
  WiFi.begin("", ""); 
  
  // 2. COnnect to your hotspot (for direct connection with phone) - change SSID and password to your own!
  WiFi.softAP(" ", " ");

  Serial.print("Connecting to WiFi");
  long startWifi = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startWifi < 5000) {
      delay(500); Serial.print(".");
  }
  
  Serial.println("\n--- Connect status ---");
  Serial.print("IP (Connected with router): "); Serial.println(WiFi.localIP());
  Serial.print("IP (Connected to HotSpot): "); Serial.println(WiFi.softAPIP());

  if (MDNS.begin("bmw")) {
    Serial.println("MDNS started: http://bmw.local");
  }

  
  SPI.begin(18, 19, 23, 5); // SCK=18, MISO=19, MOSI=23, CS=5
  
  if(CAN_OK == CAN0.begin(MCP_ANY, CAN_100KBPS, MCP_8MHZ)) {
      Serial.println("CAN OK!");
  } else {
      Serial.println("CAN FAIL - Sprawdź kable!");
  }
  CAN0.setMode(MCP_NORMAL);

  server.on("/", [](){ server.send_P(200, "text/html", MAIN_page); });
  server.on("/sniffer", [](){ snifferActive = true; server.send_P(200, "text/html", SNIFFER_page); });
  server.on("/sniff_data", [](){ server.send(200, "text/plain", lastCanMsgHtml); lastCanMsgHtml=""; });

  // --- BUTTON OPERATION ---
  server.on("/cmd", [](){
      String act = server.arg("act");
      Serial.println("CMD: " + act);

      if(act == "test_clocks") performNeedleSweep();
      
      // SUNROOF
      else if(act == "roof_open")  sendCan(ID_BODY_MODULE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "roof_close") sendCan(ID_BODY_MODULE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "roof_tilt")  sendCan(ID_BODY_MODULE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

      // WINDOWS
      else if(act == "win_fr_up") sendCan(ID_BODY_MODULE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "win_fl_up") sendCan(ID_BODY_MODULE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "win_rl_up") sendCan(ID_BODY_MODULE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "win_rr_up") sendCan(ID_BODY_MODULE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "win_fr_down") sendCan(ID_BODY_MODULE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "win_fl_down") sendCan(ID_BODY_MODULE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "win_rl_down") sendCan(ID_BODY_MODULE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "win_rr_down") sendCan(ID_BODY_MODULE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "win_all_down") sendCan(ID_BODY_MODULE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); 
      else if(act == "win_all_up")   sendCan(ID_BODY_MODULE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); 

      // AIR CONDITIONING
      else if(act == "ac_max") sendCan(ID_CLIMATE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "ac_off") sendCan(ID_CLIMATE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "temp_up") sendCan(ID_CLIMATE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "temp_down") sendCan(ID_CLIMATE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "fan_up") sendCan(ID_CLIMATE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "fan_down") sendCan(ID_CLIMATE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

      // LIGHTS
      else if(act == "light_fog") sendCan(ID_LIGHTS_CTRL, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "light_hazards") sendCan(ID_LIGHTS_CTRL, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "light_welcome") sendCan(ID_LIGHTS_CTRL, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "light_interior") sendCan(ID_LIGHTS_CTRL, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "police_mode_toggle") {
          policeModeActive = !policeModeActive; // Toggle on/off
          if(!policeModeActive) {
              // If turning off, send command to turn off lights
              Serial.println("POLICE MODE: OFF");
          } else {
              Serial.println("POLICE MODE: ON");
          }
      }

      // MEDIA
      else if(act == "vol_up") sendCan(ID_MEDIA_CTRL, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      else if(act == "vol_down") sendCan(ID_MEDIA_CTRL, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

      server.send(200, "text/plain", "OK"); 
  });

  server.on("/toggle", [](){
      int id = server.arg("id").toInt();
      if(id==1) settings.needleSweep = !settings.needleSweep;
      if(id==2) settings.cornering = !settings.cornering;
      if(id==3) settings.essStop = !settings.essStop;
      if(id==4) settings.autoMute = !settings.autoMute;
      if(id==5) settings.rearWelcome = !settings.rearWelcome;
      if(id==6) settings.rainClose = !settings.rainClose;
      if(id==7) settings.mirrorDip = !settings.mirrorDip;
      saveSettings();
      server.send(200, "text/plain", "OK");
  });

  server.on("/data", [](){
      String json = "{";
      json += "\"temp\":" + String(currentTemp) + ",";
      json += "\"volt\":" + String(batteryVolt) + ",";
      json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
      for(int i=1; i<=7; i++) {
          bool val = false;
          if(i==1) val=settings.needleSweep; 
          else if(i==2) val=settings.cornering;
          else if(i==3) val=settings.essStop; 
          else if(i==4) val=settings.autoMute;
          else if(i==5) val=settings.rearWelcome; 
          else if(i==6) val=settings.rainClose;
          else if(i==7) val=settings.mirrorDip;
          
          json += "\"s" + String(i) + "\":" + (val?"true":"false");
          if(i<7) json += ",";
      }
      json += "}";
      server.send(200, "application/json", json);
  });

  server.begin();
  if(settings.needleSweep) { delay(2000); performNeedleSweep(); }
}

void loop() {
  server.handleClient();
  handlePoliceMode();
  
  // --- ESS F1  ---
  if (essActive) {
      // blink very quickly (e.g. every 80ms)
      if (millis() - lastEssStrobe > 80) {
          lastEssStrobe = millis();
          essState = !essState;
          
          if (essState) {
              // ENABLE STOP (Hard)
              // sendCan(ID_LIGHTS_CTRL, ... bit STOP ON ...);
              Serial.println("F1 STOP: BLINK!");
          } else {
              // DISABLE STOP (Or leave position)
              // sendCan(ID_LIGHTS_CTRL, ... bit STOP OFF ...);
          }
      }
  }

  // --- WELCOME LIGHTS SHUTDOWN OPERATION ---
  if (rearWelcomeActive) {
      if (millis() - rearWelcomeTimer > 10000) { // 10 seconds of light
          Serial.println(">>> WELCOME: KONIEC");
          // sendCan(ID_LIGHTS_CTRL, 0x00, ... WSZYSTKO OFF ...); 
          rearWelcomeActive = false;
      }
  }

 // --- CAN READING AND AUTOMATION OPERATION ---

  if(CAN_MSGAVAIL == CAN0.checkReceive()) {
      long unsigned int rxId;
      unsigned char len = 0;
      unsigned char rxBuf[8];
      CAN0.readMsgBuf(&rxId, &len, rxBuf);

      // Automation support
      handleAutomaticFeatures(rxId, len, rxBuf);

      if(snifferActive) {
          String dataStr = "";
          for(int i=0; i<len; i++) {
              if(rxBuf[i]<0x10) dataStr += "0";
              dataStr += String(rxBuf[i], HEX) + " ";
          }
          unsigned long now = millis();
          String t = String(now/1000) + "." + String((now%1000)/100);
          lastCanMsgHtml = "<td>" + t + "</td><td>" + String(rxId, HEX) + "</td><td>" + String(len) + "</td><td>" + dataStr + "</td>";
      }
  }
}