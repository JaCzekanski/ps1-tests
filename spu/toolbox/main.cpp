#include <common.h>
#include <io.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include <psxspu.h>
#include "spu_ram.h"
#include "spu_reverb.h"

#define SCR_W 640
#define SCR_H 240

#define OT_LEN 2
#define PACKET_LEN 65535

struct DB {
    DISPENV disp;
    DRAWENV draw;
    uint32_t ot[OT_LEN];
    uint32_t ptr[PACKET_LEN];
};

DB db[2]; 
int activeDb = 0; // Double buffering index
uint32_t* ptr;

char padBuffer[2][34];
unsigned short buttons = 0xffff, prevButtons = 0xffff;

bool BUTTON_DOWN(uint16_t button) {
	return (buttons & button) == 0;
}

bool BUTTON(uint16_t button) {
	return (buttons & button) == 0 && ((prevButtons & button) != 0);
}

void init() {
    ResetGraph(0);
    SetVideoMode(MODE_NTSC);
    for (int i = 0; i<=1; i++) {
        SetDefDispEnv(&db[i].disp, 0, !i ? 0 : SCR_H, SCR_W, SCR_H);
        SetDefDrawEnv(&db[i].draw, 0, !i ? SCR_H : 0, SCR_W, SCR_H);

        db[i].disp.isinter = false; // Progressive mode
        db[i].disp.isrgb24 = false; // 16bit mode

        db[i].draw.dtd = false; // Disable dither
        db[i].draw.isbg = true; // Clear bg on PutDrawEnv
        setRGB0(&db[i].draw, 0, 0, 0);

        ClearOTagR(db[i].ot, OT_LEN);
    }
    activeDb = 0;

	PutDrawEnv(&db[activeDb].draw);
    ptr = db[activeDb].ptr;

    SetDispMask(1);

	InitPAD(padBuffer[0], 34, padBuffer[1], 34);
	StartPAD();
	ChangeClearPAD(0);

	FntLoad(960, 0);
	FntOpen(16, 8, SCR_W-8, SCR_H-8, 0, 2048);	
}

void swapBuffers() {
    FntFlush(-1);

    DrawSync(0); // Wait for GPU to finish rendering 
    VSync(0);

    activeDb = !activeDb;
    ptr = db[activeDb].ptr;

    // ClearOTagR(db[activeDb].ot, OT_LEN);

    PutDrawEnv(&db[activeDb].draw);
    PutDispEnv(&db[activeDb].disp);

    // DrawOTag(&db[1 - activeDb].ot[OT_LEN - 1]);

	prevButtons = buttons;
	buttons = ((PADTYPE*)padBuffer[0])->btn;
}


void SpuKeyOn(uint32_t mask) {
    volatile uint32_t* SPU_KEYON  = (uint32_t*)0x1F801D88;
    *SPU_KEYON = mask;
}

void SpuKeyOff(uint32_t mask) {
    volatile uint32_t* SPU_KEYOFF = (uint32_t*)0x1F801D8C;
    *SPU_KEYOFF = mask;
}

const uint32_t SPU_DELAY = 0x1F801014;
const uint32_t COM_DELAY = 0x1F801020;

const uint32_t CH_VOL_L = 0x1F801C00;
const uint32_t CH_VOL_R = 0x1F801C02;

const uint32_t MAIN_VOLUME_L = 0x1F801D80;
const uint32_t MAIN_VOLUME_R = 0x1F801D82;
const uint32_t REVERB_VOLUME_L = 0x1F801D84;
const uint32_t REVERB_VOLUME_R = 0x1F801D86;
const uint32_t SPUCNT = 0x1F801DAA;
const uint32_t SPUSTAT = 0x1F801DAE;
const uint32_t SPUDTC = 0x1F801DAC;

const uint32_t REVERB = 0x1F801D98; // Reverb enabled
const uint32_t NON = 0x1F801D94; // ADPCM/Noise
const uint32_t PMON = 0x1F801D90; // Pitch modulation
const uint32_t KON = 0x1F801D88;  // KeyON
const uint32_t KOFF = 0x1F801D8C; // KeyOff
const uint32_t ENDX = 0x1F801D9C;

const uint32_t CURR_VOLUME_L = 0x1F801DB8;
const uint32_t CURR_VOLUME_R = 0x1F801DBA;

const uint32_t REVERB_AREA_START = 0x1F801DA2;
const uint32_t REVERB_REG_BASE = 0x1F801DC0;

const uint32_t voiceBase = 0x1F801C00;

int muted = false;

void setMainVolume(int16_t volume) {
    write16(MAIN_VOLUME_L, volume);
    write16(MAIN_VOLUME_R, volume);
}

