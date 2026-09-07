#include "build.h"

BuildEnvironment::BuildEnvironment(const std::string& name, const int stdVersion, const std::filesystem::path& dest) :
    name(name), stdVersion(stdVersion), dest(dest) {}

void Build::run(const BuildOptions* options)
{
    const std::filesystem::path root = std::filesystem::weakly_canonical(options->root.value_or(""));

    if (!std::filesystem::exists(root))
    {
        throw RadialFileException("The specified project directory does not exist.");
    }

    if (!std::filesystem::is_directory(root))
    {
        throw RadialFileException("The specified project path is not a directory.");
    }

    const std::filesystem::path configPath = root / "build.toml";

    if (!std::filesystem::exists(configPath) || !std::filesystem::is_regular_file(configPath))
    {
        throw RadialFileException("The specified project directory does not contain a build.toml file.");
    }

    Utils::info("Loading configuration for project in \"" + root.string() + "\"");

    const std::unique_ptr<TOML> config(TOML::parse(configPath));

    const std::string radialVersion = config->get("radial_version")->string().get(RADIAL_VERSION);

    if (Utils::numericVersion(radialVersion) > Utils::numericVersion(RADIAL_VERSION))
    {
        Utils::warning("Current version is " + std::string(RADIAL_VERSION) + ", but configuration file specifies " + radialVersion + ". Some configuration features may not work properly.");
    }

    const std::string name = config->get("name")->string().require("No project name specified.");

    const int stdVersion = config->get("std_version")->integer().require("No C++ standard version specified.");

    const std::filesystem::path dest = root / config->get("out_dir")->string().get("build");

    std::filesystem::create_directories(dest);

    Utils::info("Building project " + name);

    BuildEnvironment env = BuildEnvironment(name, stdVersion, dest);

    for (const TOMLValue* include : config->get("include_dirs")->array().get(std::vector<const TOMLValue*>()))
    {
        const std::string value = include->string().require("`include_dirs` must be an array of strings.");

        if (!value.empty())
        {
            env.includes.insert(root / value);
        }
    }

    for (const TOMLValue* dir : config->get("source_dirs")->array().require("Configuration does not specify any sources."))
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

    for (const std::filesystem::path& source : env.sources)
    {
        compile(env, source);
    }

    link(env, dest / name);
}

void Build::compile(BuildEnvironment& env, const std::filesystem::path& file)
{
    const std::string name = file.filename().string();

    const std::filesystem::path object = env.dest / (name + ".o");

    env.objects.insert(object);

    if (std::filesystem::exists(object) && std::filesystem::last_write_time(file) <= Cache::readTime(env, file))
    {
        return;
    }

    Cache::writeTime(env, file);

    Utils::info("Compiling " + name);

    std::string cmd = "g++ -std=c++" + std::to_string(env.stdVersion) + " -c";

    if (!env.includes.empty())
    {
        cmd += " -I";

        for (const std::filesystem::path& include : env.includes)
        {
            cmd += " \"" + include.string() + "\"";
        }
    }

    cmd += " -o \"" + object.string() + "\"";
    cmd += " \"" + file.string() + "\"";

    runProc(cmd);
}

void Build::link(BuildEnvironment& env, const std::filesystem::path& file)
{
    Utils::info("Linking " + file.filename().string());

    std::string cmd = "g++ -std=c++" + std::to_string(env.stdVersion) + " -o \"" + file.string() + "\"";

    for (const std::filesystem::path& object : env.objects)
    {
        cmd += " \"" + object.string() + "\"";
    }

    runProc(cmd);
}

void Build::runProc(const std::string& cmd)
{
    FILE* proc = popen(cmd.c_str(), "r");

    if (!proc)
    {
        throw RadialException("Failed to create compiler subprocess.");
    }

    if (const int code = pclose(proc))
    {
        throw RadialException("Compiler exited with non-zero exit code: " + std::to_string(code));
    }
}

std::filesystem::file_time_type Cache::readTime(const BuildEnvironment& env, const std::filesystem::path& file)
{
    std::ifstream cache(cachePath(env, file));

    if (!cache.is_open())
    {
        return {};
    }

    char data[256];

    cache.read(data, 256);

    data[cache.gcount()] = '\0';

    cache.close();

    return std::filesystem::file_time_type(std::filesystem::file_time_type::duration(atoll(data)));
}

void Cache::writeTime(const BuildEnvironment& env, const std::filesystem::path& file)
{
    const std::filesystem::path path = cachePath(env, file);

    std::filesystem::create_directories(path.parent_path());

    std::ofstream cache(path);

    if (cache.is_open())
    {
        cache << (size_t)std::filesystem::file_time_type::clock::now().time_since_epoch().count() << "\n";
    }

    cache.close();
}

std::filesystem::path Cache::cachePath(const BuildEnvironment& env, const std::filesystem::path& file)
{
    return env.dest / ".cache" / (file.filename().string() + ".txt");
}
