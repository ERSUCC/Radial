#pragma once

#include <memory>

#include "config.h"
#include "options.h"
#include "path.h"
#include "toml.h"
#include "utils.h"

struct Clean
{
    static void run(const CleanOptions* options);
};
