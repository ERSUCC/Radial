#include "build.h"

void Build::run(const BuildOptions* options)
{
    const std::filesystem::path root = std::filesystem::weakly_canonical(options->root.value_or("."));

    BuildEnvironment env = BuildEnvironment::create(root);

    build(options, env);
}

void Build::build(const BuildOptions* options, BuildEnvironment& env)
{
    Utils::info("Building project " + env.name);

    for (const std::filesystem::path& source : env.sources)
    {
        compile(options, env, source);
    }

    link(options, env, env.dest / (env.name + BIN_EXT));
}

void Build::compile(const BuildOptions* options, BuildEnvironment& env, const std::filesystem::path& file)
{
    const std::string name = file.filename().string();

    const std::filesystem::path object = env.dest / (name + OBJ_EXT);

    env.objects.insert(object);

    if (!options->force && std::filesystem::exists(object) && !shouldUpdate(options, env, file, object))
    {
        return;
    }

    Utils::info("Compiling " + name);

    if (const int code = Process::run(compileCommand(options, env, file, object)))
    {
        throw RadialException("Compiler returned non-zero exit code " + std::to_string(code));
    }

    updateCache(options, env, file);
}

void Build::link(const BuildOptions* options, BuildEnvironment& env, const std::filesystem::path& file)
{
    Utils::info("Linking " + file.filename().string());

    if (const int code = Process::run(linkCommand(options, env, file)))
    {
        throw RadialException("Compiler returned non-zero exit code " + std::to_string(code));
    }
}

#ifdef _WIN32

std::string Build::compileCommand(const BuildOptions* options, const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object)
{
    std::string cmd = "\"" + env.compilerPath + "\" /nologo /std:c++" + std::to_string(env.stdVersion) + " /EHsc /c";

    if (options->debug)
    {
        cmd += " /MTd";
    }

    for (const std::filesystem::path& path : env.includeDirs)
    {
        cmd += " /I\"" + path.string() + "\"";
    }

    for (const std::string& key : env.defines.keys())
    {
        cmd += " /D" + key + "=" + Utils::escapeQuotes(env.defines.get(key));
    }

    cmd += " /Fo\"" + object.string() + "\" \"" + file.string() + "\"";

    return cmd;
}

std::string Build::linkCommand(const BuildOptions* options, const BuildEnvironment& env, const std::filesystem::path& file)
{
    std::string cmd = "\"" + env.linkerPath + "\" /nologo /out:\"" + file.string() + "\"";

    if (options->debug)
    {
        cmd += " /debug:full";
    }

    for (const std::filesystem::path& path : env.libDirs)
    {
        cmd += " /libpath:\"" + path.string() + "\"";
    }

    for (const std::string& lib : env.libs)
    {
        cmd += " " + lib;
    }

    for (const std::filesystem::path& object : env.objects)
    {
        cmd += " \"" + object.string() + "\"";
    }

    return cmd;
}

#else

std::string Build::compileCommand(const BuildOptions* options, const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object)
{
    std::string cmd = "\"" + env.compilerPath + "\" -std=c++" + std::to_string(env.stdVersion) + " -c";

    if (options->debug)
    {
        cmd += " -g";
    }

    for (const std::filesystem::path& path : env.includeDirs)
    {
        cmd += " -I \"" + path.string() + "\"";
    }

    for (const std::string& key : env.defines.keys())
    {
        cmd += " -D" + key + "=" + Utils::escapeQuotes(env.defines.get(key));
    }

    cmd += " -o \"" + object.string() + "\" \"" + file.string() + "\"";

    return cmd;
}

std::string Build::linkCommand(const BuildOptions* options, const BuildEnvironment& env, const std::filesystem::path& file)
{
    std::string cmd = "\"" + env.linkerPath + "\" -std=c++" + std::to_string(env.stdVersion) + " -o \"" + file.string() + "\"";

    for (const std::filesystem::path& path : env.libDirs)
    {
        cmd += " -L \"" + path.string() + "\"";
    }

    for (const std::string& lib : env.libs)
    {
        cmd += " -l" + lib;
    }

    for (const std::filesystem::path& object : env.objects)
    {
        cmd += " \"" + object.string() + "\"";
    }

    return cmd;
}

#endif

bool Build::shouldUpdate(const BuildOptions* options, const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object)
{
    if (std::filesystem::last_write_time(file) > std::filesystem::last_write_time(object))
    {
        return true;
    }

    for (const std::filesystem::path& include : env.includes.at(file))
    {
        if (std::filesystem::last_write_time(include) > std::filesystem::last_write_time(object))
        {
            return true;
        }
    }

    const std::filesystem::path cacheFile = env.cache / "source" / (file.filename().string() + ".toml");

    if (!std::filesystem::is_regular_file(cacheFile))
    {
        return true;
    }

    const std::unique_ptr<TOML> toml(TOML::parse(cacheFile));

    if (toml->get("debug")->boolean().get(!options->debug) != options->debug)
    {
        return true;
    }

    const ListMap<std::string, const TOMLValue*> defines = toml->get("defines")->table().get({});

    if (defines.size() != env.defines.size())
    {
        return true;
    }

    for (const std::string& key : defines.keys())
    {
        if (!env.defines.contains(key) || env.defines.get(key) != defines.get(key)->string().get(""))
        {
            return true;
        }
    }

    return false;
}

void Build::updateCache(const BuildOptions* options, const BuildEnvironment& env, const std::filesystem::path& file)
{
    const std::filesystem::path cacheFile = env.cache / "source" / (file.filename().string() + ".toml");

    std::filesystem::create_directories(cacheFile.parent_path());

    std::vector<const TOMLEntry*> entries =
    {
        new TOMLEntry("debug", new TOMLBoolean(options->debug))
    };

    if (!env.defines.empty())
    {
        std::vector<const TOMLEntry*> defines;

        for (const std::string& key : env.defines.keys())
        {
            defines.push_back(new TOMLEntry(key, new TOMLString(env.defines.get(key), true)));
        }

        entries.push_back(new TOMLEntry("defines", new TOMLTable(defines), true));
    }

    const TOML* toml = new TOML(entries);

    toml->write(cacheFile);

    delete toml;
}
