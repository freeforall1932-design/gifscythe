// Gifscythe Phase-1 spike: hardcoded Auto/optimize-3 run through the real engine.
// Throwaway-allowed. Proves: dotnet toolchain, honest subprocess semantics
// (distinct exit codes), Unicode paths, self-contained single-file publish.
// Exit codes: 0 ok · 2 usage/caller error · 3 engine missing ·
//             4 engine failed · 5 invalid output (rc=0 but bad/missing GIF) ·
//             124 engine timed out · 127 engine could not be started.
// MUST exit non-zero unless a verified GIF was produced. No silent success.
//
// U-91 / P3-17: these numbers are the spike's OWN legacy contract, which is NOT
// Gifscythe.Core.ExitCodes. Merging the two is the open U-91 decision and is
// deliberately not done here. What changed below is only HOW the codes are
// reached, never which code means what.

using System.Diagnostics;
using System.Text;

return await Spike.RunAsync(args);

static class Spike
{
    // Mirrors the shape of the future Gifscythe.Core port: a settings object
    // rendered to argv by one builder. Only optimize_level exists this phase.
    private sealed record Settings(int OptimizeLevel = 3);

    // Bounded stderr capture: a chatty or hostile engine must not be able to
    // grow this process without limit. Keep the first StderrCapChars characters,
    // count and drop the rest, and SAY how much was dropped so a truncated log
    // never poses as a complete one.
    private const int StderrCapChars = 32 * 1024; // 64 KiB of UTF-16
    private const int DefaultTimeoutMs = 120_000;
    private const int DrainGraceMs = 5_000;
    private const int ReapGraceMs = 5_000;

    private static string[] BuildArgs(Settings s, string input, string output) =>
        ["-O" + s.OptimizeLevel, input, "-o", output];

    // Overridable so CI can prove the deadline with a 4 s test deadline instead
    // of waiting out the real one.
    private static int TimeoutMs()
    {
        var raw = Environment.GetEnvironmentVariable("GIFSCYTHE_SPIKE_TIMEOUT_MS");
        return int.TryParse(raw, out var v) && v > 0 ? v : DefaultTimeoutMs;
    }

    public static async Task<int> RunAsync(string[] args)
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

