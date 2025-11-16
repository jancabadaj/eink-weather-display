# How to store logs in Google Sheets

**1. Create the Apps Script (one-time setup):**
- Open a Google Sheet
- Extensions → Apps Script
- Paste the code from `appscript.js`

**2. Change API key:**
- Use some random string, set the same string in `config.h` -> `logApiKey`

**3. Change Sheet ID:**
- Create and open new Google Sheet
- Get ID from the URL: `https://docs.google.com/spreadsheets/d/SHEET_ID_HERE/edit`

**4. Deploy:**
- Click "Deploy" → "New deployment"
- Type: "Web app"
- Execute as: "Me"
- Who has access: "Anyone"
- Deploy

**5. Get deployment URL like:**
- URL will look like `https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec`
- Set deployment ID to `config.h` -> `logDeploymentId`

## ESP32 request
```
POST 
Content-Type: application/x-www-form-urlencoded
key=api-key&level=log-level&log=log-message
```

## About the Warning:

Deployment warning is Google's generic Apps Script permission - but by using `openById()` with a specific sheet ID, the script can **only** access that one spreadsheet, nothing else.
