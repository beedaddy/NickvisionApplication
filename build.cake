#addin nuget:?package=Cake.FileHelpers&version=6.1.3

const string appId = "org.nickvision.application";
const string projectName = "NickvisionApplication";
const string shortName = "application";

var sep = System.IO.Path.DirectorySeparatorChar;

var target = Argument("target", "Run");
var configuration = Argument("configuration", "Debug");
var ui = Argument("ui", EnvironmentVariable<string>("NICK_UI", ""));
var projectSuffix = "";

//////////////////////////////////////////////////////////////////////
// TASKS
//////////////////////////////////////////////////////////////////////

Task("Clean")
    .WithCriteria(c => HasArgument("rebuild"))
    .Does(() =>
{
    CleanDirectory($".{sep}{projectName}.{projectSuffix}{sep}bin{sep}{configuration}");
});

Task("Build")
    .IsDependentOn("Clean")
    .Does(() =>
{
    DotNetBuild($".{sep}{projectName}.{projectSuffix}{sep}{projectName}.{projectSuffix}.csproj", new DotNetBuildSettings
    {
        Configuration = configuration
    });
});

Task("Run")
    .IsDependentOn("Build")
    .Does(() =>
{
    DotNetRun($".{sep}{projectName}.{projectSuffix}{sep}{projectName}.{projectSuffix}.csproj", new DotNetRunSettings
    {
        Configuration = configuration,
        NoBuild = true
    });
});

Task("Publish")
    .Does(() =>
{
    var selfContained = Argument("self-contained", false) || HasArgument("self-contained") || HasArgument("sc");
    var outDir = EnvironmentVariable<string>("NICK_BUILDDIR", "_nickbuild");
    CleanDirectory(outDir);
    var prefix = Argument("prefix", IsRunningOnWindows() ? "" : "usr");
    var libDir = string.IsNullOrEmpty(prefix) ? "lib" : $"{prefix}{sep}lib";
    var publishDir = $"{outDir}{sep}{libDir}{sep}{appId}";
    DotNetPublish($".{sep}{projectName}.{projectSuffix}{sep}{projectName}.{projectSuffix}.csproj", new DotNetPublishSettings
    {
        Configuration = "Release",
        SelfContained = selfContained,
        OutputDirectory = publishDir
    });
    if (projectSuffix == "GNOME")
    {
        PostPublishGNOME(outDir, prefix, libDir);
    }

    if (IsRunningOnWindows())
    {
        FinishPublishWindows(outDir, prefix);
    }
    else
    {
        FinishPublishLinux(outDir, prefix, libDir, selfContained);
    }
});

//////////////////////////////////////////////////////////////////////
// FUNCTIONS
//////////////////////////////////////////////////////////////////////

private void PostPublishGNOME(string outDir, string prefix, string libDir)
{
    var shareDir = string.IsNullOrEmpty(prefix) ? "share" : $"{prefix}{sep}share";

    CreateDirectory($"{outDir}{sep}{shareDir}{sep}{appId}");
    MoveFileToDirectory($"{outDir}{sep}{libDir}{sep}{appId}{sep}{appId}.gresource", $"{outDir}{sep}{shareDir}{sep}{appId}");

    var iconsScalableDir = $"{outDir}{sep}{shareDir}{sep}icons{sep}hicolor{sep}scalable{sep}apps";
    CreateDirectory(iconsScalableDir);
    CopyFileToDirectory($".{sep}{projectName}.Shared{sep}Resources{sep}{appId}.svg", iconsScalableDir);
    CopyFileToDirectory($".{sep}{projectName}.Shared{sep}Resources{sep}{appId}-devel.svg", iconsScalableDir);
    var iconsSymbolicDir = $"{outDir}{sep}{shareDir}{sep}icons{sep}hicolor{sep}symbolic{sep}apps";
    CreateDirectory(iconsSymbolicDir);
    CopyFileToDirectory($".{sep}{projectName}.Shared{sep}Resources{sep}{appId}-symbolic.svg", iconsSymbolicDir);

    var servicesDir = $"{outDir}{sep}{shareDir}{sep}dbus-1{sep}services";
    CreateDirectory(servicesDir);
    CopyFileToDirectory($".{sep}{projectName}.GNOME{sep}{appId}.service.in", servicesDir);
    ReplaceTextInFiles($"{servicesDir}{sep}{appId}.service.in", "@PREFIX@", $"{sep}{prefix}");
    MoveFile($"{servicesDir}{sep}{appId}.service.in", $"{servicesDir}{sep}{appId}.service");
}

private void FinishPublishWindows(string outDir, string prefix)
{
    // TODO
}

private void FinishPublishLinux(string outDir, string prefix, string libDir, bool selfContained)
{
    var binDir = string.IsNullOrEmpty(prefix) ? $"{outDir}/bin" : $"{outDir}/{prefix}/bin";
    CreateDirectory(binDir);
    CopyFileToDirectory($"./{projectName}.Shared/{appId}.in", binDir);
    ReplaceTextInFiles($"{binDir}/{appId}.in", "@EXEC@", selfContained ? $"/{libDir}/{appId}/{projectName}.{projectSuffix}" : $"dotnet /{libDir}/{appId}/{projectName}.{projectSuffix}.dll");
    MoveFile($"{binDir}/{appId}.in", $"{binDir}/{appId}");

    var shareDir = string.IsNullOrEmpty(prefix) ? $"{outDir}/share" : $"{outDir}/{prefix}/share";

    var desktopDir = $"{shareDir}/applications";
    CreateDirectory(desktopDir);
    CopyFileToDirectory($"./{projectName}.Shared/{appId}.desktop.in", desktopDir);
    ReplaceTextInFiles($"{desktopDir}/{appId}.desktop.in", "@EXEC@", $"/{prefix}/bin/{appId}");
    MoveFile($"{desktopDir}/{appId}.desktop.in", $"{desktopDir}/{appId}.desktop");

    var metainfoDir = $"{shareDir}/metainfo";
    CreateDirectory(metainfoDir);
    CopyFileToDirectory($"./{projectName}.Shared/{appId}.metainfo.xml.in", metainfoDir);
    ReplaceTextInFiles($"{metainfoDir}/{appId}.metainfo.xml.in", "@PROJECT@", $"{projectName}.{projectSuffix}");
    MoveFile($"{metainfoDir}/{appId}.metainfo.xml.in", $"{metainfoDir}/{appId}.metainfo.xml");
}

//////////////////////////////////////////////////////////////////////
// EXECUTION
//////////////////////////////////////////////////////////////////////

if (string.IsNullOrEmpty(ui))
{
    throw new Exception("UI is not set. Use --ui option or NICK_UI environment variable.");
}
projectSuffix = ui.ToLower() switch
{
    "fluent" => "Fluent",
    "gnome" => "GNOME",
    _ => ""
};
if (string.IsNullOrEmpty(projectSuffix))
{
    throw new Exception("Unknown UI. Possible values: fluent, gnome.");
}

RunTarget(target);