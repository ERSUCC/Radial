#include "config.h"

std::unique_ptr<const TOML> Config::readConfig(const Path& root)
{
    if (!root.exists())
    {
        throw RadialFileException("The specified project directory does not exist.");
    }

    if (!root.isDirectory())
    {
        throw RadialFileException("The specified project path is not a directory.");
    }

    const Path configPath = root / "build.toml";

    if (!configPath.isFile())
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

    const Path root = Path::absolute(options->root.value_or("."));

    std::unique_ptr<const TOML> config(Config::readConfig(root));

    const std::string name = config->get("name")->string().require("No project name specified.");

    const int stdVersion = config->get("std_version")->integer().require("No C++ standard version specified.");

    const Path dest = root / config->get("out_dir")->string().get("build");

    dest.createDirectories();

    BuildEnvironment env = BuildEnvironment(options, std::move(config), name, stdVersion, root, dest);

    findCompiler(env);

    for (const TOMLValue* include : global->get("include_dirs")->array().get({}))
    {
        const std::string value = Utils::trim(include->string().get(""));

        if (!value.empty())
        {
            const Path path(value);

            if (path.isAbsolute() && path.isDirectory())
            {
                env.includeDirs.insert(path);
            }
        }
    }

    for (const TOMLValue* include : env.config->get("include_dirs")->array().get({}))
    {
        const std::string value = Utils::trim(include->string().require("`include_dirs` must be an array of strings."));

        if (!value.empty() && (root / value).isDirectory())
        {
            env.includeDirs.insert(root / value);
            env.includeDirsLocal.insert(root / value);
        }
    }

    for (const TOMLValue* path : env.config->get("source_paths")->array().require("Configuration does not specify any sources."))
    {
        const std::string value = Utils::trim(path->string().require("`source_paths` must be an array of strings."));

        if (!value.empty())
        {
            const Path resolved = root / value;

            if (resolved.isDirectory())
            {
                for (const Path& path : resolved.children(true))
                {
                    if (path.extension() == ".cpp")
                    {
                        env.sources.insert(path);
                    }
                }
            }

            else if (resolved.isFile() && resolved.extension() == ".cpp")
            {
                env.sources.insert(resolved);
            }
        }
    }

    PathMap visited;

    for (const Path& path : env.sources)
    {
        findIncludes(env, path, env.includes[path], visited);
    }

    for (const TOMLValue* dir : global->get("link_dirs")->array().get({}))
    {
        const std::string value = Utils::trim(dir->string().get(""));

        if (!value.empty())
        {
            const Path path(value);

            if (path.isAbsolute() && path.isDirectory())
            {
                env.libDirs.insert(path);
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

BuildEnvironment::BuildEnvironment(const BuildOptions* options, std::unique_ptr<const TOML> config, const std::string& name, const int stdVersion, const Path& root, const Path& dest) :
    options(options), config(std::move(config)), name(name), stdVersion(stdVersion), root(root), dest(dest), cache(dest / ".cache") {}

#ifdef _WIN32

#include <wchar.h>

#include <Windows.h>
#include <objbase.h>
#include <ShlObj.h>

Path BuildEnvironment::homePath()
{
    wchar_t* wpath;

    if (FAILED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &wpath)))
    {
        CoTaskMemFree(wpath);

        throw RadialFileException("Failed to locate home directory.");
    }

    const Path path = Path::absolute(wpath);

    CoTaskMemFree(wpath);

    return path;
}

void BuildEnvironment::findCompiler(BuildEnvironment& env)
{
    std::unique_ptr<const TOML> toml;

    const Path compilerPath = env.cache / "compiler.toml";

    if (compilerPath.exists())
    {
        toml.reset(TOML::parse(compilerPath));
    }

    else
    {
        const Path temp = Path::temp("radial_compiler_info.txt");

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

        const std::vector<std::string> sections = Utils::split(temp.read(), "\n");

        temp.remove();

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

        env.cache.createDirectories();

        toml->write(compilerPath);
    }

    env.compilerPath = toml->get("compiler_path")->string().require("Invalid build cache.");
    env.linkerPath = toml->get("linker_path")->string().require("Invalid build cache.");

    for (const TOMLValue* path : toml->get("include_dirs")->array().require("Invalid build cache."))
    {
        env.includeDirs.insert(Path::absolute(path->string().require("Invalid build cache.")));
    }

    for (const TOMLValue* path : toml->get("link_dirs")->array().require("Invalid build cache."))
    {
        env.libDirs.insert(Path::absolute(path->string().require("Invalid build cache.")));
    }
}

#else

#include <pwd.h>
#include <unistd.h>

Path BuildEnvironment::homePath()
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

void BuildEnvironment::findIncludes(const BuildEnvironment& env, const Path& path, PathSet& includes, PathMap& visited)
{
    if (visited.count(path))
    {
        for (const Path& include : visited[path])
        {
            includes.insert(include);

            findIncludes(env, include, includes, visited);
        }

        return;
    }

    visited[path] = PathSet();

    for (const std::string& line : path.readLines())
    {
        const std::optional<std::string> name = includeName(line);

        if (!name)
        {
            continue;
        }

        const Path localPath = (path.parent() / name.value()).absolute();

        if (localPath.isFile())
        {
            includes.insert(localPath);
            visited[path].insert(localPath);

            findIncludes(env, localPath, includes, visited);

            continue;
        }

        for (const Path& dir : env.includeDirsLocal)
        {
            const Path includePath = (dir / name.value()).absolute();

            if (includePath.isFile() && env.root.contains(includePath))
            {
                includes.insert(includePath);
                visited[path].insert(includePath);

                findIncludes(env, includePath, includes, visited);

                break;
            }
        }
    }
}

std::optional<std::string> BuildEnvironment::includeName(const std::string& str)
{
    const size_t match = str.find("#include");

    if (match == std::string::npos)
    {
        return std::nullopt;
    }

    size_t start = str.find('"', match + 9);

    if (start != std::string::npos)
    {
        const size_t end = str.find('"', start + 1);

        if (end == std::string::npos)
        {
            return std::nullopt;
        }

        return str.substr(start + 1, end - start - 1);
    }

    start = str.find('<', match + 9);

    if (start == std::string::npos)
    {
        return std::nullopt;
    }

    const size_t end = str.find('>', start + 1);

    if (end == std::string::npos)
    {
        return std::nullopt;
    }

    return str.substr(start + 1, end - start - 1);
}
