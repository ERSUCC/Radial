#include "build.h"

void Build::run(const BuildOptions* options)
{
    BuildEnvironment env = BuildEnvironment::create(options);

    build(env);
}

void Build::build(BuildEnvironment& env)
{
    Utils::info("Building project " + env.name);

    for (const std::filesystem::path& source : env.sources)
    {
        compile(env, source);
    }

    link(env, env.dest / (env.name + BIN_EXT));
}

void Build::compile(BuildEnvironment& env, const std::filesystem::path& file)
{
    const std::string name = file.filename().string();

    const std::filesystem::path object = env.dest / (name + OBJ_EXT);

    env.objects.insert(object);

    if (!env.options->force && std::filesystem::exists(object) && !shouldUpdate(env, file, object))
    {
        return;
    }

    Utils::info("Compiling " + name);

    if (const int code = Process::run(compileCommand(env, file, object), false))
    {
        throw RadialException("Compiler returned non-zero exit code " + std::to_string(code));
    }

    updateCache(env, file);
}

void Build::link(BuildEnvironment& env, const std::filesystem::path& file)
{
    Utils::info("Linking " + file.filename().string());

    if (const int code = Process::run(linkCommand(env, file), false))
    {
        throw RadialException("Compiler returned non-zero exit code " + std::to_string(code));
    }
}

#ifdef _WIN32

std::string Build::compileCommand(const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object)
{
    std::string cmd = "\"" + env.compilerPath + "\" /nologo /std:c++" + std::to_string(env.stdVersion) + " /EHsc /c";

    if (env.options->debug)
    {
        cmd += " /MTd";
    }

    for (const std::filesystem::path& path : env.includeDirs)
    {
        cmd += " /I\"" + path.string() + "\"";
    }

    for (const std::string& key : env.defines.keys())
    {
        cmd += " /D" + key + '=' + Utils::escapeQuotes(env.defines.get(key));
    }

    cmd += " /Fo\"" + object.string() + "\" \"" + file.string() + "\"";

    return cmd;
}

std::string Build::linkCommand(const BuildEnvironment& env, const std::filesystem::path& file)
{
    std::string cmd = "\"" + env.linkerPath + "\" /nologo /out:\"" + file.string() + "\"";

    if (env.options->debug)
    {
        cmd += " /debug:full";
    }

    for (const std::filesystem::path& path : env.libDirs)
    {
        cmd += " /libpath:\"" + path.string() + "\"";
    }

    for (const std::string& lib : env.libs)
    {
        cmd += ' ' + Utils::ensureSuffix(lib, ".lib");
    }

    for (const std::filesystem::path& object : env.objects)
    {
        cmd += " \"" + object.string() + '"';
    }

    return cmd;
}

#else

std::string Build::compileCommand(const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object)
{
    std::string cmd = "\"" + env.compilerPath + "\" -std=c++" + std::to_string(env.stdVersion) + " -c";

    if (env.options->debug)
    {
        cmd += " -g";
    }

    for (const std::filesystem::path& path : env.includeDirs)
    {
        cmd += " -I \"" + path.string() + "\"";
    }

    for (const std::string& key : env.defines.keys())
    {
        cmd += " -D" + key + '=' + Utils::escapeQuotes(env.defines.get(key));
    }

    cmd += " -o \"" + object.string() + "\" \"" + file.string() + "\"";

    return cmd;
}

std::string Build::linkCommand(const BuildEnvironment& env, const std::filesystem::path& file)
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

bool Build::shouldUpdate(const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object)
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

    const std::filesystem::path cacheFile = cachePath(env, file);

    if (!std::filesystem::is_regular_file(cacheFile))
    {
        return true;
    }

    const std::unique_ptr<TOML> toml(TOML::parse(cacheFile));

    if (toml->get("debug")->boolean().get(!env.options->debug) != env.options->debug)
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

void Build::updateCache(const BuildEnvironment& env, const std::filesystem::path& file)
{
    const std::filesystem::path cacheFile = cachePath(env, file);

    std::filesystem::create_directories(cacheFile.parent_path());

    std::vector<const TOMLEntry*> entries =
    {
        new TOMLEntry("debug", new TOMLBoolean(env.options->debug))
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

std::filesystem::path Build::cachePath(const BuildEnvironment& env, const std::filesystem::path& file)
{
    const std::filesystem::path subdir = std::filesystem::relative(file, env.root).parent_path();

    return env.cache / "source" / subdir / (file.filename().string() + ".toml");
}
