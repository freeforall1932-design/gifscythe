// CommandBuilder.cs - builds a gifsicle command line from Settings.
//
// C# port of working_code/gifscythe/src/core/GifsicleCommand.h, which is the
// authority: this file must answer the SAME argv for the SAME settings, and the
// parity test in csharp/Gifscythe.Core.Tests checks that against the real C++
// CLI (`gifscythe-cli <conf> --engine <path>`, print mode) instead of trusting
// the transcription.
//
// Two outputs, exactly like the C++ class:
//   1. Build()         - argv list, ready to exec a gifsicle subprocess (NEVER a shell)
//   2. ToCommandLine() - shell-quoted display string for the "show me the command" pane
//
// Every range check, every sentinel branch and every "emit nothing" case below
// carries the audit id it came from, copied from the C++ source, so a port-time
// regression is traceable to the finding that created the rule.

using System.Globalization;

namespace Gifscythe.Core;

public static class CommandBuilder
{
    /// <summary>Build the argv vector (without the program name).</summary>
    public static List<string> Build(Settings s)
    {
        var args = new List<string>();
        void Add(string? x)
        {
            if (!string.IsNullOrEmpty(x)) args.Add(x);
        }

        // Mode (must come before filenames).
        // Batch emits the engine's in-place -b; the CLI --run path refuses that
        // combination when `output` is empty (GS-201 / P0-5). The builder itself
        // stays a faithful mapping so print mode still shows what would have run.
        switch (s.Mode)
        {
            case Mode.Merge: Add("-m"); break;
            case Mode.Batch: Add("-b"); break;
            case Mode.Explode: Add(s.ExplodeByName ? "-E" : "-e"); break;
            case Mode.Auto: break;
        }

        // General
        if (s.Info) Add("--info");

        // Whole-GIF
        if (s.Careful) Add("--careful");
        if (s.ColorCount >= 2 && s.ColorCount <= 256)
        {
            Add("-k");
            Add(s.ColorCount.ToString(CultureInfo.InvariantCulture));
        }
        // Dither: prefer the explicit method string; fall back to bare -f.
        if (!string.IsNullOrEmpty(s.DitherMethod))
        {
            if (s.DitherMethod != "none") Add("--dither=" + s.DitherMethod);
        }
        else if (s.Dither)
        {
            Add("-f");
        }
        if (s.Lossy >= 0 && s.Lossy <= 200)
        {
            // --lossy takes an OPTIONAL value; gifsicle wants it attached (=N).
            Add("--lossy=" + s.Lossy.ToString(CultureInfo.InvariantCulture));
        }
        // Gamma: prefer the string form (srgb | oklab | NUM); legacy double as fallback.
        if (!string.IsNullOrEmpty(s.GammaStr))
        {
            Add("--gamma=" + s.GammaStr);
        }
        else if (s.Gamma >= 0)
        {
            Add("--gamma=" + FmtDouble(s.Gamma));
        }
        if (!string.IsNullOrEmpty(s.ColorMethod))
        {
            Add("--color-method");
            Add(s.ColorMethod);
        }

        // Resize / scale
        switch (s.ResizeKind)
        {
            case ResizeKind.Fit:
                Add("--resize-fit");
                Add(U2S(s.ResizeW) + "x" + U2S(s.ResizeH));
                break;
            case ResizeKind.Touch:
                Add("--resize-touch");
                Add(U2S(s.ResizeW) + "x" + U2S(s.ResizeH));
                break;
            case ResizeKind.Exact:
                Add("--resize");
                Add(U2S(s.ResizeW) + "x" + U2S(s.ResizeH));
                break;
            case ResizeKind.Scale:
                Add("--scale");
                Add(FmtDouble(s.ScaleX) + "x" + FmtDouble(s.ScaleY));
                break;
            case ResizeKind.Width:
                Add("--resize-width");
                Add(U2S(s.ResizeW));
                break;
            case ResizeKind.Height:
                Add("--resize-height");
                Add(U2S(s.ResizeH));
                break;
            case ResizeKind.None:
                break;
        }
        if (!string.IsNullOrEmpty(s.ResizeMethod))
        {
            Add("--resize-method");
            Add(s.ResizeMethod);
        }

        // Frame / image options
        if (s.Interlace) Add("-i");
        if (s.FlipHorizontal) Add("--flip-horizontal");
        if (s.FlipVertical) Add("--flip-vertical");
        switch (s.Rotation)
        {
            case Rotation.R90: Add("--rotate-90"); break;
            case Rotation.R180: Add("--rotate-180"); break;
            case Rotation.R270: Add("--rotate-270"); break;
            case Rotation.None: break;
        }
        if (s.HasPosition)
        {
            Add("-p");
            Add(U2S(s.PositionX) + "," + U2S(s.PositionY));
        }

        // Crop - gifsicle wants X,Y+WIDTHxHEIGHT (plus form), NOT a comma before the size.
        if (s.Crop)
        {
            Add("--crop");
            Add(U2S(s.CropX) + "," + U2S(s.CropY) + "+" + U2S(s.CropW) + "x" + U2S(s.CropH));
            if (s.CropTransparency) Add("--crop-transparency");
        }

        // Background / transparency
        if (!string.IsNullOrEmpty(s.Background)) { Add("--background"); Add(s.Background); }
        if (!string.IsNullOrEmpty(s.Transparent)) { Add("--transparent"); Add(s.Transparent); }

        // Comments / names / extensions
        if (s.RemoveComments) Add("--no-comments");
        if (s.RemoveNames) Add("--no-names");
        if (s.RemoveExtensions) Add("--no-extensions");
        foreach (var c in s.Comments)
        {
            // An empty comment must not be emitted at all (audit U-13/U-48): it
            // used to push `--comment` with NO operand, which then swallowed the
            // next argument as its text.
            if (string.IsNullOrEmpty(c)) continue;
            Add("--comment");
            Add(c);
        }

        // Animation options (delay_cs is in 1/100 s, NOT milliseconds).
        if (s.DelayCs >= 0) { Add("-d"); Add(I2S(s.DelayCs)); }
        if (s.Disposal >= 0 && s.Disposal <= 7) { Add("--disposal"); Add(I2S(s.Disposal)); }
        // Four states (U-63 / P1-40). "Play once" is the ABSENT loop extension,
        // not a count of 1 (gifsicle.1: "--no-loopcount (the default) turns off
        // looping").
        if (s.Loopcount == Settings.LoopcountOnce)
        {
            Add("--no-loopcount");
        }
        else if (s.Loopcount == Settings.LoopcountForever)
        {
            Add("--loopcount=0");   // equivalent to forever (man page confirmed)
        }
        else if (s.Loopcount > 0)
        {
            Add("--loopcount=" + I2S(s.Loopcount));
        }
        if (s.OptimizeLevel >= 0 && s.OptimizeLevel <= 3)
        {
            // -O0 is valid ("off"); bare -O means 1.
            Add(OptimizationOpt(s.OptimizeLevel));
        }
        if (s.Unoptimize) Add("-U");
        // Threads: a TRI-state, because the engine has three distinct states
        // (audit U-03, then DS-06 / fix-order P0-2):
        //   < 0 -> emit NOTHING (the engine's own default is single-threaded)
        //   == 0 -> bare -j (GIFSICLE_DEFAULT_THREAD_COUNT = 8) - this is "Auto"
        //   > 0 -> -jN
        if (s.Threads > 0) Add("-j" + I2S(s.Threads));
        else if (s.Threads == Settings.ThreadsAuto) Add("-j");

        // Inputs
        foreach (var input in s.Inputs) Add(input);

        // Output: -o FILE
        if (!string.IsNullOrEmpty(s.Output)) { Add("-o"); Add(s.Output); }

        return args;
    }

