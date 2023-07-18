/*
 Name:		Pico_NEO6502
 Author:	Rien Matthijsse

 Inspired by work  http://www.8bitforce.com/blog/2019/03/12/retroshield-6502-operation/
*/

#include "pico/stdio.h"

#include "pico/stdlib.h"
#include "pico/time.h"

//#ifdef OVERCLOCK
//#include "hardware/clocks.h"
//#include "hardware/vreg.h"
//#endif

#include <PicoDVI.h>

#include "mos65C02.h"
#include "memory.h"
#include "m6821.h"
#include "sound.h"

#define FRAMERATE       10 // frames per sec

#define FRAMETIME     1000 / FRAMERATE  // msec
#define WIDTH          320
#define HEIGHT         240
#define FONT_CHAR_WIDTH  6  
#define FONT_CHAR_HEIGHT 8 
#define LINES       HEIGHT / (FONT_CHAR_HEIGHT + 1)
#define LINECHARS   WIDTH / FONT_CHAR_WIDTH

// Here's how an 320x240 256 colors graphics display is declared.
DVIGFX8 display(DVI_RES_320x240p60, true, pico_neo6502_cfg);

//
uint32_t       clockCount = 0UL;
unsigned long  lastClockTS;
unsigned long  frameClockTS;

boolean        logState = false;
boolean        statusCursor = true;
uint8_t        textMode = 0;
uint8_t        currentColor;
uint8_t        currentColorIndex = 0;
uint32_t       hasDisplayUpdate = 0;

/// <summary>
/// performa a action on the display
/// </summary>
/// <param name="vCmd"></param>
void setCommand(uint8_t vCmd) {
  int16_t cx, cy, ex, ey;

  switch (vCmd) {
  case 0x01: // clearscreen
    display.fillScreen(0);
    display.setCursor(0, 0);
    hasDisplayUpdate++;
    break;

  case 0x02: // set cursor
    switch (textMode) {
    case 0: // text mode LINES x LINECHARS
      cx = (((uint16_t)mem[0xD022] * 256 + mem[0xD021]) * FONT_CHAR_WIDTH) % WIDTH;
      cy = (((uint16_t)mem[0xD024] * 256 + mem[0xD023]) * (FONT_CHAR_HEIGHT + 1)) % HEIGHT;
      //      Serial.printf("TCURSOR: %02d %02d\n", cx, cy);
      display.setCursor(cx, cy);
      hasDisplayUpdate++;
      break;
    case 13: // graphics WIDTH x HEIGHT
      cx = (((uint16_t)mem[0xD022] * 256) + mem[0xD021]) % WIDTH;
      cy = (((uint16_t)mem[0xD024] * 256) + mem[0xD023]) % HEIGHT;
      display.setCursor(cx, cy);
      //      Serial.printf("GCURSOR: %04d %04d\n", cx, cy);
      hasDisplayUpdate++;
      break;
    default:
      break;
    }
    break;

  case 0x03: // set color
    setColor(mem[0xD029]);
//    Serial.printf("COLOR %02d\n", currentColor);
    // bg color ignored
    break;

  case 0x04: // set pixel (ignore screen mode)
    cx = (((uint16_t)mem[0xD022] * 256) + mem[0xD021]) % WIDTH;
    cy = (((uint16_t)mem[0xD024] * 256) + mem[0xD023]) % HEIGHT;

    display.drawPixel(cx, cy, mem[0xD029]);
    hasDisplayUpdate++;
    break;

  case 0x05: // draw line
    cx = (((uint16_t)mem[0xD022] * 256) + mem[0xD021]) % WIDTH;
    cy = (((uint16_t)mem[0xD024] * 256) + mem[0xD023]) % HEIGHT;
    ex = (((uint16_t)mem[0xD026] * 256) + mem[0xD025]) % WIDTH;
    ey = (((uint16_t)mem[0xD028] * 256) + mem[0xD027]) % HEIGHT;

    display.drawLine(cx, cy, ex, ey, mem[0xD029]);
    hasDisplayUpdate++;
    break;

  case 0xFE: // hide cursor
    showCursor(false);
    hasDisplayUpdate++;
    break;

  case 0xFF: // show cursor
    showCursor(true);
    hasDisplayUpdate++;
    break;
  }
}

