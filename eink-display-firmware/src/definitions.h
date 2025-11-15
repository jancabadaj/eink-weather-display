#pragma once

#define IMAGE_WIDTH 800
#define IMAGE_HEIGHT 480
#define IMAGE_WIDTH_BYTE ((IMAGE_WIDTH % 8 == 0) ? (IMAGE_WIDTH / 8) : (IMAGE_WIDTH / 8 + 1))
#define IMAGE_HEIGHT_BYTE IMAGE_HEIGHT

#define NETATMO_SERVER_AUTH "https://api.netatmo.com/oauth2/token"
#define NETATMO_SERVER_DATA "https://api.netatmo.com/api/getstationsdata"

#define TOKEN_REFRESH_MARGIN_MS 60000 // Refresh token 1 minute before expiration