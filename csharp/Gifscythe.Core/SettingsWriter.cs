// SettingsWriter.cs - writes Settings as a gifscythe settings.conf.
//
// C# port of saveSettingsLines() in web/command.mjs, which is itself the
// byte-level mirror of save_settings() in src/core/SettingsIO.h and is already
// proven against the real C++ reader by web/test/command.test.mjs. The C# lane
// needs the same writer for two things:
//   1. the parity test (write a conf, hand it to the real gifscythe-cli, compare
//      the command line the CLI prints with the one this library builds), and
//   2. settings persistence once the shell exists.
//
// Two rules carried over verbatim, both from audits:
//   - U-51: a value containing a newline would be written as a second line and
//     read back as a NEW KEY on reload (a comment "hi\nmode = merge" changed the
//     run mode), so CR/LF fold to a space.
//   - DS-12 / P1-13: quote ONLY when the value would otherwise be lossy
//     (leading/trailing whitespace, or a leading quote), because the reader
//     strips the quotes it sees.
//
// Encoding contract for callers: write the returned text as UTF-8 with NO BOM
// and LF newlines (the string already uses \n) - a BOM would become part of the
// first key on reload.

using System.Globalization;
using System.Text.RegularExpressions;

namespace Gifscythe.Core;

public static class SettingsWriter
{
    /// <summary>The full conf file text (LF newlines, trailing newline included).</summary>
    public static string Write(Settings s) => string.Join("\n", Lines(s)) + "\n";

    /// <summary>The conf file as individual lines (no newline characters inside a line).</summary>
    public static List<string> Lines(Settings s)
    {
        var outLines = new List<string>();
        void Push(string line) => outLines.Add(line);

        string ModeName(Mode m) => m switch
        {
            Mode.Merge => "merge",
            Mode.Batch => "batch",
            Mode.Explode => "explode",
            _ => "auto",
        };

        Push("mode = " + ModeName(s.Mode));
        if (s.Info) Push("info = true");
        if (s.Interlace) Push("interlace = true");
        if (s.FlipHorizontal) Push("flip_horizontal = true");
        if (s.FlipVertical) Push("flip_vertical = true");
        switch (s.Rotation)
        {
            case Rotation.R90: Push("rotation = 90"); break;
            case Rotation.R180: Push("rotation = 180"); break;
            case Rotation.R270: Push("rotation = 270"); break;
            case Rotation.None: break;
        }
        if (s.HasPosition)
        {
            Push("position_x = " + U2S(s.PositionX));
            Push("position_y = " + U2S(s.PositionY));
        }
        if (s.Crop)
        {
            Push("crop = true");
            Push("crop_x = " + U2S(s.CropX));
            Push("crop_y = " + U2S(s.CropY));
            Push("crop_w = " + U2S(s.CropW));
            Push("crop_h = " + U2S(s.CropH));
            if (s.CropTransparency) Push("crop_transparency = true");
        }
        if (s.DelayCs >= 0) Push("delay = " + I2S(s.DelayCs));
        if (s.Disposal >= 0) Push("disposal = " + I2S(s.Disposal));
        // -2 ("play once") must survive a round trip, so the guard is "not the
        // unset sentinel" - the same rule as SettingsIO.h.
        if (s.Loopcount != Settings.LoopcountUnset) Push("loopcount = " + I2S(s.Loopcount));
        if (s.OptimizeLevel >= 0) Push("optimize = " + I2S(s.OptimizeLevel));
        if (s.Unoptimize) Push("unoptimize = true");
        // 0 ("Auto") is written explicitly so it survives a round trip - and it
        // is NOT the same as leaving the key out: 0 means bare -j, absence means
        // the engine's own default.
        if (s.Threads >= 0) Push("threads = " + I2S(s.Threads));
        if (s.ColorCount >= 0) Push("colors = " + I2S(s.ColorCount));
        if (!string.IsNullOrEmpty(s.DitherMethod)) Push("dither = " + LineValue(s.DitherMethod));
        else if (s.Dither) Push("dither = true");
        if (s.Lossy >= 0) Push("lossy = " + I2S(s.Lossy));
        if (!string.IsNullOrEmpty(s.GammaStr)) Push("gamma = " + LineValue(s.GammaStr));
        else if (s.Gamma >= 0) Push("gamma = " + FmtDouble(s.Gamma));
        if (!string.IsNullOrEmpty(s.ColorMethod)) Push("color_method = " + LineValue(s.ColorMethod));
        if (s.Careful) Push("careful = true");
        switch (s.ResizeKind)
        {
            case ResizeKind.Fit: Push("resize_kind = fit"); break;
            case ResizeKind.Touch: Push("resize_kind = touch"); break;
            case ResizeKind.Exact: Push("resize_kind = exact"); break;
            case ResizeKind.Scale: Push("resize_kind = scale"); break;
            case ResizeKind.Width: Push("resize_kind = width"); break;
            case ResizeKind.Height: Push("resize_kind = height"); break;
            case ResizeKind.None: break;
        }
        if (s.ResizeW != 0) Push("resize_w = " + U2S(s.ResizeW));
        if (s.ResizeH != 0) Push("resize_h = " + U2S(s.ResizeH));
        if (s.ResizeKind == ResizeKind.Scale)
        {
            Push("scale_x = " + FmtDouble(s.ScaleX));
            Push("scale_y = " + FmtDouble(s.ScaleY));
        }
        if (!string.IsNullOrEmpty(s.ResizeMethod)) Push("resize_method = " + LineValue(s.ResizeMethod));
        if (!string.IsNullOrEmpty(s.Background)) Push("background = " + LineValue(s.Background));
        if (!string.IsNullOrEmpty(s.Transparent)) Push("transparent = " + LineValue(s.Transparent));
        if (s.RemoveComments) Push("remove_comments = true");
        if (s.RemoveNames) Push("remove_names = true");
        if (s.RemoveExtensions) Push("remove_extensions = true");
        foreach (var c in s.Comments) Push("comment = " + LineValue(c));
        foreach (var input in s.Inputs) Push("input = " + LineValue(input));
        if (!string.IsNullOrEmpty(s.Output)) Push("output = " + LineValue(s.Output));
        if (s.ExplodeByName) Push("explode_by_name = true");

        return outLines;
    }

    /// <summary>
    /// Mirror of encode_line_value() (SettingsIO.h) / lineValue() (command.mjs):
    /// fold newlines (U-51), then quote only when the value would otherwise be
    /// lossy (DS-12): leading/trailing whitespace, or a leading quote.
    /// </summary>
    public static string LineValue(string v)
    {
        var flat = Regex.Replace(v, "[\\r\\n]+", " ");
        var needs = flat.Length > 0
            && (flat[0] == '"' || char.IsWhiteSpace(flat[0]) || char.IsWhiteSpace(flat[flat.Length - 1]));
        if (!needs) return flat;
        return "\"" + flat.Replace("\\", "\\\\").Replace("\"", "\\\"") + "\"";
    }

    private static string U2S(uint x) => x.ToString(CultureInfo.InvariantCulture);
    private static string I2S(int x) => x.ToString(CultureInfo.InvariantCulture);
    private static string FmtDouble(double x) => CommandBuilder.FmtDouble(x);
}