        // AUD-04: the engine writes a FRESH staging file this run created
        // exclusively (CreateNew, zero bytes). Verifying the staging file -
        // never the destination - proves THIS run wrote the GIF: an engine that
        // exits 0 without writing leaves it empty, so a pre-existing good
        // output can no longer be reported as this run's success. The
        // destination is only replaced after verification passes; every
        // failure path leaves it untouched (same contract as OutputVerify.h).
        var staging = ClaimStaging(output);
        if (staging is null)
        {
            Console.Error.WriteLine($"cannot create a staging file beside the output (engine not started): {output}");
            return 2;
        }
        try
        {
            return await RunStagedAsync(engine, input, output, staging);
        }
        finally
        {
            // Only reached with the file still present on failure paths (a
            // successful promotion has already moved it). We created it, so
            // deleting it can never destroy user data.
            try { if (File.Exists(staging)) File.Delete(staging); } catch (IOException) { } catch (UnauthorizedAccessException) { }
        }
    }

    private static string? ClaimStaging(string output)
    {
        var full = Path.GetFullPath(output);
        var dir = Path.GetDirectoryName(full) ?? ".";
        var name = Path.GetFileName(full);
        for (var i = 0; i < 16; i++)
        {
            var cand = Path.Combine(dir, $".{name}.{Environment.ProcessId}.{Guid.NewGuid():N}.gs-partial");
            try
            {
                using (new FileStream(cand, FileMode.CreateNew, FileAccess.Write)) { }
                return cand;
            }
            catch (IOException) when (File.Exists(cand)) { /* collision: try another name */ }
            catch (Exception) { return null; }
        }
        return null;
    }

    private static async Task<int> RunStagedAsync(string engine, string input, string output, string staging)
    {
        var argv = BuildArgs(new Settings(), input, staging);
        Console.WriteLine(Quote(engine) + " " + string.Join(" ", argv.Select(Quote)));

        var (code, capture) = await StartAsync(engine, argv, TimeoutMs());

        if (code == 124)
        {
            Console.Error.WriteLine($"engine timed out: {Detail(capture)}");
            return 124;
        }
        if (code == 127)
        {
            Console.Error.WriteLine($"engine could not be started: {Detail(capture)}");
            return 127;
        }
        if (code != 0)
        {
            Console.Error.WriteLine($"engine failed (exit {code}): {Detail(capture)}");
            return 4;
        }

        var problem = VerifyGif(staging);
        if (problem is not null)
        {
            Console.Error.WriteLine($"engine exited 0 but {problem} (destination left untouched): {output}");
            return 5;
        }
        try
        {
            File.Move(staging, output, overwrite: true);
        }
        catch (Exception ex)
        {
            // Keep the verified bytes: the finally in RunAsync would delete
            // them, so move them aside under a name we report.
            var kept = staging + ".verified";
            try { File.Move(staging, kept); } catch (Exception) { kept = staging; }
            Console.Error.WriteLine($"verified output could not replace {output} ({ex.GetType().Name}: {ex.Message}); verified file kept at {kept}");
            return 6;
        }

        var (inBytes, outBytes) = (new FileInfo(input).Length, new FileInfo(output).Length);
        Console.WriteLine($"ok: {inBytes} -> {outBytes} bytes");
        return 0;
    }

    /// <summary>Run the engine under a deadline that actually holds.</summary>
    /// <remarks>
    /// The previous shape was:
    ///     var stderr = child.StandardError.ReadToEnd();       // (1)
    ///     if (!child.WaitForExit(120_000)) { ...kill... }     // (2)
    /// (1) blocks until the engine CLOSES stderr, so an engine that stalls while
    /// holding the pipe open blocks (1) forever and (2) - the timeout and the
    /// only kill branch - is never reached. The fix is to make the read
    /// CONCURRENT with the wait and let the wait own the deadline; the read is
    /// also bounded, so an engine cannot spend the caller's memory either.
    /// </remarks>
    private static async Task<(int Code, Capture Cap)> StartAsync(
        string engine, string[] argv, int timeoutMs)
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
            // Not started at all (bad binary, permissions, wrong architecture).
            return (127, Capture.FromText(ex.Message));
        }

        using var cts = new CancellationTokenSource(timeoutMs);
        var capture = new Capture();
        var drain = DrainAsync(child.StandardError, capture, cts.Token);

        try
        {
            await child.WaitForExitAsync(cts.Token).ConfigureAwait(false);
        }
        catch (OperationCanceledException)
        {
            // Cancelling an await is not process termination: kill the tree,
            // reap it, and stop the drain before the run slot is released -
            // otherwise the stalled engine outlives the caller.
            KillTree(child);
            await ReapAsync(child).ConfigureAwait(false);
            await SettleAsync(drain, cts).ConfigureAwait(false);
            return (124, capture.WithNote($"engine timed out after {timeoutMs} ms"));
        }

        await SettleAsync(drain, cts).ConfigureAwait(false);
        return (child.ExitCode, capture);
    }

    private static async Task DrainAsync(StreamReader reader, Capture capture, CancellationToken ct)
    {
        var buf = new char[4096];
        try
        {
            // The Memory overload honours the token; the (char[],int,int) one
            // does not, and an uncancellable read would outlive the deadline.
            int n;
            while ((n = await reader.ReadAsync(buf.AsMemory(0, buf.Length), ct).ConfigureAwait(false)) > 0)
            {
                capture.Add(buf, n);
            }
        }
        catch (OperationCanceledException) { /* deadline fired; keep what we read */ }
        catch (Exception) { /* pipe closed by a killed engine, or process disposed */ }
    }

    private static void KillTree(Process child)
    {
        try { child.Kill(entireProcessTree: true); } catch { /* already gone */ }
    }

    private static async Task ReapAsync(Process child)
    {
        using var reap = new CancellationTokenSource(ReapGraceMs);
        try { await child.WaitForExitAsync(reap.Token).ConfigureAwait(false); }
        catch (Exception) { /* refused to die; there is nothing further to do */ }
    }

    /// <summary>
    /// Give the drain a bounded grace period to finish, then stop it. Never leak
    /// a reader task, and never let a broken pipe fail an otherwise good run.
    /// </summary>
    private static async Task SettleAsync(Task drain, CancellationTokenSource cts)
    {
        var finished = await Task.WhenAny(drain, Task.Delay(DrainGraceMs)).ConfigureAwait(false);
        if (finished != drain) cts.Cancel();
        try { await drain.ConfigureAwait(false); } catch { /* no-op */ }
    }

    /// <summary>Bounded stderr capture plus the reason the run was cut short.</summary>
    private sealed class Capture
    {
        private readonly StringBuilder _kept = new();
        private long _dropped;

        public string Note { get; private set; } = "";

        public static Capture FromText(string text)
        {
            var c = new Capture();
            c._kept.Append(text);
            return c;
        }

        public Capture WithNote(string note)
        {
            Note = note;
            return this;
        }

        public void Add(char[] buf, int n)
        {
            var room = StderrCapChars - _kept.Length;
            if (room <= 0)
            {
                _dropped += n;
                return;
            }
            var take = Math.Min(n, room);
            _kept.Append(buf, 0, take);
            _dropped += n - take;
        }

        public string Text => _kept.ToString();

        // Appended AFTER trimming, so the fact of the drop survives a long log.
        public string DropNotice() =>
            _dropped > 0
                ? $" [gifscythe: {_dropped} stderr character(s) dropped - capture capped at {StderrCapChars}]"
                : "";
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
            // P3-17 port trap: Stream.Read may under-fill. ReadExactly either
            // fills the buffer or throws, so a short read is no longer reported
            // as "produced an empty/short file" - which is a different claim.
            stream.ReadExactly(head, 0, 6);
        }
        catch (EndOfStreamException) { return "produced an empty/short file"; }
        catch (FileNotFoundException) { return "produced no output file"; }
        catch (DirectoryNotFoundException) { return "produced no output file"; }
        catch (Exception ex) { return $"output is unreadable ({ex.GetType().Name})"; }
        var magic = Encoding.ASCII.GetString(head);
        return (magic == "GIF87a" || magic == "GIF89a") ? null : "produced a file without a GIF signature";
    }

    private static string Detail(Capture c)
    {
        var parts = new List<string>();
        if (!string.IsNullOrWhiteSpace(c.Note)) parts.Add(c.Note.Trim());
        var text = Trim(c.Text);
        if (text.Length > 0) parts.Add(text);
        return string.Join(" | ", parts) + c.DropNotice();
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
