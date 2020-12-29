#include "cd.h"
#include <io.h>
#include <psxapi.h>
#include <stdio.h>

static int _index = -1;
void writeReg(uint8_t address, uint8_t index, uint8_t data) {
    if (_index != index) {
        write8(0x1F801800, index);
        _index = index;
    }
    write8(0x1F801800 + address, data);
}
uint8_t readReg(uint8_t address, uint8_t index) {
    if (_index != index) {
        write8(0x1F801800, index);
        _index = index;
    }
    return read8(0x1F801800 + address);
}

// Status
#define CD_STAT 0x1F801800
#define BUSY_STS (1<<7)
#define RSLR_RDY (1<<5)
#define DRQ_STS  (1<<6)

// Write
#define INT_EN 2, 1
#define INT_FLAG 3, 1
#define REQUEST_REG 3, 0

#define ATV0 2, 2  // L to L
#define ATV1 3, 2  // L to R
#define ATV2 1, 3  // R to R
#define ATV3 2, 3  // R to L
#define ADP_CTL 3, 3 

#define COMMAND 1, 0
#define PARAMETER 2, 0

#define HINT_CTL 3, 1

// Read
#define RESULT 1, 0
#define HINT_STS 3, 1

void cdInit() {
    EnterCriticalSection();

    write32(0x1f801018, 0x00020943); // CDROM Delay/Size
    write32(0x1f801020, 0x00001325); // COM_DELAY

    writeReg(INT_EN, 0x1f);
    writeReg(INT_FLAG, 0x1f);
    
    cdWantData(false); // Reset data fifo

    writeReg(ATV0, 0x80);
    writeReg(ATV1, 0);
    writeReg(ATV2, 0x80);
    writeReg(ATV3, 0);
    writeReg(ADP_CTL, 0x20); // Apply volume

    ExitCriticalSection();
}

void cdClearParameterFifo() {
    writeReg(INT_FLAG, (1<<6));
}

void cdPushParameter(uint8_t param) {
    writeReg(PARAMETER, param);
}

void cdCommand(uint8_t cmd) {
    writeReg(COMMAND, cmd);
}

uint8_t cdReadResponse() {
    return readReg(RESULT);
}

uint8_t cdGetInt(bool ack) {
    uint8_t no = readReg(HINT_STS) & 0x1f;
    if (ack && no != 0) writeReg(HINT_CTL, no);
    return no;
}

uint8_t cdReadData() {
    return read8(0x1f801802);
}

void cdWantData(bool wantData) {
    writeReg(REQUEST_REG, (wantData << 7));
}

// Status bits
bool cdCommandBusy() {
    return (read8(CD_STAT) & BUSY_STS) != 0;
}

bool cdResponseEmpty() {
    return (read8(CD_STAT) & RSLR_RDY) == 0;
}

bool cdDataEmpty() {
    return (read8(CD_STAT) & DRQ_STS) == 0;
}
