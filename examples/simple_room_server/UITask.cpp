#include "UITask.h"
#include <Arduino.h>
#include <helpers/CommonCLI.h>

#ifdef NEONPOCKET_ROOM_UI

#ifndef AUTO_OFF_MILLIS
#define AUTO_OFF_MILLIS 60000
#endif
#ifndef BOOT_SCREEN_MILLIS
#define BOOT_SCREEN_MILLIS 3200
#endif
#define BOOT_FRAME_MILLIS 80
#ifndef ROOM_POWER_CONFIRM_MILLIS
#define ROOM_POWER_CONFIRM_MILLIS 8000
#endif
#ifndef ROOM_UI_LOW_BATTERY_MV
#define ROOM_UI_LOW_BATTERY_MV 3450
#endif
#ifndef ROOM_UI_LOW_BATTERY_CLEAR_MV
#define ROOM_UI_LOW_BATTERY_CLEAR_MV 3600
#endif

static constexpr ColorVal NEON_BG = 0x0000;
static constexpr ColorVal NEON_PANEL = 0x0841;
static constexpr ColorVal NEON_GRID = 0x082C;
static constexpr ColorVal NEON_CYAN = 0x07FF;
static constexpr ColorVal NEON_COBALT = 0x225F;
static constexpr ColorVal NEON_LIME = 0x87E0;
static constexpr ColorVal NEON_YELLOW = 0xFFE0;
static constexpr ColorVal NEON_ORANGE = 0xFD20;
static constexpr ColorVal NEON_WHITE = 0xFFFF;
static const char* const PAGE_TITLES[] = { "HOME", "RF", "CLIENTS", "POSTS", "POWER" };
static constexpr uint8_t PAGE_COUNT = sizeof(PAGE_TITLES) / sizeof(PAGE_TITLES[0]);

static bool timerReached(unsigned long now, unsigned long deadline) {
  return (int32_t)(now - deadline) >= 0;
}

static void drawBootGrid(DisplayDriver* display, uint8_t frame) {
  display->setColor(NEON_GRID);
  for (int x = 10; x < 220; x += 20) display->fillRect(x, 0, 1, 96);
  for (int y = 8; y < 96; y += 16) display->fillRect(0, y, 220, 1);

  const int raster = 3 + (frame * 3) % 91;
  display->setColor(NEON_PANEL);
  display->fillRect(0, raster, 220, 2);
  display->setColor(NEON_COBALT);
  display->fillRect(0, (raster + 37) % 94, 220, 1);
}

static void drawBootSparks(DisplayDriver* display, uint8_t frame) {
  for (uint8_t i = 0; i < 9; i++) {
    const int x = (i * 47 + frame * (2 + i % 3)) % 220;
    const int y = 4 + (i * 19 + frame * (1 + (i & 1))) % 86;
    display->setColor(((frame + i) & 3) == 0 ? NEON_LIME
        : (i & 1 ? NEON_CYAN : NEON_COBALT));
    display->fillRect(x, y, i % 3 == 0 ? 3 : 1, 1);
  }
}

static void drawRevealedRect(DisplayDriver* display, ColorVal color,
    int x, int y, int w, int h, uint8_t reveal, uint8_t step, int sweep) {
  if (reveal < step) return;
  const int middle = x + w / 2;
  display->setColor(abs(middle - sweep) <= 2 ? NEON_WHITE : color);
  display->fillRect(x, y, w, h);
}

