module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#include <array>
#include <chrono>
#include <deque>

export module TurnIndicators;

import ComVars;

enum Lights
{
    LIGHT_FRONT_LEFT = 70,
    LIGHT_FRONT_RIGHT = 71,
    LIGHT_REAR_LEFT = 12,
    LIGHT_REAR_RIGHT = 13,
};

static void (__fastcall* ActivateLamp)(CVehicle* veh, void* edx, Lights a3, char a4, char a5) = nullptr;

using Vehicle = CVehicle*;

void InitTurnIndicatorsRequiredRefs()
{
    pLifeSystem.SetAddress(*hook::pattern("8B 0D ? ? ? ? 3B CF 89 3D").get_first<CLifeSystem**>(2));
    pGameTime.SetAddress(*hook::pattern("A1 ? ? ? ? 8B 48 ? ? ? 8B 53").get_first<CGameTime**>(1));
}


// Configuration
constexpr float STEER_THRESHOLD = 0.50f;    // Minimum steering to trigger blinkers
constexpr float ACTIVATION_DELAY = 400.0f;  // ms delay before activation
constexpr float BLINK_INTERVAL = 400.0f;    // ms blink interval (on/off cycle)
constexpr auto TAP_THRESHOLD = std::chrono::milliseconds(300);

struct IndicatorTaps
{
    using Clock = std::chrono::steady_clock;
    struct Actions { bool left{}, right{}, hazards{}; };
    Vehicle owner = nullptr;
    bool waitForNeutral = true;
    bool leftWasDown = false, rightWasDown = false, chord = false;
    Clock::time_point leftStart{}, rightStart{};
    bool shortChord = false;

    Actions Update(Vehicle vehicle, bool active, bool leftDown, bool rightDown, Clock::time_point now)
    {
        if (vehicle != owner || !active)
        {
            owner = vehicle;
            waitForNeutral = true;
            leftWasDown = rightWasDown = chord = false;
            shortChord = false;
        }
        if (!active) return {};
        if (waitForNeutral)
        {
            waitForNeutral = leftDown || rightDown;
            return {};
        }

        if (leftDown && !leftWasDown) leftStart = now;
        if (rightDown && !rightWasDown) rightStart = now;
        const bool leftShort = now - leftStart <= TAP_THRESHOLD;
        const bool rightShort = now - rightStart <= TAP_THRESHOLD;
        const bool leftTap = leftWasDown && !leftDown && leftShort;
        const bool rightTap = rightWasDown && !rightDown && rightShort;
        if (leftDown && rightDown && leftShort && rightShort && !chord)
            chord = shortChord = true;
        if ((leftWasDown && !leftShort) || (rightWasDown && !rightShort))
            shortChord = false;
        leftWasDown = leftDown;
        rightWasDown = rightDown;

        if (!chord) return { leftTap, rightTap, false };
        if (leftDown || rightDown) return {};
        chord = false;
        return { false, false, shortChord };
    }
};
static IndicatorTaps indicatorTaps;

struct IndicatorPad
{
    static constexpr WORD Bumpers = XINPUT_GAMEPAD_LEFT_SHOULDER | XINPUT_GAMEPAD_RIGHT_SHOULDER;
    DWORD slot = XUSER_MAX_COUNT;
    std::array<WORD, XUSER_MAX_COUNT> previous{};
    std::array<bool, XUSER_MAX_COUNT> neutralSeen{};

    struct Sample { WORD buttons{}; bool disconnectedWhileHeld{}; };

