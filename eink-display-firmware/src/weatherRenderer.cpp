#include "definitions.h"
#include "weatherRenderer.h"
#include "draw/drawUtils.h"
#include "draw/font24.cpp"

void WeatherRenderer::renderWeather(const WeatherData &data)
{
    DrawUtils::clearImage(_imageData);

    DrawUtils::drawLine(_imageData, IMAGE_WIDTH / 2, IMAGE_WIDTH / 2, 0, IMAGE_HEIGHT, Black);

    DrawUtils::drawString(_imageData, 30, 20, "Dnu", &Font24, Black);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 30, 20, "Von", &Font24, Black);

    // Internal
    char tempStr[32];
    DrawUtils::drawString(_imageData, 30, 100, "Teplota: ", &Font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f C", data.internal.temperature);
    DrawUtils::drawString(_imageData, 180, 100, tempStr, &Font24, Black);

    DrawUtils::drawString(_imageData, 30, 150, "Vlhkost: ", &Font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%d %%", data.internal.humidity);
    DrawUtils::drawString(_imageData, 180, 150, tempStr, &Font24, Black);

    DrawUtils::drawString(_imageData, 30, 200, "Tlak   : ", &Font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f mbar", data.internal.pressure);
    DrawUtils::drawString(_imageData, 180, 200, tempStr, &Font24, Black);

    DrawUtils::drawString(_imageData, 30, 250, "Hluk   : ", &Font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%d dB", data.internal.noise);
    DrawUtils::drawString(_imageData, 180, 250, tempStr, &Font24, Black);

    DrawUtils::drawString(_imageData, 30, 300, "CO2    : ", &Font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%d ppm", data.internal.co2);
    DrawUtils::drawString(_imageData, 180, 300, tempStr, &Font24, Black);

    // External
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 30, 100, "Teplota: ", &Font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f C", data.external.temperature);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 180, 100, tempStr, &Font24, Black);

    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 30, 150, "Vlhkost: ", &Font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%d %%", data.external.humidity);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 180, 150, tempStr, &Font24, Black);

    /*
Paint_Clear(WHITE);
Paint_DrawPoint(10, 80, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
Paint_DrawPoint(10, 90, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
Paint_DrawPoint(10, 100, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
Paint_DrawLine(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
Paint_DrawLine(70, 70, 20, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
Paint_DrawRectangle(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
Paint_DrawRectangle(80, 70, 130, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
Paint_DrawCircle(45, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
Paint_DrawCircle(105, 95, 20, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
Paint_DrawLine(85, 95, 125, 95, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
Paint_DrawLine(105, 75, 105, 115, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
Paint_DrawString_EN(10, 0, "waveshare", &Font16, BLACK, WHITE);
Paint_DrawString_EN(10, 20, "hello world", &Font12, WHITE, BLACK);
Paint_DrawNum(10, 33, 123456789, &Font12, BLACK, WHITE);
Paint_DrawNum(10, 50, 987654321, &Font16, WHITE, BLACK);
Paint_DrawString_CN(130, 0, " 你好abc", &Font12CN, BLACK, WHITE);
Paint_DrawString_CN(130, 20, "微雪电子", &Font24CN, WHITE, BLACK);

Paint_DrawString_EN(180, 160, "Teplota :  14.1 C / 22.1 C", &Font24, WHITE, BLACK);
Paint_DrawString_EN(180, 220, "Vlhkost :  78 %   / 44 %", &Font24, WHITE, BLACK);
Paint_DrawString_EN(180, 280, "Tlak    :  1017 mb", &Font24, WHITE, BLACK);
Paint_DrawString_EN(180, 340, "CO2     :  516 ppm", &Font24, WHITE, BLACK);
*/
}
