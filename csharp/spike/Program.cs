// Gifscythe Phase-1 spike: hardcoded Auto/optimize-3 run through the real engine.
// Throwaway-allowed. Proves: dotnet toolchain, honest subprocess semantics
// (distinct exit codes), Unicode paths, self-contained single-file publish.
// Exit codes: 0 ok · 2 usage/caller error · 3 engine missing ·
//             4 engine failed · 5 invalid output (rc=0 but bad/missing GIF).
// MUST exit non-zero unless a verified GIF was produced. No silent success.

using System.Diagnostics;

var rc = Spike.Run(args);
return rc;

static class Spike
{
    // Mirrors the shape of the future Gifscythe.Core port: a settings object
    // rendered to argv by one builder. Only optimize_level exists this phase.
    private sealed record Settings(int OptimizeLevel = 3);

    private static string[] BuildArgs(Settings s, string input, string output) =>
        ["-O" + s.OptimizeLevel, input, "-o", output];

    public static int Run(string[] args)
    {
        if (args.Length != 3)
        {
            Console.Error.WriteLine("usage: gifscythe-spike <engine> <input.gif> <output.gif>");
            return 2;
        }
        var (engine, input, output) = (args[0], args[1], args[2]);
        if (!File.Exists(engine))
        {
            Console.Error.WriteLine($"engine not found: {engine}");
            return 3;
        }
        if (!File.Exists(input))
        {
            Console.Error.WriteLine($"input not found (engine not started): {input}");
            return 2;
        }

        var argv = BuildArgs(new Settings(), input, output);
        Console.WriteLine(Quote(engine) + " " + string.Join(" ", argv.Select(Quote)));

        var (code, stderr) = Start(engine, argv);
        if (code != 0)
        {
            Console.Error.WriteLine($"engine failed (exit {code}): {Trim(stderr)}");
            return 4;
        }

        var problem = VerifyGif(output);
        if (problem is not null)
        {
            Console.Error.WriteLine($"engine exited 0 but {problem}: {output}");
            return 5;
        }

        var (inBytes, outBytes) = (new FileInfo(input).Length, new FileInfo(output).Length);
        Console.WriteLine($"ok: {inBytes} -> {outBytes} bytes");
        return 0;
    }

    private static (int Code, string Stderr) Start(string engine, string[] argv)
    {
        using var child = new Process();
        child.StartInfo.FileName = engine;
        foreach (var a in argv) child.StartInfo.ArgumentList.Add(a); // argv array, never a shell
        child.StartInfo.RedirectStandardError = true;
        child.StartInfo.UseShellExecute = false;
        try
        {
            child.Start();
        }
        catch (Exception ex)
        {
            return (127, ex.Message); // not started at all (bad binary, perms, arch)
        }
        var stderr = child.StandardError.ReadToEnd();
        if (!child.WaitForExit(120_000))
        {
            try { child.Kill(entireProcessTree: true); } catch { /* already gone */ }
            child.WaitForExit(5_000);
            return (124, "engine timed out after 120 s");
        }
        return (child.ExitCode, stderr);
    }

    // Minimal output honesty: exists + non-empty + GIF87a/GIF89a magic.
    // (The full stale-output snapshot/diff is Phase 2's OutputVerify port.)
    private static string? VerifyGif(string path)
    {
        byte[] head;
        try
        {
            using var stream = File.OpenRead(path);
            head = new byte[6];
            if (stream.Read(head, 0, 6) != 6) return "produced an empty/short file";
        }
        catch (FileNotFoundException) { return "produced no output file"; }
        catch (DirectoryNotFoundException) { return "produced no output file"; }
        catch (Exception ex) { return $"output is unreadable ({ex.GetType().Name})"; }
        var magic = System.Text.Encoding.ASCII.GetString(head);
        return (magic == "GIF87a" || magic == "GIF89a") ? null : "produced a file without a GIF signature";
    }

    private static string Quote(string a)
    {
        if (a.Length > 0 && a.All(c => char.IsLetterOrDigit(c) || "/._-+=:@%,".Contains(c))) return a;
        return "'" + a.Replace("'", "'\\''") + "'";
    }

    private static string Trim(string s)
    {
        s = s.Trim();
        return s.Length <= 500 ? s : s[..500] + "…";
    }
}
