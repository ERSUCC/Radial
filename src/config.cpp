#include "config.h"

std::unique_ptr<const TOML> Config::readConfig(const std::filesystem::path& root)
{
    if (!std::filesystem::exists(root))
    {
        throw RadialFileException("The specified project directory does not exist.");
    }

    if (!std::filesystem::is_directory(root))
    {
        throw RadialFileException("The specified project path is not a directory.");
    }

    const std::filesystem::path configPath = root / "build.toml";

    if (!std::filesystem::is_regular_file(configPath))
    {
        throw RadialFileException("The specified project directory does not contain a build.toml file.");
    }

    Utils::info("Loading configuration for project in \"" + root.string() + "\"");

    std::unique_ptr<const TOML> config(TOML::parse(configPath));

    const std::string radialVersion = config->get("radial_version")->string().get(RADIAL_VERSION);

    if (Utils::numericVersion(radialVersion) > Utils::numericVersion(RADIAL_VERSION))
    {
        Utils::warning("Current version is " + std::string(RADIAL_VERSION) + ", but configuration file specifies " + radialVersion + ". Some configuration features may not work properly.");
    }

    return config;
}

BuildEnvironment BuildEnvironment::create(const BuildOptions* options)
{
    std::unique_ptr<const TOML> global(new TOML({}));

    try
    {
        global.reset(TOML::parse(homePath() / ".radial" / "config.toml"));
    }

    catch (const RadialException& ex) {}

    const std::filesystem::path root = std::filesystem::weakly_canonical(options->root.value_or("."));

    std::unique_ptr<const TOML> config(Config::readConfig(root));

    const std::string name = config->get("name")->string().require("No project name specified.");

    const int stdVersion = config->get("std_version")->integer().require("No C++ standard version specified.");

    const std::filesystem::path dest = root / config->get("out_dir")->string().get("build");

    std::filesystem::create_directories(dest);

    BuildEnvironment env = BuildEnvironment(options, std::move(config), name, stdVersion, root, dest);

    findCompiler(env);

    for (const TOMLValue* include : global->get("include_dirs")->array().get({}))
    {
        const std::string value = Utils::trim(include->string().get(""));

        if (!value.empty())
        {
            const std::filesystem::path path(value);

            if (path.is_absolute() && std::filesystem::is_directory(path))
            {
                env.includeDirs.insert(value);
            }
        }
    }

    for (const TOMLValue* include : env.config->get("include_dirs")->array().get({}))
    {
        const std::string value = Utils::trim(include->string().require("`include_dirs` must be an array of strings."));

        if (!value.empty() && std::filesystem::is_directory(root / value))
        {
            env.includeDirs.insert(root / value);
        }
    }

    for (const TOMLValue* dir : env.config->get("source_dirs")->array().require("Configuration does not specify any sources."))
    {
        const std::string value = Utils::trim(dir->string().require("`source_dirs` must be an array of strings."));

        if (!value.empty() && std::filesystem::is_directory(root / value))
        {
            for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(root / value))
            {
                if (entry.is_regular_file() && Utils::endsWith(entry.path().string(), ".cpp"))
                {
                    env.sources.insert(entry.path());

                    PathSet includes;

                    findIncludes(env, entry.path(), includes);

                    env.includes[entry.path()] = includes;
                }
            }
        }
    }

    for (const TOMLValue* dir : global->get("link_dirs")->array().get({}))
    {
        const std::string value = Utils::trim(dir->string().get(""));

        if (!value.empty())
        {
            const std::filesystem::path path(value);

            if (path.is_absolute() && std::filesystem::is_directory(path))
            {
                env.libDirs.insert(value);
            }
        }
    }

    for (const TOMLValue* dir : env.config->get("link_dirs")->array().get({}))
    {
        const std::string value = Utils::trim(dir->string().require("`link_dirs` must be an array of strings."));

        if (!value.empty())
        {
            env.libDirs.insert(value);
        }
    }

    for (const TOMLValue* lib : env.config->get("link_libraries")->array().get({}))
    {
        const std::string value = Utils::trim(lib->string().require("`link_libraries` must be an array of strings."));

        if (!value.empty())
        {
            env.libs.insert(value);
        }
    }

    const ListMap<std::string, const TOMLValue*> defines = env.config->get("defines")->table().get({});

    for (const std::string& key : defines.keys())
    {
        const std::string value = defines.get(key)->string().require("Defines must be strings.");

        env.defines.add(key, value);
    }

    return env;
}

BuildEnvironment::BuildEnvironment(const BuildOptions* options, std::unique_ptr<const TOML> config, const std::string& name, const int stdVersion, const std::filesystem::path& root, const std::filesystem::path& dest) :
    options(options), config(std::move(config)), name(name), stdVersion(stdVersion), root(root), dest(dest), cache(dest / ".cache") {}

#ifdef _WIN32

