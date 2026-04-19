const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <style>
        :root {
            --bg: #0d1117;
            --card: #161b22;
            --text: #c9d1d9;
            --accent: #58a6ff;
            --auto-color: #a371f7;
            --joy-bg: #21262d;
            --status-ok: #3fb950;
        }

        * {
            -webkit-tap-highlight-color: transparent;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            -web-kit-user-select: none;
            user-select: none;
            box-sizing: border-box;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background-color: var(--bg);
            color: var(--text);
            margin: 0;
            display: flex;
            flex-direction: column;
            align-items: center;
            height: 100vh;
            overflow: hidden;
        }

        header {
            width: 100%;
            padding: 15px 0;
            background-color: var(--card);
            border-bottom: 1px solid #30363d;
            text-align: center;
        }

        .logo{ margin:0; font-size:22px; letter-spacing:1px; font-weight:700; }
        .logo span{ color: var(--accent); text-shadow:0 0 8px rgba(88,166,255,0.8); }
        .status-bar { font-size: 11px; color: #8b949e; margin-top: 4px; }
        .mode-container { margin: 15px 0; display: flex; gap: 10px; }

        .mode-btn {
            width: 130px; height: 42px;
            font-size: 13px; font-weight: bold;
            border-radius: 20px; border: 2px solid transparent;
            background-color: var(--joy-bg); color: #8b949e;
            transition: 0.3s; cursor: pointer;
        }

        .mode-btn.active.manual { border-color: var(--accent); color: var(--accent); background: rgba(88,166,255,0.1); }
        .mode-btn.active.auto { border-color: var(--auto-color); color: var(--auto-color); background: rgba(163,113,247,0.1); }

        .joystick-container { flex-grow: 1; display: flex; align-items: center; justify-content: center; width: 100%; }

        .d-pad {
            display: grid;
            grid-template-columns: repeat(3, 80px);
            grid-template-rows: repeat(3, 80px);
            gap: 15px;
            background: radial-gradient(circle,#21262d,#0d1117);
            padding: 22px; border-radius: 50%;
            border: 1px solid #30363d;
            box-shadow: inset 0 0 20px rgba(0,0,0,0.8), 0 0 25px rgba(0,0,0,0.5);
        }

        .joy-btn {
            background:linear-gradient(145deg,#1f2933,#11161b);
            border: 1px solid #30363d; border-radius: 20px;
            color: #c9d1d9; font-size: 30px;
            display: flex; align-items: center; justify-content: center;
            transition: 0.1s; cursor: pointer;
        }

        .joy-btn:active, .joy-btn.pressing {
            background: var(--accent); color: white;
            transform: scale(0.92); box-shadow: 0 0 15px var(--accent);
        }

        .joy-btn.fwd { grid-column: 2; grid-row: 1; }
        .joy-btn.lft { grid-column: 1; grid-row: 2; }
        .joy-btn.stop-center { grid-column: 2; grid-row: 2; font-size: 12px; font-weight: bold; background: #0d1117; border-radius: 50%; border: 2px solid #30363d; }
        .joy-btn.rgt { grid-column: 3; grid-row: 2; }
        .joy-btn.bwd { grid-column: 2; grid-row: 3; }

        .slider-card { width: 90%; max-width: 400px; background-color: var(--card); border-radius: 24px; padding: 20px; margin-bottom: 30px; border: 1px solid #30363d; }
        .slider-header { display: flex; justify-content: space-between; margin-bottom: 12px; font-size: 14px; }
        .slider-value { color: var(--accent); font-weight: bold; }

        input[type=range] { -webkit-appearance: none; width: 100%; height: 8px; background: #21262d; border-radius: 5px; outline: none; }
        input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 30px; height: 30px; background: var(--accent); border-radius: 50%; border: 4px solid var(--card); box-shadow: 0 0 12px var(--accent); }

        .hidden { display: none !important; }
        #autoPanel { flex-grow: 1; display: flex; flex-direction: column; align-items: center; justify-content: center; }
        .pulse-icon { font-size: 70px; color: var(--auto-color); animation: pulse 2s infinite; }

        @keyframes pulse {
            0% { opacity: 0.5; transform: scale(0.9); }
            50% { opacity: 1; transform: scale(1.1); }
            100% { opacity: 0.5; transform: scale(0.9); }
        }
    </style>
</head>
<body oncontextmenu="return false;">

<header>
    <h1 class="logo">Humi <span>Robot</span></h1>
    <div class="status-bar">
        SYSTEM: <span id="conn" style="color: var(--status-ok);">READY</span> | 
        MODE: <span id="curModeDisplay">MANUAL</span>
    </div>
</header>

<div class="mode-container">
    <button id="btnManual" class="mode-btn manual active" onclick="setMode('manual')">MANUAL</button>
    <button id="btnAuto" class="mode-btn auto" onclick="setMode('auto')">AUTO (LINE)</button>
</div>

<div id="manualPanel" class="joystick-container">
    <div class="d-pad">
        <button class="joy-btn fwd" ontouchstart="startMove('F', this)" ontouchend="stopMove(this)" onmousedown="startMove('F', this)" onmouseup="stopMove(this)">▲</button>
        <button class="joy-btn lft" ontouchstart="startMove('L', this)" ontouchend="stopMove(this)" onmousedown="startMove('L', this)" onmouseup="stopMove(this)">◀</button>
        <div class="joy-btn stop-center" onclick="fetch('/control?cmd=S')">STOP</div>
        <button class="joy-btn rgt" ontouchstart="startMove('R', this)" ontouchend="stopMove(this)" onmousedown="startMove('R', this)" onmouseup="stopMove(this)">▶</button>
        <button class="joy-btn bwd" ontouchstart="startMove('B', this)" ontouchend="stopMove(this)" onmousedown="startMove('B', this)" onmouseup="stopMove(this)">▼</button>
    </div>
</div>

<div id="autoPanel" class="hidden">
    <div class="pulse-icon">⊙</div>
    <h3 style="color: var(--auto-color); margin-top: 15px;">LINE FOLLOWER ACTIVE</h3>
    <p style="font-size: 13px; color: #8b949e;">Processing IR Sensor Data...</p>
</div>

<div class="slider-card">
    <div class="slider-header">
        <span>Engine Power (PWM)</span>
        <span class="slider-value" id="speedVal">50</span>
    </div>
    <input type="range" min="0" max="100" value="50" 
        onchange="updateSpeed(this.value)" 
        oninput="document.getElementById('speedVal').innerText=this.value">
</div>

<script>
let currentMode = 'manual';

function setMode(mode) {
    currentMode = mode;
    document.getElementById('btnManual').classList.toggle('active', mode === 'manual');
    document.getElementById('btnAuto').classList.toggle('active', mode === 'auto');
    document.getElementById('manualPanel').classList.toggle('hidden', mode === 'auto');
    document.getElementById('autoPanel').classList.toggle('hidden', mode === 'manual');
    document.getElementById('curModeDisplay').innerText = mode.toUpperCase();

    let cmd = (mode === 'manual') ? 'M' : 'A';
    fetch('/control?cmd=' + cmd);
}

function startMove(cmd, btn) {
    if (currentMode !== 'manual') return;
    btn.classList.add('pressing');
    fetch(`/control?cmd=${cmd}`);
}

function stopMove(btn) {
    if (currentMode !== 'manual') return;
    btn.classList.remove('pressing');
    fetch('/control?cmd=S');
}

function updateSpeed(v) {
    // แก้ไขให้ส่งไปที่ /control พร้อมค่า V เพื่อให้ STM32 รับทราบ
    fetch(`/control?cmd=V${v}`);
}

// ป้องกันการ Scroll หน้าจอขณะเล่น
document.body.addEventListener('touchmove', function(e) {
    if(e.target.type !== 'range') e.preventDefault();
}, {passive: false});
</script>

</body>
</html>
)rawliteral";