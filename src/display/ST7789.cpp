#include "platform/gpio.h"
#include "platform/spi.h"
#include "platform/time.h"
#include "platform/util/util.h"
#include <display/ST7789.hpp>

#include <algorithm>

static const mcu_spi_config_t spi_cfg = {125 * 1000 * 1000, MCU_SPI_MODE_0, MCU_SPI_MSB_FIRST};

void ST7789::st7789_cmd(uint8_t cmd, const uint8_t* data, size_t len)
{
    data_mode = false;

    mcu_gpio_put(cs_pin, 0);
    mcu_gpio_put(dc_pin, 0);
    mcu_sleep_us(1);
    
    mcu_spi_write(spi_bus, &cmd, sizeof(cmd));
    
    if (len)
    {
        mcu_sleep_us(1);
        mcu_gpio_put(dc_pin, 1);
        mcu_sleep_us(1);
        
        mcu_spi_write(spi_bus, data, len);
    }

    mcu_sleep_us(1);
    mcu_gpio_put(dc_pin, 1);
    mcu_sleep_us(1);
}

void ST7789::st7789_ramwr()
{
    mcu_gpio_put(dc_pin, 0);

    uint8_t cmd = 0x2C;
    mcu_spi_write(spi_bus, &cmd, sizeof(cmd));

    mcu_gpio_put(dc_pin, 1);
}

void ST7789::st7789_caset(uint16_t xs, uint16_t xe)
{
    uint8_t data[] = {
        (uint8_t)(xs >> 8),
        (uint8_t)(xs & 0xff),
        (uint8_t)(xe >> 8),
        (uint8_t)(xe & 0xff),
    };

    st7789_cmd(0x2A, data, sizeof(data));
}

void ST7789::st7789_raset(uint16_t ys, uint16_t ye)
{
    uint8_t data[] = {
        (uint8_t)(ys >> 8),
        (uint8_t)(ys & 0xff),
        (uint8_t)(ye >> 8),
        (uint8_t)(ye & 0xff),
    };

    st7789_cmd(0x2B, data, sizeof(data));
}

void ST7789::st7789_put(uint16_t pixel)
{
    st7789_write(&pixel, 1);
}

void ST7789::st7789_write(const uint16_t* data, size_t len)
{
    if (!data_mode)
    {
        st7789_ramwr();
        data_mode = true;
    }
    mcu_spi_write_16bit(spi_bus, data, len);
}

void ST7789::st7789_set_cursor(uint16_t x, uint16_t y)
{
    x = std::clamp(x, (uint16_t)0, (uint16_t)screen_dimensions.x);
    y = std::clamp(y, (uint16_t)0, (uint16_t)screen_dimensions.y);
    st7789_caset(x, screen_dimensions.x);
    st7789_raset(y, screen_dimensions.y);
}

void ST7789::st7789_set_cursor_unchecked(uint16_t x, uint16_t y)
{
    st7789_caset(x, screen_dimensions.x);
    st7789_raset(y, screen_dimensions.y);
}

