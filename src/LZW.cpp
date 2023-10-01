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
    return {c};
}

std::string LZW::GetWordFromDict(const std::string& word, const LZWDictionary& baseDict, const LZWDictionary& altDict)
{
    if (baseDict.find(word) != baseDict.end())
    {
        return baseDict.at(word);
    }
    else if (altDict.find(word) != altDict.end())
    {
        return altDict.at(word);
    }
    else
    {
        return "";
    }
}

uint16 LZW::GetNextValidPositionFrom(const uint16& i)
{
    if (i == 0 || i == 10 || i == 13)
    {
        return GetNextValidPositionFrom(i + 1);
    }

    return i;
}

void LZW::InitializeDictionaries()
{
    for (uint16 i = 0; i < 256; ++i)
    {
        std::string ic = CharToString(static_cast<char>(i));
        std::string iic = CharToString(static_cast<char>(i)) + CharToString(static_cast<char>(1));
        basedictcompress[ic] = iic;
        basedictdecompress[iic] = ic;
    }
}

void LZW::DictAddA(const std::string& str, LZWDictionary& dict, uint16& a, uint16& b)
{
    if (a >= 256)
    {
        a = 1;
        b = GetNextValidPositionFrom(b+1);
        if (b >= 256)
        {
            b = 2;
            dict.clear();
        }
    }

    dict[str] = CharToString(static_cast<char>(a)) + CharToString(static_cast<char>(b));
    a = GetNextValidPositionFrom(a+1);
}

void LZW::DictAddB(const std::string& str, LZWDictionary& dict, uint16& a, uint16& b)
{
    if (a >= 256)
    {
        a = 1;
        b = GetNextValidPositionFrom(b+1);
        if (b >= 256)
        {
            b = 2;
            dict.clear();
        }
    }

    dict[CharToString(static_cast<char>(a)) + CharToString(static_cast<char>(b))] = str;
    a = GetNextValidPositionFrom(a+1);
}

LZWResult LZW::Compress(const std::string& input)
{
    uint16 len = input.size();
    if (len <= 1)
    {
        LOG_DEBUG("module", "LZW::Compress: Compressing input {} with size {} <= 1.", input, len);
        return {"u" + input, LZW_OK};
    }

    LZWDictionary dict;
    uint16 a = 1;
    uint16 b = 2;

    std::string result = "c";
    uint16 resultLen = 1;
    std::string word = "";

    for (uint16 i = 0; i < len; ++i)
    {
        char c = input[i];
        std::string wc = word + CharToString(c);

        if (basedictcompress.find(wc) == basedictcompress.end() && dict.find(wc) == dict.end())
        {
            std::string write = GetWordFromDict(word, basedictcompress, dict);

            if (write.empty())
            {
                LOG_ERROR("module", "LZW::Compress: Could not find word {} in dictionary at input index {}", word, i);
                return {nullptr, LZW_ERR_EMPTY_DICT_RESULT};
            }

            result += write;
            resultLen += write.size();

            if (len <= resultLen)
            {
                LOG_DEBUG("module", "LZW::Compress: Length of input is smaller than compressed result. Returning uncompressed.");
                return {"u" + input, LZW_OK};
            }

            DictAddA(wc, dict, a, b);
            word = c;
        }
        else
        {
            word = wc;
        }
    }

    std::string finalWrite = GetWordFromDict(word, basedictcompress, dict);
    if (finalWrite.empty())
    {
        LOG_ERROR("module", "LZW::Compress: Could not find word {} in dictionary after looping input.", finalWrite);
        return {nullptr, LZW_ERR_EMPTY_DICT_RESULT};
    }

    result += finalWrite;
    resultLen += finalWrite.size();

    if (len <= resultLen)
    {
        LOG_DEBUG("module", "LZW::Compress: Total length of input is smaller than compressed result. Returning uncompressed.");
        return {"u" + input, LZW_OK};
    }

    return {result, LZW_OK};
}

LZWResult LZW::Decompress(const std::string& input)
{
    if (input.empty())
    {
        LOG_WARN("module", "LZW::Decompress: Tried to decompress empty input");
        return {input, LZW_WARN_EMPTY_INPUT};
    }

    char control = input[0];
    if (control == 'u')
    {
        return {input.substr(1), LZW_OK};
    }
    else if (control != 'c')
    {
        LOG_WARN("module", "LZW::Decompress: Tried to decompress empty input. Returning Input.");
        return {input, LZW_WARN_UNCOMPRESSED_STRING};
    }

    std::string data = input.substr(1);
    uint16 len = data.size();
    if (len <= 1)
    {
        LOG_WARN("module", "LZW::Decompress: Tried to decompress input {} with size {} <= 1. Returning input.", input, len);
        return {input, LZW_WARN_UNCOMPRESSED_STRING};
    }

    LZWDictionary dict;
    uint16 a = 1;
    uint16 b = 2;
    std::string result;
    std::string last = data.substr(0, 2);

    result = GetWordFromDict(last, basedictdecompress, dict);

    for (uint16 i = 2; i < len; i += 2)
    {
        std::string code = data.substr(i, 2);
        std::string lastStr = GetWordFromDict(last, basedictdecompress, dict);

        if (lastStr.empty())
        {
            LOG_ERROR("module", "LZW::Decompress: Could not find word {} in dictionary at input index {}", last, i);
            return {nullptr, LZW_ERR_EMPTY_DICT_RESULT};
        }

        std::string toAdd = GetWordFromDict(code, basedictdecompress, dict);

        if (!toAdd.empty())
        {
            result += toAdd;
            DictAddB(lastStr + toAdd[0], dict, a, b);
        }
        else
        {
            std::string tmp = lastStr + lastStr[0];
            result += tmp;
            DictAddB(tmp, dict, a, b);
        }

        last = code;
    }

    return {result, LZW_OK};
}
