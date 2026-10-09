// Settings.cs - structured settings that mirror the Gifscythe UI controls.
//
// This is the C# port of working_code/gifscythe/src/core/GifsicleSettings.h.
// Field names, sentinels and defaults are deliberately IDENTICAL to the C++
// struct so the two implementations, the JS mirror (web/command.mjs) and the
// conf file format stay one contract instead of three dialects. The C++ header
// says it best: keeping "what the user set" separate from gifsicle's options is
// what lets one builder feed the live CLI pane, the argv executor and the UI.
//
// Sentinels (do not re-derive these from comments elsewhere - quote them):
//   threads    : ThreadsUnset (-1) = emit nothing; ThreadsAuto (0) = bare -j; >0 = -jN
//   loopcount  : LoopcountUnset (-1) = emit nothing; LoopcountOnce (-2) =
//                --no-loopcount ("play once" is the ABSENT loop extension, not
//                a count of 1); LoopcountForever (0) = --loopcount=0; 1..65535 = count
//   delay_cs   : -1 = unchanged. Unit is 1/100 s (gifsicle -d units), NOT ms.
//   disposal   : -1 = unchanged, 0..7 valid.
//   optimize   : -1 = unchanged, 0..3 valid (0 = off, 1 = bare -O).
//   color_count: -1 = unchanged, 2..256 valid.
//   lossy      : -1 = unchanged, 0..200 valid.
//   gamma      : GammaStr "" = unchanged; Gamma < 0 = unchanged.

namespace Gifscythe.Core;

/// <summary>Which top-level gifsicle mode to run (at most one).</summary>
public enum Mode
{
    Auto,     // no explicit mode; gifsicle decides (merge by default)
    Merge,    // -m
    Batch,    // -b
    Explode,  // -e / -E
}

public enum ResizeKind
{
    None,
    Fit,      // --resize-fit WxH
    Touch,    // --resize-touch WxH
    Exact,    // --resize WxH
    Scale,    // --scale XxY
    Width,    // --resize-width W
    Height,   // --resize-height H
}

public enum Rotation
{
    None,
    R90,
    R180,
    R270,
}

public sealed class Settings
{
    // ---- Sentinels: one home, quoted by every caller ----
    public const int ThreadsUnset = -1;   // emit no -j (engine default: 1 thread)
    public const int ThreadsAuto = 0;     // emit bare -j (engine default count = 8)

    public const int LoopcountUnset = -1;   // emit no loop option
    public const int LoopcountOnce = -2;    // emit --no-loopcount (play once)
    public const int LoopcountForever = 0;  // emit --loopcount=0
    // The Netscape extension stores the count in 16 bits. Measured on the
    // bundled 1.96: --loopcount=65536 exits 0 and silently means "forever".
    public const int LoopcountMax = 65535;

    // ---- Mode / general ----
    public Mode Mode { get; set; } = Mode.Auto;
    public bool Info { get; set; }

    // ---- Frame / image ----
    public bool Interlace { get; set; }
    public bool FlipHorizontal { get; set; }
    public bool FlipVertical { get; set; }
    public Rotation Rotation { get; set; } = Rotation.None;
    public uint PositionX { get; set; }
    public uint PositionY { get; set; }
    public bool HasPosition { get; set; }

    // Crop
    public bool Crop { get; set; }
    public uint CropX { get; set; }
    public uint CropY { get; set; }
    public uint CropW { get; set; }
    public uint CropH { get; set; }
    public bool CropTransparency { get; set; }

    // ---- Animation ----
    public int DelayCs { get; set; } = -1;
    public int Disposal { get; set; } = -1;
    public int Loopcount { get; set; } = LoopcountUnset;
    public int OptimizeLevel { get; set; } = -1;
    public bool Unoptimize { get; set; }
    public int Threads { get; set; } = ThreadsUnset;

    // ---- Whole-GIF ----
    public int ColorCount { get; set; } = -1;
    public bool Dither { get; set; }
    public string DitherMethod { get; set; } = "";
    public int Lossy { get; set; } = -1;
    public string GammaStr { get; set; } = "";
    public double Gamma { get; set; } = -1.0;
    public string ColorMethod { get; set; } = "";
    public bool Careful { get; set; }

    // ---- Resize ----
    public ResizeKind ResizeKind { get; set; } = ResizeKind.None;
    public uint ResizeW { get; set; }
    public uint ResizeH { get; set; }
    public double ScaleX { get; set; } = 1.0;
    public double ScaleY { get; set; } = 1.0;
    public string ResizeMethod { get; set; } = "";

    // ---- Transparency / background ----
    public string Background { get; set; } = "";
    public string Transparent { get; set; } = "";

    // ---- Comments / names / extensions ----
    public bool RemoveComments { get; set; }
    public bool RemoveNames { get; set; }
    public bool RemoveExtensions { get; set; }
    public List<string> Comments { get; } = new();

    // ---- Inputs / outputs ----
    public List<string> Inputs { get; } = new();
    public string Output { get; set; } = "";
    // -E. Only observable when the input already carries frame-name extensions.
    public bool ExplodeByName { get; set; }
}
