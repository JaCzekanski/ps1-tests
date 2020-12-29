#include <common.h>
#include <psxapi.h>
#include <psxsio.h>
#include <timer.h>
#include <malloc.h>
#include <ioctl.h>
#include <sys/fcntl.h>
#include <hexdump.h>
#include <delay.h>
#include "string.h"
#include "getline.h"
#include "cd.h"
#include "fifo.h"

struct command_t {
    const char* name;
    bool haveInt = false;
};

command_t cdCommands[] = {
    /* 0x00*/ {"unknown-0x00"}, 
    /* 0x01*/ {"Getstat"}, 
    /* 0x02*/ {"Setloc"}, 
    /* 0x03*/ {"Play"}, 
    /* 0x04*/ {"Forward"}, 
    /* 0x05*/ {"Backward"}, 
    /* 0x06*/ {"ReadN", true}, 
    /* 0x07*/ {"MotorOn", true}, 
    /* 0x08*/ {"Stop", true}, 
    /* 0x09*/ {"Pause", true}, 
    /* 0x0A*/ {"Init", true}, 
    /* 0x0B*/ {"Mute"}, 
    /* 0x0C*/ {"Demute"}, 
    /* 0x0D*/ {"Setfilter"}, 
    /* 0x0E*/ {"Setmode"}, 
    /* 0x0F*/ {"Getparam"}, 
    /* 0x10*/ {"GetlocL"}, 
    /* 0x11*/ {"GetlocP"}, 
    /* 0x12*/ {"SetSession", true}, 
    /* 0x13*/ {"GetTN"}, 
    /* 0x14*/ {"GetTD"}, 
    /* 0x15*/ {"SeekL", true}, 
    /* 0x16*/ {"SeekP", true}, 
    /* 0x17*/ {"unknown-0x17"}, 
    /* 0x18*/ {"unknown-0x18"}, 
    /* 0x19*/ {"Test"}, 
    /* 0x1A*/ {"GetID", true}, 
    /* 0x1B*/ {"ReadS", true}, 
    /* 0x1C*/ {"Reset", true}, 
    /* 0x1D*/ {"GetQ", true}, 
    /* 0x1E*/ {"ReadTOC"}, 
    /* 0x1F*/ {"VideoCD"}, 
};

// in us
uint32_t getTimer() {
    uint32_t value = readTimer(1);
    if (timerDidOverflow(1)) value += 0xffff;
    resetTimer(1);

    return (value*1'000'000) / (263 * 60);
}

int parseCdromCommand(string& arg) {
    int no = arg.asInt();
    if (no != -1) return no;

    for (int i = 0; i<sizeof(cdCommands)/sizeof(cdCommands[0]); i++) {
        if (arg.equalsIgnoreCase(cdCommands[i].name)) return i;
    }
    return 0xff;
}

uint8_t databuf[32*1024];

bool verbose = false;
bool loop = false;
bool ascii = false;

bool shouldBreak(){
    if (ioctl(0, FIOCSCAN, 0)) {
        if (getchar() == 0x03) {
            return true;
        }
    }
    return false;
};

#define HANDLE_BREAK {if (shouldBreak()) {printf("Break at line %d.\n", __LINE__); return;}}

void logCommand(uint8_t funcNo, fifo<uint8_t, 16> params) {
    printf("Issuing [0x%02x] %s", funcNo, (funcNo < 0x20) ? cdCommands[funcNo].name  : "");
    if (!params.is_empty()) {
        printf("(");
        for (int i = 0; i<params.size(); i++) {
            printf("0x%02x", params[i]);
            if (i != params.size() - 1) printf(", ");
        }
        printf(")");
    }
    printf("\n");
}

void logControllerState(int line = 0) {
    uint8_t irq = cdGetInt(false);
    uint8_t status = read8(0x1F801800);
    printf("INT_FLAGS: 0x%02x,  CD_STATUS: 0x%02x  (%8s %10s %10s %10s %11s)  (line: %d)\n", 
        irq,
        status, 
        status & (1<<7) ? "CMD_BUSY":"",
        status & (1<<6) ? "":"DATA_EMPTY",
        status & (1<<5) ? "":"RESP_EMPTY",
        status & (1<<4) ? "": "PARAM_FULL",
        status & (1<<3) ? "PARAM_EMPTY" : "",
        line
    );
}

