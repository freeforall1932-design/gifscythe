// Program.cs - Gifscythe.Core's check runner (house style: a CHECK counter and
// a non-zero exit, like working_code/gifscythe/tests/test_gifsicle_command.cpp).
//
// Lane 1 (unit) always runs. It mirrors the C++ suite's own assertions - the
// audit ids in the comments are the ones the C++ source carries - so the port is
// checked against the rules, not against my reading of them.
//
// Lane 2 (parity) hands the same settings to the REAL C++ CLI in print mode and
// compares the argv tokens behind the command line the CLI prints with the argv
// this library builds.
// That is the check that catches a transcription error, and it is the reason the
// port exists as a library instead of a copy-paste into a window. It needs a
// built `gifscythe-cli`:
//
//    GS_CLI=<path to gifscythe-cli>            (required for lane 2)
//    GS_PARITY_ENGINE=<engine token>           (optional; default /opt/gifsicle)
//    GS_REQUIRE_PROOF=1                        (optional; no CLI => FAILURE, not a skip)
//
// Run:  dotnet run --project csharp/Gifscythe.Core.Tests -c Release

using System.Diagnostics;
using Gifscythe.Core;

internal static class Program
{
    private static int _checks;
    private static int _failures;
    private static int _skips;

    private static int Main()
    {
        Console.WriteLine("== Gifscythe.Core checks ==");
        UnitLane();
        ParityLane();
        Console.WriteLine($"== result: {_checks} checks, {_failures} failures, {_skips} skipped ==");
        return _failures == 0 ? 0 : 1;
    }

    // ---------------------------------------------------------------- helpers

    private static void Check(bool ok, string what)
    {
        _checks++;
        if (ok) return;
        _failures++;
        Console.Error.WriteLine("FAIL: " + what);
    }

    private static void Skip(string what)
    {
        _skips++;
        Console.WriteLine("SKIP: " + what);
    }

    private static bool Has(List<string> args, string a) => args.Contains(a);

    private static bool HasSeq(List<string> args, string a, string b)
    {
        for (var i = 0; i + 1 < args.Count; i++)
            if (args[i] == a && args[i + 1] == b) return true;
        return false;
    }

    private static string Join(IEnumerable<string> a) => string.Join(" ", a);

    private static Settings WithInput(string input)
    {
        var s = new Settings();
        s.Inputs.Add(input);
        return s;
    }

    // ------------------------------------------------------------- unit lane

