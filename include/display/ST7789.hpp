#pragma once

#include <platform/spi.h>
#include <util/SPIDevice.hpp>

#include <interactive-ui/DisplayInterface.hpp>
#include <hardware/PulseWidthModulation.hpp>

class ST7789 : public DisplayInterface, public SPIDevice
{
protected:
    const Vec2i32 screen_dimensions;
    PulseWidthModulation pwm_brightness;
    mcu_spi_bus_config_t spi_bus_cfg;
    mcu_spi_bus_t* spi_bus;
    uint8_t dc_pin;
    uint8_t rst_pin;
    bool data_mode = false;

private:
    void st7789_cmd(uint8_t cmd, const uint8_t* data, size_t len);
    void st7789_caset(uint16_t xs, uint16_t xe);
    void st7789_raset(uint16_t ys, uint16_t ye);
    void st7789_put(uint16_t pixel);
    void st7789_ramwr();
    void st7789_write(const uint16_t* data, size_t len);
    void st7789_set_cursor(uint16_t x, uint16_t y);
    void st7789_set_cursor_unchecked(uint16_t x, uint16_t y);

public:
    ST7789(int32_t width, int32_t height, const mcu_spi_bus_config_t& spi_bus_cfg, uint8_t cs_pin, uint8_t dc_pin, uint8_t rst_pin, uint8_t bl_pin, bool active_if_gpio_high = false);
    ST7789(const Vec2i32& dimensions, const mcu_spi_bus_config_t& spi_bus_cfg, uint8_t cs_pin, uint8_t dc_pin, uint8_t rst_pin, uint8_t bl_pin, bool active_if_gpio_high = false);

    inline Vec2u32 GetDimensions() const override
    {
        return (Vec2u32)screen_dimensions;
    }

    void UpdateDisplay() override;
    void Power(bool power_on) override;
    void SetBrightness(uint8_t contrast) override;
    void ClearDisplay() override;
    void InvertColors() override;

    void DrawPixel(Vec2i32 pos, RGBA color) override;
    void DrawPixel(int32_t x, int32_t y, RGBA color) override;
    RGBA GetPixel(Vec2i32 pos) const override;
    RGBA GetPixel(int32_t x, int32_t y) const override;

    void DrawHorizontalLine(Vec2i32 pos_begin, int32_t length, RGBA color) override;
    void DrawHorizontalLineX1X2(int32_t x1, int32_t x2, int32_t y, RGBA color) override;
    void DrawVerticalLine(Vec2i32 pos_begin, int32_t length, RGBA color) override;
    void DrawVerticalLineY1Y2(int32_t y1, int32_t y2, int32_t x, RGBA color) override;

    void FillRectangle(Vec2i32 pos, Vec2i32 size, RGBA color) override;
    void FillRectangle(AABBi32 dimensions, RGBA color) override;
    void FillRectangle(int32_t x_0, int32_t y_0, int32_t len_x, int32_t len_y, RGBA color) override;
};