static void drawPocketMesh(DisplayDriver* display, uint8_t reveal, int sweep) {
  const int x = 91;
  const int y = 5;

  drawRevealedRect(display, NEON_CYAN, x + 5, y, 28, 2, reveal, 1, sweep);
  drawRevealedRect(display, NEON_CYAN, x + 1, y + 4, 2, 18, reveal, 2, sweep);
  drawRevealedRect(display, NEON_CYAN, x + 35, y + 4, 2, 18, reveal, 3, sweep);
  drawRevealedRect(display, NEON_CYAN, x + 3, y + 22, 5, 2, reveal, 4, sweep);
  drawRevealedRect(display, NEON_CYAN, x + 30, y + 22, 5, 2, reveal, 4, sweep);
  drawRevealedRect(display, NEON_CYAN, x + 7, y + 24, 7, 2, reveal, 5, sweep);
  drawRevealedRect(display, NEON_CYAN, x + 24, y + 24, 7, 2, reveal, 5, sweep);
  drawRevealedRect(display, NEON_CYAN, x + 13, y + 26, 12, 2, reveal, 6, sweep);

  drawRevealedRect(display, NEON_COBALT, x + 18, y + 10, 2, 5, reveal, 7, sweep);
  drawRevealedRect(display, NEON_COBALT, x + 12, y + 15, 7, 2, reveal, 8, sweep);
  drawRevealedRect(display, NEON_COBALT, x + 19, y + 15, 7, 2, reveal, 8, sweep);
  drawRevealedRect(display, NEON_COBALT, x + 10, y + 17, 2, 4, reveal, 9, sweep);
  drawRevealedRect(display, NEON_COBALT, x + 26, y + 17, 2, 4, reveal, 9, sweep);
  drawRevealedRect(display, NEON_COBALT, x + 12, y + 20, 15, 2, reveal, 10, sweep);

  drawRevealedRect(display, NEON_LIME, x + 17, y + 7, 5, 5, reveal, 7, sweep);
  drawRevealedRect(display, NEON_LIME, x + 8, y + 19, 5, 5, reveal, 10, sweep);
  drawRevealedRect(display, NEON_LIME, x + 25, y + 19, 5, 5, reveal, 10, sweep);
}

void UITask::begin(MyMesh& mesh, NodePrefs* node_prefs, const char* build_date,
    const char* firmware_version) {
  (void)build_date;
  _mesh = &mesh;
  _node_prefs = node_prefs;
  _page = 0;
  _next_read = 0;
  _next_refresh = 0;
  _power_armed_until = 0;
  _wait_for_release = false;
  _battery_low = false;
  _boot_started = millis();
  _boot_until = _boot_started + BOOT_SCREEN_MILLIS;
  _auto_off = _boot_started + AUTO_OFF_MILLIS;
  snprintf(_version_info, sizeof(_version_info), "%s",
      firmware_version ? firmware_version : "");
  char* dash = strchr(_version_info, '-');
  if (dash) *dash = 0;
#ifdef PIN_USER_BTN
  _button.begin();
  _wait_for_release = _button.isPressed();
#endif
  _display->turnOn();
}

void UITask::showFatal(const char* title, const char* detail) {
  if (!_display->isOn()) _display->turnOn();
  _display->startFrame(NEON_BG);
  _display->setColor(NEON_ORANGE);
  _display->drawRect(5, 5, 210, 118);
  _display->fillRect(5, 5, 4, 118);
  _display->setTextSize(2);
  _display->drawTextCentered(110, 31, title);
  _display->setColor(NEON_WHITE);
  _display->setTextSize(1);
  _display->drawTextCentered(110, 66, detail);
  _display->setColor(NEON_YELLOW);
  _display->drawTextCentered(110, 91, "USB SERIAL HAS DETAILS");
  _display->endFrame();
}

void UITask::renderBoot() {
  const unsigned long elapsed = millis() - _boot_started;
  const uint8_t frame = elapsed / BOOT_FRAME_MILLIS;
  const uint8_t reveal = min(10UL, 1 + elapsed / 110UL);
  const unsigned long bounded = min((unsigned long)BOOT_SCREEN_MILLIS, elapsed);
  const int progress = 180 * bounded / BOOT_SCREEN_MILLIS;
  const int sweep = 85 + 58 * bounded / BOOT_SCREEN_MILLIS;

  drawBootGrid(_display, frame);
  drawBootSparks(_display, frame);
  drawPocketMesh(_display, reveal, sweep);

  if (elapsed >= 160) {
    _display->setTextSize(2);
    if (elapsed < 720) {
      const int jitter = (frame % 3) - 1;
      _display->setColor(NEON_COBALT);
      _display->drawTextCentered(108 - jitter, 43, "NEONPOCKETMC");
      _display->setColor(NEON_CYAN);
      _display->drawTextCentered(112 + jitter, 41, "NEONPOCKETMC");
    }
    _display->setColor(NEON_LIME);
    _display->drawTextCentered(110, 42, "NEONPOCKETMC");
  }

  if (elapsed >= 400) {
    _display->setTextSize(1);
    _display->setColor(NEON_CYAN);
    _display->drawTextCentered(110, 65, "ROOM SERVER");
  }

  const char* status = elapsed < 640 ? "DISPLAY ONLINE"
      : elapsed < 1280 ? "RADIO BUS ONLINE"
      : elapsed < 1920 ? "IDENTITY READY"
      : elapsed < 2560 ? "ROOM SERVICES"
      : "MESH READY";
  _display->setTextSize(1);
  _display->setColor(NEON_WHITE);
  _display->drawTextCentered(110, 79, status);

  _display->setColor(NEON_COBALT);
  _display->drawRect(18, 94, 184, 8);
  _display->setColor(NEON_CYAN);
  _display->fillRect(20, 96, progress, 4);
  if (progress > 4) {
    _display->setColor(NEON_LIME);
    _display->fillRect(17 + progress, 95, 4, 6);
    _display->setColor(NEON_WHITE);
    _display->fillRect(19 + progress, 96, 1, 4);
  }

  char percent[8];
  snprintf(percent, sizeof(percent), "%lu%%", 100UL * bounded / BOOT_SCREEN_MILLIS);
  _display->setColor(NEON_CYAN);
  _display->setCursor(18, 110);
  _display->print(_version_info);
  _display->setColor(NEON_LIME);
  _display->drawTextRightAlign(202, 110, percent);
}

