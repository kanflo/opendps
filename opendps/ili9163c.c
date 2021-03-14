/*
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ILI9163C driver based on TFT_ILI9163C: https://github.com/sumotoy/TFT_ILI9163C
 * Copyright (c) 2014, .S.U.M.O.T.O.Y., coded by Max MC Costa.
 *
 */


#include "ili9163c.h"
#include "ili9163c_settings.h"
#include "ili9163c_registers.h"
#include <limits.h>
#include <gpio.h>
#include "tick.h"
#include "spi_driver.h"
#include "hw.h"

static uint8_t mad_ctrl_value;
static uint8_t colorspace_data;
static int16_t screen_width, screen_height;
static uint8_t rotation;

static void chip_init(void);
static void write_command(uint8_t c);
static void write_data(uint8_t c);
static void write_data16(uint16_t d);
static void color_space(uint8_t cspace);

void ili9163c_init(void)
{
    screen_width = _TFTWIDTH;
    screen_height = _TFTHEIGHT;
    gpio_set(TFT_RST_PORT, TFT_RST_PIN);
    delay_ms(10);
    gpio_clear(TFT_RST_PORT, TFT_RST_PIN);
    delay_ms(50);
    gpio_set(TFT_RST_PORT, TFT_RST_PIN);
    delay_ms(50);
/*
7) MY:  1(bottom to top), 0(top to bottom)  Row Address Order
6) MX:  1(R to L),        0(L to R)         Column Address Order
5) MV:  1(Exchanged),     0(normal)         Row/Column exchange
4) ML:  1(bottom to top), 0(top to bottom)  Vertical Refresh Order
3) RGB: 1(BGR),            0(RGB)               Color Space
2) MH:  1(R to L),        0(L to R)         Horizontal Refresh Order
1)
0)

     MY, MX, MV, ML,RGB, MH, D1, D0
     0 | 0 | 0 | 0 | 1 | 0 | 0 | 0  //normal
     1 | 0 | 0 | 0 | 1 | 0 | 0 | 0  //Y-Mirror
     0 | 1 | 0 | 0 | 1 | 0 | 0 | 0  //X-Mirror
     1 | 1 | 0 | 0 | 1 | 0 | 0 | 0  //X-Y-Mirror
     0 | 0 | 1 | 0 | 1 | 0 | 0 | 0  //X-Y Exchange
     1 | 0 | 1 | 0 | 1 | 0 | 0 | 0  //X-Y Exchange, Y-Mirror
     0 | 1 | 1 | 0 | 1 | 0 | 0 | 0  //XY exchange
     1 | 1 | 1 | 0 | 1 | 0 | 0 | 0
*/
    mad_ctrl_value = 0b00000000;
    colorspace_data = __COLORSPC; // start with default data;
    chip_init();
}

void ili9163c_get_geometry(uint16_t *width, uint16_t *height)
{
    if(width)
        *width = _GRAMWIDTH;
    if(height)
        *height = _GRAMHEIGH;
}

static void write_command(uint8_t c)
{
    gpio_clear(TFT_A0_PORT, TFT_A0_PIN);
    uint8_t tx_buf[1] = {c};
    (void) spi_dma_transceive((uint8_t*) tx_buf, sizeof(tx_buf), 0, 0);
}

static void write_data(uint8_t c)
{
    gpio_set(TFT_A0_PORT, TFT_A0_PIN);
    uint8_t tx_buf[1] = {c};
    (void) spi_dma_transceive((uint8_t*) tx_buf, sizeof(tx_buf), 0, 0);
}

static void write_data16(uint16_t d)
{
    gpio_set(TFT_A0_PORT, TFT_A0_PIN);
    uint8_t tx_buf[2] = {(uint8_t) (d >> 8), (uint8_t) (d & 0xff)};
    (void) spi_dma_transceive((uint8_t*) tx_buf, sizeof(tx_buf), 0, 0);
}

static void chip_init(void)
{
    uint8_t i;

    write_command(CMD_SWRESET); // software reset
    delay_ms(1);

    write_command(CMD_SLPOUT); // exit sleep

    write_command(CMD_PIXFMT); // Set Color Format 16bit
    write_data(0x05);

    write_command(CMD_GAMMASET); // default gamma curve 3
    write_data(0x04); // 0x04

    write_command(CMD_GAMRSEL); // Enable Gamma adj
    write_data(0x01);

    write_command(CMD_NORML);

    write_command(CMD_DFUNCTR);
    write_data(0b11111111);
    write_data(0b00000110);

    write_command(CMD_PGAMMAC); // Positive Gamma Correction Setting
    for (i=0;i<15;i++) {
        write_data(pGammaSet[i]);
    }

    write_command(CMD_NGAMMAC); // Negative Gamma Correction Setting
    for (i=0;i<15;i++) {
        write_data(nGammaSet[i]);
    }

    write_command(CMD_FRMCTR1); // Frame Rate Control (In normal mode/Full colors)
    write_data(0x08); // 0x0C//0x08
    write_data(0x02); // 0x14//0x08

    write_command(CMD_DINVCTR); // display inversion
    write_data(0x07);

    write_command(CMD_PWCTR1); // Set VRH1[4:0] & VC[2:0] for VCI1 & GVDD
    write_data(0x0A); // 4.30 - 0x0A
    write_data(0x02); // 0x05

    write_command(CMD_PWCTR2); // Set BT[2:0] for AVDD & VCL & VGH & VGL
    write_data(0x02);

    write_command(CMD_VCOMCTR1); // Set VMH[6:0] & VML[6:0] for VOMH & VCOML
    write_data(0x50); // 0x50
    write_data(99); // 0x5b

    write_command(CMD_VCOMOFFS);
    write_data(0); // 0x40

    write_command(CMD_CLMADRS); // Set Column Address
    write_data16(0x00);
    write_data16(_GRAMWIDTH);

    write_command(CMD_PGEADRS); // Set Page Address
    write_data16(0X00);
    write_data16(_GRAMHEIGH);
    // set scroll area (thanks Masuda)
    write_command(CMD_VSCLLDEF);
    write_data16(__OFFSET);
    write_data16(_GRAMHEIGH - __OFFSET);
    write_data16(0);

    color_space(colorspace_data);

    write_command(CMD_DISPON); // display ON
    write_command(CMD_RAMWR); // Memory Write
}

