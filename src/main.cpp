#include <iostream>
#include <array>
#include <functional>
#include <thread>

#include <csignal>
#include <unistd.h>

#include <CLI11.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <indicators/block_progress_bar.hpp>
#include <indicators/dynamic_progress.hpp>

#include "ConfigReader.h"
#include "xia_interface.h"

#ifndef PRESET_MAX_MODULES
#define PRESET_MAX_MODULES 24
#endif // PRESET_MAX_MODULES

bool end_run = false;

void HandleSignal(int signal)
{
    if (signal == SIGTERM || signal == SIGINT) {
        end_run = true;
    }
}

int IndividualRun(int period, int num_mod, int run_no,
        const char *scaler_name)
{
    // Now we can start the run
    if ( !StartXIA(period, num_mod) ){
        spdlog::error("Unable to start acquisition");
        return 13;
    }
    std::cout << "Started new run #" << run_no << std::endl;
    indicators::BlockProgressBar bar{
            indicators::option::BarWidth{50},
            indicators::option::Start{"["},
            indicators::option::End{"]"},
            indicators::option::ForegroundColor{indicators::Color::white},
            indicators::option::ShowElapsedTime{true},
            indicators::option::ShowRemainingTime{true}
    };
    bar.set_option(indicators::option::PrefixText{"Run #"+std::to_string(run_no) + ": "});
    //bars.push_back(bar);
    auto end_time = std::chrono::system_clock::now() + std::chrono::seconds(period);
    bool errorflag = false;
    while ( std::chrono::system_clock::now() < end_time && !end_run ){
        auto timediff = end_time - std::chrono::system_clock::now();
        float progress = 1 - float(std::chrono::duration_cast<std::chrono::seconds>(timediff).count())/std::chrono::seconds(period).count();
        bar.set_progress(progress*100.0);

        if ( !LogScalers(num_mod, scaler_name) ){
            return 15;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1)); // Max check rate is 1 second.
    }

    // XIA should be done by now...
    if ( !StopXIA(num_mod) ){
        return 20;
    }

    while ( XIAIsRunning(num_mod, errorflag) ){
        if ( errorflag ){
            return 14;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Max check rate is 1 second.
    }

    bar.set_progress(100.0);
    bar.mark_as_completed();
    return 0;
}

int Run(int period, int num_mod, int times, const char *scaler_name, const char *hist_path)
{
    int now = 0;
    // Setup progressbar
    //indicators::DynamicProgress<indicators::BlockProgressBar> bars;
    std::function<bool(int)> end_condition = [&times](const int now) -> bool {
        return ( times == 0 ) ? true : now < times;
    };
    while ( end_condition(now) && !end_run ){
        auto ret = IndividualRun(period, num_mod, now, scaler_name);
        if ( ret != 0 )
            return ret;
        // Write histograms to file
        ret = WriteHistogram(num_mod, now, hist_path);
        if ( ret != 0 )
            return ret;
    }
    return 0;
}


int main(int argc, char *argv[])
{
    std::string period;
    int period_int;
    int times;
    std::string firmware_file = "XIA_Firmware.txt";
    std::string config_file = "settings.set";
    std::string output_path = "/dev/null/";
    bool detach = false;
    unsigned short num_mod;
    std::array<unsigned short, PRESET_MAX_MODULES> plxMap{};
    for ( size_t i = 0 ; i < PRESET_MAX_MODULES ; ++i ){
        plxMap[i] = ushort(i) + 2;
    }

    auto file_logger = spdlog::basic_logger_mt("logger", "log.txt");
    spdlog::set_default_logger(file_logger);

    CLI::App app{"HistMaker - a small tool to periodically readout histograms from XIA Pixie-16 modules"};

    app.add_option("-p,--period", period, "Time between readout in minutes")->required();
    app.add_option("-f,--firmware", firmware_file,
            "Configuration map of the firmware files")->default_str("XIA_Firmware.txt");
    app.add_option("-c,--configuration", config_file,
                   "DSP configuration file")->default_str("settings.set");
    app.add_option("-m,--modules", num_mod, "Number of modules in DAQ")->required();
    app.add_option("-o,--output", output_path, "Path where the histograms are dumped")->default_str("/dev/null");
    app.add_option("-t,--times", times, "Number of runs to do. If zero, will run forever");
    app.add_flag("-d,--detach", detach, "Indicate if the program should be detached from the terminal session");
    //app.add_option("--plxmap", plxMap, "PLX slot mapping")->default_val(plxMap);

    CLI11_PARSE(app, argc, argv);

    if ( output_path.back() != '/' )
        output_path.push_back('/');

    // We will determine the duration in number of seconds
    if ( period.back() == 's' ){
        period_int = std::chrono::seconds(std::stoi(std::string(std::begin(period), std::end(period)-1))).count();
    } else if ( period.back() == 'm' ){
        period_int = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::minutes(
                        std::stoi(std::string(std::begin(period), std::end(period)-1)))).count();
    } else if ( period.back() == 'h' ){
        period_int = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::hours(
                        std::stoi(std::string(std::begin(period), std::end(period)-1)))).count();
    } else {
        spdlog::error("Error: unsupported duration");
        std::cerr << "Error: unsupported duration" << std::endl;
        return 10;
    }

    // First we will read the firmware mapping
    auto fmap = ReadConfigFile(firmware_file.c_str());
    if ( fmap.empty() ){
        spdlog::error("No firmwares found in firmware config file");
        std::cerr << "No firmwares found in firmware config file" << std::endl;
        return 11;
    }

    // Boot the modules
    if ( !BootXIA(fmap, config_file.c_str(), num_mod, plxMap.data()) ){
        spdlog::error("Unable to boot modules");
        if ( !Exit(num_mod) ){
            return 16;
        }
        return 12;
    }

    // Next we will ensure that all channels have the histogram bit set
    if ( !EnableHistMode(num_mod) ){
        spdlog::error("Unable to enable histogramming");
        if ( !Exit(num_mod) ){
            return 16;
        }
        return 13;
    }

    // This is the point where we will detach if required.
    /*if ( detach ){
        pid_t proc_id = 0;
        pid_t sess_id = 0;

        // Fork the processes
        proc_id = fork();
        if ( proc_id < 0 ){
            spdlog::error("Unable to fork()");
            return 14;
        }

        // Need to kill parent process
        if ( proc_id > 0 ){
            exit(EXIT_SUCCESS);
        }

        // Detach stdin, stdout, stderr
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

    }*/
    int res = Run(period_int, num_mod, times, "XIA_scalers.csv", output_path.c_str());
    if ( !Exit(num_mod) ){
        return 16;
    }
    return res;
}
