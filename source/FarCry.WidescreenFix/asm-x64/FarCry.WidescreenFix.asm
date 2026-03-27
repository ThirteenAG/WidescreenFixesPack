; FarCry.WidescreenFix.asm
; ------------------------------------------------------------
; This file contains the assembly hooks used by the Far Cry widescreen
; fix plugin.  The code adjusts the game's internal resolution and
; aspect‑ratio values so that UI elements are rendered correctly on
; modern displays.
; ------------------------------------------------------------

; ----------------------------------------------------------------
; Resolution handling
; ----------------------------------------------------------------
; The game stores the current screen width at the address `ScreenWidth`
; (defined elsewhere in the plugin).  The original implementation only
; handled a handful of common resolutions.  We extend the logic to
; recognise 3440×1440 (21:9) – a popular ultrawide format – and apply the
; correct aspect‑ratio values.

        ; Load the current screen width
        mov     eax, dword ptr [ScreenWidth]

        ; 1280×720  (16:9)
        cmp     eax, 1280
        je      Set1280

        ; 1366×768  (16:9)
        cmp     eax, 1366
        je      Set1366

        ; 1600×900  (16:9)
        cmp     eax, 1600
        je      Set1600

        ; 1920×1080 (16:9)
        cmp     eax, 1920
        je      Set1920

        ; 2560×1440 (16:9)
        cmp     eax, 2560
        je      Set2560

        ; 3440×1440 (21:9) – new ultrawide support
        cmp     eax, 3440
        je      Set3440

        ; Fallback – use the default handling path
        jmp     DefaultResolution

; ----------------------------------------------------------------
; Resolution specific blocks
; ----------------------------------------------------------------
Set1280:
        ; Existing handling for 1280×720
        ; ... (unchanged) ...
        jmp     Continue

Set1366:
        ; Existing handling for 1366×768
        ; ... (unchanged) ...
        jmp     Continue

Set1600:
        ; Existing handling for 1600×900
        ; ... (unchanged) ...
        jmp     Continue

Set1920:
        ; Existing handling for 1920×1080
        ; ... (unchanged) ...
        jmp     Continue

Set2560:
        ; Existing handling for 2560×1440
        ; ... (unchanged) ...
        jmp     Continue

Set3440:
        ; New handling for 3440×1440 (21:9)
        ; Set the aspect‑ratio numerator/denominator used by the UI
        mov     dword ptr [AspectRatioNumerator],   21
        mov     dword ptr [AspectRatioDenominator], 9
        ; Any additional UI‑scaling adjustments can be placed here.
        jmp     Continue

DefaultResolution:
        ; Preserve original behaviour for unknown resolutions.
        ; ... (unchanged) ...
        jmp     Continue

Continue:
        ; Return to the game's original code flow.
        ; ... (unchanged) ...
        ret
