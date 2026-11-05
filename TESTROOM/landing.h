// ============================================================================
// landing.h — landingspagina "/" van de ROOM-sketch (hoort bij v3.1)
// Eenvoudige pagina voor huisgenoten en gasten: temperatuur + setpoint, lampen,
// kleur, bed-modus, thuis/weg. Technische pagina: /advanced (⚙ onderaan).
//
// Apart .h-bestand: de Arduino IDE zet anders soms C++-vooraankondigingen midden
// in JavaScript dat in een .ino staat. .h-bestanden laat de IDE ongemoeid.
//
// Opbouw (zie de "/"-handler in ESP32_C6_ROOM.ino):
//   LANDING_KOP  + kamernaam + LANDING_MIDDEN + <script>var CFG={...}</script> + LANDING_SCRIPT
//   CFG wordt door de ESP32 ingevuld: pixelnamen en of MOV2 actief is.
// Gebruikt enkel bestaande adressen: /json, /set_setpoint, /toggle_pixel,
// /toggle_pixel_mode, /setcolor, /toggle_bed, /toggle_home.
// ============================================================================
#pragma once
#include <Arduino.h>

const char LANDING_KOP[] PROGMEM = R"HTML(<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Zarlar</title><style>
body{font-family:Arial,sans-serif;margin:0;background:#fff;color:#222}
.hdr{background:#ffcc00;padding:12px 16px;font-weight:bold;font-size:20px}
.box{padding:14px;max-width:520px;margin:auto}
.kaart{background:#e6f0ff;border-radius:12px;padding:14px;margin:12px 0}
.rij{display:flex;align-items:center;gap:14px}
.groot{font-size:46px;font-weight:bold;color:#336699;line-height:1}
.sp{font-size:20px;color:#336699;margin-left:6px}
.sub{color:#666;font-size:14px;margin-top:4px}
input[type=range]{width:100%;height:34px;margin-top:10px}
.icring{width:52px;height:52px;border-radius:50%;border:3px solid #ccc;background:#fff;display:flex;align-items:center;justify-content:center;font-size:26px;flex-shrink:0}
.icring.actief{background:#ffd6d6;border-color:#c00}
h3{color:#336699;margin:18px 0 8px;font-size:17px}
.tegels{display:grid;grid-template-columns:repeat(auto-fill,minmax(100px,1fr));gap:10px}
.tegel{aspect-ratio:1;border:2px solid #ccc;border-radius:12px;display:flex;flex-direction:column;align-items:center;justify-content:center;cursor:pointer;background:#fff;font-size:14px;text-align:center;padding:6px;user-select:none;-webkit-user-select:none}
.tegel .ic{font-size:28px}
.tegel .bd{font-size:11px;margin-top:4px;opacity:.8}
.knoppen{display:flex;gap:18px;justify-content:center;align-items:center;margin:16px 0}
.rond{width:64px;height:64px;border-radius:50%;border:3px solid #ccc;background:#fff;font-size:28px;cursor:pointer;padding:0}
.rond.bed{background:#1d2b6b;border-color:#1d2b6b}
.rond.thuis{background:#d8f5d8;border-color:#2a9d2a}
.kleur{display:flex;flex-direction:column;align-items:center;font-size:12px;color:#666}
input[type=color]{width:64px;height:64px;border:0;padding:0;background:none;cursor:pointer}
.uitleg{font-size:12px;color:#666;text-align:center}
.voet{text-align:center;margin:20px 0 10px}
.voet a{font-size:28px;text-decoration:none}
</style></head><body>
<div class="hdr">)HTML";

const char LANDING_MIDDEN[] PROGMEM = R"HTML(</div><div class="box">
<div class="kaart">
 <div class="rij"><div class="icring" id="heat">🔥</div>
  <div><div><span class="groot" id="temp">…</span><span class="sp" id="sp"></span></div>
  <div class="sub" id="vocht"></div></div></div>
 <input type="range" id="sl" min="10" max="30" step="1">
 <div class="uitleg">Gewenste temperatuur: <b id="slv">…</b></div>
</div>
<h3>Verlichting</h3>
<div class="tegels" id="tegels"></div>
<div class="knoppen">
 <button class="rond" id="bed" onclick="knop('/toggle_bed')" title="Bed-modus">🛏️</button>
 <button class="rond" id="thuis" onclick="knop('/toggle_home')" title="Thuis / weg">🏠</button>
 <label class="kleur"><input type="color" id="kl">kleur</label>
</div>
<div class="uitleg">🛏️ bed-modus: de bewegingslampen blijven uit · 🏠 groen = thuis</div>
<div class="voet"><a href="/advanced" title="Technische pagina">⚙️</a></div>
</div>
)HTML";

const char LANDING_SCRIPT[] PROGMEM = R"HTML(<script>
var $=function(i){return document.getElementById(i);},laatst=null,bezig=0;
function hex(v){return ('0'+Math.round(v).toString(16)).slice(-2);}
function isMov(i){return i==0||(i==1&&CFG.mov2);}
function tegels(){var h='';for(var i=0;i<CFG.px.length;i++){
 h+='<div class="tegel" id="t'+i+'" onclick="tik('+i+')"><div class="ic">💡</div><div>'+CFG.px[i]+'</div><div class="bd" id="b'+i+'"></div></div>';}
 $('tegels').innerHTML=h;}
function aan(d,i){var s=(d.t||'').replace('P=','');
 if(i==0)return d.y==1; if(i==1&&CFG.mov2)return d.z==1; return s.charAt(i)=='1';}
function toon(d){laatst=d;
 var t=(d.f>0?d.f:d.e);$('temp').textContent=(t>0?t.toFixed(1):'–')+'°';
 $('sp').textContent='(→ '+d.c+'°)';$('vocht').textContent=d.h>0?'Vocht '+Math.round(d.h)+' %':'';
 $('heat').classList.toggle('actief',d.b==1);
 if(document.activeElement!==$('sl')){$('sl').value=d.c;$('slv').textContent=d.c+' °C';}
 var k='#'+hex(d.q)+hex(d.r)+hex(d.s),licht=(0.3*d.q+0.59*d.r+0.11*d.s)>140;
 if(document.activeElement!==$('kl'))$('kl').value=k;
 for(var i=0;i<CFG.px.length;i++){var e=$('t'+i);if(!e)continue;var on=aan(d,i);
  e.style.background=on?k:'#fff';e.style.color=on&&!licht?'#fff':'#222';e.style.borderColor=on?k:'#ccc';
  var b='';if(isMov(i))b=(d.u||'00').charAt(i)=='1'?'altijd aan':(d.p==1?'bed-modus':'automatisch');
  $('b'+i).textContent=b;}
 $('bed').classList.toggle('bed',d.p==1);$('thuis').classList.toggle('thuis',d.v==1);}
function ververs(){if(bezig)return;fetch('/json?'+Date.now(),{cache:'no-store'}).then(function(r){return r.json();}).then(toon).catch(function(){});}
function knop(u){bezig=1;fetch(u).then(function(){bezig=0;ververs();}).catch(function(){bezig=0;});}
function tik(i){knop((isMov(i)?'/toggle_pixel_mode?idx=':'/toggle_pixel?idx=')+i);}
$('sl').addEventListener('input',function(){$('slv').textContent=this.value+' °C';});
$('sl').addEventListener('change',function(){knop('/set_setpoint?setpoint='+this.value);});
$('kl').addEventListener('change',function(){var v=this.value;
 knop('/setcolor?r='+parseInt(v.substr(1,2),16)+'&g='+parseInt(v.substr(3,2),16)+'&b='+parseInt(v.substr(5,2),16));});
tegels();ververs();setInterval(ververs,3000);
</script></body></html>)HTML";
