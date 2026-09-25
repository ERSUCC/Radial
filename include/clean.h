#pragma once

#include <filesystem>
#include <memory>

#include "config.h"
#include "options.h"
#include "toml.h"
#include "utils.h"

struct Clean
{
    static void run(const CleanOptions* options);
};
