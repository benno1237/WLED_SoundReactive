#pragma once

#include "wled.h"
#include <U8x8lib.h> // from https://github.com/olikraus/u8g2/

//
// Insired by the v1 usermod: ssd1306_i2c_oled_u8g2
//
// v2 usermod for using 128x32 or 128x64 i2c
// OLED displays to provide a four line display
// for WLED.
//
// Dependencies
// * This usermod REQURES the ModeSortUsermod
// * This Usermod works best, by far, when coupled 
//   with RotaryEncoderUIUsermod.
//
// Make sure to enable NTP and set your time zone in WLED Config | Time.
//
// REQUIREMENT: You must add the following requirements to
// REQUIREMENT: "lib_deps" within platformio.ini / platformio_override.ini
// REQUIREMENT: *  U8g2  (the version already in platformio.ini is fine)
// REQUIREMENT: *  Wire
//

//The SCL and SDA pins are defined here. 
#ifdef ARDUINO_ARCH_ESP32
  #ifndef FLD_PIN_SCL
    #define FLD_PIN_SCL 22
  #endif
  #ifndef FLD_PIN_SDA
    #define FLD_PIN_SDA 21
  #endif
  #ifndef FLD_PIN_CLOCKSPI
    #define FLD_PIN_CLOCKSPI 18
  #endif
   #ifndef FLD_PIN_DATASPI
    #define FLD_PIN_DATASPI 23
  #endif   
  #ifndef FLD_PIN_DC
    #define FLD_PIN_DC 19
  #endif
  #ifndef FLD_PIN_CS
    #define FLD_PIN_CS 5
  #endif
  #ifndef FLD_PIN_RESET
    #define FLD_PIN_RESET 26
  #endif
#else
  #ifndef FLD_PIN_SCL
    #define FLD_PIN_SCL 5
  #endif
  #ifndef FLD_PIN_SDA
    #define FLD_PIN_SDA 4
  #endif
  #ifndef FLD_PIN_CLOCKSPI
    #define FLD_PIN_CLOCKSPI 14
  #endif
   #ifndef FLD_PIN_DATASPI
    #define FLD_PIN_DATASPI 13
  #endif   
  #ifndef FLD_PIN_DC
    #define FLD_PIN_DC 12
  #endif
    #ifndef FLD_PIN_CS
    #define FLD_PIN_CS 15
  #endif
  #ifndef FLD_PIN_RESET
    #define FLD_PIN_RESET 16
  #endif
#endif

// When to time out to the clock or blank the screen
// if SLEEP_MODE_ENABLED.
#define SCREEN_TIMEOUT_MS  60*1000    // 1 min

#define TIME_INDENT        0
#define DATE_INDENT        2

// Minimum time between redrawing screen in ms
#define USER_LOOP_REFRESH_RATE_MS 1000

// Extra char (+1) for null
#define LINE_BUFFER_SIZE            16+1

typedef enum {
  FLD_LINE_BRIGHTNESS = 0,
  FLD_LINE_EFFECT_SPEED,
  FLD_LINE_EFFECT_INTENSITY,
  FLD_LINE_EFFECT_FFT1, //WLEDSR
  FLD_LINE_EFFECT_FFT2, //WLEDSR
  FLD_LINE_EFFECT_FFT3, //WLEDSR
  FLD_LINE_MODE,
  FLD_LINE_PALETTE,
  FLD_LINE_TIME,
  FLD_LINE_PRESET, //WLEDSR
  FLD_LINE_OTHER, //WLEDSR: no Line4Type
  FLD_LINE_NULL //WLEDSR: no Line4Type
} Line4Type;

typedef enum {
  NONE = 0,
  SSD1306,      // U8X8_SSD1306_128X32_UNIVISION_HW_I2C
  SH1106,       // U8X8_SH1106_128X64_WINSTAR_HW_I2C
  SSD1306_64,   // U8X8_SSD1306_128X64_NONAME_HW_I2C
  SSD1305,      // U8X8_SSD1305_128X32_ADAFRUIT_HW_I2C
  SSD1305_64,   // U8X8_SSD1305_128X64_ADAFRUIT_HW_I2C
  SSD1306_SPI,  // U8X8_SSD1306_128X32_NONAME_HW_SPI
  SSD1306_SPI64,// U8X8_SSD1306_128X64_NONAME_HW_SPI
  SH1106_SPI    // U8X8_SH1106_128X64_WINSTAR_HW_SPI  
} DisplayType;

    char sliderNames[5][LINE_BUFFER_SIZE*2]; //WLEDSR
    const char sliderDefaults[5][LINE_BUFFER_SIZE] = {"FX Speed", "FX Intens.", "FX Custom1", "FX Custom2", "FX Custom3"}; //WLEDSR

class FourLineDisplayUsermod : public Usermod {

  private:

    bool initDone = false;
    unsigned long lastTime = 0;