    private static void UnitLane()
    {
        // 1. Simple merge/optimize command (C++ case 1).
        {
            var s = new Settings { Mode = Mode.Merge, OptimizeLevel = 3, Loopcount = 0, DelayCs = 5, Output = "out.gif" };
            s.Inputs.Add("a.gif");
            s.Inputs.Add("b.gif");
            var args = CommandBuilder.Build(s);
            Check(Has(args, "-m"), "merge emits -m");
            Check(Has(args, "a.gif") && Has(args, "b.gif"), "both inputs are emitted");
            Check(HasSeq(args, "-o", "out.gif"), "output is emitted as -o FILE");
            Check(Has(args, "-O3"), "optimize level 3 emits -O3");
            Check(Has(args, "--loopcount=0"), "loopcount 0 emits --loopcount=0 (forever)");
            Check(HasSeq(args, "-d", "5"), "delay 5 emits -d 5 (1/100 s, NOT ms)");
        }

        // 2. Resize-fit + colors + lossy + bare dither (C++ case 2).
        {
            var s = new Settings
            {
                ResizeKind = ResizeKind.Fit, ResizeW = 320, ResizeH = 200,
                ColorCount = 128, Lossy = 60, Dither = true,
            };
            s.Inputs.Add("a.gif");
            var args = CommandBuilder.Build(s);
            Check(Has(args, "--resize-fit"), "resize-fit emits --resize-fit");
            Check(Has(args, "320x200"), "resize-fit emits WxH as one token");
            Check(HasSeq(args, "-k", "128"), "colors emit -k N");
            Check(Has(args, "--lossy=60"), "lossy emits --lossy=N (attached value)");
            Check(Has(args, "-f"), "bool dither with no method emits bare -f");
        }

        // 3. Explode mode + rotate + flip (C++ case 3).
        {
            var s = new Settings { Mode = Mode.Explode, ExplodeByName = true, Rotation = Rotation.R90, FlipHorizontal = true };
            s.Inputs.Add("a.gif");
            var args = CommandBuilder.Build(s);
            Check(Has(args, "-E"), "explode_by_name emits -E (not -e)");
            Check(Has(args, "--rotate-90"), "rotation 90 emits --rotate-90");
            Check(Has(args, "--flip-horizontal"), "horizontal flip emits --flip-horizontal");
        }

        // 4. Defaults: the input must be there (not tautologically empty).
        {
            var args = CommandBuilder.Build(WithInput("a.gif"));
            Check(args.Count == 1 && args[0] == "a.gif", "defaults emit exactly the input");
        }

        // 5. Crop plus-form + transparency + background/transparent (C++ case 5).
        {
            var s = new Settings
            {
                Crop = true, CropX = 0, CropY = 0, CropW = 30, CropH = 60,
                CropTransparency = true, Background = "#ffffff", Transparent = "#000000",
            };
            s.Inputs.Add("a.gif");
            var args = CommandBuilder.Build(s);
            Check(HasSeq(args, "--crop", "0,0+30x60"), "crop uses X,Y+WxH (plus form)");
            Check(Has(args, "--crop-transparency"), "crop transparency flag is emitted");
            Check(HasSeq(args, "--background", "#ffffff"), "background colour is emitted");
            Check(HasSeq(args, "--transparent", "#000000"), "transparent colour is emitted");
        }

        // 6. Threads tri-state (audit U-03, then DS-06 / P0-2).
        {
            Check(WithInput("a.gif").Threads == Settings.ThreadsUnset, "threads default is the unset sentinel");
            var unset = WithInput("a.gif");
            var ua = CommandBuilder.Build(unset);
            Check(!Has(ua, "-j") && !Has(ua, "-j1"), "threads unset says NOTHING (engine default)");
            Check(Join(ua) == "a.gif", "threads unset emits no thread flag at all");

            var auto = WithInput("a.gif");
            auto.Threads = Settings.ThreadsAuto;
            var aa = CommandBuilder.Build(auto);
            Check(Has(aa, "-j") && !Has(aa, "-j0"), "threads 0 (Auto) emits a BARE -j");
            Check(Join(aa) == "-j a.gif", "auto threads emits bare -j before the inputs");

            var four = WithInput("a.gif");
            four.Threads = 4;
            var a4 = CommandBuilder.Build(four);
            Check(Has(a4, "-j4") && !Has(a4, "-j"), "threads 4 emits -j4 and NOT a bare -j");

            var one = WithInput("a.gif");
            one.Threads = 1;
            Check(Has(CommandBuilder.Build(one), "-j1"), "single-threaded stays expressible (-j1)");

            var neg = WithInput("a.gif");
            neg.Threads = -7;
            Check(!Has(CommandBuilder.Build(neg), "-j"), "threads below the sentinel is treated as unset by the builder");
        }

        // 7. Loop count is a FOUR-state control (U-63 / P1-40).
        {
            var once = WithInput("a.gif");
            once.Loopcount = Settings.LoopcountOnce;
            var oa = CommandBuilder.Build(once);
            Check(Has(oa, "--no-loopcount"), "loopcount -2 plays once via --no-loopcount");
            Check(Join(oa) == "--no-loopcount a.gif", "play-once emits exactly --no-loopcount + input");

            var forever = WithInput("a.gif");
            forever.Loopcount = Settings.LoopcountForever;
            Check(Has(CommandBuilder.Build(forever), "--loopcount=0"), "loopcount 0 means forever");

            var count = WithInput("a.gif");
            count.Loopcount = 5;
            Check(Has(CommandBuilder.Build(count), "--loopcount=5"), "loopcount N is passed through");

            var unchanged = WithInput("a.gif");
            Check(Join(CommandBuilder.Build(unchanged)) == "a.gif", "loopcount unset says nothing");
        }

        // 8. Optimize level: -O0, bare -O for 1, -O3; out-of-range says nothing.
        {
            var off = WithInput("a.gif");
            off.OptimizeLevel = 0;
            Check(Has(CommandBuilder.Build(off), "-O0"), "-O0 is emitted for level 0 (off)");
            var oneLevel = WithInput("a.gif");
            oneLevel.OptimizeLevel = 1;
            Check(Has(CommandBuilder.Build(oneLevel), "-O"), "level 1 emits bare -O");
            var tooHigh = WithInput("a.gif");
            tooHigh.OptimizeLevel = 4;
            Check(Join(CommandBuilder.Build(tooHigh)) == "a.gif", "out-of-range optimize level says nothing");
        }

        // 9. An empty comment must not emit a bare --comment (audit U-13/U-48).
        {
            var s = WithInput("a.gif");
            s.Comments.Add("");
            s.Comments.Add("real one");
            var args = CommandBuilder.Build(s);
            Check(HasSeq(args, "--comment", "real one"), "a real comment is emitted with its text");
            Check(!HasSeq(args, "--comment", "a.gif"), "an empty comment does not swallow the next argument");
            Check(args.Count(a => a == "--comment") == 1, "exactly one --comment is emitted for one real comment");
        }

        // 10. gifsicle's non-path tokens stay literal (U-60 / U-61).
        {
            var s = new Settings { Output = "-" };
            s.Inputs.Add("in.gif");
            s.Inputs.Add("#0");
            var args = CommandBuilder.Build(s);
            Check(Has(args, "#0"), "a frame selector stays a literal token");
            Check(HasSeq(args, "-o", "-"), "a dash output stays literal (stdout)");
            var stdin = new Settings();
            stdin.Inputs.Add("-");
            Check(Join(CommandBuilder.Build(stdin)) == "-", "a dash input stays literal (stdin)");
        }

        // 11. Dither named / "none" / colour method / careful.
        {
            var named = WithInput("a.gif");
            named.DitherMethod = "ro64";
            Check(Has(CommandBuilder.Build(named), "--dither=ro64"), "a named dither method is attached with =");
            var none = WithInput("a.gif");
            none.DitherMethod = "none";
            Check(Join(CommandBuilder.Build(none)) == "a.gif", "dither method 'none' emits nothing");
            var method = WithInput("a.gif");
            method.ColorMethod = "median-cut";
            Check(HasSeq(CommandBuilder.Build(method), "--color-method", "median-cut"), "colour method is a two-token option");
            var careful = WithInput("a.gif");
            careful.Careful = true;
            Check(Has(CommandBuilder.Build(careful), "--careful"), "careful is emitted");
        }

        // 12. Gamma: the string form wins; the legacy double is the fallback.
        {
            var named = WithInput("a.gif");
            named.GammaStr = "srgb";
            Check(Has(CommandBuilder.Build(named), "--gamma=srgb"), "a named gamma is used verbatim");
            var numeric = WithInput("a.gif");
            numeric.Gamma = 2.2;
            Check(Has(CommandBuilder.Build(numeric), "--gamma=2.2"), "a numeric gamma is formatted invariantly");
            var unset = WithInput("a.gif");
            Check(Join(CommandBuilder.Build(unset)) == "a.gif", "an unset gamma says nothing");
        }

        // 13. Resize: scale is asymmetric-capable (U-42) and width/height are single values.
        {
            var scale = WithInput("a.gif");
            scale.ResizeKind = ResizeKind.Scale;
            scale.ScaleX = 0.5;
            scale.ScaleY = 2;
            Check(HasSeq(CommandBuilder.Build(scale), "--scale", "0.5x2"), "scale keeps X and Y independent");
            var width = WithInput("a.gif");
            width.ResizeKind = ResizeKind.Width;
            width.ResizeW = 320;
            Check(HasSeq(CommandBuilder.Build(width), "--resize-width", "320"), "resize-width takes one value");
            var method = WithInput("a.gif");
            method.ResizeKind = ResizeKind.Fit;
            method.ResizeW = 10;
            method.ResizeH = 20;
            method.ResizeMethod = "lanczos3";
            Check(HasSeq(CommandBuilder.Build(method), "--resize-method", "lanczos3"), "resize method is a two-token option");
        }

        // 14. shell_quote: bare safe tokens stay bare; spaces and quotes are wrapped.
        {
            Check(CommandBuilder.ShellQuote("a.gif") == "a.gif", "a safe token stays bare");
            Check(CommandBuilder.ShellQuote("") == "''", "an empty argument is quoted");
            Check(CommandBuilder.ShellQuote("a b.gif") == "'a b.gif'", "a space forces quoting");
            Check(CommandBuilder.ShellQuote("it's") == "'it'\\''s'", "an embedded quote is escaped POSIX-style");
            Check(CommandBuilder.ShellQuote("C:/x/y.gif") == "C:/x/y.gif", "a Windows path with forward slashes is safe");
        }

        // 15. The exit-code contract (U-91 / P3-17).
        {
            Check(ExitCodes.Ok == 0, "ok is 0");
            Check(ExitCodes.Failure == 1, "failure is 1 (the CLI's engine/path/output code)");
            Check(ExitCodes.UsageError == 2, "usage error is 2 (the CLI's usage/unsafe-target code)");
            Check(ExitCodes.StrictRefusal == 3, "strict refusal is 3 (the CLI's --strict code)");
            var distinct = new HashSet<int>
            {
                ExitCodes.Ok, ExitCodes.Failure, ExitCodes.UsageError, ExitCodes.StrictRefusal,
            };
            Check(distinct.Count == 4, "the four contract codes are distinct");
            Check(ExitCodes.FromFailure(ExitCodes.FailureKind.EngineMissing) == ExitCodes.Failure,
                "engine-missing maps onto the shared contract (no second meaning for 3)");
            Check(ExitCodes.FromFailure(ExitCodes.FailureKind.EngineFailed) == ExitCodes.Failure,
                "engine-failed maps onto the shared contract");
            Check(ExitCodes.FromFailure(ExitCodes.FailureKind.OutputInvalid) == ExitCodes.Failure,
                "invalid output maps onto the shared contract");
        }

        // 16. The settings writer (U-51 newline fold, DS-12 quoting, sentinels).
        {
            var text = SettingsWriter.Write(new Settings());
            Check(text.EndsWith("\n"), "the conf ends with a newline");
            Check(text.StartsWith("mode = auto"), "the mode line comes first and defaults to auto");
            var lines = SettingsWriter.Lines(new Settings());
            Check(!lines.Any(l => l.StartsWith("threads")), "an unset thread count writes no key at all");
            Check(!lines.Any(l => l.StartsWith("loopcount")), "an unset loop count writes no key at all");

            var auto = new Settings { Threads = Settings.ThreadsAuto };
            Check(SettingsWriter.Lines(auto).Contains("threads = 0"), "Auto (0) is written explicitly");
            var once = new Settings { Loopcount = Settings.LoopcountOnce };
            Check(SettingsWriter.Lines(once).Contains("loopcount = -2"), "play-once (-2) survives a round trip");

            var padded = new Settings();
            padded.Comments.Add("  (draft)  ");
            Check(SettingsWriter.Lines(padded).Contains("comment = \"  (draft)  \""), "a padded value is quoted (DS-12)");

            var multi = new Settings();
            multi.Comments.Add("hi\nmode = merge");
            var multiLines = SettingsWriter.Lines(multi);
            Check(!multiLines.Any(l => l == "mode = merge"), "a newline in a value never becomes a new key (U-51)");
            Check(multiLines.Contains("comment = hi mode = merge"), "the newline folds to a space");

            var quoted = new Settings { Output = "\"weird\".gif" };
            Check(SettingsWriter.Lines(quoted).Any(l => l.Contains("\\\"weird\\\"")), "a leading quote is escaped when quoting");
        }
    }

