#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>
#include <stdlib.h>

#include "MyMesh.h"

#ifdef ETHERNET_ENABLED
  #define ETHERNET_CLI_BANNER "MeshCore Room Server CLI"
  #include <helpers/nrf52/EthernetCLI.h>
#endif

#ifdef DISPLAY_CLASS
  #include "UITask.h"
  static UITask ui_task(display);
#endif

#if defined(RC52_ROOM_SERVER) && !defined(DISPLAY_CLASS) && defined(PIN_USER_BTN)
  #include <helpers/ui/MomentaryButton.h>
  static MomentaryButton headless_power_button(PIN_USER_BTN, 1500, true, true, false);
  static unsigned long headless_power_armed_until = 0;
  static bool headless_wait_for_release = false;
#endif

StdRNG fast_rng;
SimpleMeshTables tables;
MyMesh the_mesh(board, radio_driver, *new ArduinoMillis(), fast_rng, rtc_clock, tables);

void halt() {
#ifdef RC52_ROOM_SERVER
  while (1) delay(1000);
#else
  while (1) ;
#endif
}

#ifdef RC52_ROOM_SERVER
static void fatalHalt(const char* serial_message, const char* title, const char* detail) {
  Serial.print("FATAL: ");
  Serial.println(serial_message);
#if defined(DISPLAY_CLASS) && defined(NEONPOCKET_ROOM_UI)
  ui_task.showFatal(title, detail);
#else
  (void)title;
  (void)detail;
#endif
  halt();
}

#ifdef NEONPOCKET_MEMORY_GATE_BYTES
static bool probeRequiredMemory() {
  void* probe = malloc(NEONPOCKET_MEMORY_GATE_BYTES);
  if (!probe) return false;
  free(probe);
  return true;
}
#endif
#endif

static char command[MAX_POST_TEXT_LEN+1];
#ifdef ETHERNET_ENABLED
static char ethernet_command[MAX_POST_TEXT_LEN+1];
#endif

#if defined(RC52_ROOM_SERVER) && !defined(DISPLAY_CLASS) && defined(PIN_USER_BTN)
static void pollHeadlessPowerButton() {
  const unsigned long now = millis();
  if (headless_wait_for_release) {
    if (!headless_power_button.isPressed()) {
      headless_power_button.cancelClick();
      headless_wait_for_release = false;
    }
    return;
  }

  if (headless_power_armed_until &&
      (int32_t)(now - headless_power_armed_until) >= 0) {
    headless_power_armed_until = 0;
    Serial.println("POWER: system-off request expired");
  }

  const int event = headless_power_button.check();
  if (event == BUTTON_EVENT_CLICK) {
    if (headless_power_armed_until) {
      headless_power_armed_until = 0;
      Serial.println("POWER: system-off request cancelled");
    }
  } else if (event == BUTTON_EVENT_LONG_PRESS) {
    if (headless_power_armed_until &&
        (int32_t)(headless_power_armed_until - now) >= 0) {
      Serial.println("POWER: confirmed; entering system off");
      delay(100);
      board.powerOff();
    } else {
      headless_power_armed_until = now + 8000;
      Serial.println("POWER: hold again within 8 seconds to enter system off");
    }
  }
}
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

  board.begin();

#if defined(RC52_ROOM_SERVER) && !defined(DISPLAY_CLASS) && defined(PIN_USER_BTN)
  headless_power_button.begin();
  headless_wait_for_release = headless_power_button.isPressed();
#endif

#ifdef HAS_EXTERNAL_WATCHDOG
  external_watchdog.begin();
#endif

#ifdef DISPLAY_CLASS
  if (display.begin()) {
    display.startFrame(0x0000);
  #ifdef NEONPOCKET_ROOM_UI
    // The animated renderer starts after radio and storage pass their gates.
  #else
    display.setCursor(0, 0);
    display.print("Please wait...");
  #endif
    display.endFrame();
  } else {
    Serial.println("FATAL: display/framebuffer initialization failed; direct rendering disabled");
  #ifdef DISPLAY_REQUIRED
    halt();
  #endif
  }
#endif

  if (!radio_init()) {
#ifdef RC52_ROOM_SERVER
    fatalHalt("radio initialization failed", "RADIO FAILED", "CHECK RF HARDWARE / RESET");
#else
    halt();
#endif
  }

  fast_rng.begin(radio_driver.getRngSeed());

  FILESYSTEM* fs;
#if defined(NRF52_PLATFORM)
  const bool fs_ready = InternalFS.begin();
