#include "interactive-ui/graphics/Color.hpp"
#include "platform/gpio.h"
#include "platform/spi.h"
#include "platform/util/util.h"
#include <cstdint>
#include <pico/stdio.h>

#include <display/ST7789.hpp>
#include <interactive-ui/Screen.hpp>
#include <interactive-ui/components/TextComponent.hpp>

mcu_spi_bus_t* spi_bus;
constexpr mcu_spi_bus_config_t spi_bus_cfg = {19, 18, INVALID_UNUSED_GPIO, 0, 625 * 100 * 1000};

struct _init
{
    _init()
    {
        stdio_init_all();
        mcu_spi_init(&spi_bus_cfg, &spi_bus);
    }
} _init_inst;

ST7789 display(320, 240, spi_bus_cfg, 17, 20, INVALID_UNUSED_GPIO, 22, false);
ScreenManager manager(&display);
Screen main_screen(&manager, display.GetDimensions());

TextComponent text(&manager, static_cast<Vec2i32>(display.GetDimensions() / 2), "ST7789", &fonts::default_font_group, 0, &main_screen);

int main()
{
    mcu_gpio_init(25);
    mcu_gpio_dir(25, MCU_GPIO_OUTPUT);
    mcu_gpio_put(25, 1);
    
    // text.SetAlignment(AlignmentHorizontal::CENTER, AlignmentVertical::CENTER);
    // text.color = colors::RED;
    
    display.Power(true);
    display.SetBrightness(UINT8_MAX);
    display.ClearDisplay();
    // main_screen.SortComponents();
    // manager.PushScreen(&main_screen);
    // manager.EnableCBF(true);
    // manager.SetTargetRefreshRate(20.f);
    // manager.ForceUpdate();
    
    display.DrawRectangle(0, 0, 50, 50, colors::AZURE_BLUE);

    mcu_gpio_put(25, 0);
    while (1)
    {
        //manager.Update();
    }
}