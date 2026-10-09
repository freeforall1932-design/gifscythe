// FakeEngine - a hostile-engine test double for the gifscythe-spike runner.
//
// Not shipped and not part of any product surface. It exists so CI can exercise
// engine behaviours a real gifsicle will never show on demand: stalling while
// holding stderr OPEN, flooding stderr, exiting non-zero, and exiting zero while
// leaving a non-GIF behind.
//
// Driven by ENVIRONMENT VARIABLES, not argv, because the spike owns the argv it
// hands to the engine (["-O3", <input>, "-o", <output>]) - a test double should
// not have to impersonate the real engine's argument grammar in order to
// misbehave. The child inherits the caller's environment, so the test sets these
// before invoking the spike.
//
//   FAKE_MODE=ok      FAKE_SRC=<file> FAKE_OUT=<file>   copy src -> out, exit 0
//   FAKE_MODE=stall                                     one stderr line, then
//                                                       never exit (stderr OPEN)
//   FAKE_MODE=flood   FAKE_FLOOD_BYTES=<n>              <n> bytes of stderr,
//                                                       exit 0, write nothing
//   FAKE_MODE=garbage FAKE_OUT=<file>                   exit 0, non-GIF left behind
//   FAKE_MODE=fail    FAKE_EXIT=<n>                     write stderr, exit <n>
//
// Unknown mode: exits 2 rather than silently doing something plausible.

using System.Text;

var mode = Environment.GetEnvironmentVariable("FAKE_MODE") ?? "ok";
var src = Environment.GetEnvironmentVariable("FAKE_SRC");
var outPath = Environment.GetEnvironmentVariable("FAKE_OUT");

switch (mode)
{
    case "ok":
        if (string.IsNullOrEmpty(src) || string.IsNullOrEmpty(outPath))
        {
            Console.Error.WriteLine("fake engine: FAKE_SRC and FAKE_OUT must be set for mode ok");
            return 2;
        }
        File.Copy(src, outPath, overwrite: true);
        return 0;

    case "stall":
        Console.Error.WriteLine("fake engine: starting, then stalling with stderr held open");
        Console.Error.Flush();
        // Hold the pipe open forever. A caller that reads stderr to EOF before
        // waiting on the process will never come back from this line.
        Thread.Sleep(Timeout.Infinite);
        return 0;

    case "flood":
    {
        var total = long.TryParse(Environment.GetEnvironmentVariable("FAKE_FLOOD_BYTES"), out var n)
            ? n
            : 64L * 1024 * 1024;
        var chunk = Encoding.UTF8.GetBytes(new string('x', 64 * 1024) + "\n");
        // Raw stream + explicit buffer: Console.Error is autoflush and would
        // make a 64 MiB flood uselessly slow.
        using var raw = Console.OpenStandardError();
        using var err = new BufferedStream(raw, 64 * 1024);
        long written = 0;
        while (written < total)
        {
            err.Write(chunk, 0, chunk.Length);
            written += chunk.Length;
        }
        err.Flush();
        // FAKE_EXIT lets one mode prove two things: a bounded capture on the
        // success path, and a bounded capture that is actually REPORTED on the
        // failure path (where the spike prints the engine's stderr).
        return int.TryParse(Environment.GetEnvironmentVariable("FAKE_EXIT"), out var code) ? code : 0;
    }

    case "garbage":
        if (!string.IsNullOrEmpty(outPath)) File.WriteAllText(outPath, "not a gif\n");
        Console.Error.WriteLine("fake engine: exited 0 after writing a non-GIF");
        return 0;

    case "fail":
    {
        var code = int.TryParse(Environment.GetEnvironmentVariable("FAKE_EXIT"), out var c) ? c : 7;
        Console.Error.WriteLine($"fake engine: failing on purpose with exit {code}");
        return code;
    }

    default:
        Console.Error.WriteLine($"fake engine: unknown FAKE_MODE '{mode}'");
        return 2;
}
