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
#include "Log.h"
#include <iostream>
#include <string>
#include <map>

enum LZWResultCode
{
    LZW_OK,
    LZW_ERR_EMPTY_DICT_RESULT,
    LZW_ERR_UNCOMPRESSED_STRING
};

typedef std::tuple<std::string, LZWResultCode> LZWResult;
typedef std::map<std::string, std::string> LZWDictionary;
static LZWDictionary basedictcompress;
static LZWDictionary basedictdecompress;
static bool DictionariesInitialized = false;

/**
 * TODO
 * - [ ] Add Logs
 */
class LZW
{
public:
    static LZW* instance();

    std::string CharToString(char c);
    std::string GetWordFromDict(const std::string& word, const LZWDictionary& baseDict, const LZWDictionary& altDict);
    uint16 GetNextValidPositionFrom(const uint16& i);

    LZWResult Compress(const std::string& input);
    LZWResult Decompress(const std::string& input);

protected:
    void DictAddA(const std::string& str, LZWDictionary& dict, uint16& a, uint16& b);
    void DictAddB(const std::string& str, LZWDictionary& dict, uint16& a, uint16& b);

private:
    void InitializeDictionaries();
};

#define sLZW LZW::instance()

#endif
