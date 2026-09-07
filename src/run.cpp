#include "run.h"

void Run::run(const RunOptions* options)
{
    const std::filesystem::path root = std::filesystem::weakly_canonical(options->root.value_or(""));

    BuildEnvironment env = BuildEnvironment::create(root);

    Build::build(env);

    Utils::info("Running " + env.name);

    FILE* proc = popen(("\"" + (env.dest / env.name).string() + "\"").c_str(), "r");

    if (!proc)
    {
        throw RadialException("Failed to create subprocess.");
    }

    char data[1025];

    while (const size_t read = fread(data, sizeof(char), 1024, proc))
    {
        data[read] = '\0';

        std::cout << data;
    }

    Utils::info("Process exited with code " + std::to_string(pclose(proc)));
}
