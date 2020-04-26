//
// Created by Vetle Wegner Ingeberg on 24/04/2020.
//

#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <map>
#include <string>


typedef std::map<std::string, std::string> firmware_map;

/*!
 * A function to read and parse a file with a mapping
 * of all the firmwares
 * \param fname - name of the file to read from
 * \return map of all firmwares
 */
extern firmware_map ReadConfigFile(const char *fname);

#endif // CONFIGREADER_H
