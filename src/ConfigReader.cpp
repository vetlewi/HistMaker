//
// Created by Vetle Wegner Ingeberg on 24/04/2020.
//

#include "ConfigReader.h"

#include <iostream>
#include <fstream>


std::string strip(const std::string& s)
{
    std::string::size_type start = s.find_first_not_of(" \t\r\n");
    if( start==std::string::npos )
        start = 0;

    std::string::size_type stop = s.find_last_not_of(" \t\r\n");
    if( stop==std::string::npos )
        stop = s.size()-1;

    return s.substr(start, stop+1-start);
}

bool next_line(std::istream &in, std::string &line)
{
    line = "";

    std::string tmp;
    while ( std::getline(in, tmp) ){
        size_t ls = tmp.size();
        if ( ls == 0 ){
            break;
        } else if ( tmp[ls-1] != '\\' ){
            line += tmp;
            break;
        } else {
            line += tmp.substr(0, ls-1);
        }
    }
    return in || !line.empty();
}

firmware_map ReadConfigFile(const char *config)
{
    // We expect the file to have the following setup.
    /*
     * # - Indicates a comment
     * \\ - Indicates that the input continues on the next line
     * "comFPGAConfigFile_Rev<R>_<S>MHz_<B>Bit = /path/to/com/syspixie16_xx.bin" - R: Revision, S: ADC freqency and B: ADC bits
     * "SPFPGAConfigFile_Rev<R>_<S>MHz_<B>Bit = /path/to/SPFPGA/fippixie16_xx.bin" - R: Revision, S: ADC freqency and B: ADC bits
     * "DSPCodeFile_Rev<R>_<S>MHz_<B>Bit = /path/to/DSPCode/Pixie16DSP_xx.ldr" - R: Revision, S: ADC freqency and B: ADC bits
     * "DSPVarFile_Rev<R>_<S>MHz_<B>Bit = /path/to/DSPVar/Pixie16DSP_xx.var" - R: Revision, S: ADC freqency and B: ADC bits
     */

    std::ifstream input(config);
    std::string line;

    std::map<std::string, std::string> fw;

    if ( !input.is_open() ){
        std::cerr << "Unable to read firmware config file." << std::endl;
        return firmware_map();
    }

    while ( next_line(input, line) ){
        if ( line.empty() || line[0] == '#' )
            continue; // Ignore empty lines or comments.

        // Search for "=" sign on the line.
        std::string::size_type pos_eq = line.find('=');

        // If not found, write a warning and continue to next line.
        if ( pos_eq == std::string::npos ){
            std::cerr << "Could not understand line '" << line << ", skipping..." << std::endl;
            continue;
        }

        std::string key = strip(line.substr(0, pos_eq));
        std::string val = strip(line.substr(pos_eq+1));

        // If the key have already been entered.
        if ( fw.find(key) != fw.end() ){
            std::cerr << "Found multiple definitions of '" << key << "', using latest..." << std::endl;
        }

        fw[key] = val;
    }
    return fw;
}