    // ----------------------------------------------------------- parity lane

    private static void ParityLane()
    {
        var cli = Environment.GetEnvironmentVariable("GS_CLI");
        var requireProof = Environment.GetEnvironmentVariable("GS_REQUIRE_PROOF") == "1";
        if (string.IsNullOrEmpty(cli) || !File.Exists(cli))
        {
            if (requireProof)
            {
                Check(false, $"GS_REQUIRE_PROOF=1 but no CLI to compare against (GS_CLI='{cli}')");
            }
            else
            {
                Skip("parity lane: GS_CLI is not set (or does not exist) - set it to a built gifscythe-cli");
            }
            return;
        }

        var engineToken = Environment.GetEnvironmentVariable("GS_PARITY_ENGINE");
        if (string.IsNullOrEmpty(engineToken)) engineToken = "/opt/gifsicle";

        var workDir = Path.Combine(Path.GetTempPath(), "gifscythe-core-parity");
        Directory.CreateDirectory(workDir);
        // Forward slashes on purpose: they are absolute on both platforms this
        // lane runs on, and they keep the conf line free of escape characters.
        var prefix = workDir.Replace('\\', '/');
        string P(string name) => prefix + "/" + name;

        var fixtures = new List<(string Name, Settings S)>
        {
            ("defaults + input", WithInput(P("in.gif"))),
            ("empty comment is skipped (U-48)", Build(s =>
            {
                s.Comments.Add("");
                s.Comments.Add("real one");
                s.Inputs.Add(P("in.gif"));
                s.Output = P("out.gif");
            })),
            ("merge optimize lossy loop delay", Build(s =>
            {
                s.Mode = Mode.Merge;
                s.OptimizeLevel = 3;
                s.Lossy = 40;
                s.Loopcount = 0;
                s.DelayCs = 5;
                s.Inputs.Add(P("a.gif"));
                s.Inputs.Add(P("b.gif"));
                s.Output = P("out.gif");
            })),
            ("resize fit + colors + bare -f", Build(s =>
            {
                s.ResizeKind = ResizeKind.Fit;
                s.ResizeW = 320;
                s.ResizeH = 200;
                s.ColorCount = 128;
                s.Dither = true;
                s.Inputs.Add(P("in.gif"));
            })),
            ("named dither + color method + careful", Build(s =>
            {
                s.DitherMethod = "ro64";
                s.ColorMethod = "median-cut";
                s.Careful = true;
                s.Inputs.Add(P("in.gif"));
            })),
            ("crop plus-form + transparency + bg + transparent", Build(s =>
            {
                s.Crop = true;
                s.CropX = 1; s.CropY = 2; s.CropW = 30; s.CropH = 40;
                s.CropTransparency = true;
                s.Background = "#ffffff";
                s.Transparent = "#000000";
                s.Inputs.Add(P("in.gif"));
            })),
            ("geometry: rotate flip position interlace", Build(s =>
            {
                s.Rotation = Rotation.R90;
                s.FlipHorizontal = true;
                s.FlipVertical = true;
                s.HasPosition = true;
                s.PositionX = 5; s.PositionY = 6;
                s.Interlace = true;
                s.Inputs.Add(P("in.gif"));
            })),
            ("padded comment survives the conf round trip (DS-12)", Build(s =>
            {
                s.Comments.Add("  (draft)  ");
                s.Inputs.Add(P("in.gif"));
            })),
            ("threads unset (-1) emits NO -j (P0-2)", Build(s =>
            {
                s.Threads = Settings.ThreadsUnset;
                s.Inputs.Add(P("in.gif"));
            })),
            ("threads auto (0) emits a bare -j (P0-2)", Build(s =>
            {
                s.Threads = Settings.ThreadsAuto;
                s.Inputs.Add(P("in.gif"));
            })),
            ("loopcount -2 plays once (U-63)", Build(s =>
            {
                s.Loopcount = Settings.LoopcountOnce;
                s.Inputs.Add(P("in.gif"));
            })),
            ("frame selector input stays literal (U-60)", Build(s =>
            {
                s.Inputs.Add(P("in.gif"));
                s.Inputs.Add("#0");
                s.Output = P("out.gif");
            })),
            ("stdin input stays a literal dash (U-60)", Build(s =>
            {
                s.Inputs.Add("-");
                s.Output = P("out.gif");
            })),
            ("stdout output stays a literal dash (U-61)", Build(s =>
            {
                s.Inputs.Add(P("in.gif"));
                s.Output = "-";
            })),
            ("scale percent + resize method", Build(s =>
            {
                s.ResizeKind = ResizeKind.Scale;
                s.ScaleX = 0.5; s.ScaleY = 0.5;
                s.ResizeMethod = "lanczos3";
                s.Inputs.Add(P("in.gif"));
            })),
            ("asymmetric scale X/Y (U-42)", Build(s =>
            {
                s.ResizeKind = ResizeKind.Scale;
                s.ScaleX = 0.5; s.ScaleY = 2;
                s.Inputs.Add(P("in.gif"));
            })),
            ("gamma named + disposal + threads + unoptimize", Build(s =>
            {
                s.GammaStr = "srgb";
                s.Disposal = 2;
                s.Threads = 4;
                s.Unoptimize = true;
                s.Inputs.Add(P("in.gif"));
            })),
            ("explode by name + strip comments/names/extensions", Build(s =>
            {
                s.Mode = Mode.Explode;
                s.ExplodeByName = true;
                s.RemoveComments = true;
                s.RemoveNames = true;
                s.RemoveExtensions = true;
                s.Inputs.Add(P("in.gif"));
            })),
        };

        var index = 0;
        foreach (var (name, settings) in fixtures)
        {
            index++;
            var confPath = Path.Combine(workDir, $"parity-{index}.conf");
            File.WriteAllText(confPath, SettingsWriter.Write(settings));

            var printed = RunCliPrintMode(cli, confPath, engineToken, out var error);
            if (printed is null)
            {
                Check(false, $"parity '{name}': CLI did not print a command line ({error})");
                continue;
            }

            // The CLI prints `<engine token> <shell-quoted args>`. The argv it
            // would exec is the contract, so compare TOKENS: how a platform
            // echoes an engine/path back, or which separator flavour a
            // std::filesystem path keeps, is not the C# builder's business.
            var space = printed.IndexOf(' ');
            var argsPart = space < 0 ? "" : printed[(space + 1)..];
            var gotTokens = SplitShellQuoted(argsPart);
            var wantTokens = CommandBuilder.Build(settings);

            if (gotTokens.Count != wantTokens.Count)
            {
                Check(false, $"parity '{name}': argv token count differs\n  got:  {gotTokens.Count} [{Join(gotTokens)}]\n  want: {wantTokens.Count} [{Join(wantTokens)}]");
            }
            else
            {
                for (var t = 0; t < wantTokens.Count; t++)
                {
                    // The one tolerated difference: the separator flavour a
                    // std::filesystem path keeps when it echoes a Windows path
                    // back. A token must otherwise match byte for byte.
                    Check(Norm(gotTokens[t]) == Norm(wantTokens[t]),
                          $"parity '{name}': argv[{t}] must match\n  got:  {gotTokens[t]}\n  want: {wantTokens[t]}");
                }
            }

            // And the display line itself, byte for byte - but only where the
            // platform has not rewritten separators behind the writer's back
            // (one backslash forces shell_quote to wrap a token this side
            // emitted bare; the argv check above already sees through that).
            // When it cannot apply that is reported as a SKIP, never as a
            // silent pass.
            var wantLine = CommandBuilder.ToCommandLine(settings);
            if (argsPart.IndexOf('\\') < 0 && wantLine.IndexOf('\\') < 0)
            {
                Check(Norm(argsPart) == Norm(wantLine),
                      $"parity '{name}': CLI display line must equal the C# command line\n  got:  {argsPart}\n  want: {wantLine}");
            }
            else
            {
                Skip($"parity '{name}': display-line check (the platform echoes path separators)");
            }
        }
    }