/// <summary>
/// control visibility of cursor
/// </summary>
/// <param name="vSet"></param>
void showCursor(boolean vSet) {
  statusCursor = vSet;
}

/// <summary>
/// get current x-pos of cursor
/// </summary>
/// <returns></returns>
uint8_t getCursorX() {
  return display.getCursorX() / LINECHARS;
}

/// <summary>
/// get current y-pos of cursor
/// </summary>
/// <returns></returns>
uint8_t getCursorY() {
  return display.getCursorY() / LINES;
}

/// <summary>
/// set current text color (by pallette index)
/// </summary>
/// <param name="vColor"></param>
void setColor(uint8_t vColor) {
  currentColor = vColor;
  display.setTextColor(vColor);
}

/// <summary>
/// 
/// </summary>
inline __attribute__((always_inline))
void removeCursor() {
  int16_t cursor_x, cursor_y;
  // remove optional cursor
  if (statusCursor) {
    cursor_x = display.getCursorX();
    cursor_y = display.getCursorY();
    display.fillRect(cursor_x, cursor_y, FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT, 0); // remove cursor
  }
}


/// <summary>
/// 
/// </summary>
inline __attribute__((always_inline))
void setCursor() {
  int16_t cursor_x, cursor_y;
  // remove optional cursor
  if (statusCursor) {
    cursor_x = display.getCursorX();
    cursor_y = display.getCursorY();
    display.fillRect(cursor_x, cursor_y + FONT_CHAR_HEIGHT - 2, FONT_CHAR_WIDTH , 2, currentColor); // show cursor
  }
}

/// <summary>
/// 
/// </summary>
/// <param name="c"></param>
/// <returns></returns>
inline __attribute__((always_inline))
void displayWrite(uint8_t c) {
  int16_t cursor_x, cursor_y;

  cursor_x = display.getCursorX();
  cursor_y = display.getCursorY();

  removeCursor();
  if (c == '\n') { // Carriage return
    cursor_x = 0;
  }
  else if ((c == '\r') || (cursor_x >= WIDTH)) { // Newline OR right edge
    cursor_x = 0;
    if (cursor_y >= (HEIGHT - 9)) { // Vert scroll?
      memmove(display.getBuffer(), display.getBuffer() + WIDTH * (FONT_CHAR_HEIGHT + 1), WIDTH * (HEIGHT - (FONT_CHAR_HEIGHT + 1)));
      display.drawFastHLine(0, HEIGHT - 9, WIDTH, 0); // Clear bottom line
      display.fillRect(0, HEIGHT - 9, WIDTH, FONT_CHAR_HEIGHT + 1, 0);

      cursor_y = HEIGHT - 9;
    }
    else {
      cursor_y += FONT_CHAR_HEIGHT + 1;
    }

    display.setCursor(cursor_x, cursor_y);
  }

  switch (c) {
  case '\r':
  case '\n':
    break;
  case 0x08:
    display.fillRect(cursor_x - FONT_CHAR_WIDTH, cursor_y, FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT, 0);
    display.setCursor(cursor_x - FONT_CHAR_WIDTH, cursor_y);
    break;
  case 0x20:
    display.fillRect(cursor_x, cursor_y, FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT, 0);
    display.setCursor(cursor_x + FONT_CHAR_WIDTH, cursor_y);
    break;
  default:
    display.setTextColor(currentColor);
    display.write(c);
    break;
  }
  setCursor();

  hasDisplayUpdate++;
}


/// <summary>
///  write a char to output DVI output
/// </summary>
/// <param name="vChar"></param>
void writeChar(uint8_t vChar) {
//  Serial.printf("out [%02X]\n", vChar);

  displayWrite(vChar);
  hasDisplayUpdate++;
}

