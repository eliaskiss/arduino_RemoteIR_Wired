/*
 * web_page.h — Web UI (PROGMEM)
 * 15개 IR Module 카드 + 전체 송신 버튼
 */

#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>IR Control Panel</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:'Segoe UI',sans-serif;background:#1a1a2e;color:#eee;padding:20px}
h1{text-align:center;color:#e94560;margin-bottom:6px;font-size:1.4em}
.sub{text-align:center;color:#666;font-size:.75em;margin-bottom:20px}
.top{display:flex;justify-content:center;gap:10px;margin-bottom:20px;flex-wrap:wrap}
.top select,.top input{background:#16213e;color:#eee;border:1px solid #2a2a4a;
  border-radius:8px;padding:8px 12px;font-size:.85em}
.top select{min-width:100px}
.top input{width:90px;text-align:center}
.allbtn{background:#e94560;color:#fff;border:none;border-radius:10px;
  padding:12px 24px;font-size:1em;font-weight:bold;cursor:pointer;transition:all .15s}
.allbtn:hover{background:#ff6b81}
.allbtn:active{transform:scale(.95)}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(180px,1fr));
  gap:14px;max-width:1000px;margin:0 auto}
.card{background:linear-gradient(145deg,#16213e,#0f3460);border-radius:16px;
  padding:16px;box-shadow:0 4px 15px rgba(0,0,0,.3)}
.card h3{font-size:.95em;margin-bottom:4px}
.card .pin{font-size:.7em;color:#888;margin-bottom:10px}
.card .row{display:flex;gap:8px;flex-wrap:wrap}
.card .b{border:none;border-radius:8px;padding:10px 14px;font-size:.8em;
  font-weight:bold;cursor:pointer;color:#fff;transition:all .15s;flex:1;min-width:60px;text-align:center}
.card .b:active{transform:scale(.92)}
.send{background:#2980b9}.send:hover{background:#3498db}
.card .st{font-size:.7em;min-height:18px;margin-top:8px;color:#2ecc71;text-align:center}
#gm{text-align:center;font-size:.85em;min-height:22px;margin:14px 0;color:#2ecc71}
.presets{display:flex;justify-content:center;gap:8px;margin-bottom:10px;flex-wrap:wrap}
.presets button{background:#2c3e50;color:#eee;border:none;border-radius:8px;
  padding:6px 12px;font-size:.75em;cursor:pointer}
.presets button:hover{background:#34495e}
</style>
</head>
<body>
<h1>IR Control Panel</h1>
<p class="sub">Arduino UNO R4 Minima + Ethernet + 15x DFR0095</p>

<div class="top">
  <label>Mode
    <select id="mode" onchange="toggleMode()">
      <option value="key" selected>Preset Key</option>
      <option value="raw">Raw (Manual)</option>
    </select>
  </label>
  <label id="lkey">Command
    <select id="keysel">
      <optgroup label="Power">
        <option value="power" selected>Power Toggle</option>
        <option value="on">Power ON</option>
        <option value="off">Power OFF</option>
        <option value="energy">Energy Saving</option>
      </optgroup>
      <optgroup label="Volume">
        <option value="vu">Vol+</option>
        <option value="vd">Vol-</option>
        <option value="mute">Mute</option>
      </optgroup>
      <optgroup label="Channel">
        <option value="cu">CH+</option>
        <option value="cd">CH-</option>
      </optgroup>
      <optgroup label="Input">
        <option value="input">Input</option>
      </optgroup>
      <optgroup label="Number">
        <option value="0">0</option><option value="1">1</option>
        <option value="2">2</option><option value="3">3</option>
        <option value="4">4</option><option value="5">5</option>
        <option value="6">6</option><option value="7">7</option>
        <option value="8">8</option><option value="9">9</option>
      </optgroup>
      <optgroup label="Navigation">
        <option value="up">Up</option>
        <option value="down">Down</option>
        <option value="left">Left</option>
        <option value="right">Right</option>
        <option value="ok">OK</option>
        <option value="back">Back</option>
        <option value="exit">Exit</option>
      </optgroup>
      <optgroup label="Function">
        <option value="set">Settings</option>
        <option value="home">Home</option>
        <option value="simplink">SimpLink</option>
        <option value="auto">Auto Config</option>
        <option value="tile">Tile</option>
        <option value="smenu">S.Menu</option>
        <option value="wbal">W.Balance</option>
      </optgroup>
      <optgroup label="Display">
        <option value="bru">Brightness+</option>
        <option value="brd">Brightness-</option>
        <option value="3d">3D</option>
        <option value="arc">Aspect Ratio</option>
        <option value="psm">Picture Mode</option>
      </optgroup>
      <optgroup label="Media">
        <option value="play">Play</option>
        <option value="pause">Pause</option>
        <option value="stop">Stop</option>
        <option value="rw">Rewind</option>
        <option value="ff">Fast Forward</option>
      </optgroup>
      <optgroup label="ID">
        <option value="idon">ID On</option>
        <option value="idoff">ID Off</option>
      </optgroup>
      <optgroup label="Special">
        <option value="swap">Swap</option>
        <option value="mirror">Mirror</option>
        <option value="instart">InStart</option>
        <option value="instop">InStop</option>
        <option value="adj">Adjust</option>
      </optgroup>
    </select>
  </label>
  <span id="rawfields" style="display:none">
    <label>Protocol
      <select id="proto">
        <option value="NEC" selected>NEC</option>
        <option value="SONY">SONY</option>
        <option value="RC5">RC5</option>
        <option value="RC6">RC6</option>
        <option value="SAMSUNG">SAMSUNG</option>
      </select>
    </label>
    <label>Address <input id="addr" value="0x04"></label>
    <label>Command <input id="cmd" value="0x08"></label>
  </span>
</div>

<div style="text-align:center;margin-bottom:16px">
  <button class="allbtn" onclick="sendAll()">ALL SEND</button>
</div>
<div id="gm"></div>

<div class="grid" id="cards"></div>

<script>
const PINS=[0,1,2,3,5,6,7,8,9,'A0','A1','A2','A3','A4','A5'];
const grid=document.getElementById('cards');

for(let i=1;i<=15;i++){
  const d=document.createElement('div');
  d.className='card';
  d.innerHTML=`<h3>Module ${i}</h3>
    <div class="pin">Pin: ${PINS[i-1]}</div>
    <div class="row">
      <button class="b send" onclick="send(${i})">SEND</button>
    </div>
    <div class="st" id="s${i}"></div>`;
  grid.appendChild(d);
}

function toggleMode(){
  const m=document.getElementById('mode').value;
  document.getElementById('lkey').style.display=m==='key'?'':'none';
  document.getElementById('rawfields').style.display=m==='raw'?'':'none';
}

function getPayload(){
  if(document.getElementById('mode').value==='key'){
    return JSON.stringify({key:document.getElementById('keysel').value});
  }
  return JSON.stringify({
    protocol:document.getElementById('proto').value,
    address:document.getElementById('addr').value,
    command:document.getElementById('cmd').value
  });
}

function send(id){
  const el=document.getElementById('s'+id);
  el.textContent='Sending...';el.style.color='#f39c12';
  fetch('/api/ir/'+id+'/send',{method:'POST',
    headers:{'Content-Type':'application/json'},body:getPayload()})
  .then(r=>r.json()).then(j=>{
    el.style.color=j.status==='ok'?'#2ecc71':'#e74c3c';
    el.textContent=j.status==='ok'?'OK':'Error: '+(j.message||'');
    setTimeout(()=>el.textContent='',2500);
  }).catch(()=>{el.style.color='#e74c3c';el.textContent='Failed';});
}

function sendAll(){
  const gm=document.getElementById('gm');
  gm.textContent='Sending to all...';gm.style.color='#f39c12';
  fetch('/api/ir/all/send',{method:'POST',
    headers:{'Content-Type':'application/json'},body:getPayload()})
  .then(r=>r.json()).then(j=>{
    gm.style.color=j.status==='ok'?'#2ecc71':'#e74c3c';
    gm.textContent=j.status==='ok'?'All sent OK':'Error: '+(j.message||'');
    setTimeout(()=>gm.textContent='',2500);
  }).catch(()=>{gm.style.color='#e74c3c';gm.textContent='Failed';});
}
</script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGE_H