void UITask::renderPowerConfirm() {
  _display->setColor(NEON_ORANGE);
  _display->drawRect(5, 5, 210, 118);
  _display->fillRect(5, 5, 4, 118);
  _display->setTextSize(2);
  _display->drawTextCentered(110, 29, "SYSTEM OFF");
  _display->setTextSize(1);
  _display->setColor(NEON_WHITE);
  _display->drawTextCentered(110, 65, "HOLD AGAIN TO CONFIRM");
  _display->setColor(NEON_CYAN);
  _display->drawTextCentered(110, 91, "CLICK CANCELS");
}

void UITask::renderHeader(const char* title) {
  _display->setColor(NEON_PANEL);
  _display->fillRect(0, 0, 220, 20);
  _display->setColor(NEON_COBALT);
  _display->fillRect(0, 18, 220, 2);
  _display->fillRect(0, 0, 4, 20);
  _display->setColor(NEON_WHITE);
  _display->setTextSize(1);
  _display->setCursor(10, 6);
  _display->print("ROOM");
  _display->setColor(NEON_CYAN);
  _display->drawTextRightAlign(212, 6, title);
}

void UITask::renderFooter() {
  _display->setColor(NEON_PANEL);
  _display->fillRect(0, 112, 220, 16);
  const int left = 82;
  for (uint8_t i = 0; i < PAGE_COUNT; i++) {
    _display->setColor(i == _page ? NEON_LIME : NEON_COBALT);
    if (i == _page) _display->fillRect(left + i * 12, 118, 8, 4);
    else _display->drawRect(left + i * 12, 118, 8, 4);
  }
}

void UITask::renderHome() {
  _display->setColor(NEON_WHITE);
  _display->setTextSize(2);
  _display->drawTextEllipsized(8, 27, 204, _node_prefs->node_name);

  const int xs[] = { 8, 78, 148 };
  const ColorVal colors[] = { NEON_CYAN, NEON_YELLOW, NEON_LIME };
  const char* labels[] = { "CLIENTS", "POSTS", "BAT mV" };
  char values[3][12] = {};
  snprintf(values[0], sizeof(values[0]), "%u", _snapshot.clients);
  snprintf(values[1], sizeof(values[1]), "%u", _snapshot.buffered_posts);
  snprintf(values[2], sizeof(values[2]), "%u", _snapshot.battery_mv);
  for (int i = 0; i < 3; i++) {
    _display->setColor(NEON_PANEL);
    _display->fillRect(xs[i], 55, 64, 48);
    _display->setColor(i == 2 && _battery_low ? NEON_ORANGE : colors[i]);
    _display->drawRect(xs[i], 55, 64, 48);
    _display->setTextSize(1);
    _display->drawTextCentered(xs[i] + 32, 63, labels[i]);
    _display->setColor(NEON_WHITE);
    _display->setTextSize(2);
    _display->drawTextCentered(xs[i] + 32, 79, values[i]);
  }
}