    Sample Read(bool active)
    {
        const auto getState = PadInput::XInput().getState;
        std::array<WORD, XUSER_MAX_COUNT> buttons{};
        DWORD pressed = XUSER_MAX_COUNT;
        bool disconnectedWhileHeld = false;
        const bool ownerWasHolding = slot < XUSER_MAX_COUNT && previous[slot] != 0;
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
        {
            XINPUT_STATE state{};
            if (!getState || getState(i, &state) != ERROR_SUCCESS)
            {
                if (i == slot)
                {
                    disconnectedWhileHeld = ownerWasHolding;
                    slot = XUSER_MAX_COUNT;
                }
                neutralSeen[i] = false;
            }
            else
            {
                const WORD down = state.Gamepad.wButtons & Bumpers;
                neutralSeen[i] = (active && neutralSeen[i]) || down == 0;
                if (active && neutralSeen[i])
                {
                    buttons[i] = down;
                    if (pressed == XUSER_MAX_COUNT && (down & ~previous[i]))
                        pressed = i;
                }
            }
            previous[i] = buttons[i];
        }
        if (!active)
            slot = XUSER_MAX_COUNT;
        else if (!ownerWasHolding && pressed != XUSER_MAX_COUNT &&
                 (slot == XUSER_MAX_COUNT || !buttons[slot]))
            slot = pressed;
        return { slot < XUSER_MAX_COUNT ? buttons[slot] : WORD{}, disconnectedWhileHeld };
    }
};
static IndicatorPad indicatorPad;

export void CancelIndicatorTaps()
{
    indicatorTaps = {};
    indicatorPad = {};
}

// Struct to hold all vehicle state data
struct VehicleState
{
    int current_blinker = 0;
    float turn_start_timer = 0.0f;
    float turn_stop_timer = 0.0f;
    int last_turn_direction = 0;
    bool is_turning = false;
    bool blinkers_active = false;
    bool should_stop_blinking = false;
    bool blink_state = false;
    float blink_timer = 0.0f;
    float final_blink_timer = 0.0f;
    bool manual_hazard = false;
    int pending_blinker = 0;
};

// Queue to store last 5 player vehicles with their states
static std::deque<std::pair<Vehicle, VehicleState>> recent_vehicles;
static const size_t MAX_RECENT_VEHICLES = 5;

namespace Natives
{
    float frameTime = 1.0f / 60.0f;

    void UpdateFrameTime()
    {
        static std::array<int64_t, 50> times{};
        static size_t next = 0, count = 0;
        static const int64_t frequency = []()
        {
            LARGE_INTEGER value{};
            QueryPerformanceFrequency(&value);
            return value.QuadPart;
        }();
        LARGE_INTEGER time{};
        QueryPerformanceCounter(&time);
        times[next] = time.QuadPart;
        next = (next + 1) % times.size();
        count = std::min(count + 1, times.size());
        const double elapsed = static_cast<double>(time.QuadPart - times[count == times.size() ? next : 0]);
        frameTime = count > 1 && elapsed > 0.0 && frequency > 0 ?
            static_cast<float>(std::clamp(elapsed / ((count - 1) * frequency), 0.001, 0.1)) : 1.0f / 60.0f;
    }

    float Timestep()
    {
        auto time = *pGameTime;
        if (time && time->f[0])
            return frameTime;
        return 0.0f;
    }

    void SetVehIndicatorlights(Vehicle veh, bool on)
    {
        if (!veh) return;
        if (!on)
        {
            ActivateLamp(veh, nullptr, LIGHT_FRONT_LEFT, false, false);
            ActivateLamp(veh, nullptr, LIGHT_REAR_LEFT, false, false);
            ActivateLamp(veh, nullptr, LIGHT_FRONT_RIGHT, false, false);
            ActivateLamp(veh, nullptr, LIGHT_REAR_RIGHT, false, false);
            return;
        }

        int side = 0;
        for (auto& pair : recent_vehicles)
            if (pair.first == veh) { side = pair.second.current_blinker; break; }

        if (side == 1 || side == 0) { ActivateLamp(veh, nullptr, LIGHT_FRONT_LEFT, true, true); ActivateLamp(veh, nullptr, LIGHT_REAR_LEFT, true, true); }
        if (side == 2 || side == 0) { ActivateLamp(veh, nullptr, LIGHT_FRONT_RIGHT, true, true); ActivateLamp(veh, nullptr, LIGHT_REAR_RIGHT, true, true); }
    }

