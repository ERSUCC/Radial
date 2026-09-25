#include "run.h"

void Run::run(const RunOptions* options)
{
    BuildEnvironment env = BuildEnvironment::create(options);

    Build::build(env);

    Utils::info("Running " + env.name);

    std::string cmd = "\"" + (env.dest / env.name).string() + "\"";

    for (const std::string& arg : options->args)
    {
        cmd += " \"" + arg + "\"";
    }

    const int code = Process::run(cmd, true);

    Utils::info("Process exited with code " + std::to_string(code));
}
