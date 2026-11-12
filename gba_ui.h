// TamaPoke original GBA-era inspired UI primitives.
// Pure RGB565 geometry: no sprites, SD assets, heap allocations, or save data.
// Keep all text/touch rectangles in TamaPoke.ino so hit tests share coordinates.
#pragma once
#include <stdint.h>
#include "Arduino_GFX_Library.h"

static constexpr uint16_t gbaRgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint16_t)(r >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | (b >> 3);
}
static constexpr uint16_t GBA_NAVY   = gbaRgb(33, 54, 81);
static constexpr uint16_t GBA_BLUE   = gbaRgb(58, 116, 184);
static constexpr uint16_t GBA_TEAL   = gbaRgb(39, 137, 144);
static constexpr uint16_t GBA_GREEN  = gbaRgb(80, 160, 105);
static constexpr uint16_t GBA_YELLOW = gbaRgb(241, 181, 67);
static constexpr uint16_t GBA_RED    = gbaRgb(209, 96, 103);
static constexpr uint16_t GBA_PURPLE = gbaRgb(134, 113, 179);
static constexpr uint16_t GBA_CREAM  = gbaRgb(255, 248, 222);
static constexpr uint16_t GBA_PAPER  = gbaRgb(245, 241, 215);
static constexpr uint16_t GBA_WHITE  = gbaRgb(255, 255, 251);
static constexpr uint16_t GBA_MIST   = gbaRgb(211, 233, 216);
static constexpr uint16_t GBA_WATER  = gbaRgb(204, 233, 232);
static constexpr uint16_t GBA_SHADOW = gbaRgb(116, 149, 147);
static constexpr uint16_t GBA_DISABLED = gbaRgb(183, 193, 187);

// Double-line, offset border is deliberately rectilinear, like GBA dialog boxes.
static inline void gbaFrame(Arduino_Canvas *g, int x, int y, int w, int h,
                            uint16_t fill, uint16_t accent) {
  g->fillRoundRect(x, y + 3, w, h, 6, GBA_SHADOW);
  g->fillRoundRect(x, y, w, h, 6, GBA_NAVY);
  g->fillRoundRect(x + 3, y + 3, w - 6, h - 6, 4, GBA_WHITE);
  g->fillRect(x + 6, y + 6, w - 12, h - 12, fill);
  g->drawRect(x + 6, y + 6, w - 12, h - 12, accent);
  // Small contrasting corners give a tiled/pixel-art appearance.
  g->fillRect(x + 7, y + 7, 6, 3, accent);
  g->fillRect(x + w - 13, y + 7, 6, 3, accent);
  g->fillRect(x + 7, y + h - 10, 6, 3, accent);
  g->fillRect(x + w - 13, y + h - 10, 6, 3, accent);
}

// Dark ribbon with a two-pixel highlight; text is centered within its width.
static inline void gbaRibbon(Arduino_Canvas *g, const char *label,
                             int x, int y, int w, int h,
                             uint16_t color, uint8_t size) {
  g->fillRoundRect(x, y + 3, w, h, 6, GBA_NAVY);
  g->fillRoundRect(x + 2, y + 1, w - 4, h - 3, 5, color);
  g->drawFastHLine(x + 11, y + 5, w - 22, GBA_WHITE);
  int tw = 0;
  for (const char *p = label; *p; ++p) tw += 6 * size;
  g->setTextSize(size);
  g->setTextColor(GBA_WHITE);
  g->setCursor(x + (w - tw) / 2, y + (h - 8 * size) / 2 + 1);
  g->print(label);
}

// A quiet checker pattern gives plain panels texture, without image assets.
// Pattern coordinates are limited to the safe central area of the round display.
static inline void gbaBackdrop(Arduino_Canvas *g, uint16_t base, uint16_t dots) {
  g->fillScreen(0x0000);
  g->fillCircle(233, 233, 231, base);
  for (int y = 66; y < 401; y += 20) {
    for (int x = 66; x < 401; x += 20) {
      int dx = x - 233, dy = y - 233;
      if (dx * dx + dy * dy < 174 * 174)
        g->fillRect(x, y, 3, 3, dots);
    }
  }
}
