#include "esp_app_view.h"

using namespace std;
using namespace BinaryNinja;

namespace EspApp
{
    static EspAppViewType* g_espAppViewType = nullptr;

    EspAppViewType::EspAppViewType()
        : BinaryViewType("ESP-APP", "ESP32 Application Image")
    {
        m_logger = LogRegistry::CreateLogger("BinaryViewType.EspApp");
    }

    Ref<BinaryView> EspAppViewType::Create(BinaryView* data)
    {
        try
        {
            return new EspAppView(data, false);
        }
        catch (exception& e)
        {
            m_logger->LogError("Failed to create ESP App view: %s", e.what());
            return nullptr;
        }
    }

    Ref<BinaryView> EspAppViewType::Parse(BinaryView* data)
    {
        try
        {
            return new EspAppView(data, true);
        }
        catch (exception& e)
        {
            m_logger->LogError("Failed to parse ESP App view: %s", e.what());
            return nullptr;
        }
    }

    bool EspAppViewType::IsTypeValidForData(BinaryView* data)
    {
        if (data->GetLength() < sizeof(EspImageHeader))
            return false;

        DataBuffer magic = data->ReadBuffer(0, 1);
        if (magic.GetLength() < 1)
            return false;

        uint8_t magicByte = ((uint8_t*)magic.GetData())[0];
        if (magicByte != ESP_IMAGE_HEADER_MAGIC)
            return false;

        DataBuffer segCountBuf = data->ReadBuffer(1, 1);
        if (segCountBuf.GetLength() < 1)
            return false;

        uint8_t segCount = ((uint8_t*)segCountBuf.GetData())[0];
        if (segCount == 0 || segCount > ESP_IMAGE_MAX_SEGMENTS)
            return false;

        return true;
    }

    Ref<Settings> EspAppViewType::GetLoadSettingsForData(BinaryView* data)
    {
        Ref<Settings> settings = GetDefaultLoadSettingsForData(data);

        settings->RegisterSetting("loader.esp.architecture",
            "{"
            "\"title\": \"Architecture\", "
            "\"type\": \"string\", "
            "\"enum\": [\"auto\", \"xtensa-esp32\", \"xtensa-esp8266\", \"xtensa\", \"rv32gc\"], "
            "\"enumDescriptions\": ["
            "\"Auto-detect from chip_id (recommended)\", "
            "\"ESP32/S2/S3 - Xtensa LX6/LX7 with windowed calls\", "
            "\"ESP8266 - Xtensa LX106 (CALL0 only)\", "
            "\"Generic Xtensa (no optional features)\", "
            "\"Generic RISC-V\""
            "], "
            "\"default\": \"auto\", "
            "\"description\": \"Select Xtensa architecture variant for disassembly\""
            "}");

        return settings;
    }

    void InitEspAppViewType()
    {
        static EspAppViewType type;
        BinaryViewType::Register(&type);
        g_espAppViewType = &type;
    }

} // namespace EspApp
