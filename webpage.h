#ifndef WEBPAGE_H
#define WEBPAGE_H

const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Workplace</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <style>
    body { font-family: Arial; margin: 20px; background: #222; color: #ddd; }
    h1 { color: #0f0; }
    table { width: 100%; border-collapse: collapse; }
    td { padding: 8px 4px; }
    .lbl { color: #999; font-size: .9em; }
    .val { font-size: 1.5em; font-weight: bold; text-align: right; }
    .sec { margin: 20px 0; padding: 15px; background: #333; border-radius: 8px; }
    input { width: 100%; padding: 8px; margin: 4px 0 12px; box-sizing: border-box; }
    input[type=submit] { background: #070; color: #fff; border: 0; padding: 10px; font-size: 1em; cursor: pointer; }
    label { font-weight: bold; }
    .swatch { width: 60px; height: 60px; border-radius: 50%; border: 2px solid #555; flex-shrink: 0; }
    .row { display: flex; align-items: center; gap: 15px; }
    input[type=range] { width: 100%; }
  </style>
</head>
<body>
  <h1>Workplace</h1>
  <div class="sec">
    <table>
      <tr><td class="lbl">Indoor</td><td class="val" id="in">--.-</td><td>&deg;C</td></tr>
      <tr><td class="lbl">Outdoor</td><td class="val" id="out">--</td><td>&deg;C</td></tr>
      <tr><td class="lbl">Humidity</td><td class="val" id="hum">--</td><td>%</td></tr>
      <tr><td class="lbl">Light</td><td class="val" id="light">--</td><td></td></tr>
    </table>
  </div>
  <div class="sec">
    <h2>Light thresholds</h2>
    <label>ON (dark) <input type="number" id="lo"></label>
    <label>OFF (bright) <input type="number" id="hi"></label>
    <input type="submit" value="Apply" onclick="save()">
    <p id="msg"></p>
  </div>
  <div class="sec">
    <h2>LED Color</h2>
    <div class="row">
      <div class="swatch" id="swatch"></div>
      <div style="flex:1">
        <label>Hue offset <span id="hueVal">0</span>&deg;</label>
        <input type="range" id="oh" min="0" max="360" value="0" oninput="setHue(this.value)">
      </div>
    </div>
  </div>
  <script>
    function save() {
      var b = 'lo=' + document.getElementById('lo').value + '&hi=' + document.getElementById('hi').value;
      fetch('/save', { method: 'POST', body: b, headers: { 'Content-Type': 'application/x-www-form-urlencoded' } })
        .then(function(r) { document.getElementById('msg').textContent = 'Saved'; });
    }
    function setHue(v) {
      document.getElementById('hueVal').textContent = v;
      fetch('/save', { method: 'POST', body: 'oh=' + (v / 360), headers: { 'Content-Type': 'application/x-www-form-urlencoded' } });
    }
    setInterval(function() {
      fetch('/ajax').then(function(r) { return r.json() }).then(function(d) {
        document.getElementById('in').textContent = d.i;
        document.getElementById('out').textContent = d.o;
        document.getElementById('hum').textContent = d.h;
        document.getElementById('light').textContent = d.l;
        document.getElementById('lo').value = d.cl; // Загружаем настройки света
        document.getElementById('hi').value = d.ch; // Загружаем настройки света
        document.getElementById('swatch').style.background = 'rgb(' + d.r + ',' + d.g + ',' + d.b + ')';
        document.getElementById('oh').value = Math.round(d.oh * 360);
        document.getElementById('hueVal').textContent = Math.round(d.oh * 360);
      })
    }, 2000);
  </script>
</body>
</html>
)=====";

#endif