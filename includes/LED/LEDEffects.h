#pragma once
#include "LogitechLEDLib.h"
#ifdef _M_IX86
#pragma comment(lib, "LogitechLEDLib_x86.lib")
#else
#pragma comment(lib, "LogitechLEDLib_x64.lib")
#endif
#include <chrono>
#include <vector>
#include <algorithm>

class LEDEffects
{
public:
    static inline bool bLogiLedInitialized;
private:
    static inline std::vector<HWND> AppWindows;
    static inline WNDPROC DefaultWndProc = nullptr;
    static inline std::vector<std::function<void()>> Callbacks;
    static inline std::future<void> Future;

    static void Init()
    {
        if (!bLogiLedInitialized)
            bLogiLedInitialized = LogiLedInit();
    }

    static void Shutdown()
    {
        if (bLogiLedInitialized) {
            bLogiLedInitialized = false;
            if (Future.valid())
                Future.wait();
            LogiLedShutdown();
        }
    }

    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        DWORD lpdwProcessId;
        GetWindowThreadProcessId(hwnd, &lpdwProcessId);
        if (lpdwProcessId == lParam)
        {
            if (IsWindowVisible(hwnd))
                AppWindows.push_back(hwnd);
        }
        return TRUE;
    }

    static LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_QUIT:
        case WM_DESTROY:
            Shutdown();
            break;
        default:
            break;
        }

        return CallWindowProcW(DefaultWndProc, hWnd, uMsg, wParam, lParam);
    }

public:
    static void Inject(std::function<void()> callback)
    {
        Init();

        Callbacks.push_back(callback);

        if (bLogiLedInitialized)
        {
            static std::once_flag flag;
            std::call_once(flag, []()
            {
                Future = std::async(std::launch::async, []() 
                {
                    while (true)
                    {
                        std::this_thread::yield();
                        if (LEDEffects::bLogiLedInitialized)
                        {
                            for (auto& cb : Callbacks) { cb(); }
                        }
                        else
                            break;
                    }
                });

                if (AppWindows.empty())
                {
                    std::thread([]()
                    {
                        while (AppWindows.empty())
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                            EnumWindows(EnumWindowsProc, GetCurrentProcessId());
                        }
                        DefaultWndProc = (WNDPROC)SetWindowLongPtrW(AppWindows.back(), GWLP_WNDPROC, (LONG_PTR)WndProc);
                    }).detach();
                }

                IATHook::Replace(GetModuleHandleA(NULL), "KERNEL32.DLL",
                    std::forward_as_tuple("ExitProcess", static_cast<void(__stdcall*)(UINT)>([](UINT uExitCode) {
                        Shutdown();
                        ExitProcess(uExitCode);
                    }))
                );
            });
        }
    }

