#include "build.h"

void Build::run(const BuildOptions* options)
{
    const std::filesystem::path root = std::filesystem::weakly_canonical(options->root.value_or("."));

    BuildEnvironment env = BuildEnvironment::create(root);

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

    if (std::filesystem::exists(object) && !shouldUpdate(env, file, object))
    {
        return;
    }

    Utils::info("Compiling " + name);

    if (const int code = Process::run(compileCommand(env, file, object)))
    {
        throw RadialException("Compiler returned non-zero exit code " + std::to_string(code));
    }
}

void Build::link(BuildEnvironment& env, const std::filesystem::path& file)
{
    Utils::info("Linking " + file.filename().string());

    if (const int code = Process::run(linkCommand(env, file)))
    {
        throw RadialException("Compiler returned non-zero exit code " + std::to_string(code));
    }
}

#ifdef _WIN32

std::string Build::compileCommand(const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object)
{
    std::string cmd = "\"" + env.compilerPath + "\" /std:c++" + std::to_string(env.stdVersion) + " /c";

    for (const std::filesystem::path& path : env.includeDirs)
    {
        cmd += " /I\"" + path.string() + "\"";
    }

    for (const std::pair<std::string, std::string>& define : env.defines)
    {
        cmd += " /D" + define.first + "=" + Utils::escapeQuotes(define.second);
    }

    cmd += " /Fo\"" + object.string() + "\" \"" + file.string() + "\"";

    return cmd;
}

std::string Build::linkCommand(const BuildEnvironment& env, const std::filesystem::path& file)
{
    std::string cmd = "\"" + env.linkerPath + "\" /nologo /out:\"" + file.string() + "\"";

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

std::string Build::compileCommand(const BuildEnvironment& env, const std::filesystem::path& file, const std::filesystem::path& object)
{
    std::string cmd = "\"" + env.compilerPath + "\" -std=c++" + std::to_string(env.stdVersion) + " -c";

    for (const std::filesystem::path& path : env.includeDirs)
    {
        cmd += " -I \"" + path.string() + "\"";
    }

    for (const std::pair<std::string, std::string>& define : env.defines)
    {
        cmd += " -D" + define.first + "=" + Utils::escapeQuotes(define.second);
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

    return false;
}