    // HW interface & configuration
    U8X8 *u8x8 = nullptr;           // pointer to U8X8 display object
    #ifndef FLD_SPI_DEFAULT
    int8_t ioPin[5] = {FLD_PIN_SCL, FLD_PIN_SDA, -1, -1, -1};        // I2C pins: SCL, SDA
    DisplayType type = SSD1306;     // display type
    #else
    int8_t ioPin[5] = {FLD_PIN_CLOCKSPI, FLD_PIN_DATASPI, FLD_PIN_CS, FLD_PIN_DC, FLD_PIN_RESET}; // SPI pins: CLK, MOSI, CS, DC, RST
    DisplayType type = SSD1306_SPI; // display type
    #endif
    bool flip = false;              // flip display 180°
    uint8_t contrast = 10;          // screen contrast
    uint8_t lineHeight = 1;         // 1 row or 2 rows
    uint32_t refreshRate = USER_LOOP_REFRESH_RATE_MS; // in ms
    uint32_t screenTimeout = SCREEN_TIMEOUT_MS;       // in ms
    bool sleepMode = true;          // allow screen sleep?
    bool clockMode = false;         // display clock
    bool forceRotate = false;         // WLEDSR: force rotating of variables in display, even if strip.isUpdating, this can cause led stutter, this should not be necessary if display is fast enough...
    bool wokeUp = false; //WLEDSR

    // Next variables hold the previous known values to determine if redraw is
    // required.
    String knownSsid = "";
    IPAddress knownIp;
    uint8_t knownBrightness = 0;
    uint8_t knownEffectSpeed = 0;
    uint8_t knownEffectIntensity = 0;
    uint8_t knownEffectFFT1 = 0; //WLEDSR
    uint8_t knownEffectFFT2 = 0; //WLEDSR
    uint8_t knownEffectFFT3 = 0; //WLEDSR
    uint8_t knownMode = 0;
    uint8_t knownPalette = 0;
    uint8_t knownMinute = 99;
    uint8_t knownHour = 99;

    bool displayTurnedOff = false;
    unsigned long lastUpdate = 0;
    unsigned long lastRedraw = 0;
    unsigned long overlayUntil = 0;
    Line4Type lineType = FLD_LINE_BRIGHTNESS;
    // Set to 2 or 3 to mark lines 2 or 3. Other values ignored.
    byte markLineNum = 0;

    // strings to reduce flash memory usage (used more than twice)
    static const char _name[];
    static const char _contrast[];
    static const char _refreshRate[];
    static const char _screenTimeOut[];
    static const char _flip[];
    static const char _sleepMode[];
    static const char _clockMode[];
    static const char _forceRotate[]; //WLEDSR

    // If display does not work or looks corrupted check the
    // constructor reference:
    // https://github.com/olikraus/u8g2/wiki/u8x8setupcpp
    // or check the gallery:
    // https://github.com/olikraus/u8g2/wiki/gallery

  public:

