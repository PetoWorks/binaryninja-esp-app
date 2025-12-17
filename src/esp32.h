#pragma once

#include "esp_app_view.h"

namespace EspApp
{
    extern const MemoryRegion g_esp32Regions[];
    extern const size_t g_esp32RegionCount;
    extern const ChipAttr g_esp32Attr;
    void Esp32PostInit(EspAppView* view);
    void Esp32OnPluginInit();
}  // namespace EspApp
