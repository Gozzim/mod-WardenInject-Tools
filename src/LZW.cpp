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

#include "LZW.h"

LZW* LZW::instance()
{
    static LZW instance;

    if (!DictionariesInitialized)
    {
        instance.InitializeDictionaries();
        DictionariesInitialized = true;
    }

    return &instance;
}

std::string LZW::CharToString(char c)
{
    return std::string(1, c);
}

void LZW::InitializeDictionaries()
{
    for (uint16 i = 0; i < 256; ++i)
    {
        std::string ic = CharToString(static_cast<char>(i));
        std::string iic = CharToString(static_cast<char>(i)) + CharToString(static_cast<char>(0));
        basedictcompress[ic] = iic;
        basedictdecompress[iic] = ic;
    }
}

LZWDictResult LZW::DictAddA(const std::string& str, const LZWDictionary& dict, uint16 a, uint16 b)
{
    if (a >= 256)
    {
        a = 0;
        b += 1;
        if (b >= 256)
        {
            b = 1;
        }
    }

    LZWDictionary updatedDict = dict;
    updatedDict[str] = CharToString(static_cast<char>(a)) + CharToString(static_cast<char>(b));
    a += 1;

    return std::make_tuple(updatedDict, a, b);
}

std::string LZW::Compress(const std::string& input)
{
    uint16 len = input.size();
    if (len <= 1)
    {
        return "u" + input;
    }

    LZWDictionary dict;
    uint16 a = 0;
    uint16 b = 1;

    std::string result = "c";
    uint16 resultLen = 1;
    uint16 n = 2;
    std::string word = "";

    for (uint16 i = 0; i < len; ++i)
    {
        char c = input[i];
        std::string wc = word + CharToString(c);

        if (basedictcompress.find(wc) == basedictcompress.end() && dict.find(wc) == dict.end())
        {
            std::string write = (basedictcompress.find(word) != basedictcompress.end()) ? basedictcompress[word] : dict[word];

            if (write.empty())
            {
                return "algorithm error, could not fetch word";
            }

            result += write;
            resultLen += write.size();
            n += 1;

            if (len <= resultLen)
            {
                return "u" + input;
            }

            auto dictResult = DictAddA(wc, dict, a, b);
            dict = std::get<0>(dictResult);
            a = std::get<1>(dictResult);
            b = std::get<2>(dictResult);
            word = c;
        }
        else
        {
            word = wc;
        }
    }

    std::string finalWrite = (basedictcompress.find(word) != basedictcompress.end()) ? basedictcompress[word] : dict[word];
    result += finalWrite;
    resultLen += finalWrite.size();
    n += 1;

    if (len <= resultLen)
    {
        return "u" + input;
    }

    return result;
}
