# Pico_6502_v4

***NOTE I discontinued further development on this version. For further development see the Pico_6502_v4_ehbasic project.
Main reason is that the MSBASIC can not be extended with more commands. All commands need to fit into one page (256bytes)


this version is my take of the NEO6502 memulator supporting a 320x240 256 color display, dual buffered.

In order to have the board work correctly, connect UEXT.3 to 6502BUS.40

on the 'console' u can use:

^R : reset the 6502 processor.

^L : toggle statistics output, currently the clock-speed is printed.

^D : dump the 16 VDU registers.


When the 6502 executes a BRK, its registers are dumped and the EWOZ monitor is reentered.
The ROMs are write-protected and can not be overwritten by the 6502.

At 0xF000 u will find the mini assembler/disassembler

At 0xBD47 u will find the coldstart of MSBASIC V1.1. Just type ENTER to answer the two questions.

MSBASIC has been extended and the following commands are/will be added:
- CLS
- CURSOR
- LINE
- PALETTE (TBI)
- PIXEL
- PLAY
- NOP
- SCREEN (TBI)