void UITask::renderRF() {
  char text[42];
  _display->setColor(NEON_WHITE);
  _display->setTextSize(1);
  snprintf(text, sizeof(text), "FREQ  %07.3f MHz", _node_prefs->freq);
  _display->setCursor(10, 29);
  _display->print(text);
  snprintf(text, sizeof(text), "SF %-2u  BW %6.2f  CR 4/%u",
      _node_prefs->sf, _node_prefs->bw, _node_prefs->cr);
  _display->setCursor(10, 46);
  _display->print(text);
  snprintf(text, sizeof(text), "TX %d dBm   QUEUE %u",
      _node_prefs->tx_power_dbm, _snapshot.tx_queue);
  _display->setCursor(10, 63);
  _display->print(text);
  snprintf(text, sizeof(text), "RSSI %d   SNR %.1f",
      _snapshot.last_rssi, _snapshot.last_snr_x4 / 4.0f);
  _display->setCursor(10, 80);
  _display->print(text);
  _display->setColor(NEON_CYAN);
  snprintf(text, sizeof(text), "RX %lu  TX %lu  NF %d",
      (unsigned long)_snapshot.packets_recv, (unsigned long)_snapshot.packets_sent,
      _snapshot.noise_floor);
  _display->setCursor(10, 97);
  _display->print(text);
}

void UITask::renderClients() {
  char text[36];
  _display->setTextSize(2);
  _display->setColor(NEON_WHITE);
  snprintf(text, sizeof(text), "%u / %u ACTIVE", _snapshot.clients, MAX_CLIENTS);
  _display->drawTextCentered(110, 28, text);

  const int ys[] = { 59, 76, 93 };
  const ColorVal colors[] = { NEON_ORANGE, NEON_LIME, NEON_CYAN };
  const char* labels[] = { "ADMIN", "WRITE", "READ" };
  const uint8_t values[] = { _snapshot.admins, _snapshot.writers, _snapshot.readers };
  _display->setTextSize(1);
  for (int i = 0; i < 3; i++) {
    _display->setColor(colors[i]);
    _display->fillRect(18, ys[i] + 2, 8, 8);
    _display->setCursor(34, ys[i]);
    _display->print(labels[i]);
    snprintf(text, sizeof(text), "%u", values[i]);
    _display->drawTextRightAlign(200, ys[i], text);
  }
}

void UITask::renderPosts() {
  char text[38];
  _display->setTextSize(1);
  _display->setColor(NEON_WHITE);
  snprintf(text, sizeof(text), "BUFFER  %u / %u", _snapshot.buffered_posts,
      MAX_UNSYNCED_POSTS);
  _display->setCursor(10, 28);
  _display->print(text);
  snprintf(text, sizeof(text), "TOTAL   %u    PUSHED %u",
      _snapshot.posts_total, _snapshot.posts_pushed);
  _display->setCursor(10, 44);
  _display->print(text);
  _display->setColor(NEON_YELLOW);
  _display->setCursor(10, 63);
  _display->print("LATEST");
  _display->setColor(NEON_WHITE);
  _display->setCursor(10, 78);
  if (_snapshot.latest_post[0]) _display->printWordWrap(_snapshot.latest_post, 200);
  else _display->print("No posts this boot");
}

void UITask::renderPower() {
  char text[40];
  _display->setTextSize(2);
  _display->setColor(NEON_WHITE);
  snprintf(text, sizeof(text), "%u mV", _snapshot.battery_mv);
  _display->drawTextCentered(110, 28, text);

  _display->setTextSize(1);
  const bool battery_known = _snapshot.battery_mv != 0;
  _display->setColor(!battery_known ? NEON_YELLOW : (_battery_low ? NEON_ORANGE : NEON_LIME));
  snprintf(text, sizeof(text), "STATE   %s",
      !battery_known ? "NO SAMPLE" : (_battery_low ? "LOW" : "NORMAL"));
  _display->setCursor(12, 57);
  _display->print(text);

  _display->setColor(NEON_WHITE);
  snprintf(text, sizeof(text), "SOURCE  %s", _snapshot.external_power ? "USB" : "BATTERY");
  _display->setCursor(12, 76);
  _display->print(text);
  snprintf(text, sizeof(text), "BOOT %u mV", _snapshot.boot_mv);
  _display->drawTextRightAlign(208, 76, text);
  snprintf(text, sizeof(text), "UPTIME  %lud %02lu:%02lu",
      (unsigned long)(_snapshot.uptime_secs / 86400),
      (unsigned long)((_snapshot.uptime_secs / 3600) % 24),
      (unsigned long)((_snapshot.uptime_secs / 60) % 60));
  _display->setCursor(12, 94);
  _display->print(text);
}

