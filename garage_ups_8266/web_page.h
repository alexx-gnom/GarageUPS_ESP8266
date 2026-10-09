
#pragma once

const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Garage UPS</title>

<style>
* { box-sizing: border-box; }

body {
    margin: 0;
    padding: 20px;
    background: #111820;
    color: #e8edf2;
    font-family: Arial, sans-serif;
}

main {
    max-width: 760px;
    margin: auto;
}

h1 { margin-bottom: 6px; }

.subtitle {
    color: #9baaba;
    margin-bottom: 22px;
}

.grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(210px, 1fr));
    gap: 12px;
}

.card {
    background: #202b37;
    border: 1px solid #344252;
    border-radius: 12px;
    padding: 16px;
}

.label {
    color: #aab7c5;
    font-size: 14px;
}

.value {
    margin-top: 8px;
    font-size: 27px;
    font-weight: bold;
}

.unit {
    color: #aab7c5;
    font-size: 14px;
}

button {
    border: 0;
    border-radius: 8px;
    padding: 11px 14px;
    color: white;
    background: #465466;
    font-size: 15px;
    cursor: pointer;
}

button.on { background: #16834a; }
button.off { background: #465466; }
button:disabled { opacity: 0.5; }

.output {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 12px;
    margin-top: 14px;
}

.status {
    color: #9baaba;
    font-size: 13px;
    margin-top: 18px;
}

@media (max-width: 420px) {
    body { padding: 12px; }
    .value { font-size: 24px; }
}
</style>
</head>

<body>
<main>
    <h1>🔋 Garage UPS</h1>
    <div class="subtitle">ESP8266 · локальний моніторинг</div>

    <div class="grid">
        <div class="card">
            <div class="label">Живлення зарядки</div>
            <div class="value"><span id="charger">--</span> <span class="unit">V</span></div>
        </div>

        <div class="card">
            <div class="label">Акумулятор</div>
            <div class="value"><span id="battery">--</span> <span class="unit">V</span></div>
        </div>

        <div class="card">
            <div class="label">Струм</div>
            <div class="value"><span id="current">--</span> <span class="unit">A</span></div>
        </div>

        <div class="card">
            <div class="label">Температура NTC</div>
            <div class="value"><span id="ntc">--</span> <span class="unit">°C</span></div>
        </div>

        <div class="card">
            <div class="label">Температура DS18B20</div>
            <div class="value"><span id="ds18">--</span> <span class="unit">°C</span></div>
        </div>
    </div>

    <div class="card" style="margin-top:12px">
        <h2>Канали керування</h2>

        <div class="output">
            <span>SAFETY · GPIO12</span>
            <button id="safety" onclick="toggleOutput('safety')">OFF</button>
        </div>

        <div class="output">
            <span>LOAD 0 · GPIO14</span>
            <button id="load0" onclick="toggleOutput('load0')">OFF</button>
        </div>

        <div class="output">
            <span>LOAD 1 · GPIO16</span>
            <button id="load1" onclick="toggleOutput('load1')">OFF</button>
        </div>
    </div>

    <div class="status">
        ESP: <span id="connection">підключення...</span>
        · Оновлення раз на секунду
    </div>
</main>

<script>
function setButton(id, enabled) {
    const button = document.getElementById(id);
    button.textContent = enabled ? "ON" : "OFF";
    button.className = enabled ? "on" : "off";
}

async function updateStatus() {
    try {
        const response = await fetch("/api/status", {
            cache: "no-store"
        });

        if (!response.ok) throw new Error("HTTP error");

        const data = await response.json();

        document.getElementById("charger").textContent =
            Number(data.charger).toFixed(2);

        document.getElementById("battery").textContent =
            Number(data.battery).toFixed(2);

        document.getElementById("current").textContent =
            Number(data.current).toFixed(2);

        document.getElementById("ntc").textContent =
            Number(data.ntc).toFixed(1);

        document.getElementById("ds18").textContent =
            Number(data.ds18).toFixed(2);

        setButton("safety", data.safety);
        setButton("load0", data.load0);
        setButton("load1", data.load1);

        document.getElementById("connection").textContent = "ONLINE";
    } catch (error) {
        document.getElementById("connection").textContent = "OFFLINE";
    }
}

async function toggleOutput(name) {
    const button = document.getElementById(name);
    button.disabled = true;

    try {
        const response = await fetch(
            "/api/" + name + "/toggle",
            { method: "POST" }
        );

        if (!response.ok) throw new Error("HTTP error");

        await updateStatus();
    } catch (error) {
        document.getElementById("connection").textContent =
            "Помилка команди";
    } finally {
        button.disabled = false;
    }
}

setInterval(updateStatus, 1000);
updateStatus();
</script>
</body>
</html>
)rawliteral";
