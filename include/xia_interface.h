//
// Created by Vetle Wegner Ingeberg on 24/04/2020.
//

#ifndef XIA_INTERFACE_H
#define XIA_INTERFACE_H

#include "ConfigReader.h"

extern bool BootXIA(firmware_map &map, const char *dsp_settings_file, const unsigned short &num_mod, unsigned short *plxMap);
extern bool AjustBaseline(bool reset_blcut);
extern bool EnableHistMode(int num_modules);
extern bool XIAIsRunning(int num_mod, bool &errorflag);
extern bool LogScalers(int num_mod, const char *file);
extern bool StartXIA(int preset_time /*!< Preset time in seconds */, int num_mod);
extern bool StopXIA(int num_mod);
extern bool WriteHistogram(int num_mod, const char *path);
extern bool Exit(int num_mod);

#endif // XIA_INTERFACE_H
