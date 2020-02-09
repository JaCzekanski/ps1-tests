#include <common.h>
#include <dma.hpp>
#include <io.h>
#include <psxcd.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>

#define WRITE8(addr, data) do { \
    printf("[W] " #addr ": 0x%02x\n", data); \
    write8(addr, data); \
} while(0)

#define WRITE16(addr, data) do { \
    printf("[W] " #addr ": 0x%04x\n", data); \
    write16(addr, data); \
} while(0)

#define WRITE32(addr, data) do { \
    printf("[W] " #addr ": 0x%08x\n", data); \
    write32(addr, data); \
} while(0)

const uint32_t SPU_DELAY = 0x1F801014;
const uint32_t COM_DELAY = 0x1F801020;

const uint32_t MAIN_VOLUME_L = 0x1F801D80;
const uint32_t MAIN_VOLUME_R = 0x1F801D82;
const uint32_t CD_VOLUME_L = 0x1F801DB0;
const uint32_t CD_VOLUME_R = 0x1F801DB2;
const uint32_t SPUCNT = 0x1F801DAA;
const uint32_t SPUSTAT = 0x1F801DAE;
const uint32_t SPUDTC = 0x1F801DAC;

const uint32_t CDROM_BASE = 0x1F801800;
const uint32_t CDROM_LEFT_TO_SPU_LEFT = CDROM_BASE+2;
const uint32_t CDROM_LEFT_TO_SPU_RIGHT = CDROM_BASE+3;
const uint32_t CDROM_RIGHT_TO_SPU_LEFT = CDROM_BASE+1;
const uint32_t CDROM_RIGHT_TO_SPU_RIGHT = CDROM_BASE+2;
const uint32_t CDROM_APPLY_VOLUME = CDROM_BASE+3;

char padBuffer[2][34];
unsigned short buttons = 0xffff, prevButtons = 0xffff;

bool BUTTON_DOWN(uint16_t button) {
	return (buttons & button) == 0;
}

bool BUTTON(uint16_t button) {
	return (buttons & button) == 0 && ((prevButtons & button) != 0);
}

extern "C" {
extern volatile char _cd_media_changed;

void _cd_init_mine(void);
};

int CD_init(int mode)
{
	// Sets up CD-ROM hardware and low-level subsystem
	_cd_init_mine();
	
	_cd_media_changed = 1;
	
	// Issue commands to initialize the CD-ROM hardware
	CdControl(CdlNop, 0, 0);
	// CdControl(CdlInit, 0, 0);
	
	// if( CdSync(0, 0) != CdlDiskError )
	// {
	// 	CdControl(CdlDemute, 0, 0);
	// 	printf("psxcd: Init Ok!\n");
	// }
	// else
	// {
	// 	printf("psxcd: Error initializing. Bad disc/drive or no disc inserted.\n");
	// }
	
	return 1;
}
void CD_setVolume(uint8_t left, uint8_t right) {
    write8(CDROM_BASE, 2);
    write8(CDROM_LEFT_TO_SPU_LEFT, left);
    write8(CDROM_LEFT_TO_SPU_RIGHT, 0);
    write8(CDROM_BASE, 3);
    write8(CDROM_RIGHT_TO_SPU_LEFT, 0);
    write8(CDROM_RIGHT_TO_SPU_RIGHT, right);
    write8(CDROM_APPLY_VOLUME, (1<<5));

    printf("CD Volume left: 0x%02x, right: 0x%02x\n", left, right);
}

uint8_t CD_currentTrack() {
    uint8_t result[8];
    CdControl(CdlGetlocP, NULL, result);

    return result[0];
}

bool playing = true;
bool muted = false;
uint8_t volume = 0x80;

void CD_play(uint8_t track) {
    CdControl(CdlPlay, &track, 0);
    printf("Playing track %d\n", track);
    playing = true;
}

void SPU_setTransferMode(int mode) {
    mode = (mode&3) << 4;
    uint16_t spucnt = read16(SPUCNT);
    
    spucnt &= ~(0x30);
    spucnt |= (1<<15) | mode; // DMA won't work with SPU disabled
    write16(SPUCNT, spucnt); // Sound RAM transfer mode
    while ((read16(SPUSTAT) & 0x30) != mode); // Wait to apply
}