#define WAIT_UNTIL(x) \
for (;;) { \
    if (verbose) logControllerState(__LINE__); \
    if (x) break; \
    HANDLE_BREAK; \
}

void cdromCommand(uint8_t funcNo, fifo<uint8_t, 16> params) {
    uint8_t prevStatus = 0, prevIrq = 0;

    while (cdGetInt() != 0); // Ack all INTs
    cdClearParameterFifo();

    for (int i = 0; (loop ? true : i<1); i++) {
        auto _params = params;

        while (!_params.is_empty()) cdPushParameter(_params.get());
        resetTimer(1);
        cdCommand(funcNo);

        // cmdBusy will be set only during parameter transmission and cmd execution
        // but it'll be 0 for next INTs
        WAIT_UNTIL(!cdCommandBusy());
        
        int irqCount = 0;
anotherResponse:
        int irq = 0;

        // Wait for IRQ, but do not ack it yet!
        // Doing so will tell the subcpu that it is okay to push further responses
        // ACK if after we've emptied the response fifo
        WAIT_UNTIL((irq = cdGetInt(false)) != 0);
        irqCount++;
        
        // Wait for response fifo (could be done with irq check)
        WAIT_UNTIL(!cdResponseEmpty());
        
        // Empty response fifo
        uint8_t status = cdReadResponse();
        if (prevStatus != status || prevIrq != irq) {
            auto ms = getTimer() / 1000;
            printf("[%d.%-3ds] < CD IRQ=%d, status=0x%02x ", ms / 1'000, ms % 1'000, irq, status);
            while (!cdResponseEmpty()) {
                uint8_t c = cdReadResponse();
                if (ascii && isAscii(c)) printf("%c", c);
                else printf("0x%02x ", c);
            }
            printf("\n");
        } else {
            // Just clear out the response fifo
            while (!cdResponseEmpty()) {cdReadResponse();}
        }
        prevStatus = status;
        prevIrq = irq;

        // Ack IRQ
        cdGetInt();

        // If INT5 == error - no more responses (even if the command has multiple responses)
        if (irq != 5 && cdCommands[funcNo].haveInt && irqCount < 2) {
            goto anotherResponse;
        }

        if (irq == 1) {
            cdWantData(true);
            while (cdDataEmpty());

            int dataPtr = 0;
            while (!cdDataEmpty()) {
                databuf[dataPtr++] = cdReadData();
            }

            cdWantData(false);

            hexdump(databuf, dataPtr, 16, true);
        }

        HANDLE_BREAK;
    }
    printf("\n");
}

// TODO: Display latest buffer data on INT1

