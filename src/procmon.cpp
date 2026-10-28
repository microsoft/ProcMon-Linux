/*
    Procmon-for-Linux

    Copyright (c) Microsoft Corporation

    All rights reserved.

    MIT License

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the ""Software""), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <iostream>
#include <cstdlib>
#include <version.h>
#include <unistd.h>

#include "configuration/procmon_configuration.h"
#include "display/screen.h"
#include "display/headless.h"
#include "logging/easylogging++.h"
#include "installer.h"

INITIALIZE_EASYLOGGINGPP

//--------------------------------------------------------------------
//
// main
//
// Main entry point
//
//--------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Make sure user is running elevated
    if(geteuid() != 0)
    {
        std::cout << "Procmon requires elevated credentials. Please run with sudo.\n";
        exit(-1);
    }

    /*
    * Turn off cursor for shell. Note this has to be done before EBPF spins up.
    * This is due to a conflict that leads to the consumer thread dieing when the system
    * function is executed.
    */
    curs_set(0);

    if (!ExtractEBPFPrograms())
    {
        curs_set(1);
        return EXIT_FAILURE;
    }
    // Configuration parsing can exit before normal shutdown.
    if (std::atexit([] { DeleteEBPFPrograms(); }) != 0)
    {
        std::cerr << "Cannot register eBPF cleanup\n";
        DeleteEBPFPrograms();
        curs_set(1);
        return EXIT_FAILURE;
    }

    // Program initialization: create global config from args
    auto config = std::make_shared<ProcmonConfiguration>(argc, argv);

    LOG(DEBUG) << "Tracing " << config->events.size() << " system calls";

    // Configure logging
    if(config->GetDebugTraceFilePath().size() != 0)
    {
        el::Configurations defaultConf;
        defaultConf.setToDefault();

        defaultConf.set(el::Level::Debug, el::ConfigurationType::Format, "%datetime [%level] %msg");
        defaultConf.set(el::Level::Global, el::ConfigurationType::MaxLogFileSize, "5242880");

        defaultConf.set(el::Level::Info, el::ConfigurationType::Filename, config->GetDebugTraceFilePath());
        defaultConf.set(el::Level::Error, el::ConfigurationType::Filename, config->GetDebugTraceFilePath());
        defaultConf.set(el::Level::Debug, el::ConfigurationType::Filename, config->GetDebugTraceFilePath());

        el::Loggers::reconfigureAllLoggers(defaultConf);
    }

    if(config->GetHeadlessMode())
    {
        if(config->GetTraceFilePath().compare("") != 0)
        {
            std::cerr << "Cannot open trace file in headless mode";
            CLIUtils::DisplayUsage(true);
        }

        Headless headlessDisplay;

        // init headless interface
        headlessDisplay.initialize(config);

        // run in headless mode
        headlessDisplay.run();

        // cleanup run
        headlessDisplay.shutdown();
    }
    else
    {
        Screen display;

        // initialize curses UI
        display.initScreen(config);

        // run display
        display.run();

        // shutdown curses UI
        display.shutdownScreen();
    }

    // re-enable cursor before exiting Procmon
    curs_set(1);

    config->GetTracer()->Cancel();
    config.reset();
    if (!DeleteEBPFPrograms())
    {
        return EXIT_FAILURE;
    }

    return 0;
}
