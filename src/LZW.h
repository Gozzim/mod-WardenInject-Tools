/*
 * This file was written for the AzerothCore Project.
 * Code and Implementation: Gozzim (https://github.com/Gozzim/mod-WardenInject-Tools)
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
typedef std::tuple<LZWDictionary, uint16, uint16> LZWDictResult;
static LZWDictionary basedictcompress;
static LZWDictionary basedictdecompress;
static bool DictionariesInitialized = false;

class LZW
{
public:
    static LZW* instance();

    void InitializeDictionaries();

    std::string CharToString(char c);
    LZWDictResult DictAddA(const std::string& str, const LZWDictionary& dict, uint16 a, uint16 b);

    std::string Compress(const std::string& input);
};

#define sLZW LZW::instance()

#endif
