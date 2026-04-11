#pragma once

// HTML template stored in program memory (flash) to save RAM
// Placeholders are replaced with dynamic values in WebServer::sendHomePage()

static const char WEB_UI_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <title>E-Ink Weather Display</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: Helvetica, Arial, sans-serif;
      background: #f0f2f5;
      color: #222;
      padding: 16px;
      max-width: 580px;
      margin: 0 auto;
    }
    h1 { font-size: 1.35em; margin-bottom: 2px; }
    .subtitle { color: #666; font-size: 0.85em; margin-bottom: 16px; }
    section {
      background: #fff;
      border-radius: 8px;
      padding: 16px;
      margin-bottom: 12px;
      box-shadow: 0 1px 3px rgba(0,0,0,0.08);
    }
    h2 {
      font-size: 0.8em;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.06em;
      color: #888;
      margin-bottom: 10px;
      border-bottom: 1px solid #eee;
      padding-bottom: 6px;
    }
    .row {
      display: flex;
      justify-content: space-between;
      align-items: baseline;
      margin-bottom: 7px;
      gap: 8px;
    }
    .lbl { color: #666; font-size: 0.9em; flex-shrink: 0; }
    .val { font-weight: 500; text-align: right; }
    .raw {
      font-family: monospace;
      font-size: 0.78em;
      color: #999;
      word-break: break-all;
      margin-bottom: 8px;
    }
    .raw-label { font-size: 0.78em; color: #aaa; margin-top: 10px; margin-bottom: 2px; }
    .tag-default { color: #aaa; font-size: 0.82em; font-weight: 400; }
    .tag-override { color: #1976D2; font-size: 0.82em; font-weight: 600; }
    .tag-warn { color: #c62828; font-size: 0.85em; font-weight: 600; margin-top: 6px; }
    .buttons { display: flex; flex-wrap: wrap; gap: 8px; margin-top: 12px; }
    a.btn {
      background: #43A047;
      color: #fff;
      border: none;
      padding: 9px 18px;
      font-size: 0.88em;
      border-radius: 5px;
      cursor: pointer;
      text-decoration: none;
      display: inline-block;
    }
    a.btn-secondary { background: #546E7A; }
    a.btn-danger    { background: #E53935; }
    .form-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
      margin-top: 12px;
    }
    .field label { display: block; font-size: 0.78em; color: #666; margin-bottom: 3px; }
    .field input[type=number] {
      width: 100%;
      padding: 7px 9px;
      border: 1px solid #ddd;
      border-radius: 4px;
      font-size: 0.9em;
    }
    .form-actions { display: flex; flex-wrap: wrap; gap: 8px; margin-top: 12px; }
    button.btn {
      background: #43A047;
      color: #fff;
      border: none;
      padding: 9px 18px;
      font-size: 0.88em;
      border-radius: 5px;
      cursor: pointer;
    }
  </style>
</head>
<body>

<h1>E-Ink Weather Display</h1>
<p class="subtitle">{{IP}} &nbsp;&middot;&nbsp; Uptime: {{UPTIME_HUMAN}} <span class="raw">(millis: {{MILLIS}})</span></p>

<section>
  <h2>Authentication</h2>
  <div class="row">
    <span class="lbl">Logged in</span>
    <span class="val">{{LOGGED_IN}}</span>
  </div>
  <div class="row">
    <span class="lbl">Token expires in</span>
    <span class="val">{{TOKEN_EXPIRES_HUMAN}}</span>
  </div>
  <div class="raw-label">token expiry (raw millis)</div>
  <div class="raw">{{TOKEN_EXPIRY_MS}}</div>
  <div class="raw-label">access token</div>
  <div class="raw">{{ACCESS_TOKEN}}</div>
  <div class="raw-label">refresh token</div>
  <div class="raw">{{REFRESH_TOKEN}}</div>
  <div class="buttons">
    <a href="{{LOGIN_URL}}" class="btn">Login with Netatmo</a>
  </div>
</section>

<section>
  <h2>Display Controls</h2>
  <div class="row">
    <span class="lbl">Next refresh</span>
    <span class="val">{{NEXT_REFRESH_HUMAN}}</span>
  </div>
  {{UPDATES_STOPPED}}
  <div class="buttons">
    <a href="/data/get" class="btn">Refresh now</a>
    <a href="/display/restart" class="btn btn-secondary">Restart auto-update</a>
    <a href="/display/clear" class="btn btn-secondary">Clear display</a>
  </div>
</section>

<section>
  <h2>Update Schedule</h2>
  <div class="row">
    <span class="lbl">Night suppression</span>
    <span class="val">{{NIGHT_DISPLAY}}</span>
  </div>
  <form action="/config/set" method="get">
    <div class="form-grid">
      <div class="field">
        <label>Night start (UTC hour 0&ndash;23)</label>
        <input type="number" name="night_start" value="{{NIGHT_START_VAL}}" min="0" max="23">
      </div>
      <div class="field">
        <label>Night end (UTC hour 0&ndash;23)</label>
        <input type="number" name="night_end" value="{{NIGHT_END_VAL}}" min="0" max="23">
      </div>
    </div>
    <div class="form-actions">
      <button type="submit" class="btn">Apply overrides</button>
      <a href="/config/reset" class="btn btn-danger">Reset to defaults</a>
    </div>
  </form>
</section>

</body>
</html>
)HTML";