void setReverbVolume(int16_t volume) {
    write16(REVERB_VOLUME_L, volume);
    write16(REVERB_VOLUME_R, volume);
}

void uploadToSpu(unsigned char buf[], unsigned size) {
    SpuSetTransferMode(4); // ?
    SpuSetTransferStartAddr(0);
    SpuWrite(buf, size);
    SpuWait();
}

// Observations:
// Setting bit15 to 0 in SPUCNT (SPU Enable)
// sets all VOL_ADSR to 0 which mutes all channels
// but ADPCM decoder is still working (ENDX flag is set when reached)

int main() {
    init();
    printf("\nspu/toolbox\n");
    printf("Live preview of SPU registers\n");
    printf("Controller mapping\n");
    printf("^, v - Change selected voice\n");
    printf("Start - (with R1 - init SPU, with L1 - toggle mute) else toggle SPU\n");
    printf("Select - toggle Master Reverb bit\n");
    printf("Triangle - toggle Reverb Volume\n");
    printf("Square - set current channel (with R1 - all channels) to max volume\n");
    printf("Cross - Key On for current channel (with R1 - all channels) (with L1 - play VibRibbon track)\n");
    printf("Circle - Key Off for current channel (with R1 - all channels)\n");
	DrawSync(0);

    SpuInit();
    // Upload SPU RAM state from Vib Ribbon
    uploadToSpu(spu_ram, sizeof(spu_ram));

    // Upload Reverb from BIOS
    for (int i = 0; i < sizeof(spu_reverb); i += 2) {
        write16(REVERB_REG_BASE + i, spu_reverb[i] | (spu_reverb[i+1] << 8));
    }
    write16(REVERB_AREA_START, 0xE128);

    printf("R SPU_DELAY: 0x%08x\n", read32(SPU_DELAY));
    
    uint32_t spuDelay = 0x200931E1;
    printf("W SPU_DELAY: 0x%08x\n", spuDelay);
    write32(SPU_DELAY, spuDelay);

    
    printf("R COM_DELAY: 0x%08x\n", read32(COM_DELAY));

    uint32_t comDelay = 0x00031125;
    printf("W COM_DELAY: 0x%08x\n", comDelay);
    write32(COM_DELAY, comDelay);


    printf("R SPU_DTC: 0x%04x\n", read16(SPUDTC));

    uint32_t spudtc = 4;
    printf("W SPU_DTC: 0x%04x\n", spudtc);
    write32(SPUDTC, spudtc);

    setMainVolume(0x3fff);

    int selectedVoice = 0;
    for (;;) {
		if (BUTTON(PAD_UP)) {
            if (selectedVoice>0) selectedVoice--;
        }
		if (BUTTON(PAD_DOWN)) {
            if (selectedVoice<23) selectedVoice++;
        }
		if (BUTTON(PAD_LEFT)) {
        }
		if (BUTTON(PAD_RIGHT)) {
        }
		if (BUTTON(PAD_START)) {
            uint16_t spucnt = read16(SPUCNT);
            
            if (BUTTON_DOWN(PAD_L1)) {
                spucnt ^= (1<<14); // mute
            } else {
                spucnt ^= (1<<15); // Toggle SPU
            }
            if (BUTTON_DOWN(PAD_R1)) {
                spucnt = 0xc081;
            }
            write16(SPUCNT, spucnt);
        }
		if (BUTTON(PAD_SELECT)) {
            // Toggle Reverb master enable
            uint16_t spucnt = read16(SPUCNT);
            spucnt ^= (1<<7);
            write16(SPUCNT, spucnt);
        }
		if (BUTTON(PAD_TRIANGLE)) {
            muted = !muted;
            setReverbVolume(muted ? 0 : 0x3fff);
        }
		if (BUTTON(PAD_SQUARE)) {
            if (BUTTON_DOWN(PAD_R1)) {
                for (size_t i = 0; i < 24; i++) {
                    write16(CH_VOL_L + i * 0x10, 0x3fff);
                    write16(CH_VOL_R + i * 0x10, 0x3fff);
                }
            } else {
                write16(CH_VOL_L + selectedVoice * 0x10, 0x3fff);
                write16(CH_VOL_R + selectedVoice * 0x10, 0x3fff);
            }
        }

		if (BUTTON(PAD_CROSS)) {
            if (BUTTON_DOWN(PAD_L1)) {
                uint32_t off = voiceBase + selectedVoice * 0x10;
                uint16_t sampleRate = 0x800;
                uint16_t startAddr = 0x4000;
                uint16_t repeatAddr = 0x8000;
                uint32_t adsr = 0x1fc080ff;
                
                write16(CH_VOL_L + selectedVoice * 0x10, 0x3fff);
                write16(CH_VOL_R + selectedVoice * 0x10, 0x3fff);

                write16(off + 0x04, sampleRate);
                write16(off + 0x06, startAddr);
                write16(off + 0x08, adsr&0xfffff);
                write16(off + 0x0a, (adsr>>16)&0xffff);
                write16(off + 0x0e, repeatAddr);
                
                write16(REVERB, 1 << selectedVoice);
                SpuKeyOn(1<<selectedVoice);
            } else if (BUTTON_DOWN(PAD_R1)) {
                SpuKeyOn(0xFFFFFF);
            } else {
                SpuKeyOn(1<<selectedVoice);
            }
        }
		if (BUTTON(PAD_CIRCLE)) {
            if (BUTTON_DOWN(PAD_R1)) {
                SpuKeyOff(0xFFFFFF);
            } else {
                SpuKeyOff(1<<selectedVoice);
            }
        }

        FntPrint(-1, " Vx "
                    "V_L  "
                    "V_R  "
                    "V_ADSR  "
                    "Rate  "
                    "Start "
                    "Repeat  "
                    "adsr     "
                    "Rev "
                    "NON "
                    "PMON "
                    "KON "
                    "KOF "
                    "END "
                    "\n");

        uint32_t reverb = read16(REVERB) | read16(REVERB+2) << 16;
        uint32_t non = read16(NON) | read16(NON+2) << 16;
        uint32_t pmon = read16(PMON) | read16(PMON+2) << 16;
        uint32_t kon = read16(KON) | read16(KON+2) << 16;
        uint32_t koff = read16(KOFF) | read16(KOFF+2) << 16;
        uint32_t endx = read16(ENDX) | read16(ENDX+2) << 16;
        for (int v = 0; v<24; v++) {
            uint32_t off = voiceBase + v * 0x10;
            uint16_t volumeLeft    = read16(off + 0x00);
            uint16_t volumeRight   = read16(off + 0x02);
            uint16_t sampleRate    = read16(off + 0x04);
            uint16_t startAddress  = read16(off + 0x06);
            uint32_t adsr          = read16(off + 0x08) | read16(off + 0x0a)<<16;
            uint16_t volumeAdsr    = read16(off + 0x0c);
            uint16_t repeatAddress = read16(off + 0x0e);

            FntPrint(-1, "%c%2d "
                         "%04x "
                         "%04x "
                         "%04x    "
                         "%04x  "
                         "%04x  "
                         "%04x    "
                         "%08x "
                         "%d   "
                         "%d   "
                         "%d    "
                         "%d   "
                         "%d   "
                         "%d",
                     v == selectedVoice ? '>' : ' ', v + 1,
                     volumeLeft,
                     volumeRight,
                     volumeAdsr,
                     sampleRate,
                     startAddress,
                     repeatAddress,
                     adsr,
                     reverb & (1 << v) ? 1 : 0,
                     non & (1 << v) ? 1 : 0,
                     pmon & (1 << v) ? 1 : 0,
                     kon & (1 << v) ? 1 : 0,
                     koff & (1 << v) ? 1 : 0,
                     endx & (1 << v) ? 1 : 0);
            FntPrint(-1, "\n");
        }

        FntPrint(-1, "MAIN_VOLUME: %04x%04x  ", read16(MAIN_VOLUME_L), read16(MAIN_VOLUME_R));
        FntPrint(-1, "CURR_VOLUME: %04x%04x  ", read16(CURR_VOLUME_L), read16(CURR_VOLUME_R));
        FntPrint(-1, "RVRB_VOLUME: %04x%04x\n", read16(REVERB_VOLUME_L), read16(REVERB_VOLUME_R));
        uint16_t spucnt = read16(SPUCNT);
        FntPrint(-1, "SPUCNT: 0x%04x  (spu_en=%d, mute=%d, master_reverb=%d, cd_reverb=%d)\n", spucnt, 
            (spucnt & (1<<15)) != 0, // spu enable
            (spucnt & (1<<14)) != 0, // mute
            (spucnt & (1<<7)) != 0, // master_reverb
            (spucnt & (1<<2)) != 0 // cd_reverb
        );
        FntPrint(-1, "IRQ_ADDR: 0x%04x\n", read16(0x1F801DA4));
        FntPrint(-1, "SPUSTAT: 0x%04x\n", read16(SPUSTAT));
        FntPrint(-1, "0x1F801DA0: 0x%04x  ", read16(0x1F801DA0));
        FntPrint(-1, "0x1F801DBC: 0x%08x\n", read32(0x1F801DBC));

        swapBuffers();    
    }

    return 0;
}
