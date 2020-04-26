//
// Created by Vetle Wegner Ingeberg on 24/04/2020.
//

#include "xia_interface.h"

//#include <xia/pixie16app_export.h>
#include <pixie16app_export.h>

#include <iostream>
#include <chrono>

#include <spdlog/spdlog.h>

// Define some addresses...
#define LIVETIMEA_ADDRESS 0x0004a37f
#define LIVETIMEB_ADDRESS 0x0004a38f
#define FASTPEAKSA_ADDRESS 0x0004a39f
#define FASTPEAKSB_ADDRESS 0x0004a3af
#define RUNTIMEA_ADDRESS 0x0004a342
#define RUNTIMEB_ADDRESS 0x0004a343
#define CHANEVENTSA_ADDRESS 0x0004a41f
#define CHANEVENTSB_ADDRESS 0x0004a42f

static unsigned int last_stat[PRESET_MAX_MODULES][448];
static int timestamp_factor[PRESET_MAX_MODULES];

bool GetFirmwareFile(firmware_map &fmap,
        const unsigned short &revision, const unsigned short &ADCbits, const unsigned short &ADCMSPS,
                     char *ComFPGA, char *SPFPGA, char *DSPcode, char *DSPVar)
{
    std::string key_Com, key_SPFPGA, key_DSPcode, key_DSPVar;

    // First, if Rev 11, 12 or 13.
    if ( (revision == 11 || revision == 12 || revision == 13) ){

        // We set the keys.
        key_Com = "comFPGAConfigFile_RevBCD";
        key_SPFPGA = "SPFPGAConfigFile_RevBCD";
        key_DSPcode = "DSPCodeFile_RevBCD";
        key_DSPVar = "DSPVarFile_RevBCD";

    } else if ( revision == 15 ){

        key_Com = "comFPGAConfigFile_RevF_" + std::to_string(ADCMSPS) + "MHz_" + std::to_string(ADCbits) + "Bit";
        key_SPFPGA = "SPFPGAConfigFile_RevF_" + std::to_string(ADCMSPS) + "MHz_" + std::to_string(ADCbits) + "Bit";
        key_DSPcode = "DSPCodeFile_RevF_" + std::to_string(ADCMSPS) + "MHz_" + std::to_string(ADCbits) + "Bit";
        key_DSPVar = "DSPVarFile_RevF_" + std::to_string(ADCMSPS) + "MHz_" + std::to_string(ADCbits) + "Bit";

    } else {
        spdlog::error("Unknown Pixie-16 revision, rev="+std::to_string(revision));
        std::cerr << "Unknown Pixie-16 revision, rev=" << revision << std::endl;
        return false;
    }

    // Search our map for the firmware files.
    if ( fmap.find(key_Com) == fmap.end() ){
        spdlog::error("Unable to locate firmware file '"+key_Com+"'");
        std::cerr << "Missing firmware file '" << key_Com << "'" << std::endl;
        return false;
    }

    if ( fmap.find(key_SPFPGA) == fmap.end() ){
        spdlog::error("Unable to locate firmware file '"+key_SPFPGA+"'");
        std::cerr << "Missing firmware file '" << key_SPFPGA << "'" << std::endl;
        return false;
    }

    if ( fmap.find(key_DSPcode) == fmap.end() ){
        spdlog::error("Unable to locate firmware file '"+key_DSPcode+"'");
        std::cerr << "Missing firmware file '" << key_DSPcode << "'" << std::endl;
        return false;
    }

    if ( fmap.find(key_DSPVar) == fmap.end() ){
        spdlog::error("Unable to locate firmware file '"+key_DSPVar+"'");
        std::cerr << "Missing firmware file '" << key_DSPVar << "'" << std::endl;
        return false;
    }

    // If we reach this point, we know that we have all the firmwares!
    strcpy(ComFPGA, fmap[key_Com].c_str());
    strcpy(SPFPGA, fmap[key_SPFPGA].c_str());
    strcpy(DSPcode, fmap[key_DSPcode].c_str());
    strcpy(DSPVar, fmap[key_DSPVar].c_str());

    return true;
}