    // gets called once at boot. Do all initialization that doesn't depend on
    // network here
    void setup() {
      if (type == NONE) return;
      bool allocated = false;
      byte i;
      if (type == SSD1306_SPI || type == SSD1306_SPI64 || type == SH1106_SPI) {
        for (i=0; i<5; i++) if (!pinManager.allocatePin(ioPin[i])) { allocated=true; break; }
        if (i<5 && allocated) { for (byte i=0; i<5; i++) pinManager.deallocatePin(ioPin[i]); type=NONE; return; }
      } else {
        for (i=0; i<2; i++) if (!pinManager.allocatePin(ioPin[i])) { allocated=true; break; }
        if (i<2 && allocated) { for (byte i=0; i<5; i++) pinManager.deallocatePin(ioPin[i]); type=NONE; return; }
      }
      DEBUG_PRINTLN(F("Allocating display."));
      switch (type) {
        case SSD1306:
          #ifdef ESP8266
          if (!(ioPin[0]==5 && ioPin[1]==4))
            u8x8 = (U8X8 *) new U8X8_SSD1306_128X32_UNIVISION_SW_I2C(ioPin[0], ioPin[1]); // SCL, SDA, reset
          else
          #endif
            u8x8 = (U8X8 *) new U8X8_SSD1306_128X32_UNIVISION_HW_I2C(U8X8_PIN_NONE, ioPin[0], ioPin[1]); // Pins are Reset, SCL, SDA
          lineHeight = 1;
          break;
        case SH1106:
          #ifdef ESP8266
          if (!(ioPin[0]==5 && ioPin[1]==4))
            u8x8 = (U8X8 *) new U8X8_SH1106_128X64_WINSTAR_SW_I2C(ioPin[0], ioPin[1]); // SCL, SDA, reset
          else
          #endif
            u8x8 = (U8X8 *) new U8X8_SH1106_128X64_WINSTAR_HW_I2C(U8X8_PIN_NONE, ioPin[0], ioPin[1]); // Pins are Reset, SCL, SDA
          lineHeight = 2;
          break;
        case SSD1306_64:
          #ifdef ESP8266
          if (!(ioPin[0]==5 && ioPin[1]==4))
            u8x8 = (U8X8 *) new U8X8_SSD1306_128X64_NONAME_SW_I2C(ioPin[0], ioPin[1]); // SCL, SDA, reset
          else
          #endif
            u8x8 = (U8X8 *) new U8X8_SSD1306_128X64_NONAME_HW_I2C(U8X8_PIN_NONE, ioPin[0], ioPin[1]); // Pins are Reset, SCL, SDA
          lineHeight = 2;
          break;
        case SSD1305:
          #ifdef ESP8266
          if (!(ioPin[0]==5 && ioPin[1]==4))
            u8x8 = (U8X8 *) new U8X8_SSD1305_128X32_NONAME_SW_I2C(ioPin[0], ioPin[1]); // SCL, SDA, reset
          else
          #endif
            u8x8 = (U8X8 *) new U8X8_SSD1305_128X32_ADAFRUIT_HW_I2C(U8X8_PIN_NONE, ioPin[0], ioPin[1]); // Pins are Reset, SCL, SDA
          lineHeight = 1;
          break;
        case SSD1305_64:
          #ifdef ESP8266
          if (!(ioPin[0]==5 && ioPin[1]==4))
            u8x8 = (U8X8 *) new U8X8_SSD1305_128X64_ADAFRUIT_SW_I2C(ioPin[0], ioPin[1]); // SCL, SDA, reset
          else
          #endif
            u8x8 = (U8X8 *) new U8X8_SSD1305_128X64_ADAFRUIT_HW_I2C(U8X8_PIN_NONE, ioPin[0], ioPin[1]); // Pins are Reset, SCL, SDA
          lineHeight = 2;
          break;
        case SSD1306_SPI:
          if (!(ioPin[0]==FLD_PIN_CLOCKSPI && ioPin[1]==FLD_PIN_DATASPI)) // if not overridden these sould be HW accellerated
            u8x8 = (U8X8 *) new U8X8_SSD1306_128X32_UNIVISION_4W_SW_SPI(ioPin[0], ioPin[1], ioPin[2], ioPin[3], ioPin[4]);
          else
            u8x8 = (U8X8 *) new U8X8_SSD1306_128X32_UNIVISION_4W_HW_SPI(ioPin[2], ioPin[3], ioPin[4]); // Pins are cs, dc, reset
          lineHeight = 1;
          break;
        case SSD1306_SPI64:
          if (!(ioPin[0]==FLD_PIN_CLOCKSPI && ioPin[1]==FLD_PIN_DATASPI)) // if not overridden these sould be HW accellerated
            u8x8 = (U8X8 *) new U8X8_SSD1306_128X64_NONAME_4W_SW_SPI(ioPin[0], ioPin[1], ioPin[2], ioPin[3], ioPin[4]);
          else
            u8x8 = (U8X8 *) new U8X8_SSD1306_128X64_NONAME_4W_HW_SPI(ioPin[2], ioPin[3], ioPin[4]); // Pins are cs, dc, reset
          lineHeight = 2;
          break;
        case SH1106_SPI:
          if (!(ioPin[0]==FLD_PIN_CLOCKSPI && ioPin[1]==FLD_PIN_DATASPI)) // if not overridden these sould be HW accellerated
            u8x8 = (U8X8 *) new U8X8_SH1106_128X64_WINSTAR_4W_SW_SPI(ioPin[0], ioPin[1], ioPin[2], ioPin[3], ioPin[4]);
          else
                 u8x8 = (U8X8 *) new U8X8_SH1106_128X64_WINSTAR_4W_HW_SPI(ioPin[2], ioPin[3], ioPin[4]); // Pins are cs, dc, reset
          lineHeight = 2;
          break;
        default:
          u8x8 = nullptr;
          type = NONE;
          return;
      }
      initDone = true;
      if (u8x8 != nullptr) {
        DEBUG_PRINTLN(F("Starting display."));
        (static_cast<U8X8*>(u8x8))->begin();
      } else {
        DEBUG_PRINTLN(F("Display init failed."));
        type = NONE;
        return;
      }
      setFlipMode(flip);
      setContrast(contrast); //Contrast setup will help to preserve OLED lifetime. In case OLED need to be brighter increase number up to 255
      setPowerSave(0);
      drawString(0, 0, "Loading...");
    }

    // gets called every time WiFi is (re-)connected. Initialize own network
    // interfaces here
    void connected() {}

    /**
     * Da loop.
     */
    void loop() {
      if (millis() - lastUpdate < (clockMode?1000:refreshRate) || (!forceRotate && strip.isUpdating() && checkChangedType() != FLD_LINE_NULL && !wokeUp )) return; //WLEDSR(Harry B 210730): prevented display from updating within reasonable time on certain effects from SR-fork
      lastUpdate = millis();
      redraw(false);
      wokeUp = false; //WLEDSR: wokeUp handled
    }

    /**
     * Wrappers for screen drawing
     */
    void setFlipMode(uint8_t mode) {
      if (type==NONE) return;
      (static_cast<U8X8*>(u8x8))->setFlipMode(mode);
    }
    void setContrast(uint8_t contrast) {
      if (type==NONE) return;
      (static_cast<U8X8*>(u8x8))->setContrast(contrast);
    }
    void drawString(uint8_t col, uint8_t row, const char *string, bool ignoreLH=false) {
      if (type==NONE) return;
      (static_cast<U8X8*>(u8x8))->setFont(u8x8_font_chroma48medium8_r);
      if (!ignoreLH && lineHeight==2) (static_cast<U8X8*>(u8x8))->draw1x2String(col, row, string);
      else                            (static_cast<U8X8*>(u8x8))->drawString(col, row, string);
    }
    void draw2x2String(uint8_t col, uint8_t row, const char *string) {
      if (type==NONE) return;
      (static_cast<U8X8*>(u8x8))->setFont(u8x8_font_chroma48medium8_r);
      (static_cast<U8X8*>(u8x8))->draw2x2String(col, row, string);
    }
    void drawGlyph(uint8_t col, uint8_t row, char glyph, const uint8_t *font, bool ignoreLH=false) {
      if (type==NONE) return;
      (static_cast<U8X8*>(u8x8))->setFont(font);
      if (!ignoreLH && lineHeight==2) (static_cast<U8X8*>(u8x8))->draw1x2Glyph(col, row, glyph);
      else                            (static_cast<U8X8*>(u8x8))->drawGlyph(col, row, glyph);
    }
    uint8_t getCols() {
      if (type==NONE) return 0;
      return (static_cast<U8X8*>(u8x8))->getCols();
    }
    void clear() {
      if (type==NONE) return;
      (static_cast<U8X8*>(u8x8))->clear();
    }
    void setPowerSave(uint8_t save) {
      if (type==NONE) return;
      (static_cast<U8X8*>(u8x8))->setPowerSave(save);
    }

