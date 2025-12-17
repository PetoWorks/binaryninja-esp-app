#pragma once

#include "binaryninjaapi.h"
#include <vector>
#include <cstdint>

namespace EspApp
{
    constexpr uint8_t ESP_IMAGE_HEADER_MAGIC = 0xE9;
    constexpr uint8_t ESP_IMAGE_MAX_SEGMENTS = 16;

    enum class EspChipId : uint16_t
    {
        ESP32 = 0x0000,
        ESP32_S2 = 0x0002,
        ESP32_C3 = 0x0005,
        ESP32_S3 = 0x0009,
        ESP32_C2 = 0x000C,
        ESP32_C6 = 0x000D,
        ESP32_H2 = 0x0010,
        ESP32_P4 = 0x0012,
        Invalid = 0xFFFF
    };

    struct MemoryRegion
    {
        const char* name;
        uint64_t start_addr;
        uint64_t end_addr;
        uint32_t flags;
        uint32_t section_semantics;
    };

    class EspAppView;

    using PostInitCallback = void (*)(EspAppView* view);
    using PluginInitCallback = void (*)();

    struct ChipAttr
    {
        EspChipId chip_id;
        const char* chip_name;
        const char* arch_name;
        const MemoryRegion* regions;
        size_t region_count;
        PostInitCallback post_init;
        PluginInitCallback on_plugin_init;
    };

// ESP32 Image Header (24 bytes)
#pragma pack(push, 1)
    struct EspImageHeader
    {
        uint8_t magic;               // 0xE9
        uint8_t segment_count;       // Number of segments (max 16)
        uint8_t spi_mode;            // Flash read mode
        uint8_t spi_speed_size;      // spi_speed: 4, spi_size: 4
        uint32_t entry_addr;         // Entry point address
        uint8_t wp_pin;              // Write protect pin
        uint8_t spi_pin_drv[3];      // SPI pin drive settings
        uint16_t chip_id;            // Chip ID
        uint8_t min_chip_rev;        // Minimum chip revision
        uint16_t min_chip_rev_full;  // Minimum chip revision (full)
        uint16_t max_chip_rev_full;  // Maximum chip revision (full)
        uint8_t reserved[4];         // Reserved
        uint8_t hash_appended;       // SHA256 hash appended flag
    };
#pragma pack(pop)

    static_assert(sizeof(EspImageHeader) == 24, "EspImageHeader must be 24 bytes");

// ESP32 Segment Header (8 bytes)
#pragma pack(push, 1)
    struct EspSegmentHeader
    {
        uint32_t load_addr;  // Memory address to load segment
        uint32_t data_len;   // Length of segment data
    };
#pragma pack(pop)

    static_assert(sizeof(EspSegmentHeader) == 8, "EspSegmentHeader must be 8 bytes");

    // Internal segment info with file offset
    struct SegmentInfo
    {
        uint32_t load_addr;
        uint32_t data_len;
        uint64_t file_offset;  // Offset in raw file where data starts
    };

    void InitializeChips();
    const ChipAttr* GetChipAttrById(EspChipId chipId);
    const MemoryRegion* FindRegionForAddress(const ChipAttr* attr, uint64_t addr);

    class EspAppView : public BinaryNinja::BinaryView
    {
        bool m_parseOnly;
        uint64_t m_entryPoint;
        EspImageHeader m_header;
        std::vector<SegmentInfo> m_segments;
        const ChipAttr* m_chipAttr;
        BinaryNinja::Ref<BinaryNinja::Logger> m_logger;

        virtual uint64_t PerformGetEntryPoint() const override;
        virtual bool PerformIsExecutable() const override;
        virtual BNEndianness PerformGetDefaultEndianness() const override;
        virtual size_t PerformGetAddressSize() const override;

    public:
        EspAppView(BinaryNinja::BinaryView* data, bool parseOnly = false);
        virtual ~EspAppView() = default;

        virtual bool Init() override;
    };

}  // namespace EspApp
