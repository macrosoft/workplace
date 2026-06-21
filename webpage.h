#ifndef WEBPAGE_H
#define WEBPAGE_H

const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>Workplace Control</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <style>
    /* Цветовая палитра: Мята и Олива */
    :root {
      --bg-color: #121612;        /* Очень темная олива (фон) */
      --card-bg: #2a3325;         /* Темная олива (карточки) */
      --mint: #7af5b4;            /* Мятный (акценты) */
      --mint-dim: #4a9e70;        /* Темная мята (кнопки при наведении) */
      --olive-border: #4f6143;    /* Оливковые рамки */
      --text-main: #e8eee4;       /* Светло-серый текст */
      --text-muted: #9ba892;      /* Приглушенный оливковый текст */
    }

    body {
      font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background-color: var(--bg-color);
      color: var(--text-main);
    }

    h1 {
      color: var(--mint);
      text-align: center;
      font-weight: 300;
      letter-spacing: 2px;
      margin-bottom: 30px;
    }

    .container {
      max-width: 500px;
      margin: 0 auto;
    }

    /* Карточки (Блоки) */
    .card {
      background-color: var(--card-bg);
      border: 1px solid var(--olive-border);
      border-radius: 12px;
      padding: 20px;
      margin-bottom: 20px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }

    h2 {
      color: var(--mint);
      margin-top: 0;
      font-size: 1.2em;
      border-bottom: 1px solid var(--olive-border);
      padding-bottom: 10px;
    }

    /* Таблица сенсоров */
    table { width: 100%; border-collapse: collapse; }
    td { padding: 10px 0; border-bottom: 1px dashed var(--olive-border); }
    tr:last-child td { border-bottom: none; }
    .lbl { color: var(--text-muted); font-size: 0.9em; text-transform: uppercase; letter-spacing: 1px; }
    .val { font-size: 1.4em; font-weight: bold; text-align: right; color: var(--text-main); }
    .unit { color: var(--mint); padding-left: 5px; font-size: 0.9em;}

    /* Формы и ползунки */
    label { display: block; margin-top: 15px; color: var(--text-muted); font-size: 0.9em; }
    input[type=number] {
      width: 100%; padding: 10px; margin-top: 5px;
      background: var(--bg-color); border: 1px solid var(--olive-border);
      color: var(--mint); border-radius: 6px; font-size: 1.1em; outline: none;
    }
    
    input[type=submit] {
      width: 100%; background: var(--mint); color: var(--bg-color);
      border: none; border-radius: 6px; padding: 12px; margin-top: 20px;
      font-size: 1.1em; font-weight: bold; cursor: pointer; transition: 0.2s;
    }
    input[type=submit]:hover { background: var(--mint-dim); color: #fff;}

    /* Блок цвета */
    .color-row { display: flex; align-items: center; gap: 20px; margin-top: 15px; }
    .swatch {
      width: 50px; height: 50px; border-radius: 50%;
      border: 3px solid var(--olive-border); flex-shrink: 0;
      box-shadow: 0 0 15px rgba(0,0,0,0.5);
      transition: background-color 0.5s ease;
    }
    
    /* Стилизация ползунка в мятный цвет */
    input[type=range] {
      width: 100%; margin-top: 10px; accent-color: var(--mint);
    }
    
    .note { font-size: 0.8em; color: var(--text-muted); margin-top: 5px; line-height: 1.4; }
    #msg { text-align: center; color: var(--mint); min-height: 20px; margin-top: 10px;}
  </style>
</head>
<body>
  <div class="container">
    <h1>WORKPLACE</h1>

    <!-- БЛОК СЕНСОРОВ -->
    <div class="card">
      <h2>Environment</h2>
      <table>
        <tr><td class="lbl">Indoor</td><td class="val" id="in">--.-</td><td class="unit">&deg;C</td></tr>
        <tr><td class="lbl">Outdoor</td><td class="val" id="out">--</td><td class="unit">&deg;C</td></tr>
        <tr><td class="lbl">Humidity</td><td class="val" id="hum">--</td><td class="unit">%</td></tr>
        <tr><td class="lbl">Light Lvl</td><td class="val" id="light">--</td><td class="unit">lx</td></tr>
      </table>
    </div>

    <!-- БЛОК ПОРОГОВ СВЕТА -->
    <div class="card">
      <h2>Auto Light Thresholds</h2>
      <label>ON Threshold (Darkness)
        <input type="number" id="lo">
      </label>
      <label>OFF Threshold (Brightness)
        <input type="number" id="hi">
      </label>
      <input type="submit" value="Save Thresholds" onclick="save()">
      <p id="msg"></p>
    </div>

    <!-- БЛОК СДВИГА РАДУГИ -->
    <div class="card">
      <h2>LED Ambient Color</h2>
      <p class="note">The color shifts slowly over 6 hours. Use the slider to offset the current hue.</p>
      <div class="color-row">
        <div class="swatch" id="swatch"></div>
        <div style="flex:1">
          <label>Rainbow Offset: <span id="hueVal" style="color:var(--mint); font-weight:bold;">0</span>&deg;</label>
          <!-- oninput меняет цифру на экране, onchange сохраняет на ESP32 (Защита Флеш-памяти!) -->
          <input type="range" id="oh" min="0" max="360" value="0" oninput="updateUI(this.value)" onchange="sendHueToServer(this.value)">
        </div>
      </div>
    </div>
  </div>

  <script>
    // Сохранение порогов света
    function save() {
      var b = 'lo=' + document.getElementById('lo').value + '&hi=' + document.getElementById('hi').value;
      fetch('/save', { method: 'POST', body: b, headers: { 'Content-Type': 'application/x-www-form-urlencoded' } })
        .then(function(r) { 
          var m = document.getElementById('msg');
          m.textContent = 'Settings Saved!';
          setTimeout(() => m.textContent = '', 3000);
        });
    }

    // Мгновенное обновление цифры при перетаскивании ползунка
    function updateUI(v) {
      document.getElementById('hueVal').textContent = v;
    }

    // Отправка на сервер ТОЛЬКО когда ползунок отпустили
    function sendHueToServer(v) {
      fetch('/save', { method: 'POST', body: 'oh=' + (v / 360), headers: { 'Content-Type': 'application/x-www-form-urlencoded' } });
    }

    // Опрос ESP32 каждые 2 секунды
    setInterval(function() {
      fetch('/ajax').then(function(r) { return r.json() }).then(function(d) {
        document.getElementById('in').textContent = d.i;
        document.getElementById('out').textContent = d.o;
        document.getElementById('hum').textContent = d.h;
        document.getElementById('light').textContent = d.l;
        
        // Обновляем поля ввода света ТОЛЬКО если мы в них сейчас не печатаем
        if(document.activeElement.id !== 'lo') document.getElementById('lo').value = d.cl;
        if(document.activeElement.id !== 'hi') document.getElementById('hi').value = d.ch;
        
        // Цвет кружочка отображает текущий реальный цвет ленты
        document.getElementById('swatch').style.background = 'rgb(' + d.r + ',' + d.g + ',' + d.b + ')';
        
        // Обновляем ползунок ТОЛЬКО если мы его сейчас не двигаем
        if(document.activeElement.id !== 'oh') {
          var degrees = Math.round(d.oh * 360);
          document.getElementById('oh').value = degrees;
          document.getElementById('hueVal').textContent = degrees;
        }
      }).catch(e => console.log("ESP offline or busy"));
    }, 2000);
  </script>
</body>
</html>
)=====";

#endif