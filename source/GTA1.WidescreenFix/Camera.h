#pragma once

// GTA3-style overhead camera for GTA1. The game computes, per player, a camera
// position (with a dead zone and speed look-ahead), a height and a focal length.
// This keeps the view locked to the followed object, smooths the game's offset
// and height in real time, and can raise the camera for a wider view.
namespace GTA1Camera
{
    inline std::array<SafetyHookMid, 2> UpdateHooks;
    inline int* (__cdecl* Control)(int) = nullptr;   // per-player camera control record
    inline bool Enabled = true;
    inline double FollowSeconds = 0.25, HeightSeconds = 0.35;
    inline double Zoom = 1.0;                       // multiplier for the visible area
    inline double CounterFrequency = 1;
    struct State { double offset[2]{}, velocity[2]{}, height = 0, heightVelocity = 0; LONGLONG time = 0; };
    inline std::array<State, 4> Players{};
    // Where a hook site has its values: the player in a register, x and y in
    // stack slots, the height in a register or (London) a stack slot.
    enum Register { Ebx, Esi, Edi };
    struct Site { Register player = Ebx; int x = 0x20, y = 0x1C; Register heightRegister = Esi; int heightSlot = -1; };
    inline std::array<Site, 2> Sites{};
    inline uint32_t& Value(SafetyHookContext& context, Register r)
    {
        return r == Ebx ? context.ebx : r == Esi ? context.esi : context.edi;
    }

    inline void Spring(double& value, double& velocity, double goal, double time, double elapsed)
    {
        if (time <= 0.0) { value = goal; velocity = 0; return; }
        double omega = 2.0 / time, offset = value - goal;
        double change = (velocity + omega * offset) * elapsed, decay = std::exp(-omega * elapsed);
        velocity = (velocity - omega * change) * decay;
        value = goal + (offset + change) * decay;
    }

    inline void Update(SafetyHookContext& context, const Site& site)
    {
        const int player = int(Value(context, site.player));
        if (!Enabled) { if (player >= 0 && player < int(Players.size())) Players[player].time = 0; return; }
        if (player < 0 || player >= int(Players.size()) || !Control) return;
        auto control = Control(player);
        if (!control) return;
        auto& state = Players[player];
        auto y = reinterpret_cast<int*>(context.esp + site.y);
        auto x = reinterpret_cast<int*>(context.esp + site.x);
        // Height and focal length: the visible half-width is
        // 160 * (height + 384) / focal world units.
        auto heightSlot = site.heightSlot >= 0 ? reinterpret_cast<int*>(context.esp + site.heightSlot) : nullptr;
        int height = heightSlot ? *heightSlot : int(Value(context, site.heightRegister));
        // Full zoom at walking height, none at the game's top speed height:
        // the game already raises the camera with vehicle speed.
        double distance = height + 384.0, taper = std::clamp((1100.0 - distance) / (1100.0 - 462.0), 0.0, 1.0);
        double goalHeight = distance * (1.0 + (Zoom - 1.0) * taper) - 384.0;
        double goal[2] = { double(*x - control[4]), double(*y - control[5]) };
        LARGE_INTEGER counter{}; QueryPerformanceCounter(&counter);
        double elapsed = state.time ? (counter.QuadPart - state.time) / CounterFrequency : 0.0;
        state.time = counter.QuadPart;
        // Snap on level changes, teleports and long pauses.
        if (elapsed <= 0.0 || elapsed > 0.5 || std::hypot(goal[0] - state.offset[0], goal[1] - state.offset[1]) > 1024.0)
        {
            state.offset[0] = goal[0]; state.offset[1] = goal[1]; state.height = goalHeight;
            state.velocity[0] = state.velocity[1] = state.heightVelocity = 0;
        }
        else
        {
            for (int i = 0; i < 2; ++i) Spring(state.offset[i], state.velocity[i], goal[i], FollowSeconds, elapsed);
            Spring(state.height, state.heightVelocity, goalHeight, goalHeight > state.height ? HeightSeconds * 0.5 : HeightSeconds, elapsed);
        }
        *x = control[4] + int(std::lround(state.offset[0]));
        *y = control[5] + int(std::lround(state.offset[1]));
        int result = std::max(16, int(std::lround(state.height)));
        if (heightSlot) *heightSlot = result;
        else Value(context, site.heightRegister) = uint32_t(result);
    }
    inline void UpdateFirst(SafetyHookContext& context) { Update(context, Sites[0]); }
    inline void UpdateSecond(SafetyHookContext& context) { Update(context, Sites[1]); }

    inline void Install()
    {
        CIniReader ini("");
        Enabled = ini.ReadBoolean("CAMERA", "SmoothFollow", true);
        FollowSeconds = std::clamp(double(ini.ReadFloat("CAMERA", "FollowTime", 0.25f)), 0.0, 2.0);
        HeightSeconds = std::clamp(double(ini.ReadFloat("CAMERA", "HeightTime", 0.35f)), 0.0, 3.0);
        Zoom = std::clamp(double(ini.ReadFloat("CAMERA", "Zoom", 1.0f)), 0.5, 3.0);
        LARGE_INTEGER frequency{}; QueryPerformanceFrequency(&frequency);
        CounterFrequency = double(frequency.QuadPart);
        // After the native camera constraints, before projection and visibility bounds.
        // Classics and Retail inline this in two camera functions, London calls it.
        using GTA1Build::Kind;
        auto update = GTA1Build::Find("8B 54 24 1C 8B C8 8B 44 24 20 53 89 51 20 8B 51 08 0F AF D7 89 41 1C B8 67 66 66 66",
            "E8 ? ? ? ? 8B 54 24 ? 83 C4 04 89 50 1C 8B C8 8B 44 24 ? 56 89 41 20 89 79 18 BF 40 01 00 00 89 59 28",
            "56 52 8B 54 24 18 50 51 52 E8 ? ? ? ? 8B 44 24 2C 8B 4C 24 1C 8B 54 24 20 83 C4 14");
        auto control = GTA1Build::Find("53 E8 ? ? ? ? 0F BE C0 53 8D 04 C0 8D 2C 85 ? ? ? ? E8",
            "56 E8 ? ? ? ? 0F BE E8 C1 E5 02 83 C4 04 56 E8", "56 57 8B 7C 24 1C 57 E8 ? ? ? ? 83 C4 04 8B F0 57 E8");
        const size_t sites = GTA1Build::Current == Kind::London ? 1 : 2;
        if (update.size() != sites || control.empty())
        { Log::Write("GTA1 camera signatures unavailable."); return; }
        Control = reinterpret_cast<decltype(Control)>(injector::GetBranchDestination(control.get_first(GTA1Build::Off(20, 16, 7))).get<void>());
        const int at = GTA1Build::Off(0, 5, 14);
        for (size_t i = 0; i < update.size(); ++i)
        {
            auto match = update.get(i).get<uint8_t>();
            auto& site = Sites[i];
            if (GTA1Build::Current == Kind::Retail)
                site = { Esi, match[8], match[20] + 4, Edi, -1 };       // the stack slots differ between the two sites
            else if (GTA1Build::Current == Kind::London)
                site = { Esi, 0x24, 0x20, Esi, 0x1C };
            UpdateHooks[i] = safetyhook::create_mid(match + at, i == 0 ? UpdateFirst : UpdateSecond);
        }
        Log::Write("GTA1 smooth overhead camera installed.");
    }
}
