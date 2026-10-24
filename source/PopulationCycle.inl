namespace population
{
    SafetyHookMid cabHook;
    SafetyHookMid dealerHook;
    int pendingCab{ -1 };
    int cabLevel{ -1 };
    uint32_t cabChangedAt{ 0 };

    uint8_t* StreamingInfo(int model)
    {
        return *(uint8_t**)0x9654B4 + model * 0x14;
    }

    void CycleCab(SafetyHookContext&)
    {
        auto& current = *(int*)0x965524;
        auto level = *(int*)0xBA6718;
        auto now = *(uint32_t*)0xB7CB84;
        if (current < 0 || current >= 6 || level < 0 || level > 3)
            return;

        if (cabLevel == -1)
        {
            cabLevel = level;
            cabChangedAt = now;
        }

        // Keep the selected driver stable while its car and ped are being created. Request the next variant
        // asynchronously; existing peds keep their model and the native selector sees only a loaded replacement.
        if (level != cabLevel)
        {
            pendingCab = level ? (level - 1) * 2 + (current & 1) : -1;
            cabLevel = level;
            cabChangedAt = now;
        }
        else if (pendingCab == -1 && uint32_t(now - cabChangedAt) >= 30000)
        {
            pendingCab = level ? (level - 1) * 2 + ((current & 1) ^ 1) : current ^ 1;
            cabChangedAt = now;
        }

        if (pendingCab < 0)
            return;
        auto model = ((int*)0x8A5AF4)[pendingCab];
        if (model < 0 || model >= 20000)
            return;
        if (StreamingInfo(model)[0x10] == 1)
        {
            current = pendingCab;
            pendingCab = -1;
            cabChangedAt = now;
        }
        else
        {
            ((void (__cdecl*)(int, int))0x4087E0)(model, 0);
        }
    }

    void ChooseDealer(SafetyHookContext& regs)
    {
        auto group = *(int*)0x8D2690;
        if (group < 0 || group >= 57)
            return;
        auto count = ((int16_t*)0xC0ECC0)[group];
        if (count < 0 || count > 21)
            return;

        // Streaming marks the variant for the current zone as game-required. Other loaded variants may remain
        // resident indefinitely with a larger budget, so they must not override that choice.
        for (int i = 0; i < count; i++)
        {
            auto model = ((uint16_t*)0xC0F358)[group * 21 + i];
            if (model >= 20000)
                continue;
            auto info = StreamingInfo(model);
            if (info[0x10] == 1 && (info[6] & 2))
                *(int*)regs.esi = model;
        }
    }

    void Install()
    {
        if (!core::GetSetting("MemoryAvailable", "auto").apply)
            return;

        const mem::Expect cabValues[] =
        {
            { 0x407D53, 0x965524 }, { 0x407D5A, 0x8A5AF4 }, { 0x407D65, 0x8E4CD0 },
            { 0x407D71, 0xBA6718 },
        };
        if (mem::CheckValues(cabValues, "Cab driver cycling") && *(uint16_t*)0x407D51 == 0x0D8B)
        {
            cabHook = safetyhook::create_mid(0x407D51, CycleCab);
            core::Log("Cab driver cycling: requests a loaded replacement every 30 game seconds");
        }

        const mem::Expect dealerValues[] =
        {
            { 0x60FEB4, 0x8D2690 }, { 0x60FEBE, 0xC0ECC0 }, { 0x60FECF, 0xC0F358 },
            { 0x60FED9, 0x8E4CD0 }, { 0x60FEE3, 0x8D2690 },
        };
        if (mem::CheckValues(dealerValues, "Dealer model cycling") && mem::IsCompare(0x60FEF4, 0))
        {
            dealerHook = safetyhook::create_mid(0x60FEF4, ChooseDealer);
            core::Log("Dealer model cycling: follows the current zone's requested variant");
        }
    }
}