bool BootXIA(firmware_map &map, const char *dsp_settings_file, const unsigned short &num_mod, unsigned short *plxMap)
{
    char ComFPGA[2048], SPFPGA[2048], DSPCode[2048], DSPVar[2048];
    char TrigFPGA[] = "trig";
    char DSPSet[2048];
    strcpy(DSPSet, dsp_settings_file);

    unsigned short rev[PRESET_MAX_MODULES], bit[PRESET_MAX_MODULES], MHz[PRESET_MAX_MODULES];
    unsigned int sn[PRESET_MAX_MODULES];

    unsigned short plx_map[PRESET_MAX_MODULES];
    for ( int i = 0 ; i < num_mod ; ++i ){
        plx_map[i] = i + 2;
    }

    int retval = Pixie16InitSystem(num_mod, plx_map, 0);

    if ( retval < 0 ){
        spdlog::error("*ERROR* Pixie16InitSystem failed, retval = " + std::to_string(retval));
        std::cerr << "*ERROR* Pixie16InitSystem failed, retval = " << retval << std::endl;
        return false;
    }

    std::cout << "Reading XIA hardware information" << std::endl;
    for ( int i = 0 ; i < num_mod ; ++i ){
        retval = Pixie16ReadModuleInfo(i, &rev[i], &sn[i], &bit[i], &MHz[i]);
        if ( retval < 0 ){
            spdlog::error("*ERROR* Pixie16ReadModuleInfo failed, retval = " + std::to_string(retval));
            std::cerr << "*ERROR* Pixie16ReadModuleInfo failed, retval = " << retval << std::endl;
            return false;
        }
        if ( MHz[i] == 100 || MHz[i] == 500 )
            timestamp_factor[i] = 10;
        else
            timestamp_factor[i] = 8;
    }

    for ( int i = 0 ; i < num_mod ; ++i ){
        spdlog::info("Booting Pixie-16 module #"+std::to_string(i)+", Rev="+std::to_string(rev[i])+
        ", S/N="+std::to_string(sn[i])+", Bits="+std::to_string(bit[i])+", MSPS="+std::to_string(MHz[i]));
        std::cout << "Booting Pixie-16 module #" << i << ", Rev=" << rev[i] << ", S/N=" << sn[i] << ", Bits=" << bit[i];
        std::cout << ", MSPS=" << MHz[i];
        if ( !GetFirmwareFile(map, rev[i], bit[i], MHz[i], ComFPGA, SPFPGA, DSPCode, DSPVar) ){
            std::cerr << ": Firmware not found!" << std::endl;
            return false;
        } else {
            std::cout << std::endl;
        }

        std::cout << "ComFPGAConfigFile: " << ComFPGA << std::endl;
        std::cout << "SPFPGAConfigFile: " << SPFPGA << std::endl;
        std::cout << "DSPCodeFile: " << DSPCode << std::endl;
        std::cout << "DSPVarFile: " << DSPVar << std::endl;
        std::cout << "DSPParFile: " << DSPSet << std::endl;

        retval = Pixie16BootModule(ComFPGA, SPFPGA, TrigFPGA, DSPCode, DSPSet, DSPVar, i, 0x7F);
        if ( retval < 0 ){
            spdlog::error("*ERROR* Pixie16BootModule failed, retval = "+std::to_string(retval));
            std::cerr << "*ERROR* Pixie16BootModule failed, retval = " << retval << std::endl;
            return false;
        }

        std::cout << "Pixie-16 module #" << i << ": Booted\n\n" << std::endl;
    }

    std::cout << "All modules booted" << std::endl;
    std::cout << "DSPParFile: " << DSPSet << std::endl;
    return true;
}