void UITask::renderCurrent() {
  _mesh->copyUiSnapshot(_snapshot);
  if (_snapshot.battery_mv != 0) {
    if (!_battery_low && _snapshot.battery_mv <= ROOM_UI_LOW_BATTERY_MV) {
      _battery_low = true;
    } else if (_battery_low && _snapshot.battery_mv >= ROOM_UI_LOW_BATTERY_CLEAR_MV) {
      _battery_low = false;
    }
  }
  renderHeader(PAGE_TITLES[_page]);
  switch (_page) {
    case 0: renderHome(); break;
    case 1: renderRF(); break;
    case 2: renderClients(); break;
    case 3: renderPosts(); break;
    default: renderPower(); break;
  }
  renderFooter();
}

void UITask::loop() {
  const unsigned long now = millis();
#ifdef PIN_USER_BTN
  if (timerReached(now, _next_read)) {
    if (_wait_for_release) {
      if (!_button.isPressed()) {
        _button.cancelClick();
        _wait_for_release = false;
      }
    } else {
      const int event = _button.check();
      if (event == BUTTON_EVENT_LONG_PRESS) {
        if (!_display->isOn()) {
          _display->turnOn();  // consume the first hold as wake
        } else if (_power_armed_until && !timerReached(now, _power_armed_until)) {
          Serial.println("POWER: confirmed; entering system off");
          _display->startFrame(NEON_BG);
          _display->setColor(NEON_ORANGE);
          _display->setTextSize(2);
          _display->drawTextCentered(110, 42, "POWERING OFF");
          _display->setColor(NEON_WHITE);
          _display->setTextSize(1);
          _display->drawTextCentered(110, 77, "BUTTON WAKES DEVICE");
          _display->endFrame();
          delay(250);
          board.powerOff();
        } else {
          Serial.println("POWER: hold again within 8 seconds to enter system off");
          _boot_until = now;
          _power_armed_until = now + ROOM_POWER_CONFIRM_MILLIS;
        }
        _auto_off = now + AUTO_OFF_MILLIS;
        _next_refresh = 0;
      } else if (event == BUTTON_EVENT_CLICK) {
        if (_power_armed_until) {
          Serial.println("POWER: system-off request cancelled");
          _power_armed_until = 0;
        } else if (!_display->isOn()) {
          _display->turnOn();  // consume the first click as wake
        } else if (!timerReached(now, _boot_until)) {
          _boot_until = now;
        } else {
          _page = (_page + 1) % PAGE_COUNT;
        }
        _auto_off = now + AUTO_OFF_MILLIS;
        _next_refresh = 0;
      }
    }
    _next_read = now + 20;
  }
#endif

  if (_power_armed_until && timerReached(now, _power_armed_until)) {
    _power_armed_until = 0;
    _next_refresh = 0;
    Serial.println("POWER: system-off request expired");
  }
  if (!_display->isOn()) return;
  if (timerReached(now, _next_refresh)) {
    _display->startFrame(NEON_BG);
    const bool booting = !timerReached(now, _boot_until);
    if (booting) renderBoot();
    else if (_power_armed_until) renderPowerConfirm();
    else renderCurrent();
    _display->endFrame();
    _next_refresh = now + (booting ? BOOT_FRAME_MILLIS : (_power_armed_until ? 250 : 1000));
  }
  if (timerReached(now, _auto_off)) _display->turnOff();
}

#else

#ifndef USER_BTN_PRESSED
#define USER_BTN_PRESSED LOW
#endif

#define AUTO_OFF_MILLIS      20000  // 20 seconds
#define BOOT_SCREEN_MILLIS   4000   // 4 seconds

