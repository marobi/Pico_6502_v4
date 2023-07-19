// 
// Author: Rien Matthijsse
// 
#include "memory.h"
#include "m6821.h"
#include "sound.h"

#include "roms.h"

extern void writeChar(uint8_t);
extern void setCommand(uint8_t);
extern void showCursor(boolean);
extern uint8_t getCursorX();
extern uint8_t getCursorY();

/// <summary>
/// 64k RAM
/// </summary>
uint8_t mem[MEMORY_SIZE];

// address and data registers
uint16_t address;
uint8_t  data;

/// <summary>
/// initialise memory
/// </summary>
void initmemory() {
  address = 0UL;
  data = 0;

  // lets install some ROMS
  if (loadROMS()) {
    Serial.println("ROMs installed");
  }
}

/// <summary>
/// read a byte from memory
/// </summary>
/// <param name=address"</param>
/// <param name=data"</param>
/// <returns></returns>
void readmemory() {
  // 6821?
  if (KBD <= address && address <= DSPCR) {
    switch (address) {
    case KBD: // KBD?
      if (regKBDCR & 0x02) {
        // KBD register  
        data = regKBD;
        regKBDCR &= 0x7F;    // clear IRQA bit upon read
      }
      else
        data = regKBDDIR;
      break;

    case KBDCR:
      // KBDCR register
      data = regKBDCR;
      break;

    case DSP:
      if (regDSPCR & 0x02) {
        // DSP register  
        data = regDSP;
        regDSPCR &= 0x7F;    // clear IRQA bit upon read
      }
      else
        data = regDSPDIR;
      break;

    case DSPCR:
      // DSPCR register
      data = regDSPCR;
      break;
    }
  }
  else if (0xD020 <= address && address < 0xD040) { // VDU/SOUND
    switch (address) {
    case 0XD020:
      data = 0x00;
      break;
    case 0xD021:
      data = getCursorX();
      break;
    case 0xD022:
      data = getCursorY();
      break;

    case 0xD030:
      data = SoundQueueIsEmpty();
      break;

    default:
      data = 0x00;
      break;
    }
  }
  else
    data = mem[address];
}

/// <summary>
/// store a byte into memory
/// </summary>
/// <param name="address"></param>
/// <param name="data"></param>
void writememory() {
  // 6821?
  if (KBD <= address && address <= DSPCR) {
    switch (address) {
    case KBD: // KBD?
      if (regKBDCR & 0x02) {
        // KBD register
        regKBD = data;
      }
      else
        regKBDDIR = data;
      break;

    case KBDCR:
      // KBDCR register
      regKBDCR = data & 0X7F;
      break;

    case DSP:
      if (regDSPCR & 0x02) {
        // DSP register
        writeChar(regDSP = (data & 0x7F));
      }
      else
        regDSPDIR = data;
      break;

    case DSPCR:
      // DSPCR register
      regDSPCR = data;
      break;
    }
  }
  else if (0xD020 <= address && address < 0xD040) { // VDU/SOUND controller
    switch (address) {
    case 0XD020:
      setCommand(data);
      break;

    case 0xD030:      // SOUND CMD
      switch (data) {
      case 0:         // RESET
        SoundReset();
        break;
      case 1:        // PUSH
        SoundPushTheNote();
        break;
      }
      break;

    case 0xD031:     // NOTE
      SoundSetNote(data);
      break;

    case 0xD032:    // DURATION
      SoundSetDuration(data);
      break;
    }

    mem[address] = data;
  }
  else if ((0x8000 <= address && address <= 0xCFFF) || (0xF000 <= address && address <= 0xFFF9)) { // exclude writing ROM
    Serial.printf("access violation [%04X]\n", address);
  }
  else
    mem[address] = data;
}
