#include <u8g2_esp32_hal.h>
#include "pin_layout.h"
#include "oled.h"
u8g2_t u8g2;
void oled_init()
{
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.clk = VSPI_CLK;
    u8g2_esp32_hal.mosi = VSPI_MOSI;
    u8g2_esp32_hal.cs = OLED_CS;
    u8g2_esp32_hal.dc = OLED_DC;
    u8g2_esp32_hal.reset = OLED_RESET;
    u8g2_esp32_hal_init(u8g2_esp32_hal);
    u8g2_Setup_ssd1306_128x64_noname_f(
        &u8g2, U8G2_R0, u8g2_esp32_spi_byte_cb,
        u8g2_esp32_gpio_and_delay_cb);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
}

void oled_welcome()
{
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);
    u8g2_DrawStr(&u8g2, 0, 12, "Welcome to use");
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&u8g2, 0, 32, "SMRATCLK");
    u8g2_SendBuffer(&u8g2);
}