//! Ensure that the histogramming register flag are set
//! \param num_modules total number of modules connected
//! \return true if successful, false otherwise
bool EnableHistMode(int num_modules)
{
    int retval;
    unsigned long CSR;
    double value;
    for ( int module = 0 ; module < num_modules ; ++module ){
        for ( int channel = 0 ; channel < NUMBER_OF_CHANNELS ; ++channel ) {
            // Read CSR
            retval = Pixie16ReadSglChanPar("CHANNEL_CSRA", &value, module, channel);
            if (retval < 0) {
                spdlog::error("*ERROR* Pixie16ReadSglChanPar failed retval = " + std::to_string(retval));
                std::cerr << "*ERROR* Pixie16ReadSglChanPar failed retval = " << retval << std::endl;
                return false;
            }
            CSR = value;
            // Enabling the histogram bit
            APP32_SetBit(CCSRA_HISTOE, CSR);

            // Write CSR
            retval = Pixie16WriteSglChanPar("CHANNEL_CSRA", CSR, module, channel);
            if (retval < 0) {
                spdlog::error("*ERROR* Pixie16WriteSglChanPar failed, retval = " + std::to_string(retval));
                std::cerr << "*ERROR* Pixie16WriteSglChanPar failed, retval = " << retval << std::endl;
                return false;
            }
        }
        // Write IN_SYNCH
        retval = Pixie16WriteSglModPar("IN_SYNCH", 1, module);
        if ( retval < 0 ){
            spdlog::error("*ERROR* Pixie16WriteSglChanPar 'IN_SYNCH' failed, retval = " + std::to_string(retval));
            std::cerr << "*ERROR* Pixie16WriteSglChanPar 'IN_SYNCH' failed, retval = " << retval << std::endl;
            return false;
        }
        retval = Pixie16WriteSglModPar("SYNCH_WAIT", 1, module);
        if ( retval < 0 ){
            spdlog::error("*ERROR* Pixie16WriteSglChanPar 'SYNCH_WAIT' failed, retval = " + std::to_string(retval));
            std::cerr << "*ERROR* Pixie16WriteSglChanPar 'SYNCH_WAIT' failed, retval = " << retval << std::endl;
            return false;
        }
    }
    return true;
}

bool StartXIA(int preset_time, int num_modules)
{
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(preset_time)).count();

    // Convert preset run time to IEEE 32-bit floating point number
    auto ieee_preset_run_time = Decimal2IEEEFloating(seconds);

    // Download the preset run time to the DSP
    for (int module = 0 ; module < num_modules ; ++module) {
        auto retval = Pixie16WriteSglModPar("HOST_RT_PRESET", ieee_preset_run_time, module);
        if ( retval < 0 ){
            spdlog::error("*ERROR* Pixie16WriteSglModPar failed, retval = " + std::to_string(retval));
            std::cerr << "*ERROR* Pixie16WriteSglModPar failed, retval = " << retval << std::endl;
            return false;
        }
    }

    // Now we start the actual histogram run!
    auto retval = Pixie16StartHistogramRun(num_modules, NEW_RUN);
    if ( retval < 0 ){
        spdlog::error("*ERROR* Pixie16StartHistogramRun failed, retval = " + std::to_string(retval));
        std::cerr << "*ERROR* Pixie16StartHistogramRun failed, retval = " << retval << std::endl;
        return false;
    }
    return true;
}

bool XIAIsRunning(int num_mod, bool &errorflag){
    bool running = false;
    for ( int module = 0 ; module < num_mod ; ++num_mod ) {
        auto retval = Pixie16CheckRunStatus(module);
        if ( retval == -1 ){
            spdlog::error("*ERROR* Pixie16CheckRunStatus failed, retval = " + std::to_string(retval));
            std::cerr << "*ERROR* Pixie16CheckRunStatus failed, retval = " << retval << std::endl;
            return false;
        } else if ( retval == 1 ){
            running = true;
        } else if ( retval == 0 ){
            continue; // Nothing happens...
        }
    }
    return running;
}