    //WLEDSR: move check to function to reuse code
    Line4Type checkChangedType() {
      if (((apActive) ? String(apSSID) : WiFi.SSID()) != knownSsid)
        return FLD_LINE_OTHER;
      else if (knownIp != (apActive ? IPAddress(4, 3, 2, 1) : Network.localIP()))
        return FLD_LINE_OTHER;
      else if (knownBrightness != bri)
        return FLD_LINE_BRIGHTNESS;
      else if (knownEffectSpeed != effectSpeed)
        return FLD_LINE_EFFECT_SPEED;
      else if (knownEffectIntensity != effectIntensity)
        return FLD_LINE_EFFECT_INTENSITY;
      else if (knownEffectFFT1 != effectFFT1)
        return FLD_LINE_EFFECT_FFT1;
      else if (knownEffectFFT2 != effectFFT2)
        return FLD_LINE_EFFECT_FFT2;
      else if (knownEffectFFT3 != effectFFT3)
        return FLD_LINE_EFFECT_FFT3;
      else if (knownMode != strip.getMode())
        return FLD_LINE_MODE;
      else if (knownPalette != strip.getSegment(0).palette)
        return FLD_LINE_PALETTE;
      else
        return FLD_LINE_NULL;
    }

    /**
     * Redraw the screen (but only if things have changed
     * or if forceRedraw).
     */
    void redraw(bool forceRedraw) {
      static bool showName = false;
      unsigned long now = millis();

      if (type==NONE) return;
      if (overlayUntil > 0) {
        if (now >= overlayUntil) {
          // Time to display the overlay has elapsed.
          overlayUntil = 0;
          forceRedraw = true;
        } else {
          // We are still displaying the overlay
          // Don't redraw.
          return;
        }
      }

      // Check if values which are shown on display changed from the last time.
      Line4Type changed = checkChangedType();
      if (forceRedraw || changed != FLD_LINE_NULL) {
        knownHour = 99; // force time update
        if (changed != FLD_LINE_OTHER) //not  ip or ssid
          lineType = changed; //WLEDSR: Always show last changed value
        clear();
      } else if (!displayTurnedOff && ((now - lastRedraw)/1000)%5 == 0) { //WLEDSR: remove if sleepMode, as rotating should take place independent of sleepmode
        // change line every 5s
        showName = !showName;
        switch (lineType) {
          case FLD_LINE_BRIGHTNESS:
            lineType = FLD_LINE_EFFECT_SPEED;
            break;
          case FLD_LINE_MODE:
            lineType = FLD_LINE_BRIGHTNESS;
            break;
          case FLD_LINE_PALETTE:
            lineType = clockMode ? FLD_LINE_MODE : FLD_LINE_BRIGHTNESS;
            break;
          case FLD_LINE_EFFECT_SPEED:
            lineType = FLD_LINE_EFFECT_INTENSITY;
            break;
          case FLD_LINE_EFFECT_INTENSITY:
            lineType = FLD_LINE_EFFECT_FFT1; //WLEDSR
            break;
          case FLD_LINE_EFFECT_FFT1:
            lineType = FLD_LINE_EFFECT_FFT2; //WLEDSR
            break;
          case FLD_LINE_EFFECT_FFT2:
            lineType = FLD_LINE_EFFECT_FFT3; //WLEDSR
            break;
          case FLD_LINE_EFFECT_FFT3:
            lineType = FLD_LINE_PRESET;
            break;
          case FLD_LINE_PRESET:
            lineType = FLD_LINE_PALETTE;
            break;
          default:
            lineType = FLD_LINE_MODE;
            break;
        }

        //WLEDSR: Skip slider if not used
        if (lineType == FLD_LINE_EFFECT_SPEED && strlen_P(sliderNames[0]) == 0) //slidername empty
          lineType = FLD_LINE_EFFECT_INTENSITY;
        if (lineType == FLD_LINE_EFFECT_INTENSITY && strlen_P(sliderNames[1]) == 0)
          lineType = FLD_LINE_EFFECT_FFT1;
        if (lineType == FLD_LINE_EFFECT_FFT1 && strlen_P(sliderNames[2]) == 0)
          lineType = FLD_LINE_EFFECT_FFT2;
        if (lineType == FLD_LINE_EFFECT_FFT2 && strlen_P(sliderNames[3]) == 0)
          lineType = FLD_LINE_EFFECT_FFT3;
        if (lineType == FLD_LINE_EFFECT_FFT3 && strlen_P(sliderNames[4]) == 0)
          lineType = FLD_LINE_PRESET;
        if (lineType == FLD_LINE_PRESET && currentPreset == -1)
          lineType = FLD_LINE_PALETTE;

        knownHour = 99; // force time update
      } else {
        // Nothing to change.
        // Turn off display after 3 minutes with no change.
        if(sleepMode && !displayTurnedOff && (millis() - lastRedraw > screenTimeout)) {
          // We will still check if there is a change in redraw()
          // and turn it back on if it changed.
          clear(); // force screen clear
          sleepOrClock(true);
        } else if (displayTurnedOff && clockMode) {
          showTime();
        }
        return;
      }

      // do not update lastRedraw marker if just switching row contenet
      if (((now - lastRedraw)/1000)%5 != 0) lastRedraw = now;
      
      // Turn the display back on
      if (displayTurnedOff) sleepOrClock(false);

      // Update last known values.
      #if defined(ESP8266)
        knownSsid = apActive ? WiFi.softAPSSID() : WiFi.SSID();
      #else
        knownSsid = WiFi.SSID();
      #endif
      knownIp = apActive ? IPAddress(4, 3, 2, 1) : Network.localIP();
      knownBrightness = bri;
      knownMode = strip.getMode();
      knownPalette = strip.getSegment(0).palette;
      knownEffectSpeed = effectSpeed;
      knownEffectIntensity = effectIntensity;
      knownEffectFFT1 = effectFFT1; //WLEDSR
      knownEffectFFT2 = effectFFT2; //WLEDSR
      knownEffectFFT3 = effectFFT3; //WLEDSR

      // Do the actual drawing

      // First row with Wifi name
      drawGlyph(0, 0, 80, u8x8_font_open_iconic_embedded_1x1); // home icon
      String ssidString = knownSsid.substring(0, getCols() > 1 ? getCols() - 2 : 0);
      drawString(1, 0, ssidString.c_str());
      // Print `~` char to indicate that SSID is longer, than our display
      if (knownSsid.length() > getCols()) {
        drawString(getCols() - 1, 0, "~");
      }

      // Second row with IP or Psssword
      drawGlyph(0, lineHeight, 68, u8x8_font_open_iconic_embedded_1x1); // wifi icon
      // Print password in AP mode and if led is OFF.
      if (apActive && bri == 0) {
        drawString(1, lineHeight, apPass);
      } else {
        // alternate IP address and server name
        String secondLine = knownIp.toString();
        if (showName && strcmp(serverDescription, "WLED") != 0) {
          secondLine = serverDescription;
        }
        for (uint8_t i=secondLine.length(); i<getCols()-1; i++) secondLine += ' ';
        drawString(1, lineHeight, secondLine.c_str());
      }

      // draw third and fourth row
      drawLine(2, clockMode ? lineType : FLD_LINE_MODE);
      drawLine(3, clockMode ? FLD_LINE_TIME : lineType);

      //WLEDSR: Show icon per variable
      if (clockMode)
        drawGlyph(0, 3*lineHeight, 65, u8x8_font_open_iconic_embedded_1x1); // clock icon
      //else done in showCurrentEffectOrPalette

      switch (lineType) {
        case FLD_LINE_BRIGHTNESS:
          drawGlyph(0, (clockMode?2:3)*lineHeight, 66 + (bri > 0 ? 3 : 0), u8x8_font_open_iconic_weather_1x1); // sun/moon icon
          break;
        case FLD_LINE_MODE: //nothing displayed as this is done in showCurrentEffectOrPalette
          break;
        case FLD_LINE_PALETTE: //nothing displayed as this is done in showCurrentEffectOrPalette
          break;
        case FLD_LINE_EFFECT_SPEED:
          drawGlyph(0, (clockMode?2:3)*lineHeight, 72, u8x8_font_open_iconic_play_1x1); // fast forward icon
          break;
        case FLD_LINE_EFFECT_INTENSITY:
          drawGlyph(0, (clockMode?2:3)*lineHeight, 78, u8x8_font_open_iconic_thing_1x1); // kind of fire icon
          break;
        case FLD_LINE_EFFECT_FFT1:
          drawGlyph(0, (clockMode?2:3)*lineHeight, 68, u8x8_font_open_iconic_weather_1x1); // star icon
          break;
        case FLD_LINE_EFFECT_FFT2:
          drawGlyph(0, (clockMode?2:3)*lineHeight, 68, u8x8_font_open_iconic_weather_1x1); // star icon
          break;
        case FLD_LINE_EFFECT_FFT3:
          drawGlyph(0, (clockMode?2:3)*lineHeight, 68, u8x8_font_open_iconic_weather_1x1); // star icon
          break;
        case FLD_LINE_PRESET:
          drawGlyph(0, (clockMode?2:3)*lineHeight, 88, u8x8_font_open_iconic_arrow_1x1); // circle like icon
          break;
        case FLD_LINE_TIME:
          drawGlyph(0, (clockMode?2:3)*lineHeight, 65, u8x8_font_open_iconic_embedded_1x1); //  clock icon
          break;
        default:
        //   drawGlyph(0, (clockMode?2:3)*lineHeight, 72, u8x8_font_open_iconic_play_1x1); //  icon
          break;
      }
      //if (markLineNum>1) drawGlyph(2, markLineNum*lineHeight, 66, u8x8_font_open_iconic_arrow_1x1); // arrow icon
    }

    //WLEDSR: Use custom slidernames
    void drawLine(uint8_t line, Line4Type lineType) {
      char lineBuffer[LINE_BUFFER_SIZE];
      switch(lineType) {
        case FLD_LINE_BRIGHTNESS:
          sprintf_P(lineBuffer, PSTR("Brightness  %3d"), bri);
          drawString(1, line*lineHeight, lineBuffer);
          break;
        case FLD_LINE_EFFECT_SPEED:
          sprintf_P(lineBuffer, PSTR("%.11s %3d"), sliderNames[0], effectSpeed);
          drawString(1, line*lineHeight, lineBuffer);
          break;
        case FLD_LINE_EFFECT_INTENSITY:
          sprintf_P(lineBuffer, PSTR("%.11s %3d"), sliderNames[1], effectIntensity);
          drawString(1, line*lineHeight, lineBuffer);
          break;
        case FLD_LINE_EFFECT_FFT1: //WLEDSR
          sprintf_P(lineBuffer, PSTR("%.11s %3d"), sliderNames[2], effectFFT1);
          drawString(1, line*lineHeight, lineBuffer);
          break;
        case FLD_LINE_EFFECT_FFT2: //WLEDSR
          sprintf_P(lineBuffer, PSTR("%.11s %3d"), sliderNames[3], effectFFT2);
          drawString(1, line*lineHeight, lineBuffer);
          break;
        case FLD_LINE_EFFECT_FFT3: //WLEDSR
          sprintf_P(lineBuffer, PSTR("%.11s %3d"), sliderNames[4], effectFFT3);
          drawString(1, line*lineHeight, lineBuffer);
          break;
        case FLD_LINE_PRESET:
          sprintf_P(lineBuffer, PSTR("FX Preset   %3d"), currentPreset);
          drawString(1, line*lineHeight, lineBuffer);
          break;
        case FLD_LINE_MODE:
          showCurrentEffectOrPalette(knownMode, JSON_mode_names, line);
          break;
        case FLD_LINE_PALETTE:
          showCurrentEffectOrPalette(knownPalette, JSON_palette_names, line);
          break;
        case FLD_LINE_TIME:
        default:
          showTime(false);
          break;
      }
    }