///
void initDisplay() {
  display.setColor(0, 0x0000);   // Black
  display.setColor(1, 0XF800);   // Red
  display.setColor(2, 0x07e0);   // Green
  display.setColor(3, 0xffe0);   // Yellow
  display.setColor(4, 0x001f);   // Blue
  display.setColor(5, 0xFA80);   // Orange
  display.setColor(6, 0xF8F9);   // Magenta
  display.setColor(255, 0xFFFF); // Last palette entry = White
  // Clear back framebuffer
  display.fillScreen(0);
  display.setFont();             // Use default font
  display.setCursor(0, 0);       // Initial cursor position
  display.setTextSize(1);        // Default size
  display.setTextWrap(false);
  display.swap(false, true);     // Duplicate same palette into front & back buffers
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// <summary>
/// setup emulator
/// </summary>
void setup() {
  Serial.begin(115200);
  //  while (!Serial);

  sleep_ms(2500);
  Serial.println("NEO6502 memulator v0.02c");

  if (!display.begin()) {
    Serial.println("ERROR: not enough RAM available");
    for (;;);
  }

  Serial.printf("Starting ...\n");

  initDisplay();

  initmemory();

  init6821();
  initSound();

  init6502();
  reset6502();

  // 4 stats
  clockCount = 0UL;
  lastClockTS = millis();

  // and we have lift off
  setColor(4); // BLUE
  display.print("NEO6502");
  setColor(255); // WHITE
  display.println(" memulator v0.02c");
  setColor(2); // GREEN
}

////////////////////////////////////////////////////////////////////
// Serial Event
////////////////////////////////////////////////////////////////////

/*
  SerialEvent occurs whenever a new data comes in the
  hardware serial RX. Multiple bytes of data may be available.
 */
inline __attribute__((always_inline))
void serialEvent1()
{
  if (Serial.available()) {
    switch (Serial.peek()) {

    case 0x12: // ^R
      Serial.read();
      Serial.println("RESET");
//      showCursor(true);
      reset6502();
      break;

    case 0x0C: // ^L
      Serial.read();
      Serial.println("LOGGING");
      logState = !logState;
      clockCount = 0UL;
      break;

    case 0x04: // ^D
      Serial.read();
      Serial.print("VDU: ");
      for (uint8_t i = 0; i < 16; i++) {
        Serial.printf("%02X ", mem[0XD020 + i]);
      }
      Serial.println();
      break;

    default:
      if ((regKBDCR & 0x80) == 0x00) {    // read serial byte only if we can set 6821 interrupt
        cli();                            // stop interrupts while changing 6821 guts.
        // 6821 portA is available      
        byte ch = toupper(Serial.read()); // apple1 expects upper case
        //       Serial.printf("in [%02X]\n", ch);
        regKBD = ch | 0x80;               // apple1 expects bit 7 set for incoming characters.
        //     Serial.printf("Pressed %02x\n", regKBD);
        regKBDCR |= 0x80;                 // set 6821 interrupt
        sei();
      }
      break;
    }
  }
  return;
}

uint32_t rpt;

/// <summary>
/// 
/// </summary>
void loop() {
  static uint32_t i, j, f = 1;

  tick6502();
  clockCount++;

  if (j-- == 0) {
    serialEvent1();

    j = 2500;
  }

  if (f-- == 0) {
    rpt++;
    if ((millis() - frameClockTS) >= FRAMETIME) {
      if (hasDisplayUpdate > 0) {
        display.swap(true, false);
        hasDisplayUpdate = 0;
      }

      frameClockTS = millis();
      rpt = 0;
    }


    f = 7500;
  }
    
    // only do stas when in loggin mode
  if (logState) {
    if (i-- == 0) {
      if ((millis() - lastClockTS) >= 5000UL) {
        Serial.printf("kHz = %0.1f\n", clockCount / 5000.0);

        clockCount = 0UL;
        lastClockTS = millis();
      }

      i = 20000;
    }
  }
}