void ili9163c_invert_display(bool i)
{
    write_command(i ? CMD_DINVON : CMD_DINVOF);
}

void ili9163c_display(bool on)
{
    write_command(on ? CMD_DISPON : CMD_DISPOFF);
}

void ili9163c_push_color(uint16_t color)
{
    write_data16(color);
}

void ili9163c_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (ili9163c_boundary_check(x,y)) return;
    if ((x < 0) || (y < 0)) return;
    ili9163c_set_window(x,y,x,y);
    write_data16(color);
}

void ili9163c_draw_vline(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    if (ili9163c_boundary_check(x,y)) return;
    if (((y + h) - 1) >= screen_height) h = screen_height-y;
    ili9163c_set_window(x,y,x,(y+h)-1);
    while (h-- > 0) {
        write_data16(color);
    }
}

void ili9163c_draw_hline(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if (ili9163c_boundary_check(x,y)) return;
    if (((x+w) - 1) >= screen_width) w = screen_width-x;
    ili9163c_set_window(x,y,(x+w)-1,y);
    while (w-- > 0) {
        write_data16(color);
    }
}

bool ili9163c_boundary_check(int16_t x,int16_t y)
{
    if ((x >= screen_width) || (y >= screen_height)) return true;
    return false;
}

void ili9163c_fill_screen(uint16_t color)
{
    uint32_t i;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xff;
    uint8_t fill[] = {hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo, hi, lo};
    uint8_t dummy[sizeof(fill)];
    gpio_clear(TFT_A0_PORT, TFT_A0_PIN);
    ili9163c_set_window(0, 0, _GRAMWIDTH+2, _GRAMHEIGH); // Note! For some reason filling WxH is results in two vertical lines to the far right...
    gpio_set(TFT_A0_PORT, TFT_A0_PIN);
    for (i = 0;i < ((_GRAMWIDTH+2) * _GRAMHEIGH)/(sizeof(fill)/2); i++) {
        (void) spi_dma_transceive((uint8_t*) fill, sizeof(fill), (uint8_t*) dummy, sizeof(dummy));
    }
}

// fill a rectangle
void ili9163c_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (ili9163c_boundary_check(x,y)) return;
    if (((x + w) - 1) >= screen_width)  w = screen_width  - x;
    if (((y + h) - 1) >= screen_height) h = screen_height - y;
    ili9163c_set_window(x,y,(x+w)-1,(y+h)-1);
    for (y = h;y > 0;y--) {
        for (x = w;x > 1;x--) {
            write_data16(color);
        }
        write_data16(color);
    }
}

void ili9163c_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    write_command(CMD_CLMADRS); // Column
    if (rotation == 3)
    {
        write_data16(x0 + 3);
        write_data16(x1 + 3);
    }
    else if (rotation == 2)
    {
        write_data16(x0 + 2);
        write_data16(x1 + 2);
    }
    else if (rotation == 1)
    {
        write_data16(x0 + 1);
        write_data16(x1 + 1);
    }
    else
    {
        write_data16(x0 + 2);
        write_data16(x1 + 2);
    }

    write_command(CMD_PGEADRS); // Page
    if (rotation == 3)
    {
        write_data16(y0 + 2);
        write_data16(y1 + 2);
    }
    else if (rotation == 2)
    {
        write_data16(y0 + 3);
        write_data16(y1 + 3);
    }
    else if (rotation == 1)
    {
        write_data16(y0 + 2);
        write_data16(y1 + 2);
    }
    else
    {
        write_data16(y0 + 1);
        write_data16(y1 + 1);
    }
    write_command(CMD_RAMWR); // Into RAM
}


void ili9163c_set_rotation(uint8_t m)
{
    rotation = m % 4; // can't be higher than 3
    switch (rotation) {
    case 0:
        mad_ctrl_value = 0b00001000;
        screen_width  = _TFTWIDTH;
        screen_height = _TFTHEIGHT;
        break;
    case 1:
        mad_ctrl_value = 0b01101000;
        screen_width  = _TFTHEIGHT;
        screen_height = _TFTWIDTH;
        break;
    case 2:
        mad_ctrl_value = 0b11001000;
        screen_width  = _TFTWIDTH;
        screen_height = _TFTHEIGHT;
        break;
    case 3:
        mad_ctrl_value = 0b10101000;
        screen_width  = _TFTWIDTH;
        screen_height = _TFTHEIGHT;
        break;
    }
    color_space(colorspace_data);
    write_command(CMD_MADCTL);
    write_data(mad_ctrl_value);
}

/*
Colorspace selection:
0: RGB
1: GBR
*/
static void color_space(uint8_t cspace)
{
    if (cspace < 1) {
        mad_ctrl_value &= ~(1<<3);
    } else {
        mad_ctrl_value |= 1<<3;
    }
}