    /**
     * Display the current effect or palette (desiredEntry) 
     * on the appropriate line (row).
     */
    void showCurrentEffectOrPalette(int knownMode, const char *qstring, uint8_t row) {
      char lineBuffer[LINE_BUFFER_SIZE];
      uint8_t printedChars = 0;
      char singleJsonSymbol;

      //WLEDSR
      bool insideQuotes = false;
      uint8_t qComma = 0;
      bool insideSliders = false;
      uint8_t printedCharsSliders = 0;
      uint8_t sliderCounter = 0;
      bool noteFound = false;
      bool atSignFound = false;
      bool isEffect = strlen_P(qstring) > 2000; //currently 777 vs 4451

      // Find the mode name in JSON
      //WLEDSR: check for dynamic sliders
      DEBUG_PRINTLN("Display new effect started");
      for (size_t i = 0; i < strlen_P(qstring); i++) {
        singleJsonSymbol = pgm_read_byte_near(qstring + i);
        if ((singleJsonSymbol == ']') && (qComma == knownMode)) break;       //WLEDSR(HarryB): Stop reading, we've got all we want
        if (singleJsonSymbol == '\0') break;
        switch (singleJsonSymbol) {
          case '"':
            if (!insideQuotes) { //begin of string
              noteFound = false;
              sliderCounter = 0; //start counting sliders
              atSignFound = false;
            }
            insideQuotes = !insideQuotes;
            break;
          case '[':
            break;
          case 226: //WLEDSR: first byte of the 3 byte note character is for ♪ and ♫ 226
            noteFound = true;
            break;
          case '@':
            printedCharsSliders = 0; //set char counter to 0
            insideSliders = true;
            atSignFound = true;
            break;
          case '!':
            if (insideSliders) {
              sprintf_P(sliderNames[sliderCounter], sliderDefaults[sliderCounter]);
              printedCharsSliders = strlen_P(sliderNames[sliderCounter]); //set char counter to 0
            }
            break;
          case ',':
            if (!insideQuotes)       //WLEDSR(HarryB) added condition to differentiate between comma in mode name or as seperator
              qComma++;
            if (insideSliders) { //WLEDSR: if still in slider parsing, comma is end of string
              sliderNames[sliderCounter][printedCharsSliders] = 0; //end of string
              sliderCounter++; //go to next slider
              printedCharsSliders = 0; //set char counter to 0
            }
            break;
          case ';':
            if (insideSliders) { //if still parsing sliders
              sliderNames[sliderCounter][printedCharsSliders] = 0; //end of string
              sliderCounter++; //go to next slider
              insideSliders = false; //end of sliders
            }
            break;
          case ']':
            break;
          default:
            if (!insideQuotes || (qComma != knownMode)) break; //if not current mode, do nothing
            if ((!( (printedChars >= getCols()-1) || printedChars >= sizeof(lineBuffer)-1)) && !atSignFound)
              lineBuffer[printedChars++] = singleJsonSymbol;
            if (insideSliders && sliderCounter < 5 && printedCharsSliders < LINE_BUFFER_SIZE && insideSliders) {
              sliderNames[sliderCounter][printedCharsSliders++] = singleJsonSymbol;
            }
        }
        if ((qComma > knownMode)) break;
      }
      //if no atsign add 2 default
      if (isEffect && !atSignFound) {
        sprintf_P(sliderNames[sliderCounter++], sliderDefaults[0]);
        sprintf_P(sliderNames[sliderCounter++], sliderDefaults[1]);
      }
      //else if no slidernames then nothing

      for (;printedChars < getCols()-1 && printedChars < sizeof(lineBuffer)-1; printedChars++) lineBuffer[printedChars]=' '; //add spaces
      lineBuffer[printedChars] = 0; //end of string
      char* lineBufferPtr = lineBuffer;

      if (isEffect) {
        if (noteFound)
          lineBufferPtr = lineBuffer + 4; //WLEDSR: remove 2 spaces and the 2 of the 3 byte note characters (1 is skipped in the switch)
        for (uint8_t i = 0; i < sliderCounter; i++) {
          if (strlen_P(sliderNames[i]) > 0) {
            for (printedCharsSliders = strlen_P(sliderNames[i]);printedCharsSliders < getCols()-1 && printedCharsSliders < sizeof(sliderNames[i])-1; printedCharsSliders++) sliderNames[i][printedCharsSliders]=' '; //add spaces
            sliderNames[i][printedCharsSliders] = 0; //end of string
          }
          DEBUG_PRINT(i);
          DEBUG_PRINTLN(sliderNames[i]);
        }
      }
      DEBUG_PRINTLN(lineBufferPtr);
      drawString(1, row*lineHeight, lineBufferPtr);

      //WLEDSR: Add effect or palette icon
      if (isEffect) {
        if (noteFound)
          drawGlyph(0, row*lineHeight, 77, u8x8_font_open_iconic_play_1x1); // note effect
        else
          drawGlyph(0, row*lineHeight, 70, u8x8_font_open_iconic_thing_1x1); // normal effect
      }
      else {
        drawGlyph(0, row*lineHeight, 72, u8x8_font_open_iconic_thing_1x1); // pallette
      }
    }