bool LogScalers(int num_mod, const char *file)
{
    double ICR[PRESET_MAX_MODULES][16], OCR[PRESET_MAX_MODULES][16];
    unsigned int stats[448];
    int retval;

    for (int i = 0 ; i < num_mod ; ++i){
        retval = Pixie16ReadStatisticsFromModule(stats, i);
        if (retval < 0){
            spdlog::error("\"*ERROR* Pixie16ReadStatisticsFromModule failed, retval = ", retval);
            std::cerr << "*ERROR* Pixie16ReadStatisticsFromModule failed, retval = " << retval << std::endl;
            return false;
        }

        for (int j = 0 ; j < 16 ; ++j){

            uint64_t fastPeakN = stats[FASTPEAKSA_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            fastPeakN = fastPeakN << 32;
            fastPeakN += stats[FASTPEAKSB_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            uint64_t fastPeakP = last_stat[i][FASTPEAKSA_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            fastPeakP = fastPeakP << 32;
            fastPeakP += last_stat[i][FASTPEAKSB_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            double fastPeak = fastPeakN - fastPeakP;

            uint64_t LiveTimeN = stats[LIVETIMEA_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            LiveTimeN = LiveTimeN << 32;
            LiveTimeN |= stats[LIVETIMEB_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            uint64_t LiveTimeP = last_stat[i][LIVETIMEA_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            LiveTimeP = LiveTimeP << 32;
            LiveTimeP |= last_stat[i][LIVETIMEB_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            double liveTime = LiveTimeN - LiveTimeP;
            if (timestamp_factor[i] == 8)
                liveTime *= 2e-6/250.;
            else
                liveTime *= 1e-6/100.;

            uint64_t ChanEventsN = stats[CHANEVENTSA_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            ChanEventsN = ChanEventsN << 32;
            ChanEventsN |= stats[CHANEVENTSB_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            uint64_t ChanEventsP = last_stat[i][CHANEVENTSA_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            ChanEventsP = ChanEventsP << 32;
            ChanEventsP |= last_stat[i][CHANEVENTSB_ADDRESS + j - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            double ChanEvents = ChanEventsN - ChanEventsP;

            uint64_t runTimeN = stats[RUNTIMEA_ADDRESS - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            runTimeN = runTimeN << 32;
            runTimeN |= stats[RUNTIMEB_ADDRESS - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            uint64_t runTimeP = last_stat[i][RUNTIMEA_ADDRESS - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];
            runTimeP = runTimeP << 32;
            runTimeP |= last_stat[i][RUNTIMEB_ADDRESS - DATA_MEMORY_ADDRESS - DSP_IO_BORDER];

            double runTime = runTimeN - runTimeP;

            runTime *= 1.0e-6 / 100.;

            ICR[i][j] = (liveTime !=0) ? fastPeak/liveTime : 0;
            OCR[i][j] = (runTime != 0) ? ChanEvents/runTime : 0;
        }

        for (int j = 0 ; j < 448 ; ++j){
            last_stat[i][j] = stats[j];
        }
    }
    // Get the current time
    time_t now = time(NULL);
    FILE *scaler_file = fopen(file, "w");

    // Write CSV file
    // Settings to be used in the telegraf configuration
    // that monitors the file:
    // [
    fprintf(scaler_file, "measurement,module,channel,input,output,timestamp");
    for (int i = 0 ; i < num_mod ; ++i){
        for (int j = 0 ; j < 16 ; ++j){
            fprintf(scaler_file, "\ncount_rate,%02d,%02d,%.6f,%.6f,%jd",
                    i, j, ICR[i][j], OCR[i][j], now);
        }
    }
    fclose(scaler_file);
    return true;
}

bool WriteHistogram(int num_mod, int now, const char *path)
{
    for ( int module = 0 ; module < num_mod ; ++module ){
        std::string filename = std::string(path)
                + "/run" + std::to_string(now) + "_mod" + std::to_string(module) + ".bin";
        char *tmp = new char[filename.size()+1];
        sprintf(tmp, "%s", filename.c_str());
        auto retval = Pixie16SaveHistogramToFile(tmp, num_mod);
        if ( retval < 0 ){
            spdlog::error("*ERROR* Pixie16SaveHistogramToFile failed, retval = " + std::to_string(retval));
            std::cerr << "*ERROR* Pixie16SaveHistogramToFile failed, retval = " << std::to_string(retval) << std::endl;
            return false;
        }
    }
    return true;
}

bool Exit(int num_mod)
{
    for ( int module = 0 ; module < num_mod ; ++module ){
        auto retval = Pixie16EndRun(module);
        if ( retval < 0 ){
            spdlog::error("*ERROR* Pixie16SaveHistogramToFile failed, retval = "+std::to_string(retval));
            std::cerr << "*ERROR* Pixie16SaveHistogramToFile failed, retval = " + std::to_string(retval) << std::endl;
            return false;
        }
    }
    auto retval = Pixie16ExitSystem(num_mod);
    if ( retval < 0 ){
        spdlog::error("*ERROR* Pixie16ExitSystem failed, retval = "+std::to_string(retval));
        std::cerr << "*ERROR* Pixie16ExitSystem failed, retval = " + std::to_string(retval) << std::endl;
        return false;
    }
    return true;
}