    /// <summary>
    /// Split a shell-quoted argument line into the argv tokens it represents,
    /// using POSIX shell rules - the quoting every writer in this repo emits
    /// (single quotes for anything unsafe, '\'' for an embedded quote, a bare
    /// backslash escape outside quotes). Returns the tokens as they would be
    /// passed to exec, i.e. with the quoting removed.
    /// </summary>
    private static List<string> SplitShellQuoted(string line)
    {
        var tokens = new List<string>();
        var cur = new System.Text.StringBuilder();
        var has = false;
        for (var i = 0; i < line.Length; i++)
        {
            var ch = line[i];
            if (ch == '\'')
            {
                has = true;
                i++;
                while (i < line.Length && line[i] != '\'') { cur.Append(line[i]); i++; }
                // i now sits on the closing quote (or past the end: unterminated)
            }
            else if (ch == '\\' && i + 1 < line.Length)
            {
                has = true;
                i++;
                cur.Append(line[i]);
            }
            else if (ch == ' ' || ch == '\t')
            {
                if (has) { tokens.Add(cur.ToString()); cur.Clear(); has = false; }
            }
            else
            {
                has = true;
                cur.Append(ch);
            }
        }
        if (has) tokens.Add(cur.ToString());
        return tokens;
    }

    /// <summary>The one tolerated platform difference: path separator flavour.</summary>
    private static string Norm(string v) => v.Replace("\\", "/").TrimEnd();

