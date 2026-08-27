#pragma once
// =====================================================================
//  ui.h — встроенный веб-интерфейс (PROGMEM).
//  INDEX_HTML — основное SPA-приложение.
//  DEV_HTML   — страница режима разработчика (/dev).
//  Весь CSS и JS инлайн, без внешних CDN.
// =====================================================================
#include <Arduino.h>

// ---------------------------------------------------------------------
//  ОСНОВНАЯ СТРАНИЦА (SPA)
// ---------------------------------------------------------------------
const char INDEX_HTML[] PROGMEM = R"HTMLPAGE(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<meta name="theme-color" content="#0c0c14">
<title>Такси Шашка</title>
<link rel="manifest" href="/manifest.json">
<link rel="icon" type="image/png" sizes="192x192" href="/icon.png">
<link rel="icon" type="image/svg+xml" href="/icon.svg">
<link rel="apple-touch-icon" href="/icon.png">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="apple-mobile-web-app-title" content="Такси Шашка">

<style>
:root{
  --bg:#0a0a0f; --card:#141420; --card2:#1b1b2b;
  --txt:#e8e8f0; --muted:#8a8aa0;
  --accent1:#7c3aed; --accent2:#3b82f6;
  --accent:linear-gradient(135deg,#7c3aed,#3b82f6);
  --ok:#22c55e; --warn:#f59e0b; --err:#ef4444;
  --radius:20px; --shadow:0 8px 30px rgba(0,0,0,.4);
}
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{
  font-family:'Inter',system-ui,-apple-system,'Segoe UI',Roboto,sans-serif;
  background:var(--bg);color:var(--txt);
  min-height:100vh;padding-bottom:88px;
  background-image:radial-gradient(circle at 20% 0%,rgba(124,58,237,.18),transparent 40%),
                   radial-gradient(circle at 90% 15%,rgba(59,130,246,.15),transparent 40%);
}
.wrap{max-width:480px;margin:0 auto;padding:20px 16px}
header{display:flex;align-items:center;justify-content:space-between;margin-bottom:18px}
header h1{font-size:20px;font-weight:700;letter-spacing:.3px}
header .sub{font-size:12px;color:var(--muted)}
.dot{width:9px;height:9px;border-radius:50%;display:inline-block;margin-right:5px;background:var(--muted);vertical-align:middle}
.dot.on{background:var(--ok);box-shadow:0 0 8px var(--ok)}
.dot.ap{background:var(--warn);box-shadow:0 0 8px var(--warn)}

.card{
  background:linear-gradient(160deg,var(--card),var(--card2));
  border:1px solid rgba(255,255,255,.06);
  border-radius:var(--radius);padding:18px;margin-bottom:16px;
  box-shadow:var(--shadow);backdrop-filter:blur(10px);
  transition:transform .3s ease,box-shadow .3s ease;
}
.card h3{font-size:13px;text-transform:uppercase;letter-spacing:1px;color:var(--muted);margin-bottom:14px;font-weight:600}

/* Кнопка питания */
.power-card{position:relative;overflow:hidden}
.power-wrap{display:flex;flex-direction:column;align-items:center;gap:14px;padding:8px 0}
.power-btn{
  width:150px;height:150px;border-radius:50%;border:none;cursor:pointer;
  background:radial-gradient(circle at 50% 40%,#23233a,#101019);
  color:var(--muted);display:flex;align-items:center;justify-content:center;
  transition:all .4s ease;position:relative;
}
.power-btn svg{width:60px;height:60px;transition:all .4s ease}
.power-btn.active{
  background:radial-gradient(circle at 50% 40%,#7c3aed,#3b82f6);
  color:#fff;box-shadow:0 0 40px rgba(124,58,237,.6);
  animation:pulse 2.4s infinite;
}
@keyframes pulse{0%{box-shadow:0 0 30px rgba(124,58,237,.5)}50%{box-shadow:0 0 55px rgba(59,130,246,.75)}100%{box-shadow:0 0 30px rgba(124,58,237,.5)}}
.power-label{font-size:14px;color:var(--muted);font-weight:600}

/* Батарея в правом нижнем углу карточки питания */
.power-batt{
  position:absolute;right:14px;bottom:14px;
  display:flex;align-items:center;gap:8px;
  background:rgba(0,0,0,.35);border:1px solid rgba(255,255,255,.08);
  padding:5px 10px;border-radius:12px;backdrop-filter:blur(8px);
}
.batt-ico{position:relative;width:24px;height:13px;border:1.5px solid var(--muted);border-radius:3px;padding:1.5px}
.batt-ico:after{content:"";position:absolute;right:-4px;top:3px;width:2px;height:5px;background:var(--muted);border-radius:0 1px 1px 0}
.batt-fill{height:100%;border-radius:1px;background:var(--ok);transition:width .5s ease,background .5s}
.batt-info{line-height:1.2;text-align:right}
.batt-row{font-size:12px;font-weight:700}
.batt-chg{color:var(--warn);font-size:11px;margin-left:2px}
.batt-volt{font-size:10px;color:var(--muted)}

/* Ползунок */
.slider-row{display:flex;align-items:center;gap:12px}
.slider-row .ico{font-size:18px;width:24px;text-align:center}
input[type=range]{
  -webkit-appearance:none;appearance:none;flex:1;height:10px;border-radius:8px;

  background:#26263a;outline:none;
}
input[type=range]::-webkit-slider-thumb{
  -webkit-appearance:none;width:26px;height:26px;border-radius:50%;
  background:var(--accent);cursor:pointer;box-shadow:0 2px 10px rgba(124,58,237,.6);
  border:3px solid #0a0a0f;
}
input[type=range]::-moz-range-thumb{
  width:26px;height:26px;border-radius:50%;background:#7c3aed;cursor:pointer;border:3px solid #0a0a0f;
}
.val{min-width:42px;text-align:right;font-variant-numeric:tabular-nums;color:var(--muted);font-size:13px}

/* Цвет */
.color-row{display:flex;align-items:center;gap:14px}
.color-prev{width:52px;height:52px;border-radius:16px;border:2px solid rgba(255,255,255,.15);box-shadow:inset 0 0 12px rgba(0,0,0,.4)}
input[type=color]{position:absolute;opacity:0;width:52px;height:52px;cursor:pointer}
.color-hex{font-family:monospace;color:var(--muted);font-size:14px}
.swatches{display:flex;gap:8px;margin-top:14px;flex-wrap:wrap}
.sw{width:34px;height:34px;border-radius:10px;cursor:pointer;border:2px solid transparent;transition:transform .2s}
.sw:active{transform:scale(.88)}

/* Инфо-строки */
.info-line{display:flex;justify-content:space-between;align-items:center;padding:9px 0;border-bottom:1px solid rgba(255,255,255,.05);font-size:14px}
.info-line:last-child{border-bottom:none}
.info-line .k{color:var(--muted)}
.info-line .v{font-weight:600}

/* Сетка эффектов */
.fx-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.fx-card{
  background:var(--card2);border:2px solid rgba(255,255,255,.06);border-radius:16px;
  padding:14px;cursor:pointer;transition:all .3s ease;overflow:hidden;position:relative;
}
.fx-card:active{transform:scale(.96)}
.fx-card.sel{border-color:#7c3aed;box-shadow:0 0 20px rgba(124,58,237,.4)}
.fx-name{font-size:14px;font-weight:600;margin-top:10px}
.fx-prev{height:56px;border-radius:12px;overflow:hidden;position:relative}
/* превью-анимации */
.p-static{background:#ffb000}
.p-breathe{background:#7c3aed;animation:pbreathe 2.5s infinite ease-in-out}
@keyframes pbreathe{0%,100%{opacity:.25}50%{opacity:1}}
.p-rainbow{background:linear-gradient(90deg,red,orange,yellow,green,cyan,blue,violet,red);background-size:200% 100%;animation:prain 2s linear infinite}
@keyframes prain{0%{background-position:0 0}100%{background-position:200% 0}}
.p-fire{background:linear-gradient(0deg,#ff2200,#ff8800,#ffcc00);animation:pfire .4s infinite alternate}
@keyframes pfire{0%{filter:brightness(.7)}100%{filter:brightness(1.3)}}
.p-candle{background:radial-gradient(circle at 50% 70%,#ffcc33,#ff8800);animation:pcandle .25s infinite alternate}
@keyframes pcandle{0%{filter:brightness(.6)}100%{filter:brightness(1.2)}}
.p-flow{animation:pflow 6s linear infinite}
@keyframes pflow{0%{background:#ff0055}25%{background:#7c3aed}50%{background:#0af}75%{background:#0f5}100%{background:#ff0055}}

/* -------------------------------------------------------
   Новые превью эффектов
   ------------------------------------------------------- */

/* Северное сияние */
.p-aurora{
  background:
    linear-gradient(
      120deg,
      #063b35,
      #00a878,
      #00d4aa,
      #3155d9,
      #7c3aed,
      #063b35
    );
  background-size:300% 100%;
  animation:paurora 5s ease-in-out infinite;
}

@keyframes paurora{
  0%{
    background-position:0% 50%;
    filter:brightness(.65);
  }
  50%{
    background-position:100% 50%;
    filter:brightness(1.25);
  }
  100%{
    background-position:0% 50%;
    filter:brightness(.65);
  }
}


/* Океан */
.p-ocean{
  background:
    linear-gradient(
      90deg,
      #003b73,
      #006994,
      #00a6a6,
      #0077b6,
      #003b73
    );
  background-size:300% 100%;
  animation:pocean 3s ease-in-out infinite;
}

@keyframes pocean{
  0%{
    background-position:0% 50%;
    filter:brightness(.7);
  }
  50%{
    background-position:100% 50%;
    filter:brightness(1.2);
  }
  100%{
    background-position:0% 50%;
    filter:brightness(.7);
  }
}


/* Полиция — красный / синий */
.p-police{
  background:
    linear-gradient(
      90deg,
      #ff0000 0%,
      #ff0000 20%,
      #101020 20%,
      #101020 80%,
      #0055ff 80%,
      #0055ff 100%
    );
  animation:ppolice 1.2s infinite;
}

@keyframes ppolice{
  0%, 35%{
    filter:brightness(1.4);
  }

  50%, 85%{
    filter:brightness(.65);
  }

  100%{
    filter:brightness(1.4);
  }
}


/* Полицейская мигалка */
.p-police-flash{
  background:
    linear-gradient(
      90deg,
      #ff0000 0%,
      #ff0000 20%,
      #050510 20%,
      #050510 80%,
      #0055ff 80%,
      #0055ff 100%
    );
  animation:ppoliceflash .8s steps(1,end) infinite;
}

@keyframes ppoliceflash{
  0%{
    filter:brightness(1.6);
  }

  12%{
    filter:brightness(.25);
  }

  25%{
    filter:brightness(1.6);
  }

  37%{
    filter:brightness(.25);
  }

  50%{
    filter:brightness(.15);
  }

  62%{
    filter:brightness(1.6);
  }

  75%{
    filter:brightness(.25);
  }

  87%{
    filter:brightness(1.6);
  }

  100%{
    filter:brightness(.15);
  }
}

/* Метеор */
.p-meteor{background:#000;position:relative;overflow:hidden}
.p-meteor::after{content:"";position:absolute;width:12px;height:100%;background:linear-gradient(90deg,transparent,#fff,#7c3aed);animation:pmeteor 1.2s linear infinite;left:-12px;top:0}
@keyframes pmeteor{0%{left:-12px}100%{left:100%}}

/* Стробоскоп */
.p-strobe{background:#fff;animation:pstrobe .3s steps(1,end) infinite}
@keyframes pstrobe{0%{background:#fff}50%{background:#000}}

/* Бегущие огни */
.p-running{background:#111;position:relative;overflow:hidden}
.p-running::after{content:"";position:absolute;width:8px;height:100%;border-radius:50%;background:#7c3aed;box-shadow:0 0 10px #7c3aed;animation:prun 1s linear infinite;left:-8px}
@keyframes prun{0%{left:-8px}100%{left:100%}}

/* Ночное небо */
.p-nightsky{background:#000814;animation:pnightsky 3s ease infinite}
@keyframes pnightsky{0%,100%{box-shadow:inset 10px 10px 2px 0px #fff,inset 30px 5px 2px 0px rgba(255,255,255,.3),inset 50px 20px 1px 0px #fff}50%{box-shadow:inset 10px 10px 2px 0px rgba(255,255,255,.2),inset 30px 5px 2px 0px #fff,inset 50px 20px 1px 0px rgba(255,255,255,.5)}}

/* Лазер */
.p-laser{background:#000;position:relative;overflow:hidden}
.p-laser::after{content:"";position:absolute;width:4px;height:100%;background:#f00;box-shadow:0 0 8px #f00;animation:plaser .8s ease-in-out infinite alternate;left:0}
@keyframes plaser{0%{left:0}100%{left:calc(100% - 4px)}}

/* Вспышки */
.p-flash{background:#111;animation:pflash .5s steps(1,end) infinite}
@keyframes pflash{0%{background:#111}10%{background:#3b82f6}20%{background:#111}35%{background:#7c3aed}50%{background:#111}70%{background:#22c55e}100%{background:#111}}

/* Избранное */
.fav-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.fav-card{background:var(--card2);border:2px solid rgba(255,255,255,.06);border-radius:16px;padding:14px;transition:all .25s}
.fav-empty{display:flex;flex-direction:column;align-items:center;justify-content:center;min-height:120px;cursor:pointer;border-style:dashed !important;opacity:.6}
.fav-color-dot{width:40px;height:40px;border-radius:12px;margin-bottom:8px;border:2px solid rgba(255,255,255,.2)}
.fav-name{font-weight:600;font-size:14px}
.fav-meta{font-size:12px;color:var(--muted);margin-top:2px}

/* Таймер сна */
.timer-badge{display:inline-block;background:rgba(124,58,237,.25);border:1px solid rgba(124,58,237,.5);color:#a78bfa;border-radius:20px;padding:4px 12px;font-size:13px;font-weight:600;margin-left:8px}

/* Расписание */

.sched-row{background:var(--card2);border-radius:12px;padding:12px;margin-bottom:10px}
input[type=number]{width:100%;padding:12px 14px;border-radius:12px;border:1px solid rgba(255,255,255,.1);background:#0f0f18;color:var(--txt);font-size:15px;margin-top:6px;outline:none}
input[type=number]:focus{border-color:#7c3aed}

/* Кнопки */
.btn{
  width:100%;padding:14px;border:none;border-radius:14px;font-size:15px;font-weight:600;
  cursor:pointer;transition:all .25s ease;color:#fff;background:var(--accent);
}
.btn:active{transform:scale(.97)}
.btn.ghost{background:var(--card2);border:1px solid rgba(255,255,255,.1);color:var(--txt)}
.btn.danger{background:linear-gradient(135deg,#ef4444,#b91c1c)}
.btn.small{padding:9px 14px;font-size:13px;width:auto}

input[type=text],input[type=password],select{
  width:100%;padding:12px 14px;border-radius:12px;border:1px solid rgba(255,255,255,.1);
  background:#0f0f18;color:var(--txt);font-size:15px;margin-top:6px;outline:none;
}
input:focus,select:focus{border-color:#7c3aed}
label.fld{display:block;font-size:13px;color:var(--muted);margin-top:12px}

/* Список сетей */
.net{display:flex;justify-content:space-between;align-items:center;padding:12px;border-radius:12px;background:var(--card2);margin-top:8px;cursor:pointer;transition:.2s}
.net:active{transform:scale(.98)}
.net .n-ssid{font-weight:600}
.net .n-rssi{font-size:12px;color:var(--muted)}

/* Нижняя навигация */
.tabbar{
  position:fixed;bottom:0;left:0;right:0;z-index:50;
  display:flex;justify-content:space-around;
  background:rgba(15,15,24,.92);backdrop-filter:blur(16px);
  border-top:1px solid rgba(255,255,255,.07);padding:8px 0 12px;
}
.tabbar .tab{
  flex:1;display:flex;flex-direction:column;align-items:center;gap:3px;
  background:none;border:none;color:var(--muted);cursor:pointer;font-size:11px;transition:.25s;
}
.tabbar .tab svg{width:24px;height:24px}
.tabbar .tab.active{color:#a78bfa}

/* Экраны вкладок */
.screen{display:none;animation:fade .35s ease}
.screen.active{display:block}
@keyframes fade{from{opacity:0;transform:translateY(12px)}to{opacity:1;transform:translateY(0)}}

/* Заглушка */
.soon{text-align:center;padding:60px 20px;color:var(--muted)}
.soon .big{font-size:46px;margin-bottom:12px}

/* Toast */
#toast{position:fixed;bottom:96px;left:50%;transform:translateX(-50%) translateY(30px);
  background:#1b1b2b;border:1px solid rgba(255,255,255,.12);color:#fff;padding:12px 20px;
  border-radius:14px;font-size:14px;opacity:0;pointer-events:none;transition:.35s;z-index:100;box-shadow:var(--shadow)}
#toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
#toast.err{border-color:var(--err)}
#toast.ok{border-color:var(--ok)}

.muted{color:var(--muted);font-size:13px}
.spin{display:inline-block;width:16px;height:16px;border:2px solid rgba(255,255,255,.2);border-top-color:#a78bfa;border-radius:50%;animation:sp 1s linear infinite;vertical-align:middle}
@keyframes sp{to{transform:rotate(360deg)}}

.fs-btn{
  background:rgba(255,255,255,.07);border:1px solid rgba(255,255,255,.1);
  border-radius:10px;width:34px;height:34px;display:flex;align-items:center;
  justify-content:center;color:var(--muted);cursor:pointer;transition:.2s;
}
.fs-btn:hover{color:#fff;background:rgba(255,255,255,.15)}
.fs-btn svg{width:18px;height:18px}


/* Десктопная адаптация для мониторов */
.home-col, .set-col { display: contents; }

@media (min-width: 768px) {
  .wrap { max-width: 980px; padding: 32px 28px 100px; }
  header { margin-bottom: 26px; }
  header h1 { font-size: 24px; }
  
  /* Главная: 2 колонки */
  #scr-home.active {
    display: grid;
    grid-template-columns: 1fr 1.1fr;
    gap: 20px;
    align-items: start;
  }
  .home-col {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }
  #scr-home .power-card {
    min-height: 290px;
    display: flex;
    flex-direction: column;
    justify-content: center;
  }

  /* Эффекты: гибкая сетка от 3 до 5 колонок */
  .fx-grid {
    grid-template-columns: repeat(auto-fill, minmax(180px, 1fr));
    gap: 16px;
  }

  /* Избранное: адаптивная сетка */
  .fav-grid {
    grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
    gap: 16px;
  }

  /* Настройки: 2 колонки */
  #scr-set.active {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 20px;
    align-items: start;
  }
  .set-col {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  /* Плавающая панель навигации на десктопе */
  .tabbar {
    width: auto;
    left: 50%;
    right: auto;
    transform: translateX(-50%);
    bottom: 24px;
    border-radius: 20px;
    border: 1px solid rgba(255,255,255,.14);
    background: rgba(20,20,32,.92);
    backdrop-filter: blur(20px);
    box-shadow: 0 14px 40px rgba(0,0,0,.75);
    padding: 6px 8px;
    gap: 6px;
  }
  .tabbar .tab {
    flex: none;
    flex-direction: row;
    align-items: center;
    gap: 8px;
    padding: 10px 18px;
    border-radius: 14px;
    font-size: 13px;
    font-weight: 500;
    white-space: nowrap;
  }
  .tabbar .tab svg {
    width: 20px;
    height: 20px;
  }
  .tabbar .tab:hover {
    background: rgba(255,255,255,.08);
    color: #fff;
  }
  .tabbar .tab.active {
    background: linear-gradient(135deg, rgba(124,58,237,.35), rgba(59,130,246,.25));
    color: #fff;
    border: 1px solid rgba(124,58,237,.4);
  }
}

</style>

</head>
<body>
<div class="wrap">

  <header>
    <div>
      <h1 id="devName">Такси Шашка</h1>
      <div class="sub"><span class="dot" id="wifiDot"></span><span id="wifiTxt">—</span></div>
    </div>
    <div style="display:flex;align-items:center;gap:10px">
      <button class="fs-btn" onclick="toggleFs()" title="Во весь экран">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3"/>
        </svg>
      </button>
      <div class="sub" id="verTxt">v—</div>
    </div>
  </header>


  <!-- ================= ГЛАВНАЯ ================= -->
  <section class="screen active" id="scr-home">
    <div class="home-col">
      <div class="card power-card">
        <div class="power-wrap">
          <button class="power-btn" id="powerBtn" onclick="togglePower()">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round">
              <path d="M12 3v9"/><path d="M6.4 6.4a8 8 0 1 0 11.2 0"/>
            </svg>
          </button>
          <div class="power-label" id="powerLabel">Выключено</div>
        </div>
        <div class="power-batt">
          <div class="batt-ico"><div class="batt-fill" id="battFill" style="width:0%"></div></div>
          <div class="batt-info">
            <div class="batt-row"><span id="battPct">—</span>% <span id="battChg"></span></div>
            <div class="batt-volt"><span id="battVolt">—</span> В</div>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="info-line"><span class="k">Текущий режим</span><span class="v" id="curEffect">—</span></div>
        <div class="info-line"><span class="k">Сеть Wi-Fi</span><span class="v" id="curWifi">—</span></div>
        <div class="info-line"><span class="k">IP-адрес</span><span class="v" id="curIp">—</span></div>
      </div>
    </div>

    <div class="home-col">
      <div class="card">
        <h3>Яркость</h3>
        <div class="slider-row">
          <span class="ico">🔅</span>
          <input type="range" id="bright" min="1" max="255" value="150" oninput="onBrightInput(this.value)">
          <span class="ico">🔆</span>
          <span class="val" id="brightVal">59%</span>
        </div>
      </div>

      <div class="card">
        <h3>Цвет</h3>
        <div class="color-row">
          <div style="position:relative">
            <div class="color-prev" id="colorPrev" style="background:#ffb000"></div>
            <input type="color" id="colorPick" value="#ffb000" oninput="onColorInput(this.value)">
          </div>
          <div>
            <div style="font-weight:600">Основной цвет</div>
            <div class="color-hex" id="colorHex">#FFB000</div>
          </div>
        </div>
        <div class="swatches" id="swatches"></div>
      </div>

      <div class="card">
        <h3>Таймер сна <span class="timer-badge" id="timerBadge" style="display:none"></span></h3>
        <div style="display:flex;gap:8px;flex-wrap:wrap">
          <button class="btn ghost small" onclick="setTimer(15)">15 мин</button>
          <button class="btn ghost small" onclick="setTimer(30)">30 мин</button>
          <button class="btn ghost small" onclick="setTimer(60)">1 час</button>
          <button class="btn danger small" onclick="setTimer(0)">✕ Отменить</button>
        </div>
      </div>
    </div>
  </section>




  <!-- ================= ЭФФЕКТЫ ================= -->
  <section class="screen" id="scr-fx">
    <div class="card">
      <h3>Световые эффекты</h3>
      <div class="fx-grid" id="fxGrid"></div>
    </div>
    <div class="card">
      <h3>Скорость эффекта</h3>
      <div class="slider-row">
        <span class="ico">🐢</span>
        <input type="range" id="speed" min="0" max="255" value="128" oninput="onSpeedInput(this.value)">
        <span class="ico">🐇</span>
      </div>
    </div>
  </section>

  <!-- ================= ИЗБРАННОЕ ================= -->
  <section class="screen" id="scr-fav">
    <div class="card">
      <h3>Избранные сцены</h3>
      <div class="fav-grid" id="favGrid"></div>
    </div>
  </section>

  <!-- ================= НАСТРОЙКИ ================= -->
  <section class="screen" id="scr-set">
    <div class="set-col">
      <div class="card">
        <h3>Wi-Fi</h3>
        <div class="info-line"><span class="k">Текущая сеть</span><span class="v" id="setWifi">—</span></div>
        <button class="btn ghost small" style="margin-top:12px" onclick="scanWifi()">Сменить сеть / сканировать</button>
        <div id="netList"></div>
      </div>

      <div class="card">
        <h3>Устройство</h3>
        <label class="fld">Имя устройства</label>
        <input type="text" id="setName" placeholder="Такси Шашка">
        <label class="fld">Часовой пояс</label>
        <select id="setTz"></select>
        <label class="fld">Поведение при включении</label>
        <select id="setPom">
          <option value="0">Восстановить последнее</option>
          <option value="1">Всегда выключено</option>
          <option value="2">Всегда включено</option>
        </select>
        <button class="btn" style="margin-top:16px" onclick="saveSettings()">Сохранить</button>
      </div>
    </div>

    <div class="set-col">
      <div class="card">
        <h3>Расписание</h3>
        <div id="schedGrid"></div>
      </div>

      <div class="card">
        <h3>Информация о системе</h3>
        <div class="info-line"><span class="k">Версия прошивки</span><span class="v" id="iVer">—</span></div>
        <div class="info-line"><span class="k">IP-адрес</span><span class="v" id="iIp">—</span></div>
        <div class="info-line"><span class="k">MAC-адрес</span><span class="v" id="iMac">—</span></div>
        <div class="info-line"><span class="k">Уровень сигнала</span><span class="v" id="iRssi">—</span></div>
        <div class="info-line"><span class="k">Свободная память</span><span class="v" id="iHeap">—</span></div>
        <div class="info-line"><span class="k">NTP синхронизация</span><span class="v" id="iNtp">—</span></div>
      </div>

      <div class="card">
        <h3>Обслуживание</h3>
        <a href="/dev" style="text-decoration:none"><button class="btn ghost" style="margin-bottom:10px">Режим разработчика</button></a>
        <button class="btn danger" onclick="resetSettings()">Сбросить настройки</button>
      </div>
    </div>
  </section>

</div>


<!-- Нижняя навигация -->
<nav class="tabbar">
  <button class="tab active" data-scr="home" onclick="showTab('home')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 11l9-8 9 8"/><path d="M5 10v10h14V10"/></svg><span>Главная</span></button>
  <button class="tab" data-scr="fx" onclick="showTab('fx')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2l2.4 7.4H22l-6 4.6 2.3 7.4L12 17l-6.3 4.4L8 14 2 9.4h7.6z"/></svg><span>Эффекты</span></button>
  <button class="tab" data-scr="fav" onclick="showTab('fav')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20.8 4.6a5.5 5.5 0 0 0-7.8 0L12 5.6l-1-1a5.5 5.5 0 0 0-7.8 7.8l1 1L12 21l7.8-7.6 1-1a5.5 5.5 0 0 0 0-7.8z"/></svg><span>Избранное</span></button>
  <button class="tab" data-scr="set" onclick="showTab('set')">
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19 12a7 7 0 0 0-.1-1.3l2-1.6-2-3.4-2.4 1a7 7 0 0 0-2.2-1.3L14 2h-4l-.3 2.4a7 7 0 0 0-2.2 1.3l-2.4-1-2 3.4 2 1.6A7 7 0 0 0 5 12a7 7 0 0 0 .1 1.3l-2 1.6 2 3.4 2.4-1a7 7 0 0 0 2.2 1.3L10 22h4l.3-2.4a7 7 0 0 0 2.2-1.3l2.4 1 2-3.4-2-1.6A7 7 0 0 0 19 12z"/></svg><span>Настройки</span></button>
</nav>


<div id="toast"></div>

<script>
// ============ FX справочник (теперь 16 эффектов) ============
var FX=[
  {id:0,name:"Статика",cls:"p-static"},
  {id:1,name:"Дыхание",cls:"p-breathe"},
  {id:2,name:"Радуга",cls:"p-rainbow"},
  {id:3,name:"Огонь",cls:"p-fire"},
  {id:4,name:"Свеча",cls:"p-candle"},
  {id:5,name:"Перелив",cls:"p-flow"},
  {id:6,name:"Сев. сияние",cls:"p-aurora"},
  {id:7,name:"Океан",cls:"p-ocean"},
  {id:8,name:"Полиция",cls:"p-police"},
  {id:9,name:"Мигалка",cls:"p-police-flash"},
  {id:10,name:"Метеор",cls:"p-meteor"},
  {id:11,name:"Стробоскоп",cls:"p-strobe"},
  {id:12,name:"Гирлянда",cls:"p-running"},
  {id:13,name:"Ночное небо",cls:"p-nightsky"},
  {id:14,name:"Лазер",cls:"p-laser"},
  {id:15,name:"Вспышки",cls:"p-flash"}
];
var FXNAME={};FX.forEach(function(f){FXNAME[f.id]=f.name;});
var SWATCHES=["#ffffff","#ffb000","#ff3b30","#ff2d92","#7c3aed","#3b82f6","#00d4ff","#22c55e"];
var WIFI_LABELS={connected:"Подключено",connecting:"Подключение...",ap_mode:"Точка доступа",disconnected:"Не подключено"};
var state={on:false,brightness:150,effect:0,speed:128};
var ws=null;

function $(id){return document.getElementById(id);}
function toast(msg,type){var t=$("toast");t.textContent=msg;t.className="show "+(type||"");setTimeout(function(){t.className="";},2200);}

function api(path,method,body){
  return fetch(path,{method:method||"GET",headers:{"Content-Type":"application/json"},
    body:body?JSON.stringify(body):undefined})
    .then(function(r){return r.json();})
    .catch(function(e){toast("Ошибка связи","err");throw e;});
}
function debounce(fn,ms){var t;return function(){var a=arguments,c=this;clearTimeout(t);t=setTimeout(function(){fn.apply(c,a);},ms);};}

// ============ WebSocket ============
function connectWS(){
  try{
    ws=new WebSocket("ws://"+location.host+"/ws");
    ws.onmessage=function(e){try{applyStatus(JSON.parse(e.data));}catch(ex){}};
    ws.onclose=function(){ws=null;setTimeout(connectWS,3000);};
    ws.onerror=function(){if(ws)ws.close();};
  }catch(e){setTimeout(connectWS,3000);}
}

// ============ Навигация ============
function showTab(name){
  document.querySelectorAll(".screen").forEach(function(s){s.classList.remove("active");});
  $("scr-"+name).classList.add("active");
  document.querySelectorAll(".tabbar .tab").forEach(function(t){
    t.classList.toggle("active",t.getAttribute("data-scr")===name);});
  localStorage.setItem("tab",name);
  if(name==="fx")renderFx();
  if(name==="fav")loadFavorites();
  if(name==="set"){loadSettings();loadSchedules();}
}

// ============ Питание ============
function togglePower(){
  state.on=!state.on;renderPower();
  api("/api/power","POST",{on:state.on}).then(applyStatus);
}
function renderPower(){
  var b=$("powerBtn");
  b.classList.toggle("active",state.on);
  $("powerLabel").textContent=state.on?"Включено":"Выключено";
}

// ============ Яркость ============
var sendBright=debounce(function(v){api("/api/brightness","POST",{value:parseInt(v)});},400);
function onBrightInput(v){
  $("brightVal").textContent=Math.round(v/255*100)+"%";
  state.brightness=parseInt(v);sendBright(v);
}

// ============ Цвет ============
function hexToRgb(h){h=h.replace("#","");return{r:parseInt(h.substr(0,2),16),g:parseInt(h.substr(2,2),16),b:parseInt(h.substr(4,2),16)};}
var sendColor=debounce(function(rgb){api("/api/color","POST",rgb);},400);
function onColorInput(hex){
  $("colorPrev").style.background=hex;$("colorHex").textContent=hex.toUpperCase();sendColor(hexToRgb(hex));
}
function pickSwatch(hex){$("colorPick").value=hex;onColorInput(hex);}

// ============ Эффекты ============
function renderFx(){
  var g=$("fxGrid");if(g.childElementCount)return updateFxSel();
  g.innerHTML="";
  FX.forEach(function(f){
    var d=document.createElement("div");
    d.className="fx-card";d.setAttribute("data-fx",f.id);
    d.innerHTML='<div class="fx-prev"><div class="'+f.cls+'" style="width:100%;height:100%"></div></div><div class="fx-name">'+f.name+'</div>';
    // Превью при долгом нажатии (500 мс)
    var tmr;
    d.addEventListener("touchstart",function(){tmr=setTimeout(function(){previewEffect(f.id);},500);},{passive:true});
    d.addEventListener("touchend",function(){clearTimeout(tmr);});
    d.onclick=function(){setEffect(f.id);};
    g.appendChild(d);
  });
  updateFxSel();
}
function updateFxSel(){
  document.querySelectorAll(".fx-card").forEach(function(c){
    c.classList.toggle("sel",parseInt(c.getAttribute("data-fx"))===state.effect);});
}
function previewEffect(id){api("/api/effect","POST",{id:id,speed:state.speed});}
function setEffect(id){
  state.effect=id;updateFxSel();
  if(!state.on){state.on=true;renderPower();}
  api("/api/effect","POST",{id:id,speed:state.speed}).then(applyStatus);
  toast("Эффект: "+FXNAME[id],"ok");
}
var sendSpeed=debounce(function(v){api("/api/effect","POST",{id:state.effect,speed:parseInt(v)});},400);
function onSpeedInput(v){state.speed=parseInt(v);sendSpeed(v);}

// ============ Wi-Fi ============
function scanWifi(){
  var l=$("netList");l.innerHTML='<div class="muted" style="margin-top:12px"><span class="spin"></span> Поиск сетей...</div>';
  api("/api/wifi/scan").then(function(nets){
    if(!nets||!nets.length){l.innerHTML='<div class="muted" style="margin-top:12px">Сети не найдены, повторите ещё раз</div>';return;}
    l.innerHTML="";
    nets.sort(function(a,b){return b.rssi-a.rssi;}).forEach(function(n){
      var d=document.createElement("div");d.className="net";
      d.innerHTML='<div><div class="n-ssid">'+(n.secure?"🔒 ":"")+n.ssid+'</div><div class="n-rssi">'+n.rssi+' dBm</div></div><div class="muted">→</div>';
      d.onclick=function(){connectWifiNet(n.ssid,n.secure);};
      l.appendChild(d);
    });
  });
}
function connectWifiNet(ssid,secure){
  var pass="";
  if(secure){pass=prompt('Пароль для сети "'+ssid+'":',[]);if(pass===null)return;}
  toast("Подключение к "+ssid+"...");
  api("/api/wifi/connect","POST",{ssid:ssid,pass:pass}).then(function(r){
    toast(r.ok?"Данные сохранены, подключаемся":"Ошибка",r.ok?"ok":"err");
  });
}

// ============ Таймер сна ============
function setTimer(min){
  api("/api/timer","POST",{minutes:min}).then(applyStatus);
  toast(min?"Таймер: "+min+" мин":"Таймер отменён","ok");
}

// ============ Избранное ============

function loadFavorites(){
  api("/api/favorites").then(renderFavorites);
}
function renderFavorites(favs){
  var g=$("favGrid");if(!g)return;
  g.innerHTML="";
  favs.forEach(function(f){
    var d=document.createElement("div");
    d.className="fav-card";
    if(f.used){
      d.innerHTML='<div class="fav-color-dot" style="background:'+f.color+'"></div>'+
        '<div class="fav-name">'+(f.name||"Сцена "+(f.slot+1))+'</div>'+
        '<div class="fav-meta">'+(FXNAME[f.effect]||"")+" · "+Math.round(f.brightness/2.55)+"%</div>"+
        '<div style="display:flex;gap:8px;margin-top:10px">'+
        '<button class="btn small" onclick="favLoad('+f.slot+')">▶ Применить</button>'+
        '<button class="btn ghost small" onclick="favDelete('+f.slot+')">удалить</button></div>';
    }else{
      d.className+=" fav-empty";
      d.innerHTML='<div style="font-size:28px">+</div>'+
        '<div style="font-size:13px;color:var(--muted);margin-top:4px">Слот '+(f.slot+1)+"</div>"+
        '<button class="btn small" style="margin-top:10px" onclick="favSave('+f.slot+')">Сохранить сцену</button>';
    }
    g.appendChild(d);
  });
}
function favLoad(slot){
  api("/api/favorites/load","POST",{slot:slot}).then(function(s){applyStatus(s);loadFavorites();});
  toast("Загружена сцена","ok");
}
function favSave(slot){
  var name=prompt("Название сцены:","Сцена "+(slot+1));
  if(name===null)return;
  api("/api/favorites/save","POST",{slot:slot,name:name}).then(renderFavorites);
  toast("Сцена сохранена","ok");
}
function favDelete(slot){
  if(!confirm("Удалить слот "+(slot+1)+"?"))return;
  api("/api/favorites/delete","POST",{slot:slot}).then(renderFavorites);
}

// ============ Расписание ============
function loadSchedules(){
  api("/api/schedules").then(renderSchedules);
}
function renderSchedules(scheds){
  var g=$("schedGrid");if(!g)return;
  g.innerHTML="";
  var days=["Пн","Вт","Ср","Чт","Пт","Сб","Вс"];
  scheds.forEach(function(s){
    var daysHtml=days.map(function(d,i){
      return '<label style="font-size:11px;display:inline-flex;align-items:center;gap:3px">'+
        '<input type="checkbox"'+(s.days&(1<<i)?" checked":"")+
        ' onchange="schedDayToggle('+s.slot+','+i+',this.checked)"> '+d+"</label>";
    }).join(" ");
    var row=document.createElement("div");
    row.className="sched-row";
    row.innerHTML=
      '<div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap;margin-bottom:8px">'+
      '<input type="checkbox"'+(s.enabled?" checked":"")+' onchange="schedToggle('+s.slot+',this.checked)" style="width:auto;margin:0">'+
      '<input type="number" min="0" max="23" value="'+s.hour+'" style="width:60px;margin:0" id="sh'+s.slot+'" placeholder="Ч">'+
      '<span style="color:var(--muted)">:</span>'+
      '<input type="number" min="0" max="59" value="'+s.minute+'" style="width:60px;margin:0" id="sm'+s.slot+'" placeholder="М">'+
      '<select id="sa'+s.slot+'" style="width:auto;margin:0">'+
        '<option value="1"'+(s.action?" selected":"")+'>ВКЛ</option>'+
        '<option value="0"'+(!s.action?" selected":"")+'>ВЫКЛ</option>'+
      "</select>"+
      '<button class="btn small" onclick="schedSave('+s.slot+')">Сохранить</button>'+
      '</div>'+
      '<div style="display:flex;gap:8px;flex-wrap:wrap">'+daysHtml+"</div>";
    g.appendChild(row);
  });
}
function schedToggle(slot,enabled){api("/api/schedules","POST",{slot:slot,enabled:enabled});}
function schedSave(slot){
  var h=parseInt($("sh"+slot).value)||0;
  var m=parseInt($("sm"+slot).value)||0;
  var a=parseInt($("sa"+slot).value);
  api("/api/schedules","POST",{slot:slot,enabled:true,hour:h,minute:m,action:!!a}).then(renderSchedules);
  toast("Расписание сохранено","ok");
}
function schedDayToggle(slot,day,checked){
  api("/api/schedules").then(function(scheds){
    var s=scheds.find(function(x){return x.slot===slot;});
    if(!s)return;
    var d=s.days;if(checked)d|=(1<<day);else d&=~(1<<day);
    api("/api/schedules","POST",{slot:slot,days:d});
  });
}

// ============ Настройки ============
function buildTz(){
  var s=$("setTz");for(var i=-12;i<=14;i++){var o=document.createElement("option");o.value=i;o.textContent="UTC"+(i>=0?"+":"")+i;s.appendChild(o);}
}
function saveSettings(){
  var body={deviceName:$("setName").value,timezone:parseInt($("setTz").value),powerOnMode:parseInt($("setPom").value)};
  api("/api/settings","POST",body).then(function(){toast("Настройки сохранены","ok");loadSettings();});
}
function resetSettings(){
  if(!confirm("Сбросить все настройки к заводским?"))return;
  api("/api/settings/reset","POST").then(function(){toast("Сброшено","ok");setTimeout(loadAll,1200);});
}
function loadSettings(){
  api("/api/settings").then(function(s){
    $("setName").value=s.deviceName||"";
    $("setTz").value=s.timezone;
    $("setPom").value=s.powerOnMode;
  });
}

// ============ Применение статуса ============
function applyStatus(s){
  if(!s)return;
  state.on=s.on;state.brightness=s.brightness;state.effect=s.effect;state.speed=s.speed;
  renderPower();
  $("bright").value=s.brightness;$("brightVal").textContent=Math.round(s.brightness/255*100)+"%";
  $("speed").value=s.speed;
  if(s.color){$("colorPrev").style.background=s.color;$("colorHex").textContent=s.color.toUpperCase();$("colorPick").value=s.color;}
  // батарея
  $("battPct").textContent=s.battery;$("battFill").style.width=s.battery+"%";
  $("battVolt").textContent=(s.voltage||0).toFixed(2);
  $("battChg").textContent=s.charging?"⚡":"";
  var f=$("battFill");f.style.background=s.battery>50?"#22c55e":(s.battery>20?"#f59e0b":"#ef4444");

  // wi-fi: теперь machine-readable коды
  var wLabel=WIFI_LABELS[s.wifiStatus]||s.wifiStatus||"—";
  $("devName").textContent=s.deviceName;$("verTxt").textContent="v"+s.version;
  $("wifiTxt").textContent=wLabel;
  var dot=$("wifiDot");dot.className="dot"+(s.apMode?" ap":(s.wifiStatus==="connected"?" on":""));
  $("curEffect").textContent=FXNAME[s.effect]||"—";
  $("curWifi").textContent=wLabel;$("curIp").textContent=s.ip;
  $("setWifi").textContent=s.apMode?"Режим точки доступа":(s.wifiStatus==="connected"?s.ip:"Не подключено");
  $("iVer").textContent=s.version;$("iIp").textContent=s.ip;$("iMac").textContent=s.mac;
  $("iRssi").textContent=s.rssi?s.rssi+" dBm":"—";
  $("iHeap").textContent=Math.round(s.freeHeap/1024)+" КБ";
  $("iNtp").textContent=s.timeSynced?"✓ Синхронизировано":"✗ Нет связи";
  updateFxSel();
  // таймер сна
  var tb=$("timerBadge");
  if(s.timerLeft>=0){
    var mm=Math.floor(s.timerLeft/60),ss=s.timerLeft%60;
    tb.textContent=mm+":"+(ss<10?"0":"")+ss;
    tb.style.display="";
  }else{
    tb.style.display="none";
  }
}

function loadAll(){

  api("/api/status").then(applyStatus);
  loadSettings();
}

function toggleFs(){
  if(!document.fullscreenElement){
    var el=document.documentElement;
    if(el.requestFullscreen)el.requestFullscreen().catch(function(){});
    else if(el.webkitRequestFullscreen)el.webkitRequestFullscreen();
  }else{
    if(document.exitFullscreen)document.exitFullscreen().catch(function(){});
    else if(document.webkitExitFullscreen)document.webkitExitFullscreen();
  }
}

// ============ Инициализация ============

(function init(){
  var sw=$("swatches");SWATCHES.forEach(function(c){var d=document.createElement("div");d.className="sw";d.style.background=c;d.onclick=function(){pickSwatch(c);};sw.appendChild(d);});
  buildTz();
  renderFx();
  var t=localStorage.getItem("tab");if(t)showTab(t);
  connectWS();
  loadFavorites();
  loadSchedules();
  loadAll();
  if('serviceWorker' in navigator){navigator.serviceWorker.register('/sw.js').catch(function(){});}
})();
</script>

)HTMLPAGE";

// ---------------------------------------------------------------------
//  СТРАНИЦА РАЗРАБОТЧИКА (/dev)
// ---------------------------------------------------------------------
const char DEV_HTML[] PROGMEM = R"HTMLDEV(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Режим разработчика — Такси Шашка</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#000;color:#0f0;font-family:'Courier New',monospace;padding:16px;font-size:14px;line-height:1.6}
h1{font-size:18px;color:#0f0;margin-bottom:4px;text-shadow:0 0 8px #0f0}
.sub{color:#0a0;font-size:12px;margin-bottom:18px}
.panel{border:1px solid #0f0;border-radius:6px;padding:14px;margin-bottom:14px;background:rgba(0,30,0,.2)}
.panel h2{font-size:13px;color:#0f0;margin-bottom:10px;text-transform:uppercase;letter-spacing:1px}
.row{display:flex;justify-content:space-between;padding:3px 0;border-bottom:1px dotted #030}
.row .k{color:#0a0}.row .v{color:#0f0;font-weight:bold}
button{background:#001a00;color:#0f0;border:1px solid #0f0;padding:9px 14px;border-radius:5px;font-family:inherit;font-size:13px;cursor:pointer;margin:4px 4px 4px 0;transition:.2s}
button:hover{background:#0f0;color:#000}
button.warn{border-color:#f80;color:#f80}
button.warn:hover{background:#f80;color:#000}
button.err{border-color:#f00;color:#f00}
button.err:hover{background:#f00;color:#000}
input{background:#001a00;color:#0f0;border:1px solid #0a0;padding:8px;border-radius:5px;font-family:inherit;width:100%;margin:6px 0}
#log{background:#000;border:1px solid #030;border-radius:5px;padding:10px;height:220px;overflow-y:auto;font-size:12px;color:#0a0;white-space:pre-wrap}
a{color:#0af}
</style>
</head>
<body>
<h1>&gt; РЕЖИМ РАЗРАБОТЧИКА</h1>
<div class="sub">Такси Шашка · диагностика в реальном времени · <a href="/">← назад в приложение</a></div>

<div class="panel">
  <h2>Телеметрия</h2>
  <div class="row"><span class="k">Напряжение АКБ</span><span class="v"><span id="volt">—</span> В</span></div>
  <div class="row"><span class="k">Заряд</span><span class="v"><span id="pct">—</span> %</span></div>
  <div class="row"><span class="k">Зарядка</span><span class="v" id="chg">—</span></div>
  <div class="row"><span class="k">Свободная память</span><span class="v"><span id="heap">—</span> байт</span></div>
  <div class="row"><span class="k">Wi-Fi RSSI</span><span class="v"><span id="rssi">—</span> dBm</span></div>
  <div class="row"><span class="k">IP-адрес</span><span class="v" id="ip">—</span></div>
  <div class="row"><span class="k">MAC</span><span class="v" id="mac">—</span></div>
  <div class="row"><span class="k">Версия прошивки</span><span class="v" id="ver">—</span></div>
  <div class="row"><span class="k">Текущий эффект</span><span class="v" id="fx">—</span></div>
  <div class="row"><span class="k">Состояние ленты</span><span class="v" id="ledstate">—</span></div>
</div>

<div class="panel">
  <h2>Действия</h2>
  <button onclick="testLed()">Тест LED (радуга 5с)</button>
  <button class="err" onclick="reboot()">Перезагрузить устройство</button>
  <div style="margin-top:12px">
    <input type="text" id="otaUrl" placeholder="https://.../firmware.bin">
    <button class="warn" onclick="ota()">OTA по URL</button>
  </div>
</div>

<div class="panel">
  <h2>Лог событий</h2>
  <div id="log"></div>
</div>

<script>
function $(i){return document.getElementById(i);}
function api(p,m,b){return fetch(p,{method:m||"GET",headers:{"Content-Type":"application/json"},body:b?JSON.stringify(b):undefined}).then(function(r){return r.json();});}
function poll(){
  api("/api/status").then(function(s){
    $("volt").textContent=(s.voltage||0).toFixed(3);
    $("pct").textContent=s.battery;
    $("chg").textContent=s.charging?"ДА ⚡":"нет";
    $("heap").textContent=s.freeHeap;
    $("rssi").textContent=s.rssi;
    $("ip").textContent=s.ip;
    $("mac").textContent=s.mac;
    $("ver").textContent=s.version;
    $("fx").textContent=s.effect;
    $("ledstate").textContent=s.on?"ВКЛ":"ВЫКЛ";
  });
}
function loadLog(){
  api("/api/log").then(function(a){
    $("log").textContent=(a||[]).join("\n");
    $("log").scrollTop=$("log").scrollHeight;
  });
}
function testLed(){api("/api/test/led","POST").then(function(){});}
function reboot(){if(confirm("Перезагрузить устройство?"))api("/api/reboot","POST");}
function ota(){var u=$("otaUrl").value;if(!u)return;if(confirm("Начать OTA-обновление?\n"+u))api("/api/ota/url","POST",{url:u});}
poll();loadLog();
setInterval(poll,2000);
setInterval(loadLog,3000);
</script>
</body>
</html>
)HTMLDEV";

// ---------------------------------------------------------------------
//  PWA MANIFEST (/manifest.json)
// ---------------------------------------------------------------------
const char MANIFEST_JSON[] PROGMEM = R"JSON({
  "name": "Такси Шашка",
  "short_name": "TaxiLight",
  "start_url": "/",
  "scope": "/",
  "id": "/",
  "display": "fullscreen",
  "display_override": ["fullscreen", "standalone", "minimal-ui"],
  "background_color": "#0c0c14",
  "theme_color": "#0c0c14",
  "orientation": "portrait-primary",
  "icons": [
    {
      "src": "/icon.png",
      "sizes": "192x192",
      "type": "image/png",
      "purpose": "any maskable"
    },
    {
      "src": "/icon.svg",
      "sizes": "512x512",
      "type": "image/svg+xml",
      "purpose": "any maskable"
    }
  ]
})JSON";

// ---------------------------------------------------------------------
//  SERVICE WORKER (/sw.js)
// ---------------------------------------------------------------------
const char SW_JS[] PROGMEM = R"JS(
self.addEventListener('install', function(e) { self.skipWaiting(); });
self.addEventListener('activate', function(e) { e.waitUntil(clients.claim()); });
self.addEventListener('fetch', function(e) {
  e.respondWith(fetch(e.request).catch(function() { return caches.match(e.request); }));
});
)JS";

// ---------------------------------------------------------------------
//  РАСТРОВАЯ ИКОНКА PNG (/icon.png)
// ---------------------------------------------------------------------
#include "icon_png.inl"

// ---------------------------------------------------------------------
//  МИНИМАЛИСТИЧНАЯ ВЕКТОРНАЯ ИКОНКА (/icon.svg)
// ---------------------------------------------------------------------
const char ICON_SVG[] PROGMEM = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512">
  <style>
    .bg { fill: #0c0c14; }
    .sq-y { fill: #ffb800; }
    .sq-d { fill: #222234; }
    @media (prefers-color-scheme: light) {
      .bg { fill: #f4f4f8; }
      .sq-y { fill: #e69500; }
      .sq-d { fill: #2c2c3e; }
    }
  </style>
  <rect width="512" height="512" class="bg"/>
  <g transform="translate(106, 175)">
    <rect x="0" y="0" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="61" y="0" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="122" y="0" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="183" y="0" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="244" y="0" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="0" y="56" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="61" y="56" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="122" y="56" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="183" y="56" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="244" y="56" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="0" y="112" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="61" y="112" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="122" y="112" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="183" y="112" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="244" y="112" width="56" height="50" rx="8" class="sq-y"/>
  </g>
</svg>
)SVG";

