using Microsoft.Build.Logging.StructuredLogger;
using System.Text.Json;

if (args.Length < 1)
{
    Console.Error.WriteLine("Usage: bl2oc <path-to.binlog> [output-dir]");
    return 1;
}

string binlogPath = args[0];
string outputDir  = args.Length > 1 ? args[1] : Path.GetDirectoryName(Path.GetFullPath(binlogPath))!;

Directory.CreateDirectory(outputDir);

Build build = BinaryLog.ReadBuild(binlogPath);

var entries = new List<CompileCommandEntry>();

build.VisitAllChildren<Microsoft.Build.Logging.StructuredLogger.Task>(task =>
{
    if (!string.Equals(task.Name, "CL", StringComparison.OrdinalIgnoreCase))
        return;

    string commandLine = task.CommandLineArguments ?? "";
    if (string.IsNullOrEmpty(commandLine))
        return;

    // Walk up to find the project file
    string projectFile = "";
    TreeNode? node = task.Parent;
    while (node != null)
    {
        if (node is Project proj) { projectFile = proj.ProjectFile ?? ""; break; }
        node = node.Parent;
    }

    string workingDir = projectFile != ""
        ? Path.GetDirectoryName(projectFile) ?? Directory.GetCurrentDirectory()
        : Directory.GetCurrentDirectory();

    // Split off the exe path
    int slashPos   = commandLine.IndexOf(" /");
    int dashPos    = commandLine.IndexOf(" -");
    int flagsStart = (slashPos, dashPos) switch
    {
        ( < 0, < 0) => -1,
        ( < 0, _  ) => dashPos,
        (_,   < 0 ) => slashPos,
        _            => Math.Min(slashPos, dashPos)
    };

    if (flagsStart < 0)
        return;

    string   flagsAndFiles = commandLine.Substring(flagsStart + 1);
    string[] tokens        = SplitCommandLine(flagsAndFiles);

    string[] sourceFiles   = tokens.Where(t => IsSourceFile(t.Trim('"'))).ToArray();
    string[] flagTokens    = tokens.Where(t => !IsSourceFile(t.Trim('"'))).ToArray();
    string   rewritten     = RewriteFlags(flagTokens);

    foreach (string src in sourceFiles)
    {
        string absoluteSrc = Path.IsPathRooted(src.Trim('"'))
            ? src.Trim('"')
            : Path.Combine(workingDir, src.Trim('"'));

        entries.Add(new CompileCommandEntry
        {
            directory = workingDir.Replace('\\', '/'),
            file      = absoluteSrc.Replace('\\', '/'),
            command   = $"clang-cl.exe {rewritten} \"{absoluteSrc.Replace('\\', '/')}\""
        });
    }
});

string ccPath = Path.Combine(outputDir, "compile_commands.json");
File.WriteAllText(ccPath, JsonSerializer.Serialize(entries, new JsonSerializerOptions
        { WriteIndented = true, Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping }));

Console.WriteLine($"Written: {ccPath} ({entries.Count} entries)");
return 0;

static string RewriteFlags(string[] tokens)
{
    // MSVC-only flags that clang-cl does not accept
    var drop = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
    {
        "/c", "/JMC", "/Gm-", "/sdl", "/RTC1", "/analyze-",
        "/errorReport:queue", "/external:W3", "/ZI",
        "/FC", "/TP", "/Gd"
    };

    // Flag prefixes to drop (flags that take an argument concatenated)
    var dropPrefixes = new[]
    {
        "/Fd", "/external:" // , "/Fo" // keeping /Fo so that OdrCop3 can find the .obj files for the COFF reader
    };

    var kept = new List<string>();
    foreach (string token in tokens)
    {
        if (drop.Contains(token)) continue;
        if (dropPrefixes.Any(p => token.StartsWith(p, StringComparison.OrdinalIgnoreCase))) continue;
        kept.Add(token);
    }

    return string.Join(" ", kept);
}

static bool IsSourceFile(string path) =>
    new[] { ".cpp", ".cxx", ".cc", ".c", ".ixx", ".cppm" }.Any(ext => path.TrimEnd('"').EndsWith(ext, StringComparison.OrdinalIgnoreCase));

static string[] SplitCommandLine(string cmdLine)
{
    var  result  = new List<string>();
    var  token   = new System.Text.StringBuilder();
    bool inQuote = false;

    for (int i = 0; i < cmdLine.Length; i++)
    {
        char c = cmdLine[i];
        if (c == '"')
        {
            if (inQuote && i + 1 < cmdLine.Length && cmdLine[i + 1] == '"')
            { token.Append('"'); i++; }
            else
            { inQuote = !inQuote; }
        }
        else if ((c == ' ' || c == '\t') && !inQuote)
        {
            if (token.Length > 0) { result.Add(token.ToString()); token.Clear(); }
        }
        else token.Append(c);
    }

    if (token.Length > 0) result.Add(token.ToString());
    return result.ToArray();
}

record CompileCommandEntry
{
    public string directory { get; init; } = "";
    public string file      { get; init; } = "";
    public string command   { get; init; } = "";
}