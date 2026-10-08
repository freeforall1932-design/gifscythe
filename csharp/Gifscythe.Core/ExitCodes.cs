// ExitCodes.cs - the ONE exit-code contract for the C# lane.
//
// Register row U-91 / P3-17: the C++ CLI and the Phase-1 spike disagreed about
// the same numbers ("3 = engine missing" in the spike vs "3 = --strict refusal"
// in the CLI), so a script that read the code could not tell which program it
// had run. The parked C# plan's own instruction was: "fix all three against one
// shared exit-code contract when Phase 2 resumes" - Phase 2 resumes here, so the
// contract is defined once, in the core, and the four values ARE the C++ CLI's
// documented ones (src/cli/main.cpp print_usage).
//
// The shell's richer failure states (engine missing, engine exited non-zero,
// output missing or unverifiable) all map onto ExitCodes.Failure: a GUI caller
// distinguishes them by MESSAGE and dialog, not by inventing a fifth number that
// collides with the CLI's contract again. That mapping is pinned by a test in
// csharp/Gifscythe.Core.Tests.
//
// U-91 stays OPEN until the spike itself is pointed at this class and the row
// says so - this file starts that work, it does not finish it.

namespace Gifscythe.Core;

public static class ExitCodes
{
    /// <summary>Everything succeeded - a verified output exists (or was not required).</summary>
    public const int Ok = 0;

    /// <summary>Engine, path or output-verification failure (the CLI's code 1).</summary>
    public const int Failure = 1;

    /// <summary>Usage or caller error, or an unsafe output target (the CLI's code 2).</summary>
    public const int UsageError = 2;

    /// <summary>--strict refusal: at least one warning was suppressed-and-refused (the CLI's code 3).</summary>
    public const int StrictRefusal = 3;

    /// <summary>
    /// The shell's failure kinds, each of which maps onto one of the four codes
    /// above. Named so a caller names a state instead of a number.
    /// </summary>
    public enum FailureKind
    {
        EngineMissing = 0,
        EngineFailed = 1,
        StartFailure = 2,
        OutputInvalid = 3,
    }

    /// <summary>Map a shell failure kind onto the shared contract.</summary>
    public static int FromFailure(FailureKind kind) => kind switch
    {
        FailureKind.EngineMissing => Failure,
        FailureKind.EngineFailed => Failure,
        FailureKind.StartFailure => Failure,
        FailureKind.OutputInvalid => Failure,
        _ => Failure,
    };
}
