#pragma once

const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Gn.O.M · Garage UPS</title>
<style>
:root {
  color-scheme: dark;
  --bg:#11100e; --panel:#24201b; --panel2:#30271e;
  --brass:#bd9250; --copper:#b96e43; --gold:#f0c77c;
  --text:#f2e7d2; --muted:#b9a78d; --green:#80d39b; --red:#ff8b72;
}
* { box-sizing:border-box; }
body {
  margin:0; padding:22px; color:var(--text); font-family:Georgia,"Trebuchet MS",sans-serif;
  background:
    radial-gradient(ellipse at 18% 0%, rgba(164,112,52,.18), transparent 42%),
    radial-gradient(ellipse at 90% 30%, rgba(120,65,35,.15), transparent 38%),
    repeating-linear-gradient(135deg,#171512 0,#171512 3px,#191713 3px,#191713 6px);
}
main { max-width:850px; margin:auto; }
h1 { margin:0 0 5px; color:var(--gold); letter-spacing:.5px; font-size:clamp(25px,5vw,35px); text-shadow:0 2px 0 #4a2d16,0 0 18px #a56c2940; }
h2 { color:var(--gold); font-size:20px; margin:0 0 14px; }
.subtitle { color:var(--muted); margin-bottom:22px; letter-spacing:1.2px; text-transform:uppercase; font-size:12px; }
.grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(210px,1fr)); gap:12px; }
.card {
  position:relative; overflow:hidden; background:linear-gradient(145deg,rgba(57,46,34,.98),rgba(30,27,23,.98));
  border:1px solid #765832; border-radius:10px; padding:16px;
  box-shadow:inset 0 1px 0 #ffffff0d,inset 0 0 0 3px #00000024,0 5px 14px #00000035;
}
.card:before { content:""; position:absolute; left:7px; right:7px; top:5px; height:1px; background:linear-gradient(90deg,transparent,#cba66a80,transparent); pointer-events:none; }
.label { color:#c4ad8b; font-size:13px; text-transform:uppercase; letter-spacing:.6px; }
.value { margin-top:8px; font-size:29px; font-weight:bold; color:#f4d9a4; text-shadow:0 0 12px #c4873b25; }
.unit { color:#b9a78d; font-size:14px; }
button {
  border:1px solid #a87c43; border-radius:6px; padding:11px 14px; color:#fff0d2;
  background:linear-gradient(#72502e,#49311e); box-shadow:inset 0 1px 0 #ffffff26,0 2px 4px #0008;
  font: bold 14px Georgia,serif; cursor:pointer; transition:filter .15s,transform .15s;
}
button:hover { filter:brightness(1.2); } button:active { transform:translateY(1px); }
button.on { background:linear-gradient(#31784a,#19482d); border-color:#72b985; color:#e2ffe9; }
button.off { background:linear-gradient(#66503a,#392b20); }
button:disabled { opacity:.48; cursor:wait; }
button.small { padding:7px 10px; font-size:12px; }
.output { display:flex; justify-content:space-between; align-items:center; gap:12px; margin-top:14px; flex-wrap:wrap; }
.outputInfo { display:flex; flex-direction:column; gap:5px; }
.actual { color:#b9a78d; font-size:12px; line-height:1.45; }
.actions { display:flex; gap:8px; align-items:center; }
.alarm { border-color:#a54835; background:linear-gradient(145deg,#4b211b,#2a1b18); }
.alarm.normal { border-color:#497a4f; background:linear-gradient(145deg,#253c2b,#1d2920); }
#faultTitle { font-weight:bold; font-size:18px; }
.status { color:#b9a78d; font-size:12px; margin-top:18px; text-align:center; letter-spacing:.3px; }
#clockTime { color:#f5ce87; font-family:"Courier New",monospace; letter-spacing:1px; }
#connection { color:var(--green); font-weight:bold; }
#protectionCard { margin-bottom:12px; }
@media (max-width:420px) {
  body { padding:12px; } .value { font-size:24px; } .card { padding:13px; }
  .actions { width:100%; justify-content:flex-start; } h2 { font-size:18px; }
}
</style>
</head>
<body>
<main>
    <h1>⚙️ Gn.O.M Garage UPS</h1>
    <div class="subtitle">ESP8266 · локальний моніторинг</div>

    <div class="card" style="margin-bottom:12px">
        <div class="output" style="margin-top:0">
            <div class="outputInfo"><strong>Годинник ESP · Київ</strong><span id="clockDate" class="actual">Очікування NTP…</span></div>
            <div class="value" id="clockTime" style="margin-top:0">--:--:--</div>
        </div>
        <div class="actual" style="margin-top:8px">NTP: <span id="ntpStatus">перевірка…</span> · Uptime: <span id="uptime">—</span></div>
        <div class="actual">Остання відповідь ESP: <span id="lastResponse">—</span></div>
    </div>

    <div class="card" style="margin-bottom:12px">
        <div class="output" style="margin-top:0">
            <div class="outputInfo"><strong>Telegram</strong><span class="actual">Надіслати поточний стан UPS у бот</span></div>
            <button id="telegramReportButton" class="small" onclick="requestTelegramReport()">Звіт у Telegram</button>
        </div>
        <div class="actual" id="telegramReportResult" style="margin-top:8px">Щоденний звіт заплановано на 08:00.</div>
    </div>

    <div id="protectionCard" class="card alarm" style="margin-bottom:12px">
        <h2>Температурний захист</h2>
        <div id="faultTitle">Перевірка датчиків…</div>
        <div class="actual" id="faultDetails">Виходи вимкнені до підтвердження безпечної температури.</div>
    </div>

    <div class="grid">
        <div class="card"><div class="label">Напруга зарядного джерела</div><div class="value"><span id="charger">—</span> <span class="unit">V</span></div></div>
        <div class="card"><div class="label">Напруга акумулятора</div><div class="value"><span id="battery">—</span> <span class="unit">V</span></div></div>
        <div class="card"><div class="label">Струм заряду</div><div class="value"><span id="current">—</span> <span class="unit">A</span></div></div>
        <div class="card"><div class="label">Температура акумулятора</div><div class="value"><span id="ntc">—</span> <span class="unit">°C</span></div></div>
        <div class="card"><div class="label">Зовнішня температура</div><div class="value"><span id="ds18">—</span> <span class="unit">°C</span></div></div>
    </div>

    <div class="card" style="margin-top:12px">
        <h2>Канали навантаження</h2>
        <div class="output">
            <div class="outputInfo"><strong>Канал 1 ·</strong><span id="load0Actual" class="actual">Фактично: —</span></div>
            <div class="actions"><button id="load0" onclick="toggleLoad(0)">OFF</button><button class="small" id="load0Restart" onclick="restartLoad(0)">Перезапуск · 7 с</button></div>
        </div>
        <div class="output">
            <div class="outputInfo"><strong>Канал 2 ·</strong><span id="load1Actual" class="actual">Фактично: —</span></div>
            <div class="actions"><button id="load1" onclick="toggleLoad(1)">OFF</button><button class="small" id="load1Restart" onclick="restartLoad(1)">Перезапуск · 7 с</button></div>
        </div>
    </div>

    <div class="card" style="margin-top:12px">
        <div class="output" style="margin-top:0">
            <div class="outputInfo"><strong>Зарядний вихід</strong><span id="chargeActual" class="actual">Фактично: —</span></div>
            <button class="small" id="chargeButton" onclick="toggleCharger()">Зарядка: ON</button>
        </div>
        <div class="actual" style="margin-top:10px">Кнопка зарядки тимчасова: після перезапуску ESP запит автоматично ON, якщо захист не активний.</div>
    </div>

    <div class="status">ESP: <span id="connection">підключення...</span> · Оновлення раз на секунду</div>
</main>
<script>
function setButton(id, enabled, onText = "ON", offText = "OFF") {
    const button = document.getElementById(id);
    button.textContent = enabled ? onText : offText;
    button.className = (enabled ? "on" : "off") + (id === "chargeButton" || id.endsWith("Restart") ? " small" : "");
}
function showNumber(id, value, decimals, clampNegative = false) {
    const element = document.getElementById(id);
    if (value === null || !Number.isFinite(Number(value))) {
        element.textContent = "—";
        return;
    }
    const number = Number(value);
    element.textContent = (clampNegative ? Math.max(0, number) : number).toFixed(decimals);
}
function actualLoadText(data, n) {
    const actual = data["load" + n];
    const desired = data["load" + n + "Desired"];

    if (actual) return "Фактично: ON · вихід увімкнено";
    if (!desired) return "Фактично: OFF · вимкнено вручну";
    if (data.overheat) return "Фактично: OFF · аварія температури";
    if (!data.temperatureOk) return "Фактично: OFF · перевір датчики температури";
    if (data["load" + n + "Restarting"]) return "Фактично: OFF · перезапуск, 7 с";
    if (data.lowBattery && !data.chargerPresent) return "Фактично: OFF · акумулятор нижче 11,2 В";
    if (!data.chargerPresent && !data.batteryOk) return "Фактично: OFF · немає достовірної напруги АКБ";
    if (!data.loadsAllowed) return "Фактично: OFF · захист блокує канал";
    return "Фактично: OFF · перевір вихід і підключення реле";
}
let lastSuccessfulResponse = 0;
function formatUptime(ms) {
    const totalSeconds = Math.floor(Number(ms) / 1000);
    const days = Math.floor(totalSeconds / 86400);
    const hours = Math.floor((totalSeconds % 86400) / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = totalSeconds % 60;
    return (days ? days + " д " : "") + String(hours).padStart(2, "0") + ":" + String(minutes).padStart(2, "0") + ":" + String(seconds).padStart(2, "0");
}
function updateLastResponseLabel() {
    const el = document.getElementById("lastResponse");
    if (!lastSuccessfulResponse) return;
    const age = Math.floor((Date.now() - lastSuccessfulResponse) / 1000);
    el.textContent = age < 2 ? "щойно" : age + " с тому";
    if (age > 5) el.style.color = "#ff7676"; else el.style.color = "#aab7c5";
}
async function updateStatus() {
    try {
        const response = await fetch("/api/status", { cache: "no-store" });
        if (!response.ok) throw new Error("HTTP error");
        const data = await response.json();
        lastSuccessfulResponse = Date.now();
        document.getElementById("clockTime").textContent = data.clockValid ? data.clock.substring(11, 19) : "--:--:--";
        document.getElementById("clockDate").textContent = data.clockValid ? data.clock.substring(0, 10) + " · Europe/Kyiv" : "Час ще не синхронізовано";
        document.getElementById("ntpStatus").textContent = data.clockValid ? "OK" : "очікування синхронізації";
        document.getElementById("ntpStatus").style.color = data.clockValid ? "#55d98b" : "#ffcc66";
        document.getElementById("uptime").textContent = formatUptime(data.uptimeMs);
        updateLastResponseLabel();

        showNumber("charger", data.charger, 2, true);
        showNumber("battery", data.battery, 2, true);
        showNumber("current", data.current, 2, true);
        showNumber("ntc", data.ntc, 1);
        showNumber("ds18", data.ds18, 2);

        setButton("load0", data.load0Desired);
        setButton("load1", data.load1Desired);
        document.getElementById("load0Actual").textContent =
            "Запит: " + (data.load0Desired ? "ON" : "OFF") + " · " + actualLoadText(data, 0);
        document.getElementById("load1Actual").textContent =
            "Запит: " + (data.load1Desired ? "ON" : "OFF") + " · " + actualLoadText(data, 1);
        document.getElementById("load0Restart").disabled = data.load0Restarting;
        document.getElementById("load1Restart").disabled = data.load1Restarting;
        document.getElementById("load0Restart").textContent = data.load0Restarting ? "Очікування…" : "Перезапуск · 7 с";
        document.getElementById("load1Restart").textContent = data.load1Restarting ? "Очікування…" : "Перезапуск · 7 с";

        setButton("chargeButton", data.chargeRequest, "Зарядка: ON", "Зарядка: OFF");
        let chargeState = data.chargeRelay ? "ON" : "OFF";
        if (!data.chargerPresent && !data.overheat && data.temperatureOk) {
            chargeState += " · немає зовнішнього живлення";
        } else if (data.overheat || !data.temperatureOk) {
            chargeState += " · заблоковано захистом";
        } else if (!data.chargeRequest) {
            chargeState += " · вимкнено вручну";
        }
        document.getElementById("chargeActual").textContent = "Фактично: " + chargeState;

        const card = document.getElementById("protectionCard");
        const title = document.getElementById("faultTitle");
        const details = document.getElementById("faultDetails");
        card.className = "card alarm";
        if (data.overheat) {
            title.textContent = "АВАРІЯ: перегрів зафіксовано";
            details.textContent = "Усі виходи вимкнені. Скиньте аварію кнопкою FLASH лише після охолодження нижче 40 °C.";
        } else if (!data.temperatureOk) {
            title.textContent = "ЗАХИСТ: немає достовірних показів температури";
            details.textContent = "Зарядка й навантаження заблоковані до відновлення обох датчиків.";
        } else {
            card.className = "card alarm normal";
            title.textContent = "Температурна аварія не активна";
            if (data.chargerPresent) {
                details.textContent = "Зарядне джерело присутнє. Канали працюють за збереженими налаштуваннями.";
            } else if (data.lowBattery) {
                details.textContent = "Низьковольтне блокування активне: LOAD відновляться після появи зарядного джерела або перезапуску ESP з повторною перевіркою напруги.";
            } else {
                details.textContent = "Робота від акумулятора. Відсічення LOAD нижче 11,2 В.";
            }
        }
        document.getElementById("connection").textContent = "ONLINE";
    } catch (error) {
        document.getElementById("connection").textContent = "OFFLINE";
    }
}
async function postCommand(path, buttonId) {
    const button = document.getElementById(buttonId);
    if (button) button.disabled = true;
    try {
        const response = await fetch(path, { method: "POST" });
        if (!response.ok) throw new Error("Command failed");
        await updateStatus();
    } catch (error) {
        document.getElementById("connection").textContent = "Помилка команди";
    } finally {
        if (button) button.disabled = false;
    }
}
function toggleLoad(n) { return postCommand("/api/load" + n + "/toggle", "load" + n); }
function restartLoad(n) { return postCommand("/api/load" + n + "/restart", "load" + n + "Restart"); }
function toggleCharger() { return postCommand("/api/charger/toggle", "chargeButton"); }
async function requestTelegramReport() {
    const button = document.getElementById("telegramReportButton");
    const result = document.getElementById("telegramReportResult");
    button.disabled = true;
    result.textContent = "Ставимо запит у чергу…";
    try {
        const response = await fetch("/api/telegram/report", { method: "POST" });
        const data = await response.json();
        if (!response.ok || !data.ok) {
            if (data.error === "telegram_not_configured") throw new Error("Спочатку налаштуй токен бота й chat ID у secrets.cpp.");
            throw new Error("Не вдалося поставити звіт у чергу.");
        }
        result.textContent = "Запит у черзі. Доставка в Telegram ще очікує підтвердження.";
    } catch (error) {
        result.textContent = error.message || "Помилка запиту Telegram.";
    } finally {
        button.disabled = false;
    }
}
setInterval(updateStatus, 1000);
setInterval(updateLastResponseLabel, 1000);
updateStatus();
</script>
</body>
</html>
)rawliteral";