    /**
     * If there screen is off or in clock is displayed,
     * this will return true. This allows us to throw away
     * the first input from the rotary encoder but
     * to wake up the screen.
     */
    bool wakeDisplay() {
      knownHour = 99;
      wokeUp = true; //WLEDSR: to force screen refresh
      if (displayTurnedOff) {
        // Turn the display back on
        sleepOrClock(false);
        redraw(true);
        return true;
      }
      return false;
    }

    /**
     * Allows you to show up to two lines as overlay for a
     * period of time.
     * Clears the screen and prints on the middle two lines.
     */
    void overlay(const char* line1, const char *line2, long showHowLong) {
      if (displayTurnedOff) {
        // Turn the display back on
        sleepOrClock(false);
      }

      // Print the overlay
      clear();
      if (line1) drawString(0, 1*lineHeight, line1);
      if (line2) drawString(0, 2*lineHeight, line2);
      overlayUntil = millis() + showHowLong;
    }

    void setLineType(byte lT) {
      lineType = (Line4Type) lT;
    }

    /**
     * Line 3 or 4 (last two lines) can be marked with an
     * arrow in the first column. Pass 2 or 3 to this to
     * specify which line to mark with an arrow.
     * Any other values are ignored.
     */
    void setMarkLine(byte newMarkLineNum) {
      if (newMarkLineNum == 2 || newMarkLineNum == 3) {
        markLineNum = newMarkLineNum;
      }
      else {
        markLineNum = 0;
      }
    }

    /**
     * Enable sleep (turn the display off) or clock mode.
     */
    void sleepOrClock(bool enabled) {
      if (enabled) {
        if (clockMode) showTime();
        else           setPowerSave(1);
        displayTurnedOff = true;
      } else {
        setPowerSave(0);
        displayTurnedOff = false;
      }
    }

    /**
     * Display the current date and time in large characters
     * on the middle rows. Based 24 or 12 hour depending on
     * the useAMPM configuration.
     */
    void showTime(bool fullScreen = true) {
      char lineBuffer[LINE_BUFFER_SIZE];

      updateLocalTime();
      byte minuteCurrent = minute(localTime);
      byte hourCurrent   = hour(localTime);
      byte secondCurrent = second(localTime);
      if (knownMinute == minuteCurrent && knownHour == hourCurrent) {
        // Time hasn't changed.
        if (!fullScreen) return;
      } else {
        //if (fullScreen) clear();
      }
      knownMinute = minuteCurrent;
      knownHour = hourCurrent;

      byte currentMonth = month(localTime);
      sprintf_P(lineBuffer, PSTR("%s %2d "), monthShortStr(currentMonth), day(localTime));
      if (fullScreen)
        draw2x2String(DATE_INDENT, lineHeight==1 ? 0 : lineHeight, lineBuffer); // adjust for 8 line displays
      else
        drawString(1, lineHeight*3, lineBuffer);

      byte showHour = hourCurrent;
      boolean isAM = false;
      if (useAMPM) {
        if (showHour == 0) {
          showHour = 12;
          isAM = true;
        } 
        else if (showHour > 12) {
          showHour -= 12;
          isAM = false;
        }
        else {
          isAM = true;
        }
      }

      sprintf_P(lineBuffer, (secondCurrent%2 || !fullScreen) ? PSTR("%2d:%02d") : PSTR("%2d %02d"), (useAMPM ? showHour : hourCurrent), minuteCurrent);
      // For time, we always use LINE_HEIGHT of 2 since
      // we are printing it big.
      if (fullScreen) {
        draw2x2String(TIME_INDENT+2, lineHeight*2, lineBuffer);
        sprintf_P(lineBuffer, PSTR("%02d"), secondCurrent);
        if (useAMPM) drawString(12+(fullScreen?0:2), lineHeight*2, (isAM ? "AM" : "PM"), true);
        else         drawString(12, lineHeight*2+1, lineBuffer, true); // even with double sized rows print seconds in 1 line
      } else {
        drawString(9+(useAMPM?0:2), lineHeight*3, lineBuffer);
        if (useAMPM) drawString(12+(fullScreen?0:2), lineHeight*3, (isAM ? "AM" : "PM"), true);
      }
    }