private:
    static inline std::vector<LogiLed::KeyName> leftSide = {
        LogiLed::KeyName::ESC,
        LogiLed::KeyName::F1,
        LogiLed::KeyName::F2,
        LogiLed::KeyName::F3,
        LogiLed::KeyName::F4,
        LogiLed::KeyName::F5,
        LogiLed::KeyName::F6,
        LogiLed::KeyName::F7,
        LogiLed::KeyName::F8,
        LogiLed::KeyName::F9,
        LogiLed::KeyName::F10,
        LogiLed::KeyName::F11,
        LogiLed::KeyName::F12,
        LogiLed::KeyName::TILDE,
        LogiLed::KeyName::ONE,
        LogiLed::KeyName::TWO,
        LogiLed::KeyName::THREE,
        LogiLed::KeyName::FOUR,
        LogiLed::KeyName::FIVE,
        LogiLed::KeyName::SIX,
        LogiLed::KeyName::SEVEN,
        LogiLed::KeyName::EIGHT,
        LogiLed::KeyName::NINE,
        LogiLed::KeyName::ZERO,
        LogiLed::KeyName::MINUS,
        LogiLed::KeyName::EQUALS,
        LogiLed::KeyName::BACKSPACE,
        LogiLed::KeyName::TAB,
        LogiLed::KeyName::Q,
        LogiLed::KeyName::W,
        LogiLed::KeyName::E,
        LogiLed::KeyName::R,
        LogiLed::KeyName::T,
        LogiLed::KeyName::Y,
        LogiLed::KeyName::U,
        LogiLed::KeyName::I,
        LogiLed::KeyName::O,
        LogiLed::KeyName::P,
        LogiLed::KeyName::OPEN_BRACKET,
        LogiLed::KeyName::CLOSE_BRACKET,
        LogiLed::KeyName::BACKSLASH,
        LogiLed::KeyName::CAPS_LOCK,
        LogiLed::KeyName::A,
        LogiLed::KeyName::S,
        LogiLed::KeyName::D,
        LogiLed::KeyName::F,
        LogiLed::KeyName::G,
        LogiLed::KeyName::H,
        LogiLed::KeyName::J,
        LogiLed::KeyName::K,
        LogiLed::KeyName::L,
        LogiLed::KeyName::SEMICOLON,
        LogiLed::KeyName::APOSTROPHE,
        LogiLed::KeyName::ENTER,
        LogiLed::KeyName::LEFT_SHIFT,
        LogiLed::KeyName::Z,
        LogiLed::KeyName::X,
        LogiLed::KeyName::C,
        LogiLed::KeyName::V,
        LogiLed::KeyName::B,
        LogiLed::KeyName::N,
        LogiLed::KeyName::M,
        LogiLed::KeyName::COMMA,
        LogiLed::KeyName::PERIOD,
        LogiLed::KeyName::FORWARD_SLASH,
        LogiLed::KeyName::RIGHT_SHIFT,
        LogiLed::KeyName::LEFT_CONTROL,
        LogiLed::KeyName::LEFT_WINDOWS,
        LogiLed::KeyName::LEFT_ALT,
        LogiLed::KeyName::SPACE,
        LogiLed::KeyName::RIGHT_ALT,
        LogiLed::KeyName::RIGHT_WINDOWS,
        LogiLed::KeyName::APPLICATION_SELECT,
        LogiLed::KeyName::RIGHT_CONTROL,
        LogiLed::KeyName::G_1,
        LogiLed::KeyName::G_2,
        LogiLed::KeyName::G_3,
        LogiLed::KeyName::G_4,
        LogiLed::KeyName::G_5,
        LogiLed::KeyName::G_6,
        LogiLed::KeyName::G_7,
        LogiLed::KeyName::G_8,
        LogiLed::KeyName::G_9,
        LogiLed::KeyName::G_LOGO,
        LogiLed::KeyName::G_BADGE,
    };
    static inline std::vector<LogiLed::KeyName> rightSide = {
        LogiLed::KeyName::PRINT_SCREEN,
        LogiLed::KeyName::SCROLL_LOCK,
        LogiLed::KeyName::PAUSE_BREAK,
        LogiLed::KeyName::INSERT,
        LogiLed::KeyName::HOME,
        LogiLed::KeyName::PAGE_UP,
        LogiLed::KeyName::NUM_LOCK,
        LogiLed::KeyName::NUM_SLASH,
        LogiLed::KeyName::NUM_ASTERISK,
        LogiLed::KeyName::NUM_MINUS,
        LogiLed::KeyName::KEYBOARD_DELETE,
        LogiLed::KeyName::END,
        LogiLed::KeyName::PAGE_DOWN,
        LogiLed::KeyName::NUM_SEVEN,
        LogiLed::KeyName::NUM_EIGHT,
        LogiLed::KeyName::NUM_NINE,
        LogiLed::KeyName::NUM_PLUS,
        LogiLed::KeyName::NUM_FOUR,
        LogiLed::KeyName::NUM_FIVE,
        LogiLed::KeyName::NUM_SIX,
        LogiLed::KeyName::ARROW_UP,
        LogiLed::KeyName::NUM_ONE,
        LogiLed::KeyName::NUM_TWO,
        LogiLed::KeyName::NUM_THREE,
        LogiLed::KeyName::NUM_ENTER,
        LogiLed::KeyName::ARROW_LEFT,
        LogiLed::KeyName::ARROW_DOWN,
        LogiLed::KeyName::ARROW_RIGHT,
        LogiLed::KeyName::NUM_ZERO,
        LogiLed::KeyName::NUM_PERIOD,
    };
    static inline std::vector<LogiLed::KeyName> FSide = {
        LogiLed::KeyName::F1,
        LogiLed::KeyName::F2,
        LogiLed::KeyName::F3,
        LogiLed::KeyName::F4,
        LogiLed::KeyName::F5,
        LogiLed::KeyName::F6,
        LogiLed::KeyName::F7,
        LogiLed::KeyName::F8,
        LogiLed::KeyName::F9,
        LogiLed::KeyName::F10,
        LogiLed::KeyName::F11,
        LogiLed::KeyName::F12,
        LogiLed::KeyName::PRINT_SCREEN,
        LogiLed::KeyName::SCROLL_LOCK,
        LogiLed::KeyName::PAUSE_BREAK,
    };
    static inline std::vector<LogiLed::KeyName> keysCardiogram = {
        LogiLed::KeyName::CAPS_LOCK,
        LogiLed::KeyName::A,
        LogiLed::KeyName::S,
        LogiLed::KeyName::D,
        LogiLed::KeyName::F,
        LogiLed::KeyName::T,
        LogiLed::KeyName::SIX,
        LogiLed::KeyName::Y,
        LogiLed::KeyName::H,
        LogiLed::KeyName::N,
        LogiLed::KeyName::J,
        LogiLed::KeyName::I,
        LogiLed::KeyName::NINE,
        LogiLed::KeyName::O,
        LogiLed::KeyName::L,
        LogiLed::KeyName::SEMICOLON,
        LogiLed::KeyName::APOSTROPHE,
        LogiLed::KeyName::ENTER,
    };
    static inline std::vector<LogiLed::KeyName> keysCardiogramDead = {
        LogiLed::KeyName::CAPS_LOCK,
        LogiLed::KeyName::A,
        LogiLed::KeyName::S,
        LogiLed::KeyName::D,
        LogiLed::KeyName::F,
        LogiLed::KeyName::G,
        LogiLed::KeyName::H,
        LogiLed::KeyName::J,
        LogiLed::KeyName::K,
        LogiLed::KeyName::L,
        LogiLed::KeyName::SEMICOLON,
        LogiLed::KeyName::APOSTROPHE,
        LogiLed::KeyName::ENTER,
    };
    static inline std::vector<LogiLed::KeyName> keysCardiogramNumpad = {
        LogiLed::KeyName::NUM_FOUR,
        LogiLed::KeyName::NUM_EIGHT,
        LogiLed::KeyName::NUM_SIX,
    };
    static inline std::vector<LogiLed::KeyName> keysCardiogramNumpadDead = {
        LogiLed::KeyName::NUM_FOUR,
        LogiLed::KeyName::NUM_FIVE,
        LogiLed::KeyName::NUM_SIX,
    };