    private static Settings Build(Action<Settings> configure)
    {
        var s = new Settings();
        configure(s);
        return s;
    }

    /// <summary>
    /// Run `gifscythe-cli &lt;conf&gt; --engine &lt;token&gt;` (print mode, no --run)
    /// and return the command line it prints, or null with a reason.
    /// </summary>
    private static string? RunCliPrintMode(string cli, string confPath, string engineToken, out string error)
    {
        error = "";
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = cli,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
            };
            psi.ArgumentList.Add(confPath);
            psi.ArgumentList.Add("--engine");
            psi.ArgumentList.Add(engineToken);

            using var proc = Process.Start(psi);
            if (proc is null)
            {
                error = "Process.Start returned null";
                return null;
            }
            var stdout = proc.StandardOutput.ReadToEnd();
            var stderr = proc.StandardError.ReadToEnd();
            if (!proc.WaitForExit(30000))
            {
                try { proc.Kill(entireProcessTree: true); } catch { /* already gone */ }
                error = "timed out after 30 s";
                return null;
            }
            if (proc.ExitCode != 0)
            {
                error = $"exit {proc.ExitCode}: {stderr.Trim()}";
                return null;
            }
            var lines = stdout.Replace("\r\n", "\n").Split('\n');
            for (var i = 0; i + 1 < lines.Length; i++)
                if (lines[i].StartsWith("# Gifscythe")) return lines[i + 1];
            error = "no '# Gifscythe ... command' header in stdout: " + stdout.Trim();
            return null;
        }
        catch (Exception ex)
        {
            error = ex.GetType().Name + ": " + ex.Message;
            return null;
        }
    }
}