ST7789::ST7789(int32_t width, int32_t height, const mcu_spi_bus_config_t& spi_bus_cfg, uint8_t cs_pin, uint8_t dc_pin, uint8_t rst_pin, uint8_t bl_pin, bool active_if_gpio_high)
    : SPIDevice(cs_pin, active_if_gpio_high), screen_dimensions(width, height), pwm_brightness(bl_pin, 1000.f, 1.f), spi_bus_cfg(spi_bus_cfg)
{
    pwm_brightness.SetEnabled(false);

    this->spi_bus_cfg.rx_pin = INVALID_UNUSED_GPIO; // unused RX pin
    mcu_spi_init(&this->spi_bus_cfg, &spi_bus);

    mcu_gpio_init(cs_pin);
    mcu_gpio_dir(cs_pin, MCU_GPIO_OUTPUT);

    mcu_gpio_init(rst_pin);
    mcu_gpio_dir(rst_pin, MCU_GPIO_OUTPUT);
    mcu_gpio_put(rst_pin, 1);
    
    // required pin
    mcu_gpio_init(dc_pin);
    mcu_gpio_dir(dc_pin, MCU_GPIO_OUTPUT);

    mcu_gpio_put(dc_pin, 1);
    mcu_sleep_ms(50);

    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();

    st7789_cmd(0x01, NULL, 0);
    mcu_sleep_ms(150);

    st7789_cmd(0x11, NULL, 0);
    mcu_sleep_ms(50);

    st7789_cmd(0x3a, (uint8_t[]){ 0x55 }, 1);
    mcu_sleep_ms(10);
    st7789_cmd(0x36, (uint8_t[]){ 0x00 }, 1);

    st7789_caset(0, width);
    st7789_raset(0, height);

    st7789_cmd(0x21, NULL, 0);
    mcu_sleep_ms(10);

    st7789_cmd(0x13, NULL, 0);
    mcu_sleep_ms(10);

    st7789_cmd(0x29, NULL, 0);
    mcu_sleep_ms(10);

    Deselect();
    mcu_spi_end(spi_bus);

    pwm_brightness.SetEnabled(true);
}

ST7789::ST7789(const Vec2i32& dimensions, const mcu_spi_bus_config_t& spi_bus_cfg, uint8_t cs_pin, uint8_t dc_pin, uint8_t rst_pin, uint8_t bl_pin, bool active_if_gpio_high)
    : ST7789(dimensions.x, dimensions.y, spi_bus_cfg, cs_pin, dc_pin, rst_pin, bl_pin, active_if_gpio_high)
{
}

void ST7789::UpdateDisplay()
{
    // no operation
}

void ST7789::Power(bool power_on)
{
    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();
    if (power_on)
    {
        st7789_cmd(0x11, nullptr, 0);
        mcu_sleep_ms(100);
        st7789_cmd(0x28, nullptr, 0);
    }
    else
        st7789_cmd(0x29, nullptr, 0);
    Deselect();
    mcu_spi_end(spi_bus);
}

void ST7789::SetBrightness(uint8_t brightness)
{
    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();
    pwm_brightness.SetDutyCycle((float)brightness / 255.f);
    Deselect();
    mcu_spi_end(spi_bus);
}

void ST7789::ClearDisplay()
{
    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();
    
    uint16_t line[screen_dimensions.x];
    std::fill(line, line + screen_dimensions.x, (uint16_t)0x0000);
    for (int32_t y = 0; y < screen_dimensions.y; y++)
    {
        st7789_set_cursor_unchecked(0, y);
        st7789_write(line, screen_dimensions.x);
    }

    Deselect();
    mcu_spi_end(spi_bus);
}

void ST7789::InvertColors()
{
    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();
    static bool inverted = false;
    if (inverted)
        st7789_cmd(0x20, nullptr, 0);
    else
        st7789_cmd(0x21, nullptr, 0);

    inverted = !inverted;
    Deselect();
    mcu_spi_end(spi_bus);
}

void ST7789::DrawPixel(Vec2i32 pos, RGBA color)
{
    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();
    st7789_set_cursor(pos.x, pos.y);
    st7789_put(color.ToRGB565());
    Deselect();
    mcu_spi_end(spi_bus);
}

void ST7789::DrawPixel(int32_t x, int32_t y, RGBA color)
{
    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();
    st7789_set_cursor(x, y);
    st7789_put(color.ToRGB565());
    Deselect();
    mcu_spi_end(spi_bus);
}

RGBA ST7789::GetPixel(Vec2i32 pos) const
{
    // unsupported
    return 0;
}

RGBA ST7789::GetPixel(int32_t x, int32_t y) const
{
    // unsupported
    return 0;
}

