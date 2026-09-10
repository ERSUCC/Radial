#include "run.h"

void Run::run(const RunOptions* options)
{
    const std::filesystem::path root = std::filesystem::weakly_canonical(options->root.value_or("."));

    BuildEnvironment env = BuildEnvironment::create(root);

    Build::build(env);

    Utils::info("Running " + env.name);

    const int code = Process::run("\"" + (env.dest / env.name).string() + "\"");

    Utils::info("Process exited with code " + std::to_string(code));
}
