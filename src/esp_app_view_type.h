#pragma once

#include "binaryninjaapi.h"

namespace EspApp
{
    class EspAppViewType : public BinaryNinja::BinaryViewType
    {
        BinaryNinja::Ref<BinaryNinja::Logger> m_logger;

    public:
        EspAppViewType();
        virtual BinaryNinja::Ref<BinaryNinja::BinaryView> Create(BinaryNinja::BinaryView* data) override;
        virtual BinaryNinja::Ref<BinaryNinja::BinaryView> Parse(BinaryNinja::BinaryView* data) override;
        virtual bool IsTypeValidForData(BinaryNinja::BinaryView* data) override;
        virtual BinaryNinja::Ref<BinaryNinja::Settings> GetLoadSettingsForData(BinaryNinja::BinaryView* data) override;
    };

    void InitEspAppViewType();

}  // namespace EspApp