void ST7789::DrawHorizontalLine(Vec2i32 pos_begin, int32_t length, RGBA color)
{
    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();

    const int32_t x = std::clamp(pos_begin.x, (int32_t)0, screen_dimensions.x);
    length = std::abs(std::min(length, screen_dimensions.x - x));
    if (length == 0)
    {
        Deselect();
        return;
    }

    st7789_set_cursor(pos_begin.x, pos_begin.y);

    const uint16_t color565 = color.ToRGB565();

    uint16_t line[length];
    std::fill(line, line + length, color565);

    st7789_write(line, length);

    Deselect();
    mcu_spi_end(spi_bus);
}

void ST7789::DrawHorizontalLineX1X2(int32_t x1, int32_t x2, int32_t y, RGBA color)
{
    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();

    if (x1 > x2)
        std::swap(x1, x2);

    x1 = std::clamp(x1, (int32_t)0, screen_dimensions.x);
    x2 = std::clamp(x2, (int32_t)0, screen_dimensions.x);

    st7789_set_cursor(x1, y);

    const size_t length = x2 - x1 + 1;
    const uint16_t color565 = color.ToRGB565();

    uint16_t line[length];
    std::fill(line, line + length, color565);

    st7789_write(line, length);

    Deselect();
    mcu_spi_end(spi_bus);
}

void ST7789::DrawVerticalLine(Vec2i32 pos_begin, int32_t length, RGBA color)
{
    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();
    
    const int32_t y = std::clamp(pos_begin.y, (int32_t)0, screen_dimensions.y);
    length = std::abs(std::min(length, screen_dimensions.y - y));
    if (length == 0)
    {
        Deselect();
        return;
    }

    st7789_set_cursor(pos_begin.x, pos_begin.y);
    
    for (int32_t i = 0; i < length; i++)
    {
        st7789_raset(y + i, screen_dimensions.y);
        st7789_put(color.ToRGB565());
    }

    Deselect();
    mcu_spi_end(spi_bus);
}

void ST7789::DrawVerticalLineY1Y2(int32_t y1, int32_t y2, int32_t x, RGBA color)
{
    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();

    if (y1 > y2)
        std::swap(y1, y2);

    y1 = std::clamp(y1, (int32_t)0, screen_dimensions.y);
    y2 = std::clamp(y2, (int32_t)0, screen_dimensions.y);

    st7789_set_cursor(x, y1);

    const uint32_t length = y2 - y1 + 1;
    const uint16_t color565 = color.ToRGB565();

    for (int32_t i = 0; i < length; i++)
    {
        st7789_raset(y1 + i, screen_dimensions.y);
        st7789_put(color565);
    }
    
    Deselect();
    mcu_spi_end(spi_bus);
}

void ST7789::FillRectangle(Vec2i32 pos, Vec2i32 size, RGBA color)
{
    mcu_spi_begin(spi_bus, &spi_cfg);
    Select();

    const int32_t x = std::clamp(pos.x, (int32_t)0, screen_dimensions.x);
    const int32_t y = std::clamp(pos.y, (int32_t)0, screen_dimensions.y);
    size.x = std::abs(std::min(size.x, screen_dimensions.x - x));
    size.y = std::abs(std::min(size.y, screen_dimensions.y - y));
    if (size.x == 0 || size.y == 0)
    {
        Deselect();
        return;
    }

    st7789_set_cursor(x, y);

    const uint16_t color565 = color.ToRGB565();

    uint16_t line[size.x];
    std::fill(line, line + size.x, color565);

    for (int32_t i = 0; i < size.y; i++)
    {
        st7789_set_cursor_unchecked(x, y + i);
        st7789_write(line, size.x);
    }

    Deselect();
    mcu_spi_end(spi_bus);
}

void ST7789::FillRectangle(AABBi32 dimensions, RGBA color)
{   
    FillRectangle(dimensions.min, dimensions.max - dimensions.min, color);
}

void ST7789::FillRectangle(int32_t x_0, int32_t y_0, int32_t len_x, int32_t len_y, RGBA color)
{
    FillRectangle(Vec2i32{x_0, y_0}, Vec2i32{len_x, len_y}, color);
}