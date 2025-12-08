#include "esp_app_view.h"
#include "esp32.h"

using namespace std;
using namespace BinaryNinja;

namespace EspApp
{
    static const ChipAttr* g_chipAttrList[] = {
        &g_esp32Attr,
        // TODO: Add ESP32-S2, ESP32-S3 attributes
    };

    static const size_t g_chipAttrCount = sizeof(g_chipAttrList) / sizeof(g_chipAttrList[0]);

    void InitializeChips()
    {
        for (size_t i = 0; i < g_chipAttrCount; i++)
        {
            if (g_chipAttrList[i]->on_plugin_init)
            {
                g_chipAttrList[i]->on_plugin_init();
            }
        }
    }

    const ChipAttr* GetChipAttrById(EspChipId chipId)
    {
        for (size_t i = 0; i < g_chipAttrCount; i++)
        {
            if (g_chipAttrList[i]->chip_id == chipId)
                return g_chipAttrList[i];
        }
        return nullptr;
    }

    const MemoryRegion* FindRegionForAddress(const ChipAttr* attr, uint64_t addr)
    {
        if (!attr || !attr->regions)
            return nullptr;

        for (size_t i = 0; i < attr->region_count; i++)
        {
            const auto& region = attr->regions[i];
            if (addr >= region.start_addr && addr < region.end_addr)
                return &region;
        }
        return nullptr;
    }

    EspAppView::EspAppView(BinaryView* data, bool parseOnly)
        : BinaryView("ESP-APP", data->GetFile(), data),
          m_parseOnly(parseOnly),
          m_entryPoint(0),
          m_header{},
          m_chipAttr(nullptr)
    {
        CreateLogger("BinaryView");
        m_logger = CreateLogger("BinaryView.EspAppView");

        BinaryReader reader(data);

        try
        {
            reader.Seek(0);
            m_header.magic = reader.Read8();
            m_header.segment_count = reader.Read8();
            m_header.spi_mode = reader.Read8();
            m_header.spi_speed_size = reader.Read8();
            m_header.entry_addr = reader.Read32();
            m_header.wp_pin = reader.Read8();
            reader.Read(m_header.spi_pin_drv, 3);
            m_header.chip_id = reader.Read16();
            m_header.min_chip_rev = reader.Read8();
            m_header.min_chip_rev_full = reader.Read16();
            m_header.max_chip_rev_full = reader.Read16();
            reader.Read(m_header.reserved, 4);
            m_header.hash_appended = reader.Read8();

            m_entryPoint = m_header.entry_addr;
            m_chipAttr = GetChipAttrById(static_cast<EspChipId>(m_header.chip_id));

            m_logger->LogDebug("ESP App Image: magic=0x%02x, segments=%d, entry=0x%08x, chip_id=0x%04x (%s)",
                m_header.magic, m_header.segment_count, m_header.entry_addr,
                m_header.chip_id, m_chipAttr->chip_name);

            uint64_t offset = sizeof(EspImageHeader);
            for (uint8_t i = 0; i < m_header.segment_count && i < ESP_IMAGE_MAX_SEGMENTS; i++)
            {
                reader.Seek(offset);
                SegmentInfo seg;
                seg.load_addr = reader.Read32();
                seg.data_len = reader.Read32();
                seg.file_offset = offset + sizeof(EspSegmentHeader);
                m_segments.push_back(seg);
                offset = seg.file_offset + seg.data_len;
            }
        }
        catch (ReadException& e)
        {
            m_logger->LogError("Failed to parse ESP app image: %s", e.what());
        }
    }

    uint64_t EspAppView::PerformGetEntryPoint() const
    {
        return m_entryPoint;
    }

    bool EspAppView::PerformIsExecutable() const
    {
        return true;
    }

    BNEndianness EspAppView::PerformGetDefaultEndianness() const
    {
        return LittleEndian;
    }

    size_t EspAppView::PerformGetAddressSize() const
    {
        return 4;
    }

    bool EspAppView::Init()
    {
        if (m_segments.empty())
        {
            m_logger->LogError("No segments found in ESP app image");
            return false;
        }

        if (!m_chipAttr || !m_chipAttr->regions)
        {
            m_logger->LogError("No attribute available for chip");
            return false;
        }

        for (size_t i = 0; i < m_segments.size(); i++)
        {
            const auto& seg = m_segments[i];
            uint64_t seg_start = seg.load_addr;
            uint64_t seg_end = seg.load_addr + seg.data_len;

            const MemoryRegion* region = FindRegionForAddress(m_chipAttr, seg_start);

            if (!region)
            {
                m_logger->LogWarn("Segment %zu at 0x%08x is not in any known memory region, skipping",
                    i, seg.load_addr);
                continue;
            }

            if (seg_end > region->end_addr)
            {
                m_logger->LogWarn("Segment %zu at 0x%08x-0x%08llx exceeds region %s boundary (0x%08llx), skipping",
                    i, seg.load_addr, seg_end, region->name, region->end_addr);
                continue;
            }

            m_logger->LogDebug("Segment %zu: name=%s addr=0x%08x, len=0x%x, offset=0x%llx",
                i, region->name, seg.load_addr, seg.data_len, seg.file_offset);

            AddAutoSegment(seg.load_addr, seg.data_len, seg.file_offset, seg.data_len, region->flags);

            BNSectionSemantics semantics = (region->flags & SegmentContainsCode)
                ? ReadOnlyCodeSectionSemantics
                : DefaultSectionSemantics;

            string sectionName = "app." + to_string(i);
            AddAutoSection(sectionName, seg.load_addr, seg.data_len, semantics);
        }

        std::string archName;
        EspChipId chipId = static_cast<EspChipId>(m_header.chip_id);

        Ref<Settings> settings = GetLoadSettings(GetTypeName());
        if (settings && settings->Contains("loader.esp.architecture"))
        {
            archName = settings->Get<std::string>("loader.esp.architecture", this);
        }

        if (archName.empty() || archName == "auto")
        {
            const char* detected = m_chipAttr->arch_name;
            if (detected)
            {
                archName = detected;
                m_logger->LogInfo("Auto-detected architecture: %s for chip %s",
                    archName.c_str(), m_chipAttr->chip_name);
            }
            else
            {
                m_logger->LogWarn("Unknown chip ID 0x%04x, cannot determine architecture.",
                    m_header.chip_id);
                return false;
            }
        }

        Ref<Architecture> arch = Architecture::GetByName(archName);
        if (!arch)
        {
            m_logger->LogError("Architecture '%s' not found.", archName.c_str());
        }
        else
        {
            SetDefaultArchitecture(arch);
            m_logger->LogInfo("Using architecture: %s", archName.c_str());

            Ref<Platform> plat = Platform::GetByName(archName);
            if (!plat)
            {
                plat = arch->GetStandalonePlatform();
            }
            if (plat)
            {
                SetDefaultPlatform(plat);
            }
        }

        if (m_parseOnly)
        {
            return true;
        }

        if (arch)
        {
            Ref<Platform> plat = GetDefaultPlatform();
            if (plat)
            {
                AddEntryPointForAnalysis(plat, m_entryPoint);
            }
            DefineAutoSymbol(new Symbol(FunctionSymbol, "_entry", m_entryPoint, GlobalBinding));
        }

        if (m_chipAttr && m_chipAttr->post_init)
        {
            m_chipAttr->post_init(this);
        }

        return true;
    }

} // namespace EspApp
