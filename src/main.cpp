#include <iostream>

#include "build.h"
#include "exception.h"
#include "options.h"
#include "utils.h"

int main(int argc, char** argv)
{
    try
    {
        const ProgramOptions options = ProgramOptions::parse(argv + 1, argc - 1);

        if (options.version)
        {
            std::cout << "Radial v" << RADIAL_VERSION << "\n";

            return 0;
        }

        switch (options.commandOptions.value()->command)
        {
            case CommandType::Build:
            {
                Build::run(dynamic_cast<const BuildOptions*>(options.commandOptions.value()));

                break;
            }
        }
    }

    catch (const RadialUsageException& ex)
    {
        std::cout << ex.what() << "\n";

        return 1;
    }

    catch (const RadialException& ex)
    {
        Utils::error(ex.what());

        return 1;
    }

    return 0;
}