public:
    class Timer
    {
    public:
        Timer()
        {
            start();
        }

        void start()
        {
            m_StartTime = std::chrono::steady_clock::now();
            m_bRunning = true;
        }

        void stop()
        {
            m_EndTime = std::chrono::steady_clock::now();
            m_bRunning = false;
        }

        void reset()
        {
            stop();
            start();
        }

        std::chrono::milliseconds::rep elapsedMS()
        {
            std::chrono::time_point<std::chrono::steady_clock> endTime;
            if (m_bRunning)
                endTime = std::chrono::steady_clock::now();
            else
                endTime = m_EndTime;
            return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_StartTime).count();
        }

        auto operator<=>(auto ms)
        {
            return elapsedMS() <=> static_cast<std::chrono::milliseconds::rep>(ms);
        }

    private:
        std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
        std::chrono::time_point<std::chrono::steady_clock> m_EndTime;
        bool m_bRunning = false;
    };

    static inline auto RGBtoPercent(auto R, auto G, auto B, float dim = 1.0f)
    {
        return std::make_tuple(
            static_cast<uint32_t>(static_cast<float>(R) / 255.0f * 100.0f * dim),
            static_cast<uint32_t>(static_cast<float>(G) / 255.0f * 100.0f * dim),
            static_cast<uint32_t>(static_cast<float>(B) / 255.0f * 100.0f * dim)
        );
    }

    static inline void SetLightingLeftSide(int redPercentage, int greenPercentage, int bluePercentage, bool ignoreCardiogram = false, bool ignoreCardiogramDead = false, bool ignoreFSide = false)
    {
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_PERKEY_RGB);
        for (auto key : leftSide)
        {
            if (ignoreCardiogram && std::any_of(std::begin(keysCardiogram), std::end(keysCardiogram), [key](auto i) { return i == key; }))
                continue;

            if (ignoreCardiogramDead && std::any_of(std::begin(keysCardiogramDead), std::end(keysCardiogramDead), [key](auto i) { return i == key; }))
                continue;

            if (ignoreFSide && std::any_of(std::begin(FSide), std::end(FSide), [key](auto i) { return i == key; }))
                continue;

            LogiLedSetLightingForKeyWithKeyName(key, redPercentage, greenPercentage, bluePercentage);
        }

        LogiLedSetTargetDevice(LOGI_DEVICETYPE_RGB);
        LogiLedSetLighting(redPercentage, greenPercentage, bluePercentage);
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_ALL);
    }

    static inline void SetLightingRightSide(int redPercentage, int greenPercentage, int bluePercentage, bool ignoreCardiogram = false, bool ignoreCardiogramDead = false, bool ignoreFSide = false)
    {
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_PERKEY_RGB);
        for (auto key : rightSide)
        {
            if (ignoreCardiogram && std::any_of(std::begin(keysCardiogramNumpad), std::end(keysCardiogramNumpad), [key](auto i) { return i == key; }))
                continue;

            if (ignoreCardiogramDead && std::any_of(std::begin(keysCardiogramNumpadDead), std::end(keysCardiogramNumpadDead), [key](auto i) { return i == key; }))
                continue;

            if (ignoreFSide && std::any_of(std::begin(FSide), std::end(FSide), [key](auto i) { return i == key; }))
                continue;

            LogiLedSetLightingForKeyWithKeyName(key, redPercentage, greenPercentage, bluePercentage);
        }

        LogiLedSetTargetDevice(LOGI_DEVICETYPE_RGB);
        LogiLedSetLighting(redPercentage, greenPercentage, bluePercentage);
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_ALL);
    }

    static inline void SetLighting(int redPercentage, int greenPercentage, int bluePercentage, bool ignoreCardiogram = false, bool ignoreCardiogramDead = false, bool ignoreFSide = false)
    {
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_PERKEY_RGB);
        SetLightingLeftSide(redPercentage, greenPercentage, bluePercentage, ignoreCardiogram, ignoreCardiogramDead, ignoreFSide);
        for (auto key : rightSide)
        {
            if (ignoreFSide && std::any_of(std::begin(FSide), std::end(FSide), [key](auto i) { return i == key; }))
                continue;

            LogiLedSetLightingForKeyWithKeyName(key, redPercentage, greenPercentage, bluePercentage);
        }

        LogiLedSetTargetDevice(LOGI_DEVICETYPE_RGB);
        LogiLedSetLighting(redPercentage, greenPercentage, bluePercentage);
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_ALL);
    }
    
    static inline void DrawCardiogram(int startRedPercentage, int startGreenPercentage, int startBluePercentage, int finishRedPercentage, int finishGreenPercentage, int finishBluePercentage, bool dead = false)
    {
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_PERKEY_RGB);
        auto& arr = dead ? keysCardiogramDead : keysCardiogram;
        static auto TIMERA = Timer();
        static auto counter = 0;
        auto interval = 5 * arr.size();

        if (TIMERA >= interval)
        {
            if (counter >= arr.size())
                counter = 0;
            LogiLedPulseSingleKey(arr[counter], startRedPercentage, startGreenPercentage, startBluePercentage, finishRedPercentage, finishGreenPercentage, finishBluePercentage, interval * 50, false);
            counter++;
            TIMERA.reset();
        }
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_ALL);
    }

    static inline void DrawCardiogramNumpad(int startRedPercentage, int startGreenPercentage, int startBluePercentage, int finishRedPercentage, int finishGreenPercentage, int finishBluePercentage, bool dead = false)
    {
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_PERKEY_RGB);
        auto& arr = dead ? keysCardiogramNumpadDead : keysCardiogramNumpad;
        static auto TIMERB = Timer();
        static auto counter = 0;
        auto interval = 150 * arr.size();

        if (TIMERB >= interval)
        {
            if (counter >= arr.size())
                counter = 0;
            LogiLedPulseSingleKey(arr[counter], startRedPercentage, startGreenPercentage, startBluePercentage, finishRedPercentage, finishGreenPercentage, finishBluePercentage, interval * 5, false);
            counter++;
            TIMERB.reset();
        }
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_ALL);
    }

    static inline void DrawPoliceSirenMouse()
    {
        static bool bColorRed = false;
        static auto TIMERC = Timer();
        static auto TIMERD = Timer();
        if (TIMERC >= 400)
        {
            static constexpr auto redVal = 100;
            static constexpr auto greenVal = 0;
            static constexpr auto blueVal = 0;

            static constexpr auto redFinishVal = 0;
            static constexpr auto greenFinishVal = 0;
            static constexpr auto blueFinishVal = 100;

            LogiLedSetTargetDevice(LOGI_DEVICETYPE_RGB);
            if (bColorRed)
                LogiLedPulseLighting(redVal, greenVal, blueVal, 0, 400);
            else
                LogiLedPulseLighting(redFinishVal, greenFinishVal, blueFinishVal, 0, 400);
            LogiLedSetTargetDevice(LOGI_DEVICETYPE_ALL);

            bColorRed = !bColorRed;
            TIMERC.reset();
        }
    }

    static void DrawPoliceSirenKeyboard(bool withWhiteStrip = false, int duration = 1200, bool isInfinite = true)
    {
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_PERKEY_RGB);
        static auto TIMERD = Timer();

        if (TIMERD >= duration || isInfinite)
        {
            static constexpr auto redVal = 100;
            static constexpr auto greenVal = 0;
            static constexpr auto blueVal = 0;

            static constexpr auto redFinishVal = 0;
            static constexpr auto greenFinishVal = 0;
            static constexpr auto blueFinishVal = 100;

            LogiLedPulseSingleKey(LogiLed::KeyName::F1, redFinishVal, greenFinishVal, blueFinishVal, redVal, greenVal, blueVal, duration, isInfinite);
            LogiLedPulseSingleKey(LogiLed::KeyName::F2, redFinishVal, greenFinishVal, blueFinishVal, redVal, greenVal, blueVal, duration, isInfinite);
            LogiLedPulseSingleKey(LogiLed::KeyName::F3, redFinishVal, greenFinishVal, blueFinishVal, redVal, greenVal, blueVal, duration, isInfinite);
            LogiLedPulseSingleKey(LogiLed::KeyName::F4, redFinishVal, greenFinishVal, blueFinishVal, redVal, greenVal, blueVal, duration, isInfinite);

            if (withWhiteStrip)
            {
                LogiLedSetLightingForKeyWithKeyName(LogiLed::KeyName::F5, 100, 100, 100);
                LogiLedSetLightingForKeyWithKeyName(LogiLed::KeyName::F6, 100, 100, 100);
                LogiLedSetLightingForKeyWithKeyName(LogiLed::KeyName::F7, 100, 100, 100);
                LogiLedSetLightingForKeyWithKeyName(LogiLed::KeyName::F8, 100, 100, 100);
            }
            else
            {
                LogiLedPulseSingleKey(LogiLed::KeyName::F5, redFinishVal, greenFinishVal, blueFinishVal, redVal, greenVal, blueVal, duration, isInfinite);
                LogiLedPulseSingleKey(LogiLed::KeyName::F6, redFinishVal, greenFinishVal, blueFinishVal, redVal, greenVal, blueVal, duration, isInfinite);
                LogiLedPulseSingleKey(LogiLed::KeyName::F7, redVal, greenVal, blueVal, redFinishVal, greenFinishVal, blueFinishVal, duration, isInfinite);
                LogiLedPulseSingleKey(LogiLed::KeyName::F8, redVal, greenVal, blueVal, redFinishVal, greenFinishVal, blueFinishVal, duration, isInfinite);
            }

            LogiLedPulseSingleKey(LogiLed::KeyName::F9,  redVal, greenVal, blueVal, redFinishVal, greenFinishVal, blueFinishVal, duration, isInfinite);
            LogiLedPulseSingleKey(LogiLed::KeyName::F10, redVal, greenVal, blueVal, redFinishVal, greenFinishVal, blueFinishVal, duration, isInfinite);
            LogiLedPulseSingleKey(LogiLed::KeyName::F11, redVal, greenVal, blueVal, redFinishVal, greenFinishVal, blueFinishVal, duration, isInfinite);
            LogiLedPulseSingleKey(LogiLed::KeyName::F12, redVal, greenVal, blueVal, redFinishVal, greenFinishVal, blueFinishVal, duration, isInfinite);

            if (isInfinite)
                TIMERD.stop();
            else
                TIMERD.reset();
        }
        LogiLedSetTargetDevice(LOGI_DEVICETYPE_ALL);
    }
};