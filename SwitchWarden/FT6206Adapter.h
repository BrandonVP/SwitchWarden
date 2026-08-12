/*
===========================================================================
Name        : FT6206Adapter.h
Author      : Brandon Van Pelt
Description : ITouch adapter for the Adafruit_FT6206 capacitive controller.

              Converts raw panel coordinates into SCREEN coordinates to match
              the display's rotation(1) landscape orientation, exactly as the
              original ScanToolFD did:  x = p.y,  y = HEIGHT - p.x.
===========================================================================
*/
#ifndef FT6206_ADAPTER_H
#define FT6206_ADAPTER_H

#include "appConfig.h"
#include <Adafruit_FT6206.h>

class FT6206Adapter : public ITouch
{
public:
    explicit FT6206Adapter(Adafruit_FT6206& ts) : m_ts(ts) {}

    bool touched() override
    {
        return m_ts.touched() > 0;
    }

    void getPoint(int& x, int& y) override
    {
        TS_Point p = m_ts.getPoint();
        x = p.y;
        y = GFX_SCREEN_HEIGHT - p.x;
    }

private:
    Adafruit_FT6206& m_ts;
};

#endif // FT6206_ADAPTER_H