    /*
     * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
     * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
     * Below it is shown how this could be used for e.g. a light sensor
     */
    //void addToJsonInfo(JsonObject& root) {
      //JsonObject user = root["u"];
      //if (user.isNull()) user = root.createNestedObject("u");
      //JsonArray data = user.createNestedArray(F("4LineDisplay"));
      //data.add(F("Loaded."));
    //}

    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    //void addToJsonState(JsonObject& root) {
    //}

    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    //void readFromJsonState(JsonObject& root) {
    //  if (!initDone) return;  // prevent crash on boot applyPreset()
    //}

    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * If you want to force saving the current state, use serializeConfig() in your loop().
     * 
     * CAUTION: serializeConfig() will initiate a filesystem write operation.
     * It might cause the LEDs to stutter and will cause flash wear if called too often.
     * Use it sparingly and always in the loop, never in network callbacks!
     * 
     * addToConfig() will also not yet add your setting to one of the settings pages automatically.
     * To make that work you still have to add the setting to the HTML, xml.cpp and set.cpp manually.
     * 
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
    void addToConfig(JsonObject& root) {
      JsonObject top   = root.createNestedObject(FPSTR(_name));
      JsonArray io_pin = top.createNestedArray("pin");
      for (byte i=0; i<5; i++) io_pin.add(ioPin[i]);
      top["help4PinTypes"]       = F("Clk,Data,CS,DC,RST"); // help for Settings page
      top["type"]                = type;
      top[FPSTR(_flip)]          = (bool) flip;
      top[FPSTR(_contrast)]      = contrast;
      top[FPSTR(_refreshRate)]   = refreshRate/1000;
      top[FPSTR(_screenTimeOut)] = screenTimeout/1000;
      top[FPSTR(_sleepMode)]     = (bool) sleepMode;
      top[FPSTR(_clockMode)]     = (bool) clockMode;
      top[FPSTR(_forceRotate)]     = (bool) forceRotate;
      DEBUG_PRINTLN(F("4 Line Display config saved."));
    }

    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens once immediately after boot)
     * 
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     */
    bool readFromConfig(JsonObject& root) {
      bool needsRedraw    = false;
      DisplayType newType = type;
      int8_t newPin[5]; for (byte i=0; i<5; i++) newPin[i] = ioPin[i];

      JsonObject top = root[FPSTR(_name)];
      if (top.isNull()) {
        DEBUG_PRINT(FPSTR(_name));
        DEBUG_PRINTLN(F(": No config found. (Using defaults.)"));
        return false;
      }

      newType       = top["type"] | newType;
      for (byte i=0; i<5; i++) newPin[i] = top["pin"][i] | ioPin[i];
      flip          = top[FPSTR(_flip)] | flip;
      contrast      = top[FPSTR(_contrast)] | contrast;
      refreshRate   = (top[FPSTR(_refreshRate)] | refreshRate/1000) * 1000;
      screenTimeout = (top[FPSTR(_screenTimeOut)] | screenTimeout/1000) * 1000;
      sleepMode     = top[FPSTR(_sleepMode)] | sleepMode;
      clockMode     = top[FPSTR(_clockMode)] | clockMode;
      forceRotate     = top[FPSTR(_forceRotate)] | forceRotate;

      DEBUG_PRINT(FPSTR(_name));
      if (!initDone) {
        // first run: reading from cfg.json
        for (byte i=0; i<5; i++) ioPin[i] = newPin[i];
        type = newType;
        DEBUG_PRINTLN(F(" config loaded."));
      } else {
        DEBUG_PRINTLN(F(" config (re)loaded."));
        // changing parameters from settings page
        bool pinsChanged = false;
        for (byte i=0; i<5; i++) if (ioPin[i] != newPin[i]) { pinsChanged = true; break; }
        if (pinsChanged || type!=newType) {
          if (type != NONE) delete (static_cast<U8X8*>(u8x8));
          for (byte i=0; i<5; i++) {
            if (ioPin[i]>=0) pinManager.deallocatePin(ioPin[i]);
            ioPin[i] = newPin[i];
          }
          if (ioPin[0]<0 || ioPin[1]<0) { // data & clock must be > -1
            type = NONE;
            return true;
          } else type = newType;
          setup();
          needsRedraw |= true;
        }
        setContrast(contrast);
        setFlipMode(flip);
        if (needsRedraw && !wakeDisplay()) redraw(true);
      }
      // use "return !top["newestParameter"].isNull();" when updating Usermod with new features
      return true;
    }

    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId() {
      return USERMOD_ID_FOUR_LINE_DISP;
    }
};

// strings to reduce flash memory usage (used more than twice)
const char FourLineDisplayUsermod::_name[]          PROGMEM = "4LineDisplay";
const char FourLineDisplayUsermod::_contrast[]      PROGMEM = "contrast";
const char FourLineDisplayUsermod::_refreshRate[]   PROGMEM = "refreshRateSec";
const char FourLineDisplayUsermod::_screenTimeOut[] PROGMEM = "screenTimeOutSec";
const char FourLineDisplayUsermod::_flip[]          PROGMEM = "flip";
const char FourLineDisplayUsermod::_sleepMode[]     PROGMEM = "sleepMode";
const char FourLineDisplayUsermod::_clockMode[]     PROGMEM = "clockMode";
const char FourLineDisplayUsermod::_forceRotate[]   PROGMEM = "forceRotate";
