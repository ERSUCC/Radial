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

BuildEnvironment BuildEnvironment::create(const std::filesystem::path& root)
{
    std::unique_ptr<const TOML> config(Config::readConfig(root));

    const std::string name = config->get("name")->string().require("No project name specified.");

    const int stdVersion = config->get("std_version")->integer().require("No C++ standard version specified.");

    const std::filesystem::path dest = root / config->get("out_dir")->string().get("build");

    std::filesystem::create_directories(dest);

    BuildEnvironment env = BuildEnvironment(std::move(config), name, stdVersion, dest);

    findCompiler(env);

    for (const TOMLValue* include : env.config->get("include_dirs")->array().get(std::vector<const TOMLValue*>()))
    {
        const std::string value = include->string().require("`include_dirs` must be an array of strings.");

        if (!value.empty() && std::filesystem::is_directory(root / value))
        {
            env.includeDirs.insert(root / value);
        }
    }

    for (const TOMLValue* dir : env.config->get("source_dirs")->array().require("Configuration does not specify any sources."))
    {
        const std::string value = dir->string().require("`source_dirs` must be an array of strings.");

        if (!value.empty())
        {
            for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(root / value))
            {
                const std::string name = entry.path().filename().string();

                if (entry.is_regular_file())
                {
                    env.sources.insert(entry.path());
                }
            }
        }
    }

    return env;
}

BuildEnvironment::BuildEnvironment(std::unique_ptr<const TOML> config, const std::string& name, const int stdVersion, const std::filesystem::path& dest) :
    config(std::move(config)), name(name), stdVersion(stdVersion), dest(dest), cache(dest / ".cache") {}

#ifdef _WIN32

#include <Windows.h>

void BuildEnvironment::findCompiler(BuildEnvironment& env)
{
    const std::filesystem::path compilerPath = env.cache / "compiler-location.txt";
    const std::filesystem::path linkerPath = env.cache / "linker-location.txt";
    const std::filesystem::path includePaths = env.cache / "include-paths.txt";
    const std::filesystem::path libPaths = env.cache / "lib-paths.txt";

    const bool compilerExists = std::filesystem::exists(compilerPath);
    const bool linkerExists = std::filesystem::exists(linkerPath);
    const bool includeExists = std::filesystem::exists(includePaths);
    const bool libExists = std::filesystem::exists(libPaths);

    if (!compilerExists || !linkerExists || !includeExists || !libExists)
    {
        std::string cmd = "cmd /v:on /c \"vcvars64.bat";

        if (!compilerExists)
        {
            cmd += " && where cl.exe > \"" + compilerPath.string() + "\"";
        }

        if (!linkerExists)
        {
            cmd += " && where link.exe > \"" + linkerPath.string() + "\"";
        }

        if (!includeExists)
        {
            cmd += " && echo !INCLUDE! > \"" + includePaths.string() + "\"";
        }

        if (!libExists)
        {
            cmd += " && echo !LIB! > \"" + libPaths.string() + "\"";
        }

        cmd += "\"";

        std::filesystem::create_directories(env.cache);

        try
        {
            if (Process::run(cmd, false))
            {
                throw RadialException("Failed to find MSVC. Make sure you have the Visual Studio development tools installed.");
            }
        }

        catch (const RadialException& ex)
        {
            throw RadialException("Failed to find MSVC. Make sure you have the Visual Studio development tools installed.");
        }
    }

    env.compilerPath = Utils::trim(Utils::readFile(compilerPath));
    env.linkerPath = Utils::trim(Utils::readFile(linkerPath));

    const std::string includes = Utils::readFile(includePaths);

    char* data = (char*)malloc(sizeof(char) * (includes.size() + 1));

    strncpy(data, includes.c_str(), includes.size() + 1);

    char* token = strtok(data, ";");

    do
    {
        env.includeDirs.insert(Utils::trim(token));
    } while (token = strtok(nullptr, ";"));

    free(data);

    const std::string libs = Utils::readFile(libPaths);

    data = (char*)malloc(sizeof(char) * (libs.size() + 1));

    strncpy(data, libs.c_str(), libs.size() + 1);

    token = strtok(data, ";");

    do
    {
        env.libDirs.insert(Utils::trim(token));
    } while (token = strtok(nullptr, ";"));

    free(data);
}

#else

void BuildEnvironment::findCompiler(BuildEnvironment& env)
{
    env.compilerPath = "g++";
    env.linkerPath = "g++";
}

#endif
