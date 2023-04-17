local char = string.char
local type = type
local select = select
local sub = string.sub
local tconcat = table.concat
local skippedcharacters = { [0] = true, [10] = true, [13] = true }
local function findNextNotSkipped(i)
    repeat if not skippedcharacters[i] then
        return i
    end
        i = i + 1 until false
end
local basedictcompress = {}
local basedictdecompress = {}
local firstNotSkipped = findNextNotSkipped(0)
local secondNotSkipped = findNextNotSkipped(firstNotSkipped + 1)
if firstNotSkipped > 255 or secondNotSkipped > 255 or firstNotSkipped == secondNotSkipped then
    error('invalid configuration, no character can be used in compression')
end
for i = 0, 255 do
    local ic, iic = char(i), char(i, firstNotSkipped)
    basedictcompress[ic] = iic
    basedictdecompress[iic] = ic
end
local function dictAddA(str, dict, a, b)
    if a >= 256 then
        a, b = firstNotSkipped, findNextNotSkipped(b + 1)
        if b >= 256 then
            dict = {}
            b = secondNotSkipped
        end
    end
    dict[str] = char(a, b)
    a = findNextNotSkipped(a + 1)
    return dict, a, b
end
local function compress(input)
    if type(input) ~= 'string' then
        return nil, 'string expected, got ' .. type(input)
    end
    local len = #input
    if len <= 1 then
        return 'u' .. input
    end
    local dict = {}
    local a, b = firstNotSkipped, secondNotSkipped
    local result = { 'c' }
    local resultlen = 1
    local n = 2
    local word = ''
    for i = 1, len do
        local c = sub(input, i, i)
        local wc = word .. c
        if not (basedictcompress[wc] or dict[wc]) then
            local write = basedictcompress[word] or dict[word]
            if not write then
                return nil, 'algorithm error, could not fetch word'
            end
            result[n] = write
            resultlen = resultlen + #write
            n = n + 1
            if len <= resultlen then
                return 'u' .. input
            end
            dict, a, b = dictAddA(wc, dict, a, b)
            word = c
        else
            word = wc
        end
    end
    result[n] = basedictcompress[word] or dict[word]
    resultlen = resultlen + #result[n]
    n = n + 1
    if len <= resultlen then
        return 'u' .. input
    end
    return tconcat(result)
end
local function dictAddB(str, dict, a, b)
    if a >= 256 then
        a, b = firstNotSkipped, findNextNotSkipped(b + 1)
        if b >= 256 then
            dict = {}
            b = secondNotSkipped
        end
    end
    dict[char(a, b)] = str
    a = findNextNotSkipped(a + 1)
    return dict, a, b
end
local function decompress(input)
    if type(input) ~= 'string' then
        return nil, 'string expected, got ' .. type(input)
    end
    if #input < 1 then
        return nil, 'invalid input - not a compressed string'
    end
    local control = sub(input, 1, 1)
    if control == 'u' then
        return sub(input, 2)
    elseif control ~= 'c' then
        return nil, 'invalid input - not a compressed string'
    end
    input = sub(input, 2)
    local len = #input
    if len < 2 then
        return nil, 'invalid input - not a compressed string'
    end
    local dict = {}
    local a, b = firstNotSkipped, secondNotSkipped
    local result = {}
    local n = 1
    local last = sub(input, 1, 2)
    result[n] = basedictdecompress[last] or dict[last]
    n = n + 1
    for i = 3, len, 2 do
        local code = sub(input, i, i + 1)
        local lastStr = basedictdecompress[last] or dict[last]
        if not lastStr then
            return nil, 'could not find last from dict. Invalid input?'
        end
        local toAdd = basedictdecompress[code] or dict[code]
        if toAdd then
            result[n] = toAdd
            n = n + 1
            dict, a, b = dictAddB(lastStr .. sub(toAdd, 1, 1), dict, a, b)
        else
            local tmp = lastStr .. sub(lastStr, 1, 1)
            result[n] = tmp
            n = n + 1
            dict, a, b = dictAddB(tmp, dict, a, b)
        end
        last = code
    end
    return tconcat(result)
end
lualzw = { compress = compress, decompress = decompress, }
return lualzw
