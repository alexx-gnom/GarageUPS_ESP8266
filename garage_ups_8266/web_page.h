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
body { margin: 0; padding: 20px; background: #111820; color: #e8edf2; font-family: Arial, sans-serif; }
main { max-width: 760px; margin: auto; }
h1 { margin-bottom: 6px; }
.subtitle { color: #9baaba; margin-bottom: 22px; }
.grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(210px, 1fr)); gap: 12px; }
.card { background: #202b37; border: 1px solid #344252; border-radius: 12px; padding: 16px; }
.label { color: #aab7c5; font-size: 14px; }
.value { margin-top: 8px; font-size: 27px; font-weight: bold; }
.unit { color: #aab7c5; font-size: 14px; }
button { border: 0; border-radius: 8px; padding: 11px 14px; color: white; background: #465466; font-size: 15px; cursor: pointer; }
button.on { background: #16834a; }
button.off { background: #465466; }
button:disabled { opacity: 0.5; cursor: wait; }
button.small { padding: 7px 10px; font-size: 13px; }
.output { display: flex; justify-content: space-between; align-items: center; gap: 12px; margin-top: 14px; flex-wrap: wrap; }
.outputInfo { display: flex; flex-direction: column; gap: 5px; }
.actual { color: #aab7c5; font-size: 12px; }
.actions { display: flex; gap: 8px; align-items: center; }
.alarm { border: 1px solid #a93226; background: #481c20; }
.alarm.normal { border-color: #236b45; background: #183b2b; }
.alarm h2 { margin-top: 0; }
#faultTitle { font-weight: bold; font-size: 18px; }
.status { color: #9baaba; font-size: 13px; margin-top: 18px; }
@media (max-width: 420px) { body { padding: 12px; } .value { font-size: 24px; } }
</style>
</head>
<body>
<main>
    <h1>🔋 Garage UPS</h1>
    <div class="subtitle">ESP8266 · локальний моніторинг</div>

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
            <div class="outputInfo"><strong>Канал 1 · Камери</strong><span id="load0Actual" class="actual">Фактично: —</span></div>
            <div class="actions"><button id="load0" onclick="toggleLoad(0)">OFF</button><button class="small" id="load0Restart" onclick="restartLoad(0)">Перезапуск · 7 с</button></div>
        </div>
        <div class="output">
            <div class="outputInfo"><strong>Канал 2 · Роутер</strong><span id="load1Actual" class="actual">Фактично: —</span></div>
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
    if (data.overheat) return "Фактично: OFF · аварія температури";
    if (!data.temperatureOk) return "Фактично: OFF · перевір датчики температури";
    if (data.lowBattery && !data.chargerPresent) return "Фактично: OFF · акумулятор нижче 11,2 В";
    if (!data.chargerPresent && !data.batteryOk) return "Фактично: OFF · немає достовірної напруги АКБ";
    if (data["load" + n + "Restarting"]) return "Фактично: OFF · перезапуск, 7 с";
    if (!data["load" + n + "Desired"]) return "Фактично: OFF · вимкнено вручну";
    return "Фактично: " + (actual ? "ON" : "OFF");
}
async function updateStatus() {
    try {
        const response = await fetch("/api/status", { cache: "no-store" });
        if (!response.ok) throw new Error("HTTP error");
        const data = await response.json();

        showNumber("charger", data.charger, 2, true);
        showNumber("battery", data.battery, 2, true);
        showNumber("current", data.current, 2, true);
        showNumber("ntc", data.ntc, 1);
        showNumber("ds18", data.ds18, 2);

        setButton("load0", data.load0Desired);
        setButton("load1", data.load1Desired);
        document.getElementById("load0Actual").textContent = actualLoadText(data, 0);
        document.getElementById("load1Actual").textContent = actualLoadText(data, 1);
        document.getElementById("load0Restart").disabled = data.load0Restarting;
        document.getElementById("load1Restart").disabled = data.load1Restarting;
        document.getElementById("load0Restart").textContent = data.load0Restarting ? "Очікування…" : "Перезапуск · 7 с";
        document.getElementById("load1Restart").textContent = data.load1Restarting ? "Очікування…" : "Перезапуск · 7 с";

        setButton("chargeButton", data.chargeRequest, "Зарядка: ON", "Зарядка: OFF");
        document.getElementById("chargeActual").textContent = "Фактично: " + (data.chargeRelay ? "ON" : "OFF");

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
setInterval(updateStatus, 1000);
updateStatus();
</script>
</body>
</html>
)rawliteral";
