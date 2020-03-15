#pragma once
#include "stdint.h"

// Copied from Avocado
namespace DMA {
enum class Channel { MDECin, MDECout, GPU, CDROM, SPU, PIO, OTC };

const inline uint32_t CH_BASE_ADDR    = 0x1F801080;
const inline uint32_t CH_BLOCK_ADDR   = 0x1F801084;
const inline uint32_t CH_CONTROL_ADDR = 0x1F801088;
const inline uint32_t CONTROL_ADDR    = 0x1F8010F0;

// DMA base address
union MADDR {
    struct {
        uint32_t address : 24;
        uint32_t : 8;
    };
    uint32_t _reg;
    uint8_t _byte[4];

    MADDR() : _reg(0) {}

    MADDR(uint32_t addr) {
        address = addr;
    }
};

// DMA Block Control
union BCR {
    union {
        struct {
            uint32_t wordCount : 16;
            uint32_t : 16;
        } syncMode0;
        struct {
            uint32_t blockSize : 16;
            uint32_t blockCount : 16;
        } syncMode1;
    };
    uint32_t _reg;
    uint8_t _byte[4];

    BCR() : _reg(0) {}

    static BCR mode0(uint16_t wordCount) {
        BCR bcr;
        bcr.syncMode0.wordCount = wordCount;
        return bcr;
    }

    static BCR mode1(uint16_t blockSize, uint16_t blockCount) {
        BCR bcr;
        bcr.syncMode1.blockSize = blockSize;
        bcr.syncMode1.blockCount = blockCount;
        return bcr;
    }
};

// DMA Channel Control
union CHCR {
    enum class Direction : uint32_t { toRam = 0, fromRam = 1 };
    enum class MemoryAddressStep : uint32_t { forward = 0, backward = 1 };
    enum class SyncMode : uint32_t { startImmediately = 0, syncBlockToDmaRequests = 1, linkedListMode = 2, reserved = 3 };
    enum class ChoppingEnable : uint32_t { normal = 0, chopping = 1 };
    enum class Enabled : uint32_t { completed = 0, stop = 0, start = 1 };
    enum class StartTrigger : uint32_t { clear = 0, automatic = 0, manual = 1 };

    struct {
        Direction direction : 1;
        MemoryAddressStep memoryAddressStep : 1;
        uint32_t : 6;
        uint32_t choppingEnable : 1;
        SyncMode syncMode : 2;
        uint32_t : 5;
        uint32_t choppingDmaWindowSize : 3;  // Chopping DMA Window Size (1 SHL N words)
        uint32_t : 1;
        uint32_t choppingCpuWindowSize : 3;  // Chopping CPU Window Size(1 SHL N clks)
        uint32_t : 1;
        Enabled enabled : 1;  // stopped/completed, start/enable/busy
        uint32_t : 3;
        StartTrigger startTrigger : 1;
        uint32_t : 3;
    };
    uint32_t _reg;
    uint8_t _byte[4];

    CHCR() : _reg(0) {}

    static CHCR OTC() {
        CHCR control;
        control.direction = Direction::toRam;
        control.memoryAddressStep = MemoryAddressStep::backward;
        control.choppingEnable = 0;
        control.syncMode = SyncMode::startImmediately;
        control.choppingDmaWindowSize = 0;
        control.choppingCpuWindowSize = 0;
        control.enabled = Enabled::start;
        control.startTrigger = StartTrigger::manual;
        return control;
    }

    static CHCR MDECin() {
        CHCR control;
        control.direction = Direction::fromRam;
        control.memoryAddressStep = MemoryAddressStep::forward;
        control.choppingEnable = 0;
        control.syncMode = SyncMode::syncBlockToDmaRequests;
        control.choppingDmaWindowSize = 0;
        control.choppingCpuWindowSize = 0;
        control.enabled = Enabled::start;
        control.startTrigger = StartTrigger::automatic;
        return control;
    }

    static CHCR MDECout() {
        CHCR control;
        control.direction = Direction::toRam;
        control.memoryAddressStep = MemoryAddressStep::forward;
        control.choppingEnable = 0;
        control.syncMode = SyncMode::syncBlockToDmaRequests;
        control.choppingDmaWindowSize = 0;
        control.choppingCpuWindowSize = 0;
        control.enabled = Enabled::start;
        control.startTrigger = StartTrigger::automatic;
        return control;
    }

    static CHCR SPUwrite() {
        CHCR control;
        control.direction = Direction::fromRam;
        control.memoryAddressStep = MemoryAddressStep::forward;
        control.choppingEnable = 0;
        control.syncMode = SyncMode::syncBlockToDmaRequests;
        control.choppingDmaWindowSize = 0;
        control.choppingCpuWindowSize = 0;
        control.enabled = Enabled::start;
        control.startTrigger = StartTrigger::automatic;
        return control;
    }

    static CHCR SPUread() {
        CHCR control;
        control.direction = Direction::toRam;
        control.memoryAddressStep = MemoryAddressStep::forward;
        control.choppingEnable = 0;
        control.syncMode = SyncMode::syncBlockToDmaRequests;
        control.choppingDmaWindowSize = 0;
        control.choppingCpuWindowSize = 0;
        control.enabled = Enabled::start;
        control.startTrigger = StartTrigger::automatic;
        return control;
    }

    static CHCR VRAMwrite() {
        CHCR control;
        control.direction = Direction::fromRam;
        control.memoryAddressStep = MemoryAddressStep::forward;
        control.choppingEnable = 0;
        control.syncMode = SyncMode::syncBlockToDmaRequests;
        control.choppingDmaWindowSize = 0;
        control.choppingCpuWindowSize = 0;
        control.enabled = Enabled::start;
        control.startTrigger = StartTrigger::automatic;
        return control;
    }
};

void waitForChannel(Channel ch);
void masterEnable(Channel ch, bool enabled);
};