// Size in 16 blocks
void SPU_read(uint8_t* buffer, uint32_t address, size_t size) {
    write32(SPU_DELAY, 0x220931E1);
    write16(SPUDTC, 4);

    SPU_setTransferMode(0); // STOP
    write16(0x1F801DA6, address / 8);
    SPU_setTransferMode(3); // DMAread

    using namespace DMA;
    masterEnable(Channel::SPU, true);
    waitForChannel(Channel::SPU);

    auto addr    = MADDR((uint32_t)buffer);
    auto block   = BCR::mode1(0x10, size / 0x10 / 4);
    auto control = CHCR::SPUread();

    write32(CH_BASE_ADDR    + 0x10 * (int)Channel::SPU, addr._reg);
    write32(CH_BLOCK_ADDR   + 0x10 * (int)Channel::SPU, block._reg);
    write32(CH_CONTROL_ADDR + 0x10 * (int)Channel::SPU, control._reg);

    waitForChannel(Channel::SPU);
}

uint8_t buffer[1024];

int main() {
    initVideo(320, 240);
    printf("\ncdrom/volume-regs\n");
    clearScreen();

    printf(">   Init pad\n");
	InitPAD(padBuffer[0], 34, padBuffer[1], 34);
	StartPAD();
	ChangeClearPAD(0);

    printf(">   Init SPU\n");
    WRITE32(SPU_DELAY, 0x200931E1);
    WRITE32(COM_DELAY, 0x00031125);
    WRITE32(SPUDTC, 4);

    printf("[R] SPU Main Volume L: 0x%04x\n", read16(MAIN_VOLUME_L));
    printf("[R] SPU Main Volume R: 0x%04x\n", read16(MAIN_VOLUME_R));
    
    printf("[R] SPU CD Volume L: 0x%04x\n", read16(CD_VOLUME_L));
    printf("[R] SPU CD Volume R: 0x%04x\n", read16(CD_VOLUME_R));

    printf(">   Set SPU Main volume\n");
    WRITE16(MAIN_VOLUME_L, 0x3fff);
    WRITE16(MAIN_VOLUME_R, 0x3fff);
    
    printf(">   Set SPU CD volume\n");
    WRITE16(CD_VOLUME_L, 0x7fff);
    WRITE16(CD_VOLUME_R, 0x7fff);

    printf(">   Enable SPU and CD input in SPUCNT\n");
    WRITE16(SPUCNT, (1<<15) | 1);

    printf(">   Init CD subsystem\n");
    CD_init(1);

    // Default values for CD Volume should be 80 0 0 80
    // printf(">   Setup CD mixing registers\n");
    // CD_setVolume(volume, volume);

    printf(">   Issue play command\n");
    // CD_play(1);

    printf("Done.\n");
    for (;;) {
		if (BUTTON_DOWN(PAD_UP)) {
            if (volume < 255) {
                volume += 1;
                CD_setVolume(volume, volume);
            }
        }
		if (BUTTON_DOWN(PAD_DOWN)) {
            if (volume > 0) {
                volume -= 1;
                CD_setVolume(volume, volume);
            }
        }
		if (BUTTON(PAD_SQUARE)) {
            SPU_read(buffer, 0x0000, 1024);
            for (int i = 0; i<1024; ) {
                printf("%04x  ", i);
                for (int x = 0; x<16; x++) {
                    printf("%02x ", buffer[i++]);
                }
                printf("\n");
            }
            printf("\n");
        }
		if (BUTTON(PAD_TRIANGLE)) {
            if (muted) {
                CdControl(CdlDemute, NULL, 0);
                printf("Unmute\n");
            } else {
                CdControl(CdlMute, NULL, 0);
                printf("Mute\n");
            }
            muted = !muted;
        }
		if (BUTTON(PAD_CROSS)) {
            if (playing) {
                CdControl(CdlPause, NULL, 0);
                printf("Pause\n");
            } else {
                CdControl(CdlPlay, NULL, 0);
                printf("Play\n");
            }
            playing = !playing;
        }
		if (BUTTON(PAD_L1)) {
            CD_play(CD_currentTrack()-1);
        }
		if (BUTTON(PAD_R1)) {
            CD_play(CD_currentTrack()+1);
        }
		if (BUTTON(PAD_LEFT)) {
            // TODO: Backward/forward is lowering the volume?
            // Audible when rewinding back to the start of a disc
            // Plays with normal speed, but quieter
            CdControl(CdlBackward, NULL, 0);
            playing = false;
            printf("Backward\n");
        }
		if (BUTTON(PAD_RIGHT)) {
            CdControl(CdlForward, NULL, 0);
            playing = false;
            printf("Forward\n");
        }

        VSync(0);
        prevButtons = buttons;
        buttons = ((PADTYPE*)padBuffer[0])->btn;
    }

    return 0;
}
