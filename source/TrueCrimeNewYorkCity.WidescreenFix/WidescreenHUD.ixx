module;

#include <stdafx.h>

export module WidescreenHUD;

import ComVars;

//#define DEBUGHUD

class HUDIdentifier
{
public:
    uint32_t values[6];

    HUDIdentifier(float* input1)
    {
        for (int i = 0; i < 6; ++i)
        {
            values[i] = *(uint32_t*)&input1[i + 1];
        }
    }

    HUDIdentifier(std::initializer_list<uint32_t> list)
    {
        int i = 0;
        for (auto val : list)
        {
            if (i < 6) values[i++] = val;
        }
    }

    bool operator==(const HUDIdentifier& other) const
    {
        for (int i = 0; i < 6; ++i)
        {
            float a = *(float*)&values[i];
            float b = *(float*)&other.values[i];
            if (fabs(a - b) >= 0.001f)
                return false;
        }
        return true;
    }

    bool operator!=(const HUDIdentifier& other) const
    {
        return !(*this == other);
    }

    std::string toString() const
    {
        std::stringstream ss;
        ss << "HUDIdentifier {0x" << std::hex << std::uppercase
            << values[0] << ", 0x" << values[1] << ", 0x" << values[2] << ", 0x"
            << values[3] << ", 0x" << values[4] << ", 0x" << values[5] << "};";
        return ss.str();
    }
};

HUDIdentifier Silhouette{ 0x3F57999A, 0x3F2ADB6E, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 };
HUDIdentifier SilhouetteBars{ 0x3F660001, 0x3F32DB6F, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 };
HUDIdentifier Badge{ 0x3F473334, 0x3F364925, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 };
HUDIdentifier WeaponIcon{ 0x3ED80001, 0x3CB6DB6F, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 };
HUDIdentifier WeaponAmmoLeft{ 0x3F380001, 0x3D8DB6DC, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 };
HUDIdentifier WeaponAmmoRight{ 0x3F640001, 0x3D8DB6DC, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 };
HUDIdentifier Phone{ 0xBDCCCCCE, 0x3D924925, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000 };

export void __cdecl WidescreenHUD(float* input1, float* input2, int alignmentMode, float* output)
{
    if (!input1 || !input2 || !output) return;

    HUDIdentifier current(input1);

    #ifdef DEBUGHUD
    std::string logString = current.toString();

    static std::vector<std::string> processedElements;
    static int currentElementIndex = 0;
    static std::string currentSelectedElement = "";

    // Add to processed elements if not already present
    bool alreadyExists = false;
    for (const auto& elem : processedElements)
    {
        if (elem == logString)
        {
            alreadyExists = true;
            break;
        }
    }
    if (!alreadyExists)
    {
        processedElements.push_back(logString);
    }
    #endif

    // Scale factors
    output[5] = input1[5] * input2[5];  // Width scale
    output[6] = input1[6] * input2[6];  // Height scale
    output[3] = input2[3] * output[5];  // Scaled width component
    output[4] = input2[4] * output[6];  // Scaled height component
    output[0] = input1[0] * input2[0];  // Base scale

    float xPos;
    switch (alignmentMode)
    {
        case 0: case 3: case 6:  // Left/top alignment
            xPos = (input2[1] * output[5]) + input1[1];
            break;
        case 1: case 4: case 7:  // Center alignment
            xPos = ((input1[3] - output[3]) * 0.5f) + (input2[1] * output[5]) + input1[1];
            break;
        case 2: case 5: case 8:  // Right/bottom alignment
            xPos = (input1[3] + input1[1]) - output[3] + (input2[1] * output[5]);
            break;
        default:
            return;
    }
    output[1] = xPos + Screen.fHudOffset;  // Apply HUD offset

    if (current == Silhouette || current == SilhouetteBars || current == Badge || current == WeaponIcon || current == WeaponAmmoLeft || current == WeaponAmmoRight)
        output[1] += Screen.fWidescreenHudOffset;
    else if (current == Phone)
        output[1] -= Screen.fWidescreenHudOffsetPhone;
    #ifdef DEBUGHUD
    else
    {
        static bool wasAddDown = false;
        bool isAddDown = GetAsyncKeyState(VK_ADD) & 0x8000;
        if (wasAddDown && !isAddDown)
        {
            if (!processedElements.empty())
            {
                currentElementIndex = (currentElementIndex + 1) % processedElements.size();
                currentSelectedElement = processedElements[currentElementIndex];
                spd::log()->info("Current element: {}", currentSelectedElement);
            }
        }
        wasAddDown = isAddDown;

        static bool wasSubtractDown = false;
        bool isSubtractDown = GetAsyncKeyState(VK_SUBTRACT) & 0x8000;
        if (wasSubtractDown && !isSubtractDown)
        {
            if (!processedElements.empty())
            {
                currentElementIndex = (currentElementIndex - 1 + processedElements.size()) % processedElements.size();
                currentSelectedElement = processedElements[currentElementIndex];
                spd::log()->info("Current element: {}", currentSelectedElement);
            }
        }
        wasSubtractDown = isSubtractDown;

        // Apply offset to the currently selected element
        if (!processedElements.empty() && logString == processedElements[currentElementIndex])
        {
            output[1] += Screen.fHudOffset;
        }
    }
    #endif

    switch (alignmentMode)
    {
        case 0: case 1: case 2:  // Top alignment
            output[2] = (input2[2] * output[6]) + input1[2];
            break;
        case 3: case 4: case 5:  // Middle alignment
            output[2] = ((input1[4] - output[4]) * 0.5f) + (input2[2] * output[6]) + input1[2];
            break;
        case 6: case 7: case 8:  // Bottom alignment
            output[2] = (input1[4] + input1[2]) - output[4] + (input2[2] * output[6]);
            break;
    }
}