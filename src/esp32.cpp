#include "esp32.h"
#include "binaryninjacore.h"

using namespace BinaryNinja;

namespace EspApp
{
#define RO  (SegmentReadable)
#define RW  (SegmentReadable | SegmentWritable)
#define RX  (SegmentReadable | SegmentExecutable)
#define RWX (SegmentReadable | SegmentWritable | SegmentExecutable)

    const MemoryRegion g_esp32Regions[] = {
        {"external.data.1",        0x3F400000, 0x3F800000, RW | SegmentContainsData, DefaultSectionSemantics},
        {"external.data.2",        0x3F800000, 0x3FC00000, RW | SegmentContainsData, DefaultSectionSemantics},
        {"peripheral",             0x3FF00000, 0x3FF80000, RW,                       DefaultSectionSemantics},
        {"embedded.data.rtc_fast", 0x3FF80000, 0x3FF82000, RW,                       DefaultSectionSemantics},
        {"embedded.data.rom.1",    0x3FF90000, 0x3FFA0000, RO | SegmentContainsData, ExternalSectionSemantics},
        {"embedded.data.ram.2",    0x3FFAE000, 0x3FFE0000, RW,                       DefaultSectionSemantics},
        {"embedded.data.ram.1",    0x3FFE0000, 0x40000000, RW,                       DefaultSectionSemantics},
        {"embedded.code.rom.0.1",  0x40000000, 0x40008000, RX | SegmentContainsCode, ExternalSectionSemantics},
        {"embedded.code.rom.0.2",  0x40008000, 0x40060000, RX | SegmentContainsCode, ExternalSectionSemantics},
        {"embedded.code.ram.0.1",  0x40070000, 0x40080000, RX | SegmentContainsCode, DefaultSectionSemantics},
        {"embedded.code.ram.0.2",  0x40080000, 0x400A0000, RX | SegmentContainsCode, DefaultSectionSemantics},
        {"embedded.code.ram.1.1",  0x400A0000, 0x400B0000, RX | SegmentContainsCode, DefaultSectionSemantics},
        {"embedded.code.ram.1.2",  0x400B0000, 0x400B8000, RX | SegmentContainsCode, DefaultSectionSemantics},
        {"embedded.code.ram.1.3",  0x400B8000, 0x400C0000, RX | SegmentContainsCode, DefaultSectionSemantics},
        {"embedded.code.rtc_fast", 0x400C0000, 0x400C2000, RX | SegmentContainsCode, DefaultSectionSemantics},
        {"external.code.0",        0x400C2000, 0x40C00000, RX | SegmentContainsCode, DefaultSectionSemantics},
        {"embedded.rtc_slow",      0x50000000, 0x50002000, RWX                     , DefaultSectionSemantics},
    };

    const size_t g_esp32RegionCount = sizeof(g_esp32Regions) / sizeof(g_esp32Regions[0]);

    void Esp32PostInit(EspAppView* view)
    {
		return;
    }

    void Esp32OnPluginInit()
    {
        return;
    }

    const ChipAttr g_esp32Attr = {
        EspChipId::ESP32,
		"ESP32",
		"esp32",
		g_esp32Regions,
		g_esp32RegionCount,
		Esp32PostInit,
		Esp32OnPluginInit
	};

#undef RO
#undef RW
#undef RX
#undef RWX

}  // namespace EspApp
