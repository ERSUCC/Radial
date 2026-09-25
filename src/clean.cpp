#include "clean.h"

void Clean::run(const CleanOptions* options)
{
    const std::filesystem::path root = std::filesystem::weakly_canonical(options->root.value_or("."));

    const std::unique_ptr<const TOML> config = Config::readConfig(root);

    Utils::info("Cleaning project " + config->get("name")->string().get("in \"" + root.string() + "\""));

    std::filesystem::remove_all(root / config->get("out_dir")->string().get("build"));
}
