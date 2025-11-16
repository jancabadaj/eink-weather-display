function doPost(e) {
    var API_KEY = "change-this";
    var SPREADSHEET_ID = "change0this"

    // Get parameters
    var params = e.parameter;
    if (!params.key || params.key !== API_KEY) {
        return ContentService.createTextOutput("Unauthorized").setMimeType(ContentService.MimeType.TEXT);
    }

    // Only access specific sheet by ID
    var sheet = SpreadsheetApp.openById(SPREADSHEET_ID).getSheetByName("Logs");
    var timestamp = new Date();
    var log = params.log || "empty";
    var level = params.level || "info";

    sheet.appendRow([timestamp, level, log]);

    return ContentService.createTextOutput("OK");
}