#ifdef FILESYSTEM_REQUIRED
  if (!fs_ready) {
  #ifdef RC52_ROOM_SERVER
    fatalHalt("InternalFS mount failed; data was not formatted",
        "STORAGE FAILED", "DATA NOT ERASED");
  #else
    halt();
  #endif
  }
#endif
  fs = &InternalFS;
  IdentityStore store(InternalFS, "");
#elif defined(RP2040_PLATFORM)
  LittleFS.begin();
  fs = &LittleFS;
  IdentityStore store(LittleFS, "/identity");
  store.begin();
#elif defined(ESP32)
  SPIFFS.begin(true);
  fs = &SPIFFS;
  IdentityStore store(SPIFFS, "/identity");
#else
  #error "need to define filesystem"
#endif
  if (!store.load("_main", the_mesh.self_id)) {
    the_mesh.self_id = radio_new_identity();   // create new random identity
    int count = 0;
    while (count < 10 && (the_mesh.self_id.pub_key[0] == 0x00 || the_mesh.self_id.pub_key[0] == 0xFF)) {  // reserved id hashes
      the_mesh.self_id = radio_new_identity(); count++;
    }
    if (!store.save("_main", the_mesh.self_id)) {
#ifdef FILESYSTEM_REQUIRED
  #ifdef RC52_ROOM_SERVER
      fatalHalt("room identity could not be saved", "STORAGE FAILED", "IDENTITY NOT SAVED");
  #else
      halt();
  #endif
#endif
    }
  }

  Serial.print("Room ID: ");
  mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE); Serial.println();

  command[0] = 0;
#ifdef ETHERNET_ENABLED
  ethernet_command[0] = 0;
#endif

  sensors.begin();

  the_mesh.begin(fs);

#ifdef DISPLAY_CLASS
  #ifdef NEONPOCKET_ROOM_UI
  ui_task.begin(the_mesh, the_mesh.getNodePrefs(), FIRMWARE_BUILD_DATE, FIRMWARE_VERSION);
  #else
  ui_task.begin(the_mesh.getNodePrefs(), FIRMWARE_BUILD_DATE, FIRMWARE_VERSION);
  #endif
#endif

#if defined(RC52_ROOM_SERVER) && defined(NEONPOCKET_MEMORY_GATE_BYTES)
  if (!probeRequiredMemory()) {
    fatalHalt("post-display 16 KB memory gate failed",
        "MEMORY FAILED", "16 KB HEADROOM REQUIRED");
  }
  Serial.println("NeonPocket: post-display 16 KB memory gate passed");
#endif

#ifdef ETHERNET_ENABLED
  ethernet_start_task();
#endif

  // send out initial zero hop Advertisement to the mesh
#if ENABLE_ADVERT_ON_BOOT == 1
  the_mesh.sendSelfAdvertisement(16000, false);
#endif

  board.onBootComplete();
}

void loop() {
  int len = strlen(command);
  while (Serial.available() && len < sizeof(command)-1) {
    char c = Serial.read();
    if (c != '\n') {
      command[len++] = c;
      command[len] = 0;
    }
    Serial.print(c);
  }
  if (len == sizeof(command)-1) {  // command buffer full
    command[sizeof(command)-1] = '\r';
  }

  if (len > 0 && command[len - 1] == '\r') {  // received complete line
    command[len - 1] = 0;  // replace newline with C string null terminator
    char reply[160];
    reply[0] = 0;
#ifdef ETHERNET_ENABLED
    if (!ethernet_handle_command(command, reply)) {
      the_mesh.handleCommand(0, command, reply);
    }
#else
    the_mesh.handleCommand(0, command, reply);  // NOTE: there is no sender_timestamp via serial!
#endif
    if (reply[0]) {
      Serial.print("  -> "); Serial.println(reply);
    }

    command[0] = 0;  // reset command buffer
  }

#ifdef ETHERNET_ENABLED
  ethernet_loop_maintain();
  if (ethernet_read_line(ethernet_command, sizeof(ethernet_command))) {
    char reply[160];
    reply[0] = 0;
    if (!ethernet_handle_command(ethernet_command, reply)) {
      the_mesh.handleCommand(0, ethernet_command, reply);
    }
    ethernet_send_reply(reply);
    ethernet_command[0] = 0;
  }
#endif

  the_mesh.loop();
  sensors.loop();
#ifdef DISPLAY_CLASS
  ui_task.loop();
#endif
#if defined(RC52_ROOM_SERVER) && !defined(DISPLAY_CLASS) && defined(PIN_USER_BTN)
  pollHeadlessPowerButton();
#endif
  rtc_clock.tick();
#ifdef HAS_EXTERNAL_WATCHDOG
  external_watchdog.loop();
#endif
}
