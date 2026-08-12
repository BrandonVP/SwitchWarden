/*
===========================================================================
Name        : ILI9488Adapter.h
Author      : Brandon Van Pelt
Description : IDisplay adapter for the ILI9488_t3 driver (Teensy 4.x).

              This is the ONLY file that knows the framework is running on an
              ILI9488. To port EmbeddedGFX to another display, write an
              equivalent adapter and change nothing in the library.

              Font handling lives here: setTextSize() maps the framework's
              logical size to a Michroma font asset.
===========================================================================
*/
#ifndef ILI9488_ADAPTER_H
#define ILI9488_ADAPTER_H

#include "appConfig.h"      // pulls in <EmbeddedGFX.h> with GFX_* overrides
#include <ILI9488_t3.h>
#include "font_Michroma.h"

class ILI9488Adapter : public IDisplay
{
public:
    explicit ILI9488Adapter(ILI9488_t3& display) : m_display(display) {}

    void fillRect(int x, int y, int w, int h, uint16_t color) override
    {
        m_display.fillRect(x, y, w, h, color);
    }
    void drawRect(int x, int y, int w, int h, uint16_t color) override
    {
        m_display.drawRect(x, y, w, h, color);
    }
    void fillRoundRect(int x, int y, int w, int h, int radius, uint16_t color) override
    {
        m_display.fillRoundRect(x, y, w, h, radius, color);
    }
    void drawRoundRect(int x, int y, int w, int h, int radius, uint16_t color) override
    {
        m_display.drawRoundRect(x, y, w, h, radius, color);
    }

    void setTextColor(uint16_t color) override
    {
        m_display.setTextColor(color);
    }
    void drawString(const char* str, int len, int x, int y) override
    {
        m_display.drawString(str, len, x, y);
    }
    int strPixelLen(const char* str) override
    {
        // ILI9488_t3 requires the char-count arg (no default). Measure the
        // whole string.
        return m_display.strPixelLen((char*)str, (uint16_t)strlen(str));
    }

    void useFrameBuffer(bool enable) override
    {
        m_display.useFrameBuffer(enable);
    }
    void updateScreen() override
    {
        m_display.updateScreen();
    }

    // Map the framework's logical text size to a Michroma font.
    void setTextSize(uint8_t s) override
    {
        if      (s <= 8)  m_display.setFont(Michroma_8);
        else if (s <= 9)  m_display.setFont(Michroma_9);
        else if (s <= 10) m_display.setFont(Michroma_10);
        else if (s <= 11) m_display.setFont(Michroma_11);
        else if (s <= 12) m_display.setFont(Michroma_12);
        else if (s <= 13) m_display.setFont(Michroma_13);
        else if (s <= 15) m_display.setFont(Michroma_14);
        else if (s <= 17) m_display.setFont(Michroma_16);
        else if (s <= 19) m_display.setFont(Michroma_18);
        else if (s <= 23) m_display.setFont(Michroma_20);
        else if (s <= 27) m_display.setFont(Michroma_24);
        else if (s <= 31) m_display.setFont(Michroma_28);
        else if (s <= 39) m_display.setFont(Michroma_32);
        else if (s <= 47) m_display.setFont(Michroma_40);
        else if (s <= 59) m_display.setFont(Michroma_48);
        else if (s <= 71) m_display.setFont(Michroma_60);
        else if (s <= 95) m_display.setFont(Michroma_72);
        else              m_display.setFont(Michroma_96);
    }

private:
    ILI9488_t3& m_display;
};

#endif // ILI9488_ADAPTER_H
