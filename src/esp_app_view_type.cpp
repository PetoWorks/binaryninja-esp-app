#include "esp_app_view_type.h"
#include "esp_app_view.h"

using namespace std;
using namespace BinaryNinja;

namespace EspApp
{
    static EspAppViewType* g_espAppViewType = nullptr;

    EspAppViewType::EspAppViewType() : BinaryViewType("ESP-APP", "ESP App Image")
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

        return settings;
    }

    void InitEspAppViewType()
    {
        static EspAppViewType type;
        BinaryViewType::Register(&type);
        g_espAppViewType = &type;
    }

}  // namespace EspApp