    void SetVehHazardlights(Vehicle veh, bool on)
    {
        if (!veh) return;
        ActivateLamp(veh, nullptr, LIGHT_FRONT_LEFT, on, on);
        ActivateLamp(veh, nullptr, LIGHT_REAR_LEFT, on, on);
        ActivateLamp(veh, nullptr, LIGHT_FRONT_RIGHT, on, on);
        ActivateLamp(veh, nullptr, LIGHT_REAR_RIGHT, on, on);
    }

    void BlinkHazards(Vehicle vehicle, VehicleState& state)
    {
        state.blink_timer += 1000.0f * Timestep();
        if (state.blink_timer >= BLINK_INTERVAL)
        {
            state.blink_state = !state.blink_state;
            state.blink_timer -= BLINK_INTERVAL;
            SetVehHazardlights(vehicle, state.blink_state);
        }
    }

};

// Helper function to find vehicle in queue
VehicleState* findVehicle(Vehicle veh)
{
    for (auto& pair : recent_vehicles)
    {
        if (pair.first == veh)
        {
            return &pair.second;
        }
    }
    return nullptr;
}

VehicleState* addOrUpdateVehicle(Vehicle veh, const VehicleState& state = VehicleState{})
{
    if (!recent_vehicles.empty() && recent_vehicles.front().first == veh)
        return &recent_vehicles.front().second;

    // Check if vehicle already exists
    for (auto it = recent_vehicles.begin(); it != recent_vehicles.end(); ++it)
    {
        if (it->first == veh)
        {
            // Move to front and keep existing state
            VehicleState existing_state = it->second;
            recent_vehicles.erase(it);
            recent_vehicles.push_front({ veh, existing_state });
            return &recent_vehicles.front().second;
        }
    }

    // Add new vehicle to front
    recent_vehicles.push_front({ veh, state });

    // Remove oldest if we exceed max size
    if (recent_vehicles.size() > MAX_RECENT_VEHICLES)
    {
        // Turn off any active lights on the vehicle being removed
        Vehicle old_veh = recent_vehicles.back().first;
        VehicleState& old_state = recent_vehicles.back().second;
        if (old_state.manual_hazard || old_state.blinkers_active || old_state.current_blinker != 0)
        {
            Natives::SetVehIndicatorlights(old_veh, false);
        }
        recent_vehicles.pop_back();
    }

    return &recent_vehicles.front().second;
};

class TurnIndicators
{
public:
    TurnIndicators()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            static bool bTurnIndicators = settings.Int("TURNINDICATORS.TurnIndicators");
            static bool bManualTurnIndicators = settings.Int("TURNINDICATORS.ManualTurnIndicators");
            static uint32_t nLeftIndicatorKey = settings.Int("TURNINDICATORS.LeftIndicatorKey");
            static uint32_t nRightIndicatorKey = settings.Int("TURNINDICATORS.RightIndicatorKey");
            if (!bTurnIndicators)
                return;

            InitTurnIndicatorsRequiredRefs();
            auto pattern = hook::pattern("55 8B EC 51 8B 51 ? ? ? 83 F8");
            ActivateLamp = (decltype(ActivateLamp))pattern.get_first();