// 'meshcore', 128x13px
static const uint8_t meshcore_logo [] PROGMEM = {
    0x3c, 0x01, 0xe3, 0xff, 0xc7, 0xff, 0x8f, 0x03, 0x87, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 
    0x3c, 0x03, 0xe3, 0xff, 0xc7, 0xff, 0x8e, 0x03, 0x8f, 0xfe, 0x3f, 0xfe, 0x1f, 0xff, 0x1f, 0xfe, 
    0x3e, 0x03, 0xc3, 0xff, 0x8f, 0xff, 0x0e, 0x07, 0x8f, 0xfe, 0x7f, 0xfe, 0x1f, 0xff, 0x1f, 0xfc, 
    0x3e, 0x07, 0xc7, 0x80, 0x0e, 0x00, 0x0e, 0x07, 0x9e, 0x00, 0x78, 0x0e, 0x3c, 0x0f, 0x1c, 0x00, 
    0x3e, 0x0f, 0xc7, 0x80, 0x1e, 0x00, 0x0e, 0x07, 0x1e, 0x00, 0x70, 0x0e, 0x38, 0x0f, 0x3c, 0x00, 
    0x7f, 0x0f, 0xc7, 0xfe, 0x1f, 0xfc, 0x1f, 0xff, 0x1c, 0x00, 0x70, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x1f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x3f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x1e, 0x3f, 0xfe, 0x3f, 0xf0, 
    0x77, 0x3b, 0x87, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xfc, 0x38, 0x00, 
    0x77, 0xfb, 0x8f, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xf8, 0x38, 0x00, 
    0x73, 0xf3, 0x8f, 0xff, 0x0f, 0xff, 0x1c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x78, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfe, 0x3c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x3c, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfc, 0x3c, 0x0e, 0x1f, 0xf8, 0xff, 0xf8, 0x70, 0x3c, 0x7f, 0xf8, 
};

void UITask::begin(NodePrefs* node_prefs, const char* build_date, const char* firmware_version) {
  _prevBtnState = HIGH;
  _auto_off = millis() + AUTO_OFF_MILLIS;
  _node_prefs = node_prefs;
  _display->turnOn();

  // strip off dash and commit hash by changing dash to null terminator
  // e.g: v1.2.3-abcdef -> v1.2.3
  char *version = strdup(firmware_version);
  char *dash = strchr(version, '-');
  if(dash){
    *dash = 0;
  }

  // v1.2.3 (1 Jan 2025)
  snprintf(_version_info, sizeof(_version_info), "%s (%s)", version, build_date);
  free(version);
}

void UITask::renderCurrScreen() {
  char tmp[80];
  if (millis() < BOOT_SCREEN_MILLIS) { // boot screen
    // meshcore logo
    _display->setColor(UIColor::corp_blue);
    int logoWidth = 128;
    _display->drawXbm((_display->width() - logoWidth) / 2, 3, meshcore_logo, logoWidth, 13);

    // meshcore website
    const char* website = "https://meshcore.io";
    _display->setColor(UIColor::primary_txt);
    _display->setTextSize(1);
    uint16_t websiteWidth = _display->getTextWidth(website);
    _display->setCursor((_display->width() - websiteWidth) / 2, 22);
    _display->print(website);

    // version info
    _display->setTextSize(1);
    uint16_t versionWidth = _display->getTextWidth(_version_info);
    _display->setCursor((_display->width() - versionWidth) / 2, 35);
    _display->print(_version_info);

    // node type
    const char* node_type = "< Room Server >";
    uint16_t typeWidth = _display->getTextWidth(node_type);
    _display->setCursor((_display->width() - typeWidth) / 2, 48);
    _display->print(node_type);
  } else {  // home screen
    // node name
    _display->setCursor(0, 0);
    _display->setTextSize(1);
    _display->setColor(UIColor::primary_txt);
    _display->print(_node_prefs->node_name);

    // freq / sf
    _display->setCursor(0, 20);
    sprintf(tmp, "FREQ: %06.3f SF%d", _node_prefs->freq, _node_prefs->sf);
    _display->print(tmp);

    // bw / cr
    _display->setCursor(0, 30);
    sprintf(tmp, "BW: %03.2f CR: %d", _node_prefs->bw, _node_prefs->cr);
    _display->print(tmp);
  }
}

void UITask::loop() {
#ifdef PIN_USER_BTN
  if (millis() >= _next_read) {
    int btnState = digitalRead(PIN_USER_BTN);
    if (btnState != _prevBtnState) {
      if (btnState == USER_BTN_PRESSED) {  // pressed?
        if (_display->isOn()) {
          // TODO: any action ?
        } else {
          _display->turnOn();
        }
        _auto_off = millis() + AUTO_OFF_MILLIS;   // extend auto-off timer
      }
      _prevBtnState = btnState;
    }
    _next_read = millis() + 200;  // 5 reads per second
  }
#endif

  if (_display->isOn()) {
    if (millis() >= _next_refresh) {
      _display->startFrame();
      renderCurrScreen();
      _display->endFrame();

      _next_refresh = millis() + 1000;   // refresh every second
    }
    if (millis() > _auto_off) {
      _display->turnOff();
    }
  }
}

#endif