int main()
{
    initVideo(320, 240);
    SetVideoMode(MODE_NTSC);
    clearScreen();

    printf("\ncdrom/terminal\n");
    printf("Used for interacting with CD Controller at low level.\n");
    printf("Use UART terminal at 115200! Default stdio device will be removed.\n");
    AddSIO(115200);
    printf("\ncdrom/terminal\n");

    cdInit();

    uint16_t oldTimer1Mode = initTimer(1, 1); // Timer1, HBlank

    char buf[128];
    int ptr = 0;
    auto nextArg = [&]() -> string {
        // Skip leading whitespace
        while (true) {
            char c = buf[ptr];
            if (c == ' ' || c == '\t') ptr++;
            else break;
        }

        // Get length of string
        int start = ptr;
        int len = 0;
        for (; ;) {
            char c = buf[ptr];

            if (c == 0 || c == ' ' || c == '\t') {
                break;
            } else {
                ptr++;
                len++;
            }
        }
        return string(buf + start, len);
    };
    auto peekArg = [&]() -> string {
        int p = ptr;
        string arg = nextArg();
        ptr = p;
        return arg;
    };

    for (;;)
    {
        ptr = 0;

        printf("> ");
        getline(buf, 128);

        loop = false;
        ascii = false;
        verbose = false;

        string cmd = nextArg();
        if (cmd.empty()) {
            printf("See help for command list\n");
            continue;
        } 
        else if (cmd == "loop") {
            loop = true;
            cmd = nextArg();
        }
        else if (cmd == "quit" || cmd == 'q' || cmd == "exit") {
            printf("Exiting.\n");
            break;
        } 
        else if (cmd == "help") {
            printf("spuinit [vol] - initialize SPU registers for cd playback\n");
            printf("dumpfifo      - Read CDROM data fifo (might not work)\n");
            printf("reg / .       - Read CDROM status, interrupt and response registers\n");
            printf("CDROM commands (number or one of listed below): \n");
            for (auto cmd : cdCommands) {
                printf("  %s\n", cmd.name);
            }
            printf("\nCD commands can accept 0 or more arguments.");
            printf("Flags:\n");
            printf("  --ascii     - print response as text\n");
            printf("  --loop      - issue command in loop until response changes\n");
            printf("  --verbose   - show CD Controller status during busy loops\n\n");
            continue;
        } 
        else if (cmd == "spuinit") {
            string argVolume = nextArg();
            int volume = argVolume.asInt();
            if (volume == -1) volume = 10;

            const uint32_t SPU_DELAY = 0x1F801014;
            const uint32_t COM_DELAY = 0x1F801020;
            const uint32_t SPUCNT = 0x1F801DAA;

            const uint32_t MAIN_VOLUME_L = 0x1F801D80;
            const uint32_t MAIN_VOLUME_R = 0x1F801D82;
            const uint32_t CD_VOLUME_L = 0x1F801DB0;
            const uint32_t CD_VOLUME_R = 0x1F801DB2;
            write32(SPU_DELAY, 0x200931E1);
            write32(COM_DELAY, 0x00031125);
            write16(SPUCNT, (1<<15) | 1);
            
            write16(MAIN_VOLUME_L, 0x3fff * (volume/100.f));
            write16(MAIN_VOLUME_R, 0x3fff * (volume/100.f));
            write16(CD_VOLUME_L, 0x7fff);
            write16(CD_VOLUME_R, 0x7fff);
            printf("SPU initialized for CD playback. (volume=%d)\n", volume);
            continue;   
        }
        else if (cmd == "dumpfifo") {
            size_t len = 2048;
            string argLen = nextArg();
            if (argLen.asInt() > 0) len = argLen.asInt();

            for (int i = 0; i<len; i++) {
                databuf[i] = read8(0x1F801802);
            }
            hexdump(databuf, len);
            continue;   
        }
        else if (cmd == "reg" || cmd == '.') {
            logControllerState(__LINE__);

            if (!cdResponseEmpty()) {
                printf("Response: ");
                while (!cdResponseEmpty()) {
                    printf("0x%02x ", cdReadResponse());
                }
                printf("\n");
            }
            continue;
        }
        else if (cmd == "ack") {
            cdGetInt();
            continue;
        }
        int funcNo = parseCdromCommand(cmd);
        if (funcNo == 0xff) {
            printf("[error] Unknown command %s\n", cmd.s);
            continue;
        }

        if (peekArg() == "--ascii") {
            ascii = true;
            nextArg();
        }

        if (peekArg() == "--loop") {
            loop = true;
            nextArg();
        }

        if (peekArg() == "--verbose") {
            verbose = true;
            nextArg();
        }

        if (loop) {
            printf("press ctrl+c to break\n\n");
        }
        
        fifo<uint8_t, 16> params;
        for (int i = 0; i<16; i++) {
            auto param = nextArg();
            if (param.empty()) break;
            params.add(param.asInt());
        }

        logCommand(funcNo, params);
        cdromCommand(funcNo, params);
    }

    restoreTimer(1, oldTimer1Mode);

    return 0;
}