#include <wchar.h>

#include <Windows.h>
#include <objbase.h>
#include <ShlObj.h>

std::filesystem::path BuildEnvironment::homePath()
{
    wchar_t* wpath;

    if (FAILED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &wpath)))
    {
        CoTaskMemFree(wpath);

        throw RadialFileException("Failed to locate home directory.");
    }

    const std::filesystem::path path(wpath);

    CoTaskMemFree(wpath);

    return path;
}

void BuildEnvironment::findCompiler(BuildEnvironment& env)
{
    std::unique_ptr<const TOML> toml;

    const std::filesystem::path compilerPath = env.cache / "compiler.toml";

    if (std::filesystem::exists(compilerPath))
    {
        toml.reset(TOML::parse(compilerPath));
    }

    else
    {
        const std::filesystem::path temp = std::filesystem::temp_directory_path() / "radial_compiler_info.txt";

        std::string cmd = "cmd /v:on /c \"vcvars64 > nul";

        cmd += " && where cl > \"" + temp.string() + "\"";
        cmd += " && where link >> \"" + temp.string() + "\"";
        cmd += " && echo !INCLUDE! >> \"" + temp.string() + "\"";
        cmd += " && echo !LIB! >> \"" + temp.string() + "\"\"";

        try
        {
            if (Process::run(cmd, false, false))
            {
                throw RadialException("Failed to find MSVC. Make sure you have the Visual Studio development tools installed.");
            }
        }

        catch (const RadialException& ex)
        {
            throw RadialException("Failed to find MSVC. Make sure you have the Visual Studio development tools installed.");
        }

        const std::vector<std::string> sections = Utils::split(Utils::readString(temp), "\n");

        std::filesystem::remove(temp);

        std::vector<const TOMLValue*> includes;

        for (const std::string& path : Utils::split(sections[2], ";"))
        {
            includes.push_back(new TOMLString(Utils::trim(path), true));
        }

        std::vector<const TOMLValue*> links;

        for (const std::string& path : Utils::split(sections[3], ";"))
        {
            links.push_back(new TOMLString(Utils::trim(path), true));
        }

        toml.reset(new TOML(
        {
            new TOMLEntry("compiler_path", new TOMLString(Utils::trim(sections[0]), true)),
            new TOMLEntry("linker_path", new TOMLString(Utils::trim(sections[1]), true)),
            new TOMLEntry("include_dirs", new TOMLArray(includes)),
            new TOMLEntry("link_dirs", new TOMLArray(links))
        }));

        std::filesystem::create_directories(env.cache);

        toml->write(compilerPath);
    }

    env.compilerPath = toml->get("compiler_path")->string().require("Invalid build cache.");
    env.linkerPath = toml->get("linker_path")->string().require("Invalid build cache.");

    for (const TOMLValue* path : toml->get("include_dirs")->array().require("Invalid build cache."))
    {
        env.includeDirs.insert(path->string().require("Invalid build cache."));
    }

    for (const TOMLValue* path : toml->get("link_dirs")->array().require("Invalid build cache."))
    {
        env.libDirs.insert(path->string().require("Invalid build cache."));
    }
}

#else

#include <pwd.h>
#include <unistd.h>

std::filesystem::path BuildEnvironment::homePath()
{
    const passwd* pw = getpwuid(getuid());

    if (!pw)
    {
        throw RadialFileException("Failed to locate home directory.");
    }

    return pw->pw_dir;
}

void BuildEnvironment::findCompiler(BuildEnvironment& env)
{
    env.compilerPath = "g++";
    env.linkerPath = "g++";
}

#endif

void BuildEnvironment::findIncludes(const BuildEnvironment& env, const std::filesystem::path& path, PathSet& includes)
{
    const std::vector<std::string> lines = Utils::readLines(path);

    for (const std::string& line : lines)
    {
        const size_t match = line.find("#include");

        if (match == std::string::npos)
        {
            continue;
        }

        const size_t start = line.find('\"', match + 9);

        if (start == std::string::npos)
        {
            continue;
        }

        const size_t end = line.find('\"', start + 1);

        if (end == std::string::npos)
        {
            continue;
        }

        const std::string name = line.substr(start + 1, end - start - 1);

        const std::filesystem::path localPath = path.parent_path() / name;

        if (std::filesystem::is_regular_file(localPath))
        {
            if (!includes.count(localPath))
            {
                includes.insert(localPath);

                findIncludes(env, localPath, includes);
            }

            continue;
        }

        for (const std::filesystem::path& dir : env.includeDirs)
        {
            const std::filesystem::path includePath = std::filesystem::weakly_canonical(dir / name);

            if (std::filesystem::is_regular_file(includePath))
            {
                if (!includes.count(includePath))
                {
                    includes.insert(includePath);

                    findIncludes(env, includePath, includes);
                }

                break;
            }
        }
    }
}
