#include "esp_app_view_type.h"
#include "esp_app_view.h"
#include "binaryninjaapi.h"

using namespace std;
using namespace BinaryNinja;

extern "C"
{
    BN_DECLARE_CORE_ABI_VERSION

    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        BinaryNinja::LogInfo("ESP-APP View Plugin loaded");
        EspApp::InitializeChips();
        EspApp::InitEspAppViewType();
        return true;
    }
}
