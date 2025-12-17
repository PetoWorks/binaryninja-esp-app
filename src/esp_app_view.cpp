#include "esp_app_view.h"
#include "esp32.h"

#include <algorithm>

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

    EspAppView::EspAppView(BinaryView* data, bool parseOnly) :
        BinaryView("ESP-APP", data->GetFile(), data), m_parseOnly(parseOnly), m_entryPoint(0), m_header {},
        m_chipAttr(nullptr)
    {
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
                m_header.magic, m_header.segment_count, m_header.entry_addr, m_header.chip_id, m_chipAttr->chip_name);

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

        // Step 1: Validate all app segments and map them to regions
        // Each app segment must be fully contained within exactly one region
        struct AppSegmentMapping
        {
            size_t segmentIndex;
            const SegmentInfo* segment;
            const MemoryRegion* region;
        };
        vector<AppSegmentMapping> appSegmentMappings;

        for (size_t i = 0; i < m_segments.size(); i++)
        {
            const auto& seg = m_segments[i];
            uint64_t seg_start = seg.load_addr;
            uint64_t seg_end = seg.load_addr + seg.data_len;

            const MemoryRegion* region = FindRegionForAddress(m_chipAttr, seg_start);

            if (!region)
            {
                m_logger->LogError("Segment %zu at 0x%08x is not in any known memory region", i, seg.load_addr);
                return false;
            }

            // Check if segment end is within the same region
            if (seg_end > region->end_addr)
            {
                // Check if segment spans multiple regions
                const MemoryRegion* endRegion = FindRegionForAddress(m_chipAttr, seg_end - 1);
                if (endRegion && endRegion != region)
                {
                    m_logger->LogError("Segment %zu at 0x%08x-0x%08llx spans multiple regions (%s and %s)", i,
                        seg.load_addr, seg_end, region->name, endRegion->name);
                }
                else
                {
                    m_logger->LogError("Segment %zu at 0x%08x-0x%08llx exceeds region %s boundary (0x%08llx-0x%08llx)",
                        i, seg.load_addr, seg_end, region->name, region->start_addr, region->end_addr);
                }
                return false;
            }

            m_logger->LogDebug("Segment %zu: region=%s addr=0x%08x, len=0x%x, offset=0x%llx", i, region->name,
                seg.load_addr, seg.data_len, seg.file_offset);

            appSegmentMappings.push_back({i, &seg, region});
        }

        // Step 2: Process each region - add file-backed segments and fragment remainders
        for (size_t regionIdx = 0; regionIdx < m_chipAttr->region_count; regionIdx++)
        {
            const auto& region = m_chipAttr->regions[regionIdx];

            // Collect all app segments that belong to this region, sorted by address
            vector<const AppSegmentMapping*> regionAppSegments;
            for (const auto& mapping : appSegmentMappings)
            {
                if (mapping.region == &region)
                {
                    regionAppSegments.push_back(&mapping);
                }
            }

            // Sort by load address
            sort(regionAppSegments.begin(), regionAppSegments.end(),
                [](const AppSegmentMapping* a, const AppSegmentMapping* b) {
                    return a->segment->load_addr < b->segment->load_addr;
                });

            BNSectionSemantics semantics =
                (region.flags & SegmentContainsCode) ? ReadOnlyCodeSectionSemantics : DefaultSectionSemantics;

            if (regionAppSegments.empty())
            {
                // No app segments in this region - add entire region as non-file-backed
                uint64_t regionSize = region.end_addr - region.start_addr;
                AddAutoSegment(region.start_addr, regionSize, 0, 0, region.flags);
                AddAutoSection(region.name, region.start_addr, regionSize, semantics);
                m_logger->LogDebug("Region %s: added as non-file-backed (0x%08llx-0x%08llx)", region.name,
                    region.start_addr, region.end_addr);
            }
            else
            {
                // Region has app segments - fragment the region
                uint64_t currentAddr = region.start_addr;
                size_t fragmentIndex = 0;

                for (const auto* mapping : regionAppSegments)
                {
                    const auto& seg = *mapping->segment;
                    uint64_t seg_start = seg.load_addr;
                    uint64_t seg_end = seg.load_addr + seg.data_len;

                    // Add non-file-backed fragment before this app segment (if any gap)
                    if (currentAddr < seg_start)
                    {
                        uint64_t gapSize = seg_start - currentAddr;
                        AddAutoSegment(currentAddr, gapSize, 0, 0, region.flags);

                        string sectionName = string(region.name) + ".frag." + to_string(fragmentIndex++);
                        AddAutoSection(sectionName, currentAddr, gapSize, semantics);
                        m_logger->LogDebug("Region %s: added fragment at 0x%08llx-0x%08llx (non-file-backed)",
                            region.name, currentAddr, seg_start);
                    }

                    // Add the file-backed app segment
                    AddAutoSegment(seg.load_addr, seg.data_len, seg.file_offset, seg.data_len, region.flags);

                    string sectionName = string(region.name) + ".app." + to_string(mapping->segmentIndex);
                    AddAutoSection(sectionName, seg.load_addr, seg.data_len, semantics);
                    m_logger->LogDebug("Region %s: added app segment %zu at 0x%08x-0x%08llx (file-backed)", region.name,
                        mapping->segmentIndex, seg.load_addr, seg_end);

                    currentAddr = seg_end;
                }

                // Add non-file-backed fragment after the last app segment (if any remainder)
                if (currentAddr < region.end_addr)
                {
                    uint64_t remainderSize = region.end_addr - currentAddr;
                    AddAutoSegment(currentAddr, remainderSize, 0, 0, region.flags);

                    string sectionName = string(region.name) + ".frag." + to_string(fragmentIndex);
                    AddAutoSection(sectionName, currentAddr, remainderSize, semantics);
                    m_logger->LogDebug("Region %s: added fragment at 0x%08llx-0x%08llx (non-file-backed)", region.name,
                        currentAddr, region.end_addr);
                }
            }
        }

        // Step 3: Set up architecture and platform
        Ref<Architecture> arch = Architecture::GetByName(m_chipAttr->arch_name);
        if (!arch)
        {
            m_logger->LogError("Architecture '%s' not found.", m_chipAttr->arch_name);
        }
        else
        {
            SetDefaultArchitecture(arch);
            m_logger->LogInfo("Using architecture: %s", m_chipAttr->arch_name);

            Ref<Platform> plat = Platform::GetByName(m_chipAttr->arch_name);
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

}  // namespace EspApp
