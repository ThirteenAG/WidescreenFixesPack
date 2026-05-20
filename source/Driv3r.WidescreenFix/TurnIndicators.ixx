module;

#include <stdafx.h>
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

struct CVehicle;
static void (__fastcall* ActivateLamp)(CVehicle* veh, void* edx, Lights a3, char a4, char a5) = nullptr;

using Vehicle = CVehicle*;

export GameRef<void*> dword_8AC3F0([]() -> void**
{
    //auto pattern = hook::pattern("8B 0D ? ? ? ? 8B 41 ? 56 83 C1 ? 6A ? FF 50 ? ? ? 8B C8 FF 52 ? 8B 70");
    auto pattern = hook::pattern("8B 0D ? ? ? ? 8B 81 ? ? ? ? 8D 91 ? ? ? ? 3B C2 89 4C 24");
    if (!pattern.empty())
        return *pattern.get_first<void**>(2);
    return nullptr;
});

// Configuration
constexpr float STEER_THRESHOLD = 0.50f;    // Minimum steering to trigger blinkers
constexpr float ACTIVATION_DELAY = 400.0f;  // ms delay before activation
constexpr float BLINK_INTERVAL = 400.0f;    // ms blink interval (on/off cycle)
constexpr float TAP_THRESHOLD = 300.0f;     // ms threshold for tap detection

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
    float Timestep()
    {
        return fTimeStep;
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

    //float IsLBPressed()
    //{
    //    return *(float*)((uintptr_t)*pFrontendInputManager + 0x124);
    //}
    //
    //float IsRBPressed()
    //{
    //    return *(float*)((uintptr_t)*pFrontendInputManager + 0x118);
    //}
    //
    //float IsLeftPressed()
    //{
    //    return *(float*)((uintptr_t)*pFrontendInputManager + 0x1C0);
    //}
    //
    //float IsRightPressed()
    //{
    //    return *(float*)((uintptr_t)*pFrontendInputManager + 0x1B4);
    //}
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
        if (old_state.blinkers_active || old_state.current_blinker != 0)
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
            CIniReader iniReader("");
            static bool bTurnIndicators = iniReader.ReadInteger("TURNINDICATORS", "TurnIndicators", 1);
            static bool bManualTurnIndicators = iniReader.ReadInteger("TURNINDICATORS", "ManualTurnIndicators", 0);
            static uint32_t nLeftIndicatorKey = iniReader.ReadInteger("TURNINDICATORS", "LeftIndicatorKey", VK_OEM_4);
            static uint32_t nRightIndicatorKey = iniReader.ReadInteger("TURNINDICATORS", "RightIndicatorKey", VK_OEM_6);

            auto pattern = hook::pattern("51 56 8B B1 ? ? ? ? ? ? 83 F8 ? 57 8B B9");
            ActivateLamp = (decltype(ActivateLamp))pattern.get_first();

            pattern = hook::pattern("C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? 8D B5");
            if (!pattern.empty())
            {
                static auto CVehicleDtorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    CVehicle* veh = (CVehicle*)regs.ebp;
                    auto it = std::remove_if(recent_vehicles.begin(), recent_vehicles.end(),
                        [veh](const std::pair<Vehicle, VehicleState>& p) { return p.first == veh; });
                    recent_vehicles.erase(it, recent_vehicles.end());
                });
            }
            else
            {
                pattern = hook::pattern("C7 46 ? ? ? ? ? C7 46 ? ? ? ? ? C7 46 ? ? ? ? ? C7 46 ? ? ? ? ? C7 46 ? ? ? ? ? C7 46 ? ? ? ? ? C7 46 ? ? ? ? ? 8B 8E");
                static auto CVehicleDtorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    CVehicle* veh = (CVehicle*)regs.esi;
                    auto it = std::remove_if(recent_vehicles.begin(), recent_vehicles.end(),
                        [veh](const std::pair<Vehicle, VehicleState>& p) { return p.first == veh; });
                    recent_vehicles.erase(it, recent_vehicles.end());
                });
            }

            pattern = hook::pattern("89 7D ? 89 7D ? 89 7D ? 89 7D ? 8B 75");
            static auto dword_8AC3F0Dtor = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                dword_8AC3F0 = nullptr;
            });

            static uintptr_t prev_player_car = 0;

            WFP::onGameProcessEvent() += [&]()
            {
                static Vehicle last_player_car = 0;
                static bool prev_lb_pressed = false;
                static bool prev_rb_pressed = false;
                static float lb_press_time = 0.0f;
                static float rb_press_time = 0.0f;

                // Get current player vehicle
                Vehicle PlayerCar = nullptr;

                auto ls = *dword_8AC3F0;
                if (ls)
                {
                    auto p1 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(ls) + 0x08);
                    if (p1)
                    {
                        auto p2 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(p1) + 0x0C);
                        if (p2)
                            PlayerCar = (CVehicle*)(reinterpret_cast<uintptr_t>(p2) - 4);
                    }
                }

                // Check if player has fully entered the vehicle (can actually drive)
                auto pVehicleStruct = PlayerCar;
                bool player_fully_in_vehicle = (PlayerCar != nullptr && pVehicleStruct != nullptr);

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

                    // Skip processing if feature is disabled and no active blinkers
                    if (!bTurnIndicators && state.current_blinker == 0)
                    {
                        continue;
                    }

                    // Get vehicle structure (we already have it for player vehicle)
                    auto vehicleStruct = (is_player_vehicle) ? pVehicleStruct : nullptr;

                    if (player_in_vehicle && vehicleStruct)
                    {
                        if (bManualTurnIndicators)
                        {
                            float dt_ms = 1000.0f * Natives::Timestep();

                            float cur_lb = 0.0f;
                            float cur_rb = 0.0f;

                            bool cur_left = IsKeyboardKeyPressed(nLeftIndicatorKey);
                            bool cur_right = IsKeyboardKeyPressed(nRightIndicatorKey);

                            bool left_double_tapped = false; //dtap_left.Update(Natives::IsLeftPressed(), dt_ms);
                            bool right_double_tapped = false; //dtap_right.Update(Natives::IsRightPressed(), dt_ms);

                            bool lb_pressed = (cur_lb != 0.0f) || (cur_left != 0.0f);
                            bool rb_pressed = (cur_rb != 0.0f) || (cur_right != 0.0f);

                            if (lb_pressed && !prev_lb_pressed)
                            {
                                lb_press_time = 0.0f;
                            }
                            if (rb_pressed && !prev_rb_pressed)
                            {
                                rb_press_time = 0.0f;
                            }
                            if (lb_pressed)
                            {
                                lb_press_time += 1000.0f * Natives::Timestep();
                            }
                            if (rb_pressed)
                            {
                                rb_press_time += 1000.0f * Natives::Timestep();
                            }

                            bool lb_tapped = (!lb_pressed && prev_lb_pressed && lb_press_time <= TAP_THRESHOLD) || left_double_tapped;
                            bool rb_tapped = (!rb_pressed && prev_rb_pressed && rb_press_time <= TAP_THRESHOLD) || right_double_tapped;

                            if (lb_tapped && rb_tapped)
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
                            else if (lb_tapped)
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
                            else if (rb_tapped)
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
                            prev_lb_pressed = lb_pressed;
                            prev_rb_pressed = rb_pressed;
                        }
                        else
                        {
                            // Process player vehicle with turn indicators
                            auto m_fSteerAngle = *(float*)((uintptr_t)vehicleStruct + 0x554);

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
                            state.blink_timer += 1000.0f * Natives::Timestep();
                            if (state.blink_timer >= BLINK_INTERVAL)
                            {
                                state.blink_state = !state.blink_state;
                                state.blink_timer = 0.0f;
                                Natives::SetVehHazardlights(veh, state.blink_state);
                            }
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
                        // Vehicle is not occupied by player - continue processing blinkers
                        if (state.current_blinker != 0)
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