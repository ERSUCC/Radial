#include "build.h"

void Build::run(const BuildOptions* options)
{
    const std::filesystem::path root = std::filesystem::weakly_canonical(options->root.value_or(""));

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

    link(env, env.dest / env.name);
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
