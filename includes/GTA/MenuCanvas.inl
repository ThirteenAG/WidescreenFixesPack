// Included by the current Draw modules after CDraw is defined.
export namespace MenuCanvas
{
    inline std::optional<float> Constraint;
    inline bool Enabled = false;
    inline bool Drawing = false;
    inline int PhysicalWidth = 0;
    inline int PhysicalMaximumWidth = 0;
    inline int Depth = 0;
    inline int Suspensions = 0;
    inline float Offset = 0.0f;

    __declspec(noinline) ResChange<int, int>& onLayoutChange()
    {
        static ResChange<int, int> event;
        return event;
    }

    int GetWidth(int width, int height)
    {
        if (!Enabled || !Constraint || height <= 0)
            return width;
        const float value = *Constraint;
        // Match the HUD option's ratio and legacy pixel-offset forms.
        const float result = value < 0.0f || value > 32.0f / 9.0f
            ? width + value * 2.0f
            : height * ClampHudAspectRatio(value, float(width) / height);
        const int minimum = std::min(width, int(std::lround(height * (4.0f / 3.0f))));
        int canvasWidth = int(std::lround(std::clamp(result, float(minimum), float(width))));
        // Native mouse positions are integers. Keep the inset integral too.
        if ((width - canvasWidth) & 1) ++canvasWidth;
        return canvasWidth;
    }

    // Frontend scaling may use a narrower canvas; 3D FOV always uses CDraw.
    float GetCurrentAspectRatio()
    {
        return fMenuAspectRatio != 0.0f ? fMenuAspectRatio : CDraw::GetAspectRatio();
    }

    float GetAspectRatio()
    {
        const int width = Depth && !Suspensions ? PhysicalWidth : RsGlobal->width;
        return float(GetWidth(width, RsGlobal->height)) / RsGlobal->height;
    }

    int GetMouseX(int x)
    {
        // Input remains relative to the menu canvas, including the empty space
        // beside it. Only the physical screen edges limit cursor movement.
        const int inset = int(Offset);
        return std::clamp(x, -inset, PhysicalWidth - inset);
    }

    void Apply()
    {
        PhysicalWidth = RsGlobal->width;
        PhysicalMaximumWidth = RsGlobal->maximumWidth;
        const int width = GetWidth(PhysicalWidth, RsGlobal->height);
        Offset = float(PhysicalWidth - width) * 0.5f;
        RsGlobal->width = width;
        RsGlobal->maximumWidth = width;
        fMenuAspectRatio = float(width) / RsGlobal->height;
        onLayoutChange().executeAll(width, RsGlobal->height);
    }

    void Restore()
    {
        RsGlobal->width = PhysicalWidth;
        RsGlobal->maximumWidth = PhysicalMaximumWidth;
        fMenuAspectRatio = 0.0f;
        onLayoutChange().executeAll(RsGlobal->width, RsGlobal->height);
    }

    class Scope
    {
        bool active;
        bool previousDrawing;
    public:
        explicit Scope(bool drawing = false, bool enabled = true) : active(Enabled && enabled), previousDrawing(Drawing)
        {
            if (!active) return;
            if (!Depth++) Apply();
            Drawing = drawing || Drawing;
        }
        ~Scope()
        {
            if (!active) return;
            Drawing = previousDrawing;
            if (!--Depth) Restore();
        }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    };

    // Backgrounds, OS cursor warps and renderer resets use the real viewport.
    // Apply again on return so a newly selected video mode is retained.
    class Suspend
    {
        bool active;
    public:
        Suspend() : active(Depth != 0)
        {
            if (active && !Suspensions++) Restore();
        }
        ~Suspend()
        {
            if (active && !--Suspensions) Apply();
        }
        Suspend(const Suspend&) = delete;
        Suspend& operator=(const Suspend&) = delete;
    };
}
