#include "clean.h"

void Clean::run(const CleanOptions* options)
{
    const Path root = Path::absolute(options->root.value_or("."));

    const std::unique_ptr<const TOML> config = Config::readConfig(root);

    Utils::info("Cleaning project " + config->get("name")->string().get("in \"" + root.string() + "\""));

    (root / config->get("out_dir")->string().get("build")).remove();
}
