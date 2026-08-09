#pragma once

#include <helpers/ui/DisplayDriver.h>
#include <stdint.h>
#include <stdlib.h>

namespace NeonPocketSplash {

static constexpr uint16_t FRAME_MILLIS = 80;
static constexpr uint16_t DURATION_MILLIS = 3200;
static constexpr uint8_t FRAME_COUNT = DURATION_MILLIS / FRAME_MILLIS;
static const char BRAND[] = "NEONPOCKETMC";
static const char ROLE[] = "ROOM SERVER";

static constexpr ColorVal BLACK = 0x0000;
static constexpr ColorVal GRID = 0x0005;
static constexpr ColorVal RASTER = 0x000C;
static constexpr ColorVal COBALT = 0x001F;
static constexpr ColorVal CYAN = 0x07FF;
static constexpr ColorVal LIME = 0x07E0;
static constexpr ColorVal MAGENTA = 0xF81F;
static constexpr ColorVal WHITE = 0xFFFF;

inline uint8_t frameForElapsed(unsigned long elapsed) {
  const unsigned long frame = elapsed / FRAME_MILLIS;
  return frame < FRAME_COUNT ? (uint8_t)frame : FRAME_COUNT - 1;
}

inline const char* statusForFrame(uint8_t frame) {
  if (frame < 8) return "VECTOR CORE";
  if (frame < 17) return "MESH LINK";
  if (frame < 27) return "RADIO MATRIX";
  if (frame < 36) return "ROOM SERVICES";
  return "READY";
}

inline void line(DisplayDriver& display, int x0, int y0, int x1, int y1) {
  const int dx = abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;

  while (true) {
    display.fillRect(x0, y0, 1, 1);
    if (x0 == x1 && y0 == y1) return;
    const int twice_error = error * 2;
    if (twice_error >= dy) {
      error += dy;
      x0 += sx;
    }
    if (twice_error <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}

inline void drawPocket(DisplayDriver& display, uint8_t frame) {
  static const int x0 = 15;
  static const int y0 = 17;
  const uint8_t reveal = frame / 2;

  display.setColor(CYAN);
  line(display, x0 + 4, y0, x0 + 43, y0);
  if (reveal >= 1) line(display, x0 + 43, y0, x0 + 50, y0 + 7);
  if (reveal >= 2) line(display, x0 + 50, y0 + 7, x0 + 47, y0 + 36);
  if (reveal >= 3) line(display, x0 + 47, y0 + 36, x0 + 39, y0 + 44);
  if (reveal >= 4) line(display, x0 + 39, y0 + 44, x0 + 13, y0 + 44);
  if (reveal >= 5) line(display, x0 + 13, y0 + 44, x0 + 5, y0 + 36);
  if (reveal >= 6) line(display, x0 + 5, y0 + 36, x0, y0 + 7);
  if (reveal >= 7) line(display, x0, y0 + 7, x0 + 4, y0);

  if (frame >= 8) {
    display.setColor(COBALT);
    line(display, x0 + 13, y0 + 31, x0 + 26, y0 + 12);
    line(display, x0 + 26, y0 + 12, x0 + 39, y0 + 31);
    line(display, x0 + 13, y0 + 31, x0 + 26, y0 + 38);
    line(display, x0 + 39, y0 + 31, x0 + 26, y0 + 38);
  }

  if (frame >= 10) {
    display.setColor(LIME);
    display.fillRect(x0 + 24, y0 + 10, 5, 5);
    display.fillRect(x0 + 11, y0 + 29, 5, 5);
    display.fillRect(x0 + 37, y0 + 29, 5, 5);
    display.fillRect(x0 + 24, y0 + 36, 5, 5);
  }

  if (frame >= 12 && frame <= 34) {
    const int sweep = x0 + 3 + (frame - 12) * 2;
    display.setColor(WHITE);
    for (int y = y0 + 4; y < y0 + 39; y += 2) {
      const int x = sweep + (y - y0) / 9;
      if (x > x0 + 2 && x < x0 + 48) display.fillRect(x, y, 1, 2);
    }
  }
}

inline void draw(DisplayDriver& display, uint8_t frame, const char* version,
                 const char* build, const char* status = NULL) {
  if (frame >= FRAME_COUNT) frame = FRAME_COUNT - 1;
  const int width = display.width();
  const int height = display.height();

  display.setColor(BLACK);
  display.fillRect(0, 0, width, height);

  display.setColor(GRID);
  for (int y = 7; y < 78; y += 9) display.fillRect(0, y, width, 1);
  for (int x = 0; x < width; x += 22) display.fillRect(x, 0, 1, 78);

  for (uint8_t i = 0; i < 19; i++) {
    const int x = (i * 47 + 13) % width;
    const int y = 3 + (i * i * 7 + i * 11) % 70;
    display.setColor(i % 5 == 0 ? LIME : CYAN);
    display.fillRect(x, y, i % 6 == 0 ? 2 : 1, 1);
  }

  const int beam_y = 5 + (frame * 3) % 69;
  display.setColor(RASTER);
  display.fillRect(0, beam_y, width, 1);
  display.setColor(CYAN);
  display.fillRect((frame * 9) % (width - 48), beam_y, 48, 1);

  drawPocket(display, frame);

  display.setTextSize(2);
  const int title_x = 146 - display.getTextWidth(BRAND) / 2;
  if (frame < 7) {
    const int glitch = (frame & 1) ? 1 : -1;
    display.setColor(MAGENTA);
    display.setCursor(title_x - glitch, 23);
    display.print(BRAND);
    display.setColor(CYAN);
    display.setCursor(title_x + glitch, 25);
    display.print(BRAND);
  }
  display.setColor(frame < 7 ? WHITE : LIME);
  display.setCursor(title_x, 24);
  display.print(BRAND);

  display.setTextSize(1);
  display.setColor(CYAN);
  display.drawTextCentered(146, 48, ROLE);

  display.setColor(COBALT);
  display.fillRect(0, 78, width, 1);
  display.fillRect(0, 81, 76, 1);
  display.fillRect(width - 76, 81, 76, 1);
  display.setColor(LIME);
  display.fillRect(78, 80, width - 156, 3);

  display.setColor(WHITE);
  display.drawTextCentered(width / 2, 88, status ? status : statusForFrame(frame));

  display.setColor(COBALT);
  display.drawRect(8, 99, width - 16, 9);
  display.setColor(CYAN);
  display.fillRect(10, 102, (width - 20) * (frame + 1) / FRAME_COUNT, 3);
  display.setColor(BLACK);
  for (int x = 60; x < width - 10; x += 50) display.fillRect(x, 102, 1, 3);

  display.setColor(LIME);
  display.drawTextEllipsized(8, 115, 92, version ? version : "");
  display.setColor(CYAN);
  display.drawTextRightAlign(width - 8, 115, build ? build : "");
}

}  // namespace NeonPocketSplash
