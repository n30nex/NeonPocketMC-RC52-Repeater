#pragma once

#include <helpers/ui/DisplayDriver.h>
#include <helpers/CommonCLI.h>

#ifdef NEONPOCKET_ROOM_UI
#include "MyMesh.h"
#include <helpers/ui/MomentaryButton.h>

class UITask {
  DisplayDriver* _display;
  MyMesh* _mesh;
  NodePrefs* _node_prefs;
  RoomUiSnapshot _snapshot;
  unsigned long _boot_started;
  unsigned long _boot_until;
  unsigned long _next_read;
  unsigned long _next_refresh;
  unsigned long _auto_off;
  unsigned long _power_armed_until;
  uint8_t _page;
  bool _wait_for_release;
  bool _battery_low;
  char _version_info[24];
  char _build_info[16];
#ifdef PIN_USER_BTN
  MomentaryButton _button;
#endif

  void renderBoot(unsigned long elapsed);
  void renderPowerConfirm();
  void renderHeader(const char* title);
  void renderFooter();
  void renderHome();
  void renderRF();
  void renderClients();
  void renderPosts();
  void renderPower();
  void renderCurrent();

public:
  explicit UITask(DisplayDriver& display) : _display(&display)
#ifdef PIN_USER_BTN
      , _button(PIN_USER_BTN, 1500, true, true, false)
#endif
      { }
  void primeBoot(const char* firmware_version, const char* build_date);
  void begin(MyMesh& mesh, NodePrefs* node_prefs, const char* build_date,
      const char* firmware_version);
  void showFatal(const char* title, const char* detail);
  void loop();
};

#else
class UITask {
  DisplayDriver* _display;
  unsigned long _next_read, _next_refresh, _auto_off;
  int _prevBtnState;
  NodePrefs* _node_prefs;
  char _version_info[32];

  void renderCurrScreen();
public:
  UITask(DisplayDriver& display) : _display(&display) { _next_read = _next_refresh = 0; }
  void begin(NodePrefs* node_prefs, const char* build_date, const char* firmware_version);

  void loop();
};
#endif
