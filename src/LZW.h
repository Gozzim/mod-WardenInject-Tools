/*
 * This file was written for the AzerothCore Project.
 * Code and Implementation: Gozzim (https://github.com/Gozzim/mod-WardenInject-Tools)
 * LZW algorithm: Rochet2 (https://github.com/Rochet2/lualzw)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LZW_H_
#define _LZW_H_

#include "Common.h"
#include <iostream>
#include <string>
#include <map>

typedef std::map<std::string, std::string> LZWDictionary;
typedef std::tuple<LZWDictionary, uint16, uint16> LZWDictResult; //std::tuple<std::map<std::string, std::string>, uint16, uint16>
static LZWDictionary basedictcompress;
static LZWDictionary basedictdecompress;
static bool DictionariesInitialized = false;


/**
 * TODO
 * - [ ] Error logging instead of returning error using tuple with error code and result if OK
 * - [ ] dict searches in own function?
 * - [ ] DictAdd: could change to void and change in variables without const and using & pointers
 */
class LZW
{
public:
    static LZW* instance();

    std::string CharToString(char c);

    std::string Compress(const std::string& input);
    std::string Decompress(const std::string& input);

protected:
    LZWDictResult DictAddA(const std::string& str, const LZWDictionary& dict, uint16 a, uint16 b);
    LZWDictResult DictAddB(const std::string& str, const LZWDictionary& dict, uint16 a, uint16 b);

private:
    void InitializeDictionaries();
};

#define sLZW LZW::instance()

#endif