    /// <summary>Shell-quoted command line string for display (not for system()).</summary>
    public static string ToCommandLine(Settings s) =>
        string.Join(" ", Build(s).Select(ShellQuote));

    /// <summary>
    /// Quote a single argument for safe display / copy-paste into a POSIX shell.
    /// Used ONLY for display - execution always goes through argv (no shell).
    /// </summary>
    public static string ShellQuote(string a)
    {
        if (a.Length == 0) return "''";
        var safe = true;
        foreach (var ch in a)
        {
            var c = (int)ch;
            var ok = (c >= 48 && c <= 57) ||   // 0-9
                     (c >= 65 && c <= 90) ||   // A-Z
                     (c >= 97 && c <= 122) ||  // a-z
                     "/._-+=:@%,".IndexOf(ch) >= 0;
            if (!ok) { safe = false; break; }
        }
        if (safe) return a;
        var outStr = "'";
        foreach (var ch in a)
        {
            if (ch == '\'') outStr += "'\\''";
            else outStr += ch;
        }
        return outStr + "'";
    }

    /// <summary>-O0 is valid ("no optimization"); -O without a value means 1.</summary>
    public static string OptimizationOpt(int level) =>
        level == 1 ? "-O" : "-O" + I2S(level);

    private static string U2S(uint x) => x.ToString(CultureInfo.InvariantCulture);
    private static string I2S(int x) => x.ToString(CultureInfo.InvariantCulture);

    /// <summary>
    /// Mirror of the C++ `f2s` (ostringstream with precision(10)) for the plain
    /// decimal values this layer actually produces (scale factors, gamma). The
    /// invariant culture is mandatory: a de-DE host would otherwise emit "0,5"
    /// and hand gifsicle a different argument.
    /// </summary>
    public static string FmtDouble(double x) =>
        x.ToString("G10", CultureInfo.InvariantCulture);
}