            pattern = hook::pattern("C7 06 ? ? ? ? C7 46 ? ? ? ? ? C7 46 ? ? ? ? ? C7 46 ? ? ? ? ? C7 46 ? ? ? ? ? C7 07");
            static auto CVehicleDtorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                CVehicle* veh = (CVehicle*)regs.esi;
                if (indicatorTaps.owner == veh)
                    CancelIndicatorTaps();
                auto it = std::remove_if(recent_vehicles.begin(), recent_vehicles.end(),
                    [veh](const std::pair<Vehicle, VehicleState>& p) { return p.first == veh; });
                recent_vehicles.erase(it, recent_vehicles.end());
            });

            WFP::onGameProcessEvent() += [&]()
            {
                Natives::UpdateFrameTime();
                // Get current player vehicle
                CCharacter* PlayerPed = nullptr;
                Vehicle PlayerCar = nullptr;

                auto ls = *pLifeSystem;
                if (ls)
                {
                    auto p = ls->GetPlayer();
                    if (p)
                    {
                        auto db = p->GetDriverBehaviour();
                        if (db)
                            PlayerPed = db->GetCharacter();
                    }
                }

                if (PlayerPed)
                {
                    PlayerCar = PlayerPed->GetVehicle();

                    if (isTurret)
                        PlayerCar = nullptr;
                }

                // Check if player has fully entered the vehicle (can actually drive)
                auto pVehicleStruct = PlayerCar;
                bool player_fully_in_vehicle = (PlayerCar != nullptr && pVehicleStruct != nullptr);
                IndicatorTaps::Actions manual{};
                if (bManualTurnIndicators)
                {
                    const bool active = player_fully_in_vehicle && !BlockGameInput && Natives::Timestep() > 0.0f;
                    if (active)
                    {
                        if (indicatorTaps.owner != PlayerCar)
                            indicatorPad = {};
                        const auto pad = indicatorPad.Read(true);
                        const bool left = IsKeyboardKeyPressed(nLeftIndicatorKey) || (pad.buttons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        const bool right = IsKeyboardKeyPressed(nRightIndicatorKey) || (pad.buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        manual = indicatorTaps.Update(PlayerCar, !pad.disconnectedWhileHeld, left, right, IndicatorTaps::Clock::now());
                    }
                    else if (indicatorTaps.owner || indicatorPad.slot != XUSER_MAX_COUNT)
                    {
                        CancelIndicatorTaps();
                    }
                }

                // If player is in a vehicle, add/update it in our queue
                if (PlayerCar != nullptr)
                {
                    addOrUpdateVehicle(PlayerCar);
                }

                // Process all vehicles in our queue
                for (auto& pair : recent_vehicles)
                {
                    Vehicle veh = pair.first;
                    VehicleState& state = pair.second;

                    bool is_player_vehicle = (veh == PlayerCar);
                    bool player_in_vehicle = is_player_vehicle && player_fully_in_vehicle;

                    // Get vehicle structure (we already have it for player vehicle)
                    auto vehicleStruct = (is_player_vehicle) ? pVehicleStruct : nullptr;

                    if (player_in_vehicle && vehicleStruct)
                    {
                        if (bManualTurnIndicators)
                        {
                            if (manual.hazards)
                            {
                                state.manual_hazard = !state.manual_hazard;
                                Natives::SetVehHazardlights(veh, state.manual_hazard);
                                if (state.manual_hazard)
                                {
                                    state.blink_state = true;
                                    state.blink_timer = 0.0f;
                                    state.current_blinker = 0;
                                    state.blinkers_active = false;
                                    state.should_stop_blinking = false;
                                    state.final_blink_timer = 0.0f;
                                    state.pending_blinker = 0;
                                }
                            }
                            else if (manual.left)
                            {
                                if (state.manual_hazard)
                                {
                                    state.manual_hazard = false;
                                    Natives::SetVehHazardlights(veh, false);
                                }
                                if (state.current_blinker == 1 && state.blinkers_active)
                                {
                                    Natives::SetVehIndicatorlights(veh, false);
                                    state.current_blinker = 0;
                                    state.blinkers_active = false;
                                    state.should_stop_blinking = false;
                                    state.blink_state = false;
                                    state.blink_timer = 0.0f;
                                    state.final_blink_timer = 0.0f;
                                    state.pending_blinker = 0;
                                }
                                else
                                {
                                    if (state.current_blinker != 0 && state.blinkers_active)
                                    {
                                        // Switching indicators: stop current and set pending
                                        state.should_stop_blinking = true;
                                        state.final_blink_timer = 0.0f;
                                        state.pending_blinker = 1;
                                    }
                                    else
                                    {
                                        // Activate immediately
                                        state.current_blinker = 1;
                                        state.blinkers_active = true;
                                        state.blink_timer = 0.0f;
                                        state.blink_state = true;
                                        Natives::SetVehIndicatorlights(veh, true);
                                        state.pending_blinker = 0;
                                    }
                                }
                            }
                            else if (manual.right)
                            {
                                if (state.manual_hazard)
                                {
                                    state.manual_hazard = false;
                                    Natives::SetVehHazardlights(veh, false);
                                }
                                if (state.current_blinker == 2 && state.blinkers_active)
                                {
                                    Natives::SetVehIndicatorlights(veh, false);
                                    state.current_blinker = 0;
                                    state.blinkers_active = false;
                                    state.should_stop_blinking = false;
                                    state.blink_state = false;
                                    state.blink_timer = 0.0f;
                                    state.final_blink_timer = 0.0f;
                                    state.pending_blinker = 0;
                                }
                                else
                                {
                                    if (state.current_blinker != 0 && state.blinkers_active)
                                    {
                                        // Switching indicators: stop current and set pending
                                        state.should_stop_blinking = true;
                                        state.final_blink_timer = 0.0f;
                                        state.pending_blinker = 2;
                                    }
                                    else
                                    {
                                        // Activate immediately
                                        state.current_blinker = 2;
                                        state.blinkers_active = true;
                                        state.blink_timer = 0.0f;
                                        state.blink_state = true;
                                        Natives::SetVehIndicatorlights(veh, true);
                                        state.pending_blinker = 0;
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Process player vehicle with turn indicators
                            auto m_fSteerAngle = *(float*)((uintptr_t)vehicleStruct + 0xB4);

                            // Determine current turn direction
                            int current_turn_direction = 0;
                            if (m_fSteerAngle > STEER_THRESHOLD)
                                current_turn_direction = 1; // Left
                            else if (m_fSteerAngle < -STEER_THRESHOLD)
                                current_turn_direction = 2; // Right

                            bool currently_turning = (current_turn_direction != 0);
                            bool direction_changed = (current_turn_direction != state.last_turn_direction &&
                                current_turn_direction != 0);

                            // Handle direction change
                            if (direction_changed)
                            {
                                if (state.blinkers_active)
                                {
                                    Natives::SetVehIndicatorlights(veh, false);
                                    state.blinkers_active = false;
                                    state.current_blinker = 0;
                                    state.should_stop_blinking = false;
                                    state.blink_state = false;
                                    state.blink_timer = 0.0f;
                                }

                                state.turn_start_timer = 0.0f;
                                state.turn_stop_timer = 0.0f;
                                state.is_turning = true;
                            }
                            else if (currently_turning && !state.is_turning)
                            {
                                state.turn_start_timer = 0.0f;
                                state.turn_stop_timer = 0.0f;
                                state.is_turning = true;
                            }
                            else if (!currently_turning && state.is_turning)
                            {
                                state.turn_stop_timer = 0.0f;
                                state.is_turning = false;
                                if (state.blinkers_active)
                                {
                                    state.should_stop_blinking = true;
                                    state.final_blink_timer = 0.0f;
                                }
                            }

                            // Update timers
                            if (state.is_turning && current_turn_direction != 0 && !state.should_stop_blinking)
                            {
                                state.turn_start_timer += 1000.0f * Natives::Timestep();

                                if (state.turn_start_timer >= ACTIVATION_DELAY && !state.blinkers_active)
                                {
                                    state.current_blinker = current_turn_direction;
                                    state.blinkers_active = true;
                                    state.blink_timer = 0.0f;
                                    state.blink_state = true;
                                    Natives::SetVehIndicatorlights(veh, true);
                                }
                            }
                            state.last_turn_direction = current_turn_direction;
                        }

                        // Handle blinking logic
                        if (state.manual_hazard)
                        {
                            Natives::BlinkHazards(veh, state);
                        }
                        else if (state.blinkers_active && !state.should_stop_blinking)
                        {
                            state.blink_timer += 1000.0f * Natives::Timestep();

                            if (state.blink_timer >= BLINK_INTERVAL)
                            {
                                state.blink_state = !state.blink_state;
                                state.blink_timer = 0.0f;

                                if (state.blink_state)
                                {
                                    Natives::SetVehIndicatorlights(veh, true);
                                }
                                else
                                {
                                    Natives::SetVehIndicatorlights(veh, false);
                                }
                            }
                        }
                        else if (state.should_stop_blinking && state.blinkers_active)
                        {
                            state.final_blink_timer += 1000.0f * Natives::Timestep();
                            state.blink_timer += 1000.0f * Natives::Timestep();

                            if (state.blink_timer >= BLINK_INTERVAL)
                            {
                                if (state.blink_state)
                                {
                                    Natives::SetVehIndicatorlights(veh, false);
                                    state.blink_state = false;
                                    state.blink_timer = 0.0f;
                                }
                                else
                                {
                                    // Finished the off cycle, now stop
                                    state.blinkers_active = false;
                                    state.current_blinker = 0;
                                    state.should_stop_blinking = false;
                                    state.blink_state = false;
                                    state.blink_timer = 0.0f;
                                }
                            }
                            // Safety timeout
                            if (state.final_blink_timer >= BLINK_INTERVAL * 2)
                            {
                                Natives::SetVehIndicatorlights(veh, false);
                                state.blinkers_active = false;
                                state.current_blinker = 0;
                                state.should_stop_blinking = false;
                                state.blink_state = false;
                                state.blink_timer = 0.0f;
                            }
                        }
                        // Activate pending blinker after current one stops
                        if (!state.blinkers_active && state.pending_blinker != 0)
                        {
                            state.current_blinker = state.pending_blinker;
                            state.blinkers_active = true;
                            state.blink_timer = 0.0f;
                            state.blink_state = true;
                            Natives::SetVehIndicatorlights(veh, true);
                            state.pending_blinker = 0;
                            state.should_stop_blinking = false;
                            state.final_blink_timer = 0.0f;
                        }
                    }
                    else
                    {
                        if (state.manual_hazard)
                        {
                            Natives::BlinkHazards(veh, state);
                        }
                        else if (state.current_blinker != 0)
                        {
                            // Continue blinking process
                            if (state.blinkers_active && !state.should_stop_blinking)
                            {
                                state.blink_timer += 1000.0f * Natives::Timestep();
                                if (state.blink_timer >= BLINK_INTERVAL)
                                {
                                    state.blink_state = !state.blink_state;
                                    state.blink_timer = 0.0f;
                                    if (state.blink_state)
                                    {
                                        Natives::SetVehIndicatorlights(veh, true);
                                    }
                                    else
                                    {
                                        Natives::SetVehIndicatorlights(veh, false);
                                    }
                                }
                            }
                            else if (state.should_stop_blinking && state.blinkers_active)
                            {
                                state.final_blink_timer += 1000.0f * Natives::Timestep();
                                if (state.blink_state)
                                {
                                    state.blink_timer += 1000.0f * Natives::Timestep();
                                    if (state.blink_timer >= BLINK_INTERVAL)
                                    {
                                        Natives::SetVehIndicatorlights(veh, false);
                                        state.current_blinker = 0;
                                        state.blinkers_active = false;
                                        state.should_stop_blinking = false;
                                        state.blink_state = false;
                                    }
                                }
                                else
                                {
                                    state.current_blinker = 0;
                                    state.blinkers_active = false;
                                    state.should_stop_blinking = false;
                                    state.blink_state = false;
                                }
                                // Safety timeout
                                if (state.final_blink_timer >= BLINK_INTERVAL * 2)
                                {
                                    Natives::SetVehIndicatorlights(veh, false);
                                    state.current_blinker = 0;
                                    state.blinkers_active = false;
                                    state.should_stop_blinking = false;
                                    state.blink_state = false;
                                }
                            }
                        }
                    }
                }
            };
        };
    }
} TurnIndicators;
