#include "esp32.h"

using namespace BinaryNinja;

namespace EspApp
{
    #define RO  (SegmentReadable)
    #define RW  (SegmentReadable | SegmentWritable)
    #define RX  (SegmentReadable | SegmentExecutable)
    #define RWX (SegmentReadable | SegmentWritable | SegmentExecutable)

    const MemoryRegion g_esp32Regions[] = {
        {"external.data.1",        0x3F400000, 0x3F800000, RW | SegmentContainsData},
        {"external.data.2",        0x3F800000, 0x3FC00000, RW | SegmentContainsData},
        {"peripheral",             0x3FF00000, 0x3FF80000, RW},
        {"embedded.data.rtc_fast", 0x3FF80000, 0x3FF82000, RW},
        {"embedded.data.rom.1",    0x3FF90000, 0x3FFA0000, RO | SegmentContainsData},
        {"embedded.data.ram.2",    0x3FFAE000, 0x3FFE0000, RW},
        {"embedded.data.ram.1",    0x3FFE0000, 0x40000000, RW},
        {"embedded.code.rom.0.1",  0x40000000, 0x40008000, RX | SegmentContainsCode},
        {"embedded.code.rom.0.2",  0x40008000, 0x40060000, RX | SegmentContainsCode},
        {"embedded.code.ram.0.1",  0x40070000, 0x40080000, RX | SegmentContainsCode},
        {"embedded.code.ram.0.2",  0x40080000, 0x400A0000, RX | SegmentContainsCode},
        {"embedded.code.ram.1.1",  0x400A0000, 0x400B0000, RX | SegmentContainsCode},
        {"embedded.code.ram.1.2",  0x400B0000, 0x400B8000, RX | SegmentContainsCode},
        {"embedded.code.ram.1.3",  0x400B8000, 0x400C0000, RX | SegmentContainsCode},
        {"embedded.code.rtc_fast", 0x400C0000, 0x400C2000, RX | SegmentContainsCode},
        {"external.code.0",        0x400C2000, 0x40C00000, RX | SegmentContainsCode},
        {"embedded.rtc_slow",      0x50000000, 0x50002000, RWX},
    };

    const size_t g_esp32RegionCount = sizeof(g_esp32Regions) / sizeof(g_esp32Regions[0]);

    static const struct {
        const char* name;
        uint64_t start;
        uint64_t end;
        BNSectionSemantics semantics;
    } g_esp32RomSections[] = {
        {"rom.data",   0x3FF90000, 0x3FFA0000, ExternalSectionSemantics},
        {"rom.code.1", 0x40000000, 0x40008000, ExternalSectionSemantics},
        {"rom.code.2", 0x40008000, 0x40060000, ExternalSectionSemantics},
    };

    void Esp32PostInit(EspAppView* view)
    {
        for (const auto& rom : g_esp32RomSections)
        {
            view->AddAutoSection(rom.name, rom.start, rom.end - rom.start, rom.semantics);
        }
    }

    void Esp32OnPluginInit()
    {
        return;
    }

    const ChipAttr g_esp32Attr = {
        EspChipId::ESP32,
        "ESP32",
        "xtensa-esp32",
        g_esp32Regions,
        g_esp32RegionCount,
        Esp32PostInit,
        Esp32OnPluginInit
    };

    #undef RO
    #undef RW
    #undef RX
    #undef RWX

} // namespace EspApp
