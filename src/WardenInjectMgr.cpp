/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
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

#include "WardenInjectMgr.h"
#include "StringFormat.h"

WardenInjectMgr* WardenInjectMgr::instance()
{
    static WardenInjectMgr instance;
    return &instance;
}

// Define payloads
std::string lualzw = R"(local char = string.char local type = type local select = select local sub = string.sub local tconcat = table.concat local skippedcharacters = {[0] = true, [10] = true, [13] = true} local function findNextNotSkipped(i) repeat if not skippedcharacters[i] then return i end i = i+1 until false end local basedictcompress = {} local basedictdecompress = {} local firstNotSkipped = findNextNotSkipped(0) local secondNotSkipped = findNextNotSkipped(firstNotSkipped+1) if firstNotSkipped > 255 or secondNotSkipped > 255 or firstNotSkipped == secondNotSkipped then error('invalid configuration, no character can be used in compression') end for i = 0, 255 do local ic, iic = char(i), char(i, firstNotSkipped) basedictcompress[ic] = iic basedictdecompress[iic] = ic end local function dictAddA(str, dict, a, b) if a >= 256 then a, b = firstNotSkipped, findNextNotSkipped(b+1) if b >= 256 then dict = {} b = secondNotSkipped end end dict[str] = char(a,b) a = findNextNotSkipped(a+1) return dict, a, b end local function compress(input) if type(input) ~= 'string' then return nil, 'string expected, got '..type(input) end local len = #input if len <= 1 then return 'u'..input end local dict = {} local a, b = firstNotSkipped, secondNotSkipped local result = {'c'} local resultlen = 1 local n = 2 local word = '' for i = 1, len do local c = sub(input, i, i) local wc = word..c if not (basedictcompress[wc] or dict[wc]) then local write = basedictcompress[word] or dict[word] if not write then return nil, 'algorithm error, could not fetch word' end result[n] = write resultlen = resultlen + #write n = n+1 if len <= resultlen then return 'u'..input end dict, a, b = dictAddA(wc, dict, a, b) word = c else word = wc end end result[n] = basedictcompress[word] or dict[word] resultlen = resultlen+#result[n] n = n+1 if len <= resultlen then return 'u'..input end return tconcat(result) end local function dictAddB(str, dict, a, b) if a >= 256 then a, b = firstNotSkipped, findNextNotSkipped(b+1) if b >= 256 then dict = {} b = secondNotSkipped end end dict[char(a,b)] = str a = findNextNotSkipped(a+1) return dict, a, b end local function decompress(input) if type(input) ~= 'string' then return nil, 'string expected, got '..type(input) end if #input < 1 then return nil, 'invalid input - not a compressed string' end local control = sub(input, 1, 1) if control == 'u' then return sub(input, 2) elseif control ~= 'c' then return nil, 'invalid input - not a compressed string' end input = sub(input, 2) local len = #input if len < 2 then return nil, 'invalid input - not a compressed string' end local dict = {} local a, b = firstNotSkipped, secondNotSkipped local result = {} local n = 1 local last = sub(input, 1, 2) result[n] = basedictdecompress[last] or dict[last] n = n+1 for i = 3, len, 2 do local code = sub(input, i, i+1) local lastStr = basedictdecompress[last] or dict[last] if not lastStr then return nil, 'could not find last from dict. Invalid input?' end local toAdd = basedictdecompress[code] or dict[code] if toAdd then result[n] = toAdd n = n+1 dict, a, b = dictAddB(lastStr..sub(toAdd, 1, 1), dict, a, b) else local tmp = lastStr..sub(lastStr, 1, 1) result[n] = tmp n = n+1 dict, a, b = dictAddB(tmp, dict, a, b) end last = code end return tconcat(result) end lualzw = { compress = compress, decompress = decompress, } return lualzw)";
std::string Smallfork = R"(local M = {} Smallfolk = M local expect_object, dump_object local error, tostring, pairs, type, floor, huge, concat = error, tostring, pairs, type, math.floor, math.huge, table.concat local dump_type = {} local ob, cb, sq = '{', '}', '\"' function dump_type:string(nmemo, memo, acc) local nacc = #acc acc[nacc + 1] = sq acc[nacc + 2] = self:gsub(sq, sq .. sq) acc[nacc + 3] = sq return nmemo end function dump_type:number(nmemo, memo, acc) acc[#acc + 1] = ('%.17g'):format(self) return nmemo end function dump_type:table(nmemo, memo, acc) memo[self] = nmemo acc[#acc + 1] = ob local nself = #self for i = 1, nself do nmemo = dump_object(self[i], nmemo, memo, acc) acc[#acc + 1] = ',' end for k, v in pairs(self) do if type(k) ~= 'number' or floor(k) ~= k or k < 1 or k > nself then nmemo = dump_object(k, nmemo, memo, acc) acc[#acc + 1] = ':' nmemo = dump_object(v, nmemo, memo, acc) acc[#acc + 1] = ',' end end acc[#acc] = acc[#acc] == ob and ob .. cb or cb return nmemo end function dump_object(object, nmemo, memo, acc) if object == true then acc[#acc + 1] = 't' elseif object == false then acc[#acc + 1] = 'f' elseif object == nil then acc[#acc + 1] = 'n' elseif object ~= object then if ('' .. object):sub(1, 1) == '-' then acc[#acc + 1] = 'N' else acc[#acc + 1] = 'Q' end elseif object == huge then acc[#acc + 1] = 'I' elseif object == -huge then acc[#acc + 1] = 'i' else local t = type(object) if not dump_type[t] then error('cannot dump type ' .. t) end return dump_type[t](object, nmemo, memo, acc) end return nmemo end function M.dumps(object) local nmemo = 0 local memo = {} local acc = {} dump_object(object, nmemo, memo, acc) return concat(acc) end local function invalid(i) error('invalid input at position ' .. i) end local nonzero_digit = { ['1'] = true, ['2'] = true, ['3'] = true, ['4'] = true, ['5'] = true, ['6'] = true, ['7'] = true, ['8'] = true, ['9'] = true } local is_digit = { ['0'] = true, ['1'] = true, ['2'] = true, ['3'] = true, ['4'] = true, ['5'] = true, ['6'] = true, ['7'] = true, ['8'] = true, ['9'] = true } local function expect_number(string, start) local i = start local head = string:sub(i, i) if head == '-' then i = i + 1 head = string:sub(i, i) end if nonzero_digit[head] then repeat i = i + 1 head = string:sub(i, i) until not is_digit[head] elseif head == '0' then i = i + 1 head = string:sub(i, i) else invalid(i) end if head == '.' then local oldi = i repeat i = i + 1 head = string:sub(i, i) until not is_digit[head] if i == oldi + 1 then invalid(i) end end if head == 'e' or head == 'E' then i = i + 1 head = string:sub(i, i) if head == '+' or head == '-' then i = i + 1 head = string:sub(i, i) end if not is_digit[head] then invalid(i) end repeat i = i + 1 head = string:sub(i, i) until not is_digit[head] end return tonumber(string:sub(start, i - 1)), i end local expect_object_head = {t = function(string, i) return true, i end, f = function(string, i) return false, i end, n = function(string, i) return nil, i end, Q = function(string, i) return -(0 / 0), i end, N = function(string, i) return 0 / 0, i end, I = function(string, i) return 1 / 0, i end, i = function(string, i) return -1 / 0, i end, [sq] = function(string, i) local nexti = i - 1 repeat nexti = string:find(sq, nexti + 1, true) + 1 until string:sub(nexti, nexti) ~= sq return string:sub(i, nexti - 2):gsub(sq .. sq, sq), nexti end, ['0'] = function(string, i) return expect_number(string, i - 1) end, [ob] = function(string, i, tables) local nt, k, v = {} local j = 1 tables[#tables + 1] = nt if string:sub(i, i) == cb then return nt, i + 1 end while true do k, i = expect_object(string, i, tables) if string:sub(i, i) == ':' then v, i = expect_object(string, i + 1, tables) nt[k] = v else nt[j] = k j = j + 1 end local head = string:sub(i, i) if head == ',' then i = i + 1 elseif head == cb then return nt, i + 1 else invalid(i) end end end} expect_object_head['1'] = expect_object_head['0'] expect_object_head['2'] = expect_object_head['0'] expect_object_head['3'] = expect_object_head['0'] expect_object_head['4'] = expect_object_head['0'] expect_object_head['5'] = expect_object_head['0'] expect_object_head['6'] = expect_object_head['0'] expect_object_head['7'] = expect_object_head['0'] expect_object_head['8'] = expect_object_head['0'] expect_object_head['9'] = expect_object_head['0'] expect_object_head['-'] = expect_object_head['0'] expect_object_head['.'] = expect_object_head['0'] expect_object = function(string, i, tables) local head = string:sub(i, i) if expect_object_head[head] then return expect_object_head[head](string, i + 1, tables) end invalid(i) end function M.loads(string, maxsize) if #string > (maxsize or 10000) then error 'input too large' end return (expect_object(string, 1, {})) end return M ")";
std::string CMH = R"(local debug = false local CMH = {} local datacache = {} local delim = {'', '', '', '', ''} local pck = {REQ = '', DAT = ''} local function debugOut(prefix, x, msg) if(debug == true) then print(date('%X', time())..' '..x..' '..prefix..': '..msg) end end local function GenerateReqId() local length = 6 local reqId = '' for i = 1, length do reqId = reqId..string.char(math.random(97, 122)) end return reqId end local function ParseMessage(str) str = str or '' local output = {} local valTemp = {} local typeTemp = {} local valMatch = '[^'..table.concat(delim)..']+' local typeMatch = '['..table.concat(delim)..']+' for value in str:gmatch(valMatch) do table.insert(valTemp, value) end for varType in str:gmatch(typeMatch) do for k, v in pairs(delim) do if(v == varType) then table.insert(typeTemp, k) end end end for k, v in pairs(valTemp) do local varType = typeTemp[k] if(varType == 2) then if(v == string.char(tonumber('1A', 16))) then v = '' end elseif(varType == 3) then v = tonumber(v) elseif(varType == 4) then v = Smallfolk.loads(v, #v) elseif(varType == 5) then if(v == 'true') then v = true else v = false end end table.insert(output, v) end valTemp = nil typeTemp = nil return output end local function ProcessVariables(reqId, ...) local splitLength = 200 local arg = {...} local msg = '' for _, v in pairs(arg) do if(type(v) == 'string') then if(#v == 0) then v = string.char(tonumber('1A', 16)) end msg = msg..delim[2] elseif(type(v) == 'number') then msg = msg..delim[3] elseif(type(v) == 'table') then v = Smallfolk.dumps(v) msg = msg..delim[4] elseif(type(v) == 'boolean') then v = tostring(v) msg = msg..delim[5] end msg = msg..v end if not datacache[reqId] then datacache[reqId] = { count = 0, data = {}} end for i=1, msg:len(), splitLength do datacache[reqId].count = datacache[reqId].count + 1 datacache[reqId]['data'][datacache[reqId].count] = msg:sub(i,i+splitLength - 1) end return datacache[reqId] end function CMH.OnReceive(self, event, header, data, Type, sender) if event == 'CHAT_MSG_ADDON' and sender == UnitName('player') and Type == 'WHISPER' then local pfx, source, pckId = header:match('(.)(%u)(.)') if not pfx or not source or not pckId then return end if(pfx == delim[1] and source == 'S') then if(pckId == pck.REQ) then debugOut('REQ', 'Rx', 'REQ received, data: '..data) CMH.OnREQ(sender, data) elseif(pckId == pck.DAT) then debugOut('DAT', 'Rx', 'DAT received, data: '..data) CMH.OnDAT(sender, data) else debugOut('ERR', 'Rx', 'Invalid packet type, aborting') return end end end end local CMHFrame = CreateFrame('Frame') CMHFrame:RegisterEvent('CHAT_MSG_ADDON') CMHFrame:SetScript('OnEvent', CMH.OnReceive) function CMH.OnREQ(sender, data) debugOut('REQ', 'Rx', 'Processing data..') local functionId, linkCount, reqId, addon = data:match('(%d%d)(%d%d%d)(%w%w%w%w%w%w)(.+)'); if not functionId or not linkCount or not reqId or not addon then debugOut('REQ', 'Rx', 'Malformed data, aborting.') return end functionId, linkCount = tonumber(functionId), tonumber(linkCount); if not CMH[addon] then debugOut('REQ', 'Rx', 'Invalid addon, aborting.') return end if not CMH[addon][functionId] then debugOut('REQ', 'Rx', 'Invalid addon function, aborting.') return end if(datacache[reqId]) then datacache[reqId] = nil debugOut('REQ', 'Rx', 'Cache already exists, aborting.') return end datacache[reqId] = {addon = addon, funcId = functionId, count = linkCount, data = {}} debugOut('REQ', 'Rx', 'Header validated, cache created. Awaitng data..') end function CMH.OnDAT(sender, data) debugOut('DAT', 'Rx', 'Validating data..') local reqId = data:sub(1, 6) local payload = data:sub(#reqId+1) if not reqId then debugOut('DAT', 'Rx', 'Request ID missing, aborting.') return end if not datacache[reqId] then debugOut('DAT', 'Rx', 'Cache does not exist, aborting.') return end local reqTable = datacache[reqId] local sizeOfDataCache = #reqTable.data if reqTable.count == 0 then debugOut('DAT', 'Rx', 'Function expects no data, triggering function..') local func = CMH[reqTable.addon][reqTable.funcId] if func then _G[func](sender, {}) datacache[reqId] = nil debugOut('DAT', 'Rx', 'Function '..func..' @ '..reqTable.addon..' executed, cache cleared.') end return end if sizeOfDataCache+1 > reqTable.count then debugOut('DAT', 'Rx', 'Received more data than expected. Aborting.') return end reqTable['data'][sizeOfDataCache+1] = payload sizeOfDataCache = #reqTable.data debugOut('DAT', 'Rx', 'Data part '..sizeOfDataCache..' of '..reqTable.count..' added to cache.') if(sizeOfDataCache == reqTable.count) then debugOut('DAT', 'Rx', 'All expected data received, processing..') local fullPayload = table.concat(reqTable.data); local VarTable = ParseMessage(fullPayload) local func = CMH[reqTable.addon][reqTable.funcId] if func then _G[func](sender, VarTable) datacache[reqId] = nil debugOut('DAT', 'Rx', 'Function '..func..' @ '..reqTable.addon..' executed, cache cleared.') end end end function CMH.SendREQ(functionId, linkCount, reqId, addon) local header = string.format('%01s%01s%01s', delim[1], 'C', pck.REQ) local data = string.format('%02d%03d%06s%0'..tostring(#addon)..'s', functionId, linkCount, reqId, addon) SendAddonMessage(header, data, 'WHISPER', UnitName('player')) debugOut('REQ', 'Tx', 'Sent REQ with ID '..reqId..', Header '..header..', DAT '..data..' sending DAT..') end function CMH.SendDAT(reqId) local header = string.format('%01s%01s%01s', delim[1], 'C', pck.DAT) if(#datacache[reqId]['data'] == 0) then SendAddonMessage(header, reqId, 'WHISPER', UnitName('player')) else for _, v in pairs (datacache[reqId]['data']) do local payload = reqId..v SendAddonMessage(header, payload, 'WHISPER', UnitName('player')) end end datacache[reqId] = nil debugOut('DAT', 'Tx', 'Sent all DAT for ID '..reqId..', cache cleared, closing.') end function RegisterServerResponses(config) if(CMH[config.Prefix]) then return; end CMH[config.Prefix] = {} for functionId, functionName in pairs(config.Functions) do CMH[config.Prefix][functionId] = functionName end end function SendClientRequest(prefix, functionId, ...) local reqId = GenerateReqId() local varTable = ProcessVariables(reqId, ...) CMH.SendREQ(functionId, varTable.count, reqId, prefix) CMH.SendDAT(reqId) end)";
std::string StatPointUI = R"(local config = {Prefix = 'StatPointUI', Functions = {[1] = 'OnCacheReceived'}} StatPointUI = {['cache'] = {}} function StatPointUI.OnLoad() StatPointUI.mainFrame = CreateFrame('Frame', config.Prefix, CharacterFrame) StatPointUI.mainFrame:SetToplevel(true) StatPointUI.mainFrame:SetSize(200, 260) StatPointUI.mainFrame:SetBackdrop( { bgFile = 'Interface/TutorialFrame/TutorialFrameBackground', edgeFile = 'Interface/DialogFrame/UI-DialogBox-Border', edgeSize = 16, tileSize = 32, insets = {left = 5, right = 5, top = 5, bottom = 5} } ) StatPointUI.mainFrame:SetPoint('TOPRIGHT', 170, -20) StatPointUI.mainFrame:Hide() StatPointUI.titleBar = CreateFrame('Frame', config.Prefix..'TitleBar', StatPointUI.mainFrame) StatPointUI.titleBar:SetSize(135, 25) StatPointUI.titleBar:SetBackdrop( { bgFile = 'Interface/CHARACTERFRAME/UI-Party-Background', edgeFile = 'Interface/DialogFrame/UI-DialogBox-Border', tile = true, edgeSize = 16, tileSize = 16, insets = {left = 5, right = 5, top = 5, bottom = 5} } ) StatPointUI.titleBar:SetPoint('TOP', 0, 9) StatPointUI.titleBarText = StatPointUI.titleBar:CreateFontString(config.Prefix..'TitleBarText') StatPointUI.titleBarText:SetFont('Fonts/FRIZQT__.TTF', 13) StatPointUI.titleBarText:SetSize(190, 5) StatPointUI.titleBarText:SetPoint('CENTER', 0, 0) StatPointUI.titleBarText:SetText('|cffFFC125Attribute Points|r') local rowOffset = -30 local titleOffset = -100 local btnOffset = 40 local rowContent = {'Strength', 'Agility', 'Stamina', 'Intellect', 'Spirit'} for k, v in pairs(rowContent) do StatPointUI[v] = {} StatPointUI[v].Val = StatPointUI.mainFrame:CreateFontString(config.Prefix..v..'Val') StatPointUI[v].Val:SetFont('Fonts/FRIZQT__.TTF', 15) if (k == 1) then StatPointUI[v].Val:SetPoint('CENTER', StatPointUI.titleBar, 'CENTER', 30, rowOffset) else local tmp = rowContent[k - 1] StatPointUI[v].Val:SetPoint('CENTER', StatPointUI[tmp].Val, 'CENTER', 0, rowOffset) end StatPointUI[v].Val:SetText('0') StatPointUI[v].Title = StatPointUI.mainFrame:CreateFontString(config.Prefix..v..'Title') StatPointUI[v].Title:SetFont('Fonts/FRIZQT__.TTF', 15) StatPointUI[v].Title:SetPoint('LEFT', StatPointUI[v].Val, 'LEFT', titleOffset, 0) StatPointUI[v].Title:SetText(v..':') StatPointUI[v].Button = CreateFrame('Button', config.Prefix..v..'Button', StatPointUI.mainFrame) StatPointUI[v].Button:SetSize(20, 20) StatPointUI[v].Button:SetPoint('RIGHT', StatPointUI[v].Val, 'RIGHT', btnOffset, 0) StatPointUI[v].Button:EnableMouse(false) StatPointUI[v].Button:Disable() StatPointUI[v].Button:SetNormalTexture('Interface/BUTTONS/UI-SpellbookIcon-NextPage-Up') StatPointUI[v].Button:SetHighlightTexture('Interface/BUTTONS/UI-Panel-MinimizeButton-Highlight') StatPointUI[v].Button:SetPushedTexture('Interface/BUTTONS/UI-SpellbookIcon-NextPage-Down') StatPointUI[v].Button:SetDisabledTexture('Interface/BUTTONS/UI-SpellbookIcon-NextPage-Disabled') StatPointUI[v].Button:SetScript( 'OnMouseUp', function() SendClientRequest(config.Prefix, 2, k) PlaySound('UChatScrollButton') end ) end StatPointUI.pointsLeftVal = StatPointUI.mainFrame:CreateFontString(config.Prefix..'PointsLeftVal') StatPointUI.pointsLeftVal:SetFont('Fonts/FRIZQT__.TTF', 15) local tmp = rowContent[#rowContent] StatPointUI.pointsLeftVal:SetPoint('CENTER', StatPointUI[tmp].Val, 'CENTER', 0, rowOffset) StatPointUI.pointsLeftVal:SetText('0') StatPointUI.pointsLeftTitle = StatPointUI.mainFrame:CreateFontString(config.Prefix..'PointsLeftVal') StatPointUI.pointsLeftTitle:SetFont('Fonts/FRIZQT__.TTF', 15) StatPointUI.pointsLeftTitle:SetPoint('LEFT', StatPointUI.pointsLeftVal, 'LEFT', titleOffset, 0) StatPointUI.pointsLeftTitle:SetText('Points left:') StatPointUI.resetButton = CreateFrame('Button', config.Prefix..'ResetButton', StatPointUI.mainFrame) StatPointUI.resetButton:SetSize(100, 25) StatPointUI.resetButton:SetPoint('CENTER', StatPointUI.titleBar, 'CENTER', 0, -220) StatPointUI.resetButton:EnableMouse(true) StatPointUI.resetButton:SetText('RESET') StatPointUI.resetButton:SetNormalFontObject('GameFontNormalSmall') local ntex = StatPointUI.resetButton:CreateTexture() ntex:SetTexture('Interface/Buttons/UI-Panel-Button-Up') ntex:SetTexCoord(0, 0.625, 0, 0.6875) ntex:SetAllPoints() StatPointUI.resetButton:SetNormalTexture(ntex) local htex = StatPointUI.resetButton:CreateTexture() htex:SetTexture('Interface/Buttons/UI-Panel-Button-Highlight') htex:SetTexCoord(0, 0.625, 0, 0.6875) htex:SetAllPoints() StatPointUI.resetButton:SetHighlightTexture(htex) local ptex = StatPointUI.resetButton:CreateTexture() ptex:SetTexture('Interface/Buttons/UI-Panel-Button-Down') ptex:SetTexCoord(0, 0.625, 0, 0.6875) ptex:SetAllPoints() StatPointUI.resetButton:SetPushedTexture(ptex) StatPointUI.resetButton:SetScript( 'OnMouseUp', function() SendClientRequest(config.Prefix, 3) PlaySound('UChatScrollButton') end ) PaperDollFrame:HookScript( 'OnShow', function() StatPointUI.mainFrame:Show() end ) PaperDollFrame:HookScript( 'OnHide', function() StatPointUI.mainFrame:Hide() end ) StatPointUI.pointsLeftVal:SetText(StatPointUI.cache[6]) PaperDollFrame:HookScript( 'OnShow', function() StatPointUI.mainFrame:Show() end ) PaperDollFrame:HookScript( 'OnHide', function() StatPointUI.mainFrame:Hide() end ) end function OnCacheReceived(sender, argTable) StatPointUI.cache = argTable[1] local rowContent = {'Strength', 'Agility', 'Stamina', 'Intellect', 'Spirit'} for i = 1, 5 do local rci = rowContent[i] StatPointUI[rci].Val:SetText(StatPointUI.cache[i]) if (StatPointUI.cache[i] > 0) then StatPointUI[rci].Val:SetTextColor(0, 1, 0, 1) else StatPointUI[rci].Val:SetTextColor(1, 1, 1, 1) end if (StatPointUI.cache[6] > 0) then StatPointUI[rci].Button:EnableMouse(true) StatPointUI[rci].Button:Enable() else StatPointUI[rci].Button:EnableMouse(false) StatPointUI[rci].Button:Disable() end end StatPointUI.pointsLeftVal:SetText(StatPointUI.cache[6]) end RegisterServerResponses(config) StatPointUI.OnLoad() SendClientRequest(config.Prefix, 1))";
std::string Queue = R"(local Queue = {} function Queue.__index(que, key) return Queue[key] end function NewQueue() local t = {first = 0, last = -1} setmetatable(t, Queue) return t end function Queue.pushleft(que, value) local first = que.first - 1 que.first = first que[first] = value return first end function Queue.pushright(que, value) local last = que.last + 1 que.last = last que[last] = value return last end function Queue.popleft(que) local first = que.first if first > que.last then error('que is empty') end local value = que[first] que[first] = nil que.first = first + 1 return value end function Queue.popright(que) local last = que.last if que.first > last then error('que is empty') end local value = que[last] que[last] = nil que.last = last - 1 return value end function Queue.peekleft(que) return que[que.first] end function Queue.peekright(que) return que[que.last] end function Queue.empty(que) return que.last < que.first end function Queue.size(que) return que.last - que.first + 1 end function Queue.clear(que) local l, r = self:getrange() for i = l, r do que[idx] = nil end que.first, que.last = 0, -1 end function Queue.get(que, idx) if idx < que.first or idx > que.last then return end return que[idx] end function Queue.getrange(que) return que.first, que.last end function Queue.gettable(que) return que end return NewQueue)";
std::string LibStub = R"(local LIBSTUB_MAJOR, LIBSTUB_MINOR = 'LibStub', 2 local LibStub = _G[LIBSTUB_MAJOR] if not LibStub or LibStub.minor < LIBSTUB_MINOR then LibStub = LibStub or {libs = {}, minors = {} } _G[LIBSTUB_MAJOR] = LibStub LibStub.minor = LIBSTUB_MINOR function LibStub:NewLibrary(major, minor) assert(type(major) == 'string', 'Bad argument #2 to `NewLibrary` (string expected)') minor = assert(tonumber(strmatch(minor, '%d+')), 'Minor version must either be a number or contain a number.') local oldminor = self.minors[major] if oldminor and oldminor >= minor then return nil end self.minors[major], self.libs[major] = minor, self.libs[major] or {} return self.libs[major], oldminor end function LibStub:GetLibrary(major, silent) if not self.libs[major] and not silent then error(('Cannot find a library instance of %q.'):format(tostring(major)), 2) end return self.libs[major], self.minors[major] end function LibStub:IterateLibraries() return pairs(self.libs) end setmetatable(LibStub, { __call = LibStub.GetLibrary }) end)";
std::string LibWindow = R"(local MAJOR = 'LibWindow-1.1' local MINOR = tonumber(('$Revision: 8 $'):match('(%d+)')) local lib = LibStub:NewLibrary(MAJOR,MINOR) if not lib then return end local min,max,abs = min,max,abs local pairs = pairs local tostring = tostring local UIParent,GetScreenWidth,GetScreenHeight,IsAltKeyDown = UIParent,GetScreenWidth,GetScreenHeight,IsAltKeyDown local function print(msg) ChatFrame1:AddMessage(MAJOR..': '..tostring(msg)) end lib.utilFrame = lib.utilFrame or CreateFrame('Frame') lib.delayedSavePosition = lib.delayedSavePosition or {} lib.windowData = lib.windowData or {} lib.embeds = lib.embeds or {} local mixins = {} local function getStorageName(frame, name) local names = lib.windowData[frame].names if names then if names[name] then return names[name] end if names.prefix then return names.prefix .. name; end end return name; end local function setStorage(frame, name, value) lib.windowData[frame].storage[getStorageName(frame, name)] = value end local function getStorage(frame, name) return lib.windowData[frame].storage[getStorageName(frame, name)] end lib.utilFrame:SetScript('OnUpdate', function(this) this:Hide() for frame,_ in pairs(lib.delayedSavePosition) do lib.delayedSavePosition[frame] = nil lib.SavePosition(frame) end end) local function queueSavePosition(frame) lib.delayedSavePosition[frame] = true lib.utilFrame:Show() end mixins['RegisterConfig']=true function lib.RegisterConfig(frame, storage, names) if not lib.windowData[frame] then lib.windowData[frame] = {} end lib.windowData[frame].names = names lib.windowData[frame].storage = storage end local nilParent = { GetWidth = function() return GetScreenWidth() * UIParent:GetScale() end, GetHeight = function() return GetScreenHeight() * UIParent:GetScale() end, GetScale = function() return 1 end, } mixins['SavePosition']=true function lib.SavePosition(frame) local parent = frame:GetParent() or nilParent local s = frame:GetScale() local left,top = frame:GetLeft()*s, frame:GetTop()*s local right,bottom = frame:GetRight()*s, frame:GetBottom()*s local pwidth, pheight = parent:GetWidth(), parent:GetHeight() local x,y,point; if left < (pwidth-right) and left < abs((left+right)/2 - pwidth/2) then x = left; point='LEFT'; elseif (pwidth-right) < abs((left+right)/2 - pwidth/2) then x = right-pwidth; point='RIGHT'; else x = (left+right)/2 - pwidth/2; point=''; end if bottom < (pheight-top) and bottom < abs((bottom+top)/2 - pheight/2) then y = bottom; point='BOTTOM'..point; elseif (pheight-top) < abs((bottom+top)/2 - pheight/2) then y = top-pheight; point='TOP'..point; else y = (bottom+top)/2 - pheight/2; end if point=='' then point = 'CENTER' end setStorage(frame, 'x', x) setStorage(frame, 'y', y) setStorage(frame, 'point', point) setStorage(frame, 'scale', s) frame:ClearAllPoints() frame:SetPoint(point, frame:GetParent(), point, x/s, y/s); end mixins['RestorePosition']=true function lib.RestorePosition(frame) local x = getStorage(frame, 'x') local y = getStorage(frame, 'y') local point = getStorage(frame, 'point') local s = getStorage(frame, 'scale') if s then (frame.lw11origSetScale or frame.SetScale)(frame,s) else s = frame:GetScale() end if not x or not y then x=0; y=0; point='CENTER' end x = x/s y = y/s frame:ClearAllPoints() if not point and y==0 then point='CENTER' end if not point then frame:SetPoint('TOPLEFT', frame:GetParent(), 'BOTTOMLEFT', x, y) queueSavePosition(frame) return end frame:SetPoint(point, frame:GetParent(), point, x, y) end mixins['SetScale']=true function lib.SetScale(frame, scale) setStorage(frame, 'scale', scale); (frame.lw11origSetScale or frame.SetScale)(frame,scale) lib.RestorePosition(frame) end function lib.OnDragStart(frame) lib.windowData[frame].isDragging = true frame:StartMoving() end function lib.OnDragStop(frame) frame:StopMovingOrSizing() lib.SavePosition(frame) lib.windowData[frame].isDragging = false if lib.windowData[frame].altEnable and not IsAltKeyDown() then frame:EnableMouse(false) end end local function onDragStart(...) return lib.OnDragStart(...) end local function onDragStop(...) return lib.OnDragStop(...) end mixins['MakeDraggable']=true function lib.MakeDraggable(frame) assert(lib.windowData[frame]) frame:SetMovable(true) frame:SetScript('OnDragStart', onDragStart) frame:SetScript('OnDragStop', onDragStop) frame:RegisterForDrag('LeftButton') end function lib.OnMouseWheel(frame, dir) local scale = getStorage(frame, 'scale') if dir<0 then scale=max(scale*0.9, 0.1) else scale=min(scale/0.9, 3) end lib.SetScale(frame, scale) end local function onMouseWheel(...) return lib.OnMouseWheel(...) end mixins['EnableMouseWheelScaling']=true function lib.EnableMouseWheelScaling(frame) frame:SetScript('OnMouseWheel', onMouseWheel) end lib.utilFrame:SetScript('OnEvent', function(this, event, key, state) if event=='MODIFIER_STATE_CHANGED' then if key == 'LALT' or key == 'RALT' then for frame,_ in pairs(lib.altEnabledFrames) do if not lib.windowData[frame].isDragging then frame:EnableMouse(state == 1) end end end end end) mixins['EnableMouseOnAlt']=true function lib.EnableMouseOnAlt(frame) assert(lib.windowData[frame]) lib.windowData[frame].altEnable = true frame:EnableMouse(not not IsAltKeyDown()) if not lib.altEnabledFrames then lib.altEnabledFrames = {} lib.utilFrame:RegisterEvent('MODIFIER_STATE_CHANGED') end lib.altEnabledFrames[frame] = true end function lib:Embed(target) if not target or not target[0] or not target.GetObjectType then error('Usage: LibWindow:Embed(frame)', 1) end target.lw11origSetScale = target.SetScale for name, _ in pairs(mixins) do target[name] = self[name] end lib.embeds[target] = true return target end for target, _ in pairs(lib.embeds) do lib:Embed(target) end)";
std::string AIO = R"(assert(not AIO, 'AIO is already loaded. Possibly different versions!') local AIO_ENABLE_DEBUG_MSGS = false local AIO_ENABLE_PCALL = true local AIO_ENABLE_TRACEBACK = false local AIO_ENABLE_MSGPRINT = false local AIO_TIMEOUT_INSTRUCTIONCOUNT = 1e8 local AIO_MSG_CACHE_SPACE = 5e5 local AIO_MSG_CACHE_TIME = 15*1000 local AIO_MSG_CACHE_DELAY = 5*1000 local AIO_UI_INIT_DELAY = 5*1000 local AIO_MSG_COMPRESS = true local AIO_CODE_OBFUSCATE = true local AIO_ERROR_LOG = false local assert = assert local type = type local tostring = tostring local pairs = pairs local ipairs = ipairs local ssub = string.sub local match = string.match local ceil = ceil or math.ceil local floor = floor or math.floor local sbyte = strbyte or string.byte local schar = string.char local tconcat = table.concat local select = select local pcall = pcall local xpcall = xpcall loadstring = loadstring or load unpack = unpack or table.unpack local AIO_GetTime = os and os.time or function() return GetTime()*1000 end local AIO_GetTimeDiff = os and os.difftime or function(_now, _then) return _now-_then end local AIO_SERVER = false local AIO_VERSION = 1.74 local AIO_ShortMsg = schar(1)..schar(1) local AIO_Compressed = 'C' local AIO_Uncompressed = 'U' local AIO_Prefix = 'AIO' AIO_Prefix = ssub((AIO_Prefix), 1, 16) local AIO_ServerPrefix = ssub(('S'..AIO_Prefix), 1, 16) local AIO_ClientPrefix = ssub(('C'..AIO_Prefix), 1, 16) assert(#AIO_ServerPrefix == #AIO_ClientPrefix) local AIO_MsgLen = 255 -1 -#AIO_ServerPrefix -#AIO_ShortMsg local MSG_MIN = 1 local MSG_MAX = 2^16-767 AIO = { 	unpack = unpack, } local AIO = AIO local AIO_SAVEDFRAMES = {} local AIO_SAVEDVARS = {} local AIO_SAVEDVARSCHAR = {} local AIO_INITED = false local AIO_HANDLERS = {} local AIO_INITHOOKS = {} local AIO_BLOCKHANDLES = {} local AIO_ADDONSORDER = {} local NewQueue = NewQueue or require('queue') local Smallfolk = Smallfolk or require('smallfolk') local lualzw = lualzw or require('lualzw') local LibWindow = LibStub('LibWindow-1.1') function AIO.GetVersion() return AIO_VERSION end local function AIO_16tostring(uint16) assert(uint16 <= 2^16-767, 'Too high value') assert(uint16 >= 0, 'Negative value') local high = floor(uint16 / 254) local l = high +1 local r = uint16 - high * 254 +1 return schar(l)..schar(r) end local function AIO_stringto16(str) local l = sbyte(ssub(str, 1,1)) -1 local r = sbyte(ssub(str, 2,2)) -1 local val = l*254 + r assert(val <= 2^16-767, 'Too high value') assert(val >= 0, 'Negative value') return val end function AIO_RESET() AIO_SAVEDVARS = nil AIO_SAVEDVARSCHAR = nil AIO_sv_Addons = nil AIO_SAVEDFRAMES = {} end function AIO_debug(...) if AIO_ENABLE_DEBUG_MSGS then print('AIO:', ...) end end local function AIO_extractN(...) return select('#', ...), ... end local function AIO_pcall(f, ...) assert(type(f) == 'function') if not AIO_ENABLE_PCALL then return f(...) end local data = {AIO_extractN(pcall(f, ...))} if not data[2] then if AIO_ERROR_LOG then AIO.Handle('AIO', 'Error', data[3]) end if AIO_ENABLE_TRACEBACK then _ERRORMESSAGE(data[3]) else print(data[3]) end return end return unpack(data, 3, data[1]+1) end local function AIO_ReadFile(path) AIO_debug('Reading a file') assert(type(path) == 'string', '#1 string expected') local f = assert(io.open(path, 'rb')) local str = f:read('*all') f:close() return str end local plrdata = {} local removeque = NewQueue() local function RemoveData(guid, msgid) local pdata = plrdata[guid] if pdata then if msgid then local data = pdata[msgid] if data then pdata[msgid] = nil pdata.ramque:gettable()[data.ramquepos] = nil removeque:gettable()[data.remquepos] = nil end else local que = pdata.ramque:gettable() local l, r = pdata.ramque:getrange() for i = l, r do if que[i] then removeque:gettable()[que[i].remquepos] = nil end end plrdata[guid] = nil end end end local function ProcessRemoveQue() if removeque:empty() then return end local now = AIO_GetTime() local l, r = removeque:getrange() for i = l, r do local v = removeque:popleft() if v then if AIO_GetTimeDiff(now, v.stamp) < AIO_MSG_CACHE_TIME then AIO_debug('removing outdated incomplete message') removeque:pushleft(v) break end RemoveData(v.guid, v.id) end end end local frame = CreateFrame('Frame') local timer = AIO_MSG_CACHE_DELAY local function ONUPDATE(self, diff) if timer > diff then timer = timer - diff else ProcessRemoveQue() timer = AIO_MSG_CACHE_DELAY end end frame:SetScript('OnUpdate', ONUPDATE) local function AIO_SendAddonMessage(msg, player) SendAddonMessage(AIO_ClientPrefix, msg, 'WHISPER', UnitName('player')) end local function AIO_Send(msg, player, ...) assert(type(msg) == 'string', '#1 string expected') assert(not AIO_SERVER or type(player) == 'userdata', '#2 player expected') AIO_debug('Sending message length:', #msg) if AIO_ENABLE_MSGPRINT then print('sent:', msg) end if #msg <= AIO_MsgLen then AIO_SendAddonMessage(AIO_ShortMsg..msg, player) else local guid = 1 if not plrdata[guid] then plrdata[guid] = { stored = 0, ramque = NewQueue(), MSG_GUID = MSG_MIN, } end local pdata = plrdata[guid] local msglen = (AIO_MsgLen-4) local parts = ceil(#msg / msglen) local header = AIO_16tostring(pdata.MSG_GUID)..AIO_16tostring(parts) if pdata.MSG_GUID >= MSG_MAX then pdata.MSG_GUID = MSG_MIN else pdata.MSG_GUID = pdata.MSG_GUID+1 end for i = 1, parts do AIO_SendAddonMessage(header..AIO_16tostring(i)..ssub(msg, ((i-1)*msglen)+1, (i*msglen)), player) end end if ... then for i = 1, select('#',...) do AIO_Send(msg, select(i, ...)) end end end local msgmt = {} function msgmt.__index(tbl, key) return msgmt[key] end function msgmt:Add(Name, ...) assert(Name, '#1 Block must have name') self.params[#self.params+1] = {select('#', ...), Name, ...} self.assemble = true return self end function msgmt:Append(msg2) assert(type(msg2) == 'table', '#1 table expected') for i = 1, #msg2.params do assert(type(msg2.params[i]) == 'table', '#1['..i..'] table expected') self.params[#self.params+1] = msg2.params[i] end self.assemble = true return self end function msgmt:Assemble() if not self.assemble then return self end self.MSG = Smallfolk.dumps(self.params) self.assemble = false return self end function msgmt:Send(player, ...) assert(not AIO_SERVER or player, '#1 player is nil') AIO_Send(self:ToString(), player, ...) return self end function msgmt:Clear() for i = 1, #self.params do self.params[i] = nil end self.MSG = nil self.assemble = false return self end function msgmt:ToString() return self:Assemble().MSG end function msgmt:HasMsg() return #self.params > 0 end function AIO.Msg() local msg = {params = {}, MSG = nil, assemble = false} setmetatable(msg, msgmt) return msg end local preinitblocks = {} local function AIO_HandleBlock(player, data, skipstored) local HandleName = data[2] assert(HandleName, 'Invalid handle, no handle name') if not AIO_INITED and (HandleName ~= 'AIO' or data[3] ~= 'Init') then preinitblocks[#preinitblocks+1] = data AIO_debug('Received block before Init:', HandleName, data[1], data[3]) return end local handledata = AIO_BLOCKHANDLES[HandleName] if not handledata then error('Unknown AIO block handle: '..tostring(HandleName)) end handledata(player, unpack(data, 3, data[1]+2)) if not skipstored and AIO_INITED and HandleName == 'AIO' and data[3] == 'Init' then for i = 1, #preinitblocks do AIO_HandleBlock(player, preinitblocks[i], true) preinitblocks[i] = nil end end end local curmsg = '' local function AIO_Timeout() error(string.format('AIO Timeout. Your code ran over %s instructions with message:%s', ''..AIO_TIMEOUT_INSTRUCTIONCOUNT..'', (curmsg or 'nil'))) end local function _AIO_ParseBlocks(msg, player) AIO_debug('Received messagelength:', #msg) if AIO_ENABLE_MSGPRINT then print('received:', msg) end local data = AIO_pcall(Smallfolk.loads, msg, #msg) if not data or type(data) ~= 'table' then AIO_debug('Received invalid message - data not a table') return end for i = 1, #data do AIO_pcall(AIO_HandleBlock, player, data[i]) end end local function AIO_ParseBlocks(msg, player) AIO_pcall(_AIO_ParseBlocks, msg, player) end local function _AIO_HandleIncomingMsg(msg, player) local msgid = ssub(msg, 1,2) if msgid == AIO_ShortMsg then AIO_ParseBlocks(ssub(msg, 3), player) return end if #msg < 3*2 then return end local messageId = AIO_stringto16(msgid) local parts = AIO_stringto16(ssub(msg, 3,4)) local partId = AIO_stringto16(ssub(msg, 5,6)) if partId <= 0 or partId > parts then error('received long message with invalid amount of parts. id, parts: '..partId..' '..parts) return end msg = ssub(msg, 7) local guid = 1 if not plrdata[guid] then plrdata[guid] = { stored = 0, ramque = NewQueue(), MSG_GUID = MSG_MIN, } end local pdata = plrdata[guid] pdata[messageId] = pdata[messageId] or {} local data = pdata[messageId] if not data.parts or data.parts.n ~= parts then if data.parts then for i = 0, data.parts.n do data.parts[i] = nil end end data.guid = guid data.parts = {n=parts} data.id = messageId data.stamp = AIO_GetTime() data.remquepos = removeque:pushright(data) data.ramquepos = pdata.ramque:pushright(data) end data.parts[partId] = msg pdata.stored = pdata.stored + #msg if #data.parts == data.parts.n then local cat = tconcat(data.parts) RemoveData(guid, messageId) AIO_ParseBlocks(cat, player) end end local function AIO_HandleIncomingMsg(msg, player) AIO_pcall(_AIO_HandleIncomingMsg, msg, player) end  function AIO.RegisterEvent(name, func) assert(name ~= nil, 'name of the registered event expected not nil') assert(type(func) == 'function', 'callback function must be a function') assert(not AIO_BLOCKHANDLES[name], 'an event is already registered for the name: '..name) AIO_BLOCKHANDLES[name] = func end  function AIO.AddHandlers(name, handlertable) assert(name ~= nil, '#1 expected not nil') assert(type(handlertable) == 'table', '#2 a table expected')  for k,v in pairs(handlertable) do assert(type(v) == 'function', '#2 a table of functions expected, found a '..type(v)..' value') end  local function handler(player, key, ...) if key and handlertable[key] then handlertable[key](player, ...) end end AIO.RegisterEvent(name, handler) return handlertable end  function AIO.AddAddon(path, name) end  function AIO.Handle(name, handlername, ...) assert(name ~= nil, '#1 expected not nil') return AIO.Msg():Add(name, handlername, ...):Send() end  function AIO.AddSavedVar(key) assert(key ~= nil, '#1 table key expected') AIO_SAVEDVARS[key] = true end  function AIO.AddSavedVarChar(key) assert(key ~= nil, '#1 table key expected') AIO_SAVEDVARSCHAR[key] = true end  AIO_FRAMEPOSITIONS = AIO_FRAMEPOSITIONS or {} AIO.AddSavedVar('AIO_FRAMEPOSITIONS') AIO_FRAMEPOSITIONSCHAR = AIO_FRAMEPOSITIONSCHAR or {} AIO.AddSavedVarChar('AIO_FRAMEPOSITIONSCHAR') function AIO.SavePosition(frame, char) assert(frame:GetName(), 'Called AIO.SavePosition on a nameless frame') local store = char and AIO_FRAMEPOSITIONSCHAR or AIO_FRAMEPOSITIONS if not store[frame:GetName()] then store[frame:GetName()] = {} end LibWindow.RegisterConfig(frame, store[frame:GetName()]) LibWindow.RestorePosition(frame) LibWindow.SavePosition(frame) table.insert(AIO_SAVEDFRAMES, frame) end local function ONADDONMSG(self, event, prefix, msg, Type, sender) if prefix == AIO_ServerPrefix then if event == 'CHAT_MSG_ADDON' and sender == UnitName('player') then AIO_HandleIncomingMsg(msg, sender) end end end local MsgReceiver = CreateFrame('Frame') MsgReceiver:RegisterEvent('CHAT_MSG_ADDON') MsgReceiver:SetScript('OnEvent', ONADDONMSG) local function RunAddon(name) local code = AIO_sv_Addons[name] and AIO_sv_Addons[name].code assert(code, 'Addon doesnt exist') local compression, compressedcode = ssub(code, 1, 1), ssub(code, 2) if compression == AIO_Compressed then compressedcode = assert(lualzw.decompress(compressedcode)) end assert(loadstring(compressedcode, name))() end function AIO_HANDLERS.Init(player, version, N, addons, cached) if(AIO_VERSION ~= version) then AIO_INITED = true AIO_HandleBlock = function() end print('You have AIO version '..AIO_VERSION..' and the server uses '..(version or 'nil')..'. Get the same version') return end assert(type(N) == 'number') assert(type(addons) == 'table') assert(type(cached) == 'table') local validAddons = {} for i = 1, N do local name if addons[i] then name = addons[i].name AIO_sv_Addons[name] = addons[i] validAddons[name] = true elseif cached[i] then name = cached[i] validAddons[name] = true else error('Unexpected behavior, try /aio reset') end AIO_pcall(RunAddon, name) end local invalidAddons = {} for name, data in pairs(AIO_sv_Addons) do if not validAddons[name] then invalidAddons[#invalidAddons+1] = name end end for i = 1, #invalidAddons do local inv = invalidAddons[i] AIO_sv_Addons[inv] = nil end AIO_INITED = true print('Initialized AIO version '..AIO_VERSION..'. Type `/aio help` for commands') end function AIO_HANDLERS.ForceReload(player) local frame = CreateFrame('BUTTON') frame:SetToplevel(true) frame:SetFrameStrata('TOOLTIP') frame:SetFrameLevel(100) frame:SetAllPoints(WorldFrame) frame:SetScript('OnClick', ReloadUI) print('AIO: Force reloading UI') message('AIO: Force reloading UI') end function AIO_HANDLERS.ForceReset(player) AIO_RESET() AIO_HANDLERS.ForceReload(player) end local frame = CreateFrame('FRAME') frame:RegisterEvent('PLAYER_LOGOUT') function frame:OnEvent(event, addon) if event == 'ADDON_LOADED' and addon == 'AIO_Client' then local _,_,_, tocversion = GetBuildInfo() if tocversion and tocversion >= 40100 and RegisterAddonMessagePrefix then RegisterAddonMessagePrefix('C'..AIO_Prefix) end if type(AIO_sv) ~= 'table' then AIO_sv = {} end if type(AIO_sv_char) ~= 'table' then AIO_sv_char = {} end if type(AIO_sv_Addons) ~= 'table' then AIO_sv_Addons = {} end for k,v in pairs(AIO_sv) do if _G[k] then AIO_debug('Overwriting global var _G['..k..'] with a saved var') end _G[k] = v end for k,v in pairs(AIO_sv_char) do if _G[k] then AIO_debug('Overwriting global var _G['..k..'] with a saved character var') end _G[k] = v end local rem = {} local addons = {} for name, data in pairs(AIO_sv_Addons) do if type(name) ~= 'string' or type(data) ~= 'table' or type(data.crc) ~= 'number' or type(data.code) ~= 'string' then table.insert(rem, name) else addons[name] = data.crc end end for _,name in ipairs(rem) do AIO_sv_Addons[name] = nil end local initmsg = AIO.Msg():Add('AIO', 'Init', AIO_VERSION, addons) local reset = 1 local timer = reset local function ONUPDATE(self, diff) if AIO_INITED then self:SetScript('OnUpdate', nil) initmsg = nil reset = nil timer = nil return end if timer < diff then initmsg:Send() timer = reset reset = reset * 1.5 else timer = timer - diff end end frame:SetScript('OnUpdate', ONUPDATE) elseif event == 'PLAYER_LOGOUT' then AIO_sv = {} for key,_ in pairs(AIO_SAVEDVARS or {}) do AIO_sv[key] = _G[key] end AIO_sv_char = {} for key,_ in pairs(AIO_SAVEDVARSCHAR or {}) do AIO_sv_char[key] = _G[key] end for k,v in ipairs(AIO_SAVEDFRAMES or {}) do LibWindow.SavePosition(v) end end end frame:SetScript('OnEvent', frame.OnEvent) AIO.AddHandlers('AIO', AIO_HANDLERS) local cmds = {} local helps = {} local function pprint(player, ...) if player then player:SendBroadcastMessage(tconcat({...}, ' ')) else print(...) end end SLASH_AIO1 = '/aio' function SlashCmdList.AIO(msg) local msg = msg:lower() if msg and msg ~= '' then for k,v in pairs(cmds) do if k:find(msg, 1, true) == 1 then v() return end end end print('Unknown command /aio '..tostring(msg)) cmds.help() end helps.help = 'prints this list' function cmds.help(player) pprint(player, 'Available commands:') for k,v in pairs(cmds) do pprint(player, '/aio '..k..' - '..(helps[k] or 'no info')) end end helps.reset = 'resets local AIO cache - clears saved addons and their saved variables and reloads the UI' function cmds.reset() AIO_RESET() ReloadUI() end helps.trace = 'toggles using debug.traceback or _ERRORMESSAGE' function cmds.trace(player) AIO_ENABLE_TRACEBACK = not AIO_ENABLE_TRACEBACK pprint(player, 'using trace is now', AIO_ENABLE_TRACEBACK and 'on' or 'off') end helps.debug = 'toggles showing of debug messages' function cmds.debug(player) AIO_ENABLE_DEBUG_MSGS = not AIO_ENABLE_DEBUG_MSGS pprint(player, 'showing debug messages is now', AIO_ENABLE_DEBUG_MSGS and 'on' or 'off') end helps.pcall = 'toggles using pcall' function cmds.pcall(player) AIO_ENABLE_PCALL = not AIO_ENABLE_PCALL pprint(player, 'using pcall is now', AIO_ENABLE_PCALL and 'on' or 'off') end helps.printio = 'toggles printing all sent and received messages' function cmds.printio(player) AIO_ENABLE_MSGPRINT = not AIO_ENABLE_MSGPRINT pprint(player, 'printing IO is now', AIO_ENABLE_MSGPRINT and 'on' or 'off') end frame:OnEvent('ADDON_LOADED', 'AIO_Client') return AIO)";
std::string TooltipMod = R"(function ModifyTooltipStats(tooltip, item) for i = 1, tooltip:NumLines() do local line = _G[tooltip:GetName()..'TextLeft'..i]; local value, stat = string.match(line:GetText(), '%+(%d+) ([%w%p]+)'); if value and stat then value = math.ceil(tonumber(value) * 1.1); line:SetText('+'..value..' '..stat); end end tooltip:Show(); end GameTooltip:HookScript('OnTooltipSetItem', function(tooltip, ...) local _, item = tooltip:GetItem() if item then ModifyTooltipStats(tooltip, item) end end))";

std::string replaceCurlyBraces(std::string str) {
    std::string result;
    for (char c : str) {
        if (c == '{') {
            result += "{{";
        } else if (c == '}') {
            result += "}}";
        } else {
            result += c;
        }
    }
    return result;
}

static struct WardenLoader {
    std::vector<std::string> order = {
            "lualzw",
            "Smallfolk",
            "Queue",
            "LibStub",
            "LibWindow",
            "CMH",
            "StatPointUI",
            "AIO",
            "TooltipMod",
    };

    struct WardenData {
        int version = 1;
        bool compressed = false;
        bool cached = true;
        std::string payload;
        std::vector<std::string> savedVars;
    };

    std::map<std::string, WardenData> data = {
            {"lualzw", {1, false, true, replaceCurlyBraces(lualzw), {}}},
            {"Smallfolk", {1, false, true, replaceCurlyBraces(Smallfork), {}}},
            {"Queue", {1, false, true, replaceCurlyBraces(Queue), {}}},
            {"LibStub", {1, false, true, replaceCurlyBraces(LibStub), {}}},
            {"LibWindow", {1, false, true, replaceCurlyBraces(LibWindow), {}}},
            {"CMH", {1, false, true, replaceCurlyBraces(CMH), {}}},
            {"StatPointUI", {1, false, true, replaceCurlyBraces(StatPointUI), {}}},
            {"AIO", {1, false, true, replaceCurlyBraces(AIO), {"AIO_sv", "AIO_sv_Addons"}}},
            {"TooltipMod", {1, false, true, replaceCurlyBraces(TooltipMod), {}}},
    };
} wardenLoader;

WorldPacket CreateAddonPacket(const std::string& prefix, const std::string& msg, ChatMsg msgType, Player* player)
{
    WorldPacket data;

    std::string fullMsg = prefix + "\t" + msg;
    size_t len = fullMsg.length();

    data.Initialize(SMSG_MESSAGECHAT, 1 + 4 + 8 + 4 + 8 + 4 + 1 + len + 1);
    data << uint8(msgType); //Type
    data << uint32(LANG_ADDON); //Lang
    data << uint64(player->GetGUID().GetRawValue()); //SenderGUID
    data << uint32(0); //Flags
    data << uint64(player->GetGUID().GetRawValue()); //ReceiverGUID
    data << uint32(len + 1); //MsgLen
    data << fullMsg; //Msg
    data << uint8(0);

    return data;
}

void WardenInjectMgr::SendAddonMessage(const std::string& prefix, const std::string& payload, ChatMsg msgType, Player* player)
{
    WorldPacket payloadPacket = CreateAddonPacket(prefix, payload, msgType, player);
    player->SendDirectMessage(&payloadPacket);
}

// Loads the contents of a Lua file into a string variable
std::string WardenInjectMgr::LoadLuaFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (file.fail())
    {
        LOG_ERROR("module", "WardenInjectMgr::LoadLuaFile - Loading file {} failed.", filePath);
        return nullptr;
    }

    std::string luaCode;
    char buffer[1024];

    while (file.read(buffer, sizeof(buffer)))
    {
        luaCode.append(buffer, sizeof(buffer));
    }
    luaCode.append(buffer, file.gcount());
    file.close();

    if (luaCode.empty())
    {
        LOG_WARN("module", "WardenInjectMgr::LoadLuaFile - File {} is empty.", filePath);
        return nullptr;
    }

    return luaCode;
}

// Removes leading whitespaces and replaces line breaks with spaces
void WardenInjectMgr::ConvertToPayload(std::string& luaCode)
{
    // Trim whitespace from the beginning and end of the string
    Acore::String::Trim(luaCode);

    // Remove unwanted characters (all whitespace except spaces)
    luaCode.erase(std::remove(luaCode.begin(), luaCode.end(), '\t'), luaCode.end());
    luaCode.erase(std::remove(luaCode.begin(), luaCode.end(), '\n'), luaCode.end());
    luaCode.erase(std::remove(luaCode.begin(), luaCode.end(), '\r'), luaCode.end());

    // Remove non-printable characters
    luaCode.erase(std::remove_if(luaCode.begin(), luaCode.end(), [](unsigned char c) { return !std::isprint(c); }), luaCode.end());
}

std::string WardenInjectMgr::GetPayloadFromFile(std::string filePath)
{
    std::string luaCode = LoadLuaFile(filePath);
    if (luaCode.empty())
    {
        return nullptr;
    }

    ConvertToPayload(luaCode);
    if (luaCode.empty())
    {
        LOG_WARN("module", "WardenInjectMgr::GetPayloadFromFile - Code from file {} is empty after removing whitespaces.", filePath);
        return nullptr;
    }

    return luaCode;
}

WardenPayloadMgr* GetWardenPayloadMgr(WorldSession* session)
{
    if (!session)
    {
        return nullptr;
    }

    WardenWin* warden = (WardenWin*)session->GetWarden();
    if (!warden)
    {
        return nullptr;
    }

    if (!warden->IsInitialized())
    {
        return nullptr;
    }

    return warden->GetPayloadMgr();
}

void WardenInjectMgr::SendLargePayload(Player* player, const std::string& addon, int version, bool cache, bool comp, const std::string& data)
{
    std::vector <uint8_t> compressedBytes;
    /* TODO
    if (comp == 1)
    {
        compressedBytes = compress(data);
        if (compressedBytes.empty())
        {
            comp = 0;
        }
    }
    */
    const uint32_t maxChunkSize = 900;
    std::vector<std::string> chunks;
    for (size_t i = 0; i < data.length(); i += maxChunkSize)
    {
        chunks.push_back(data.substr(i, maxChunkSize));
    }
    if (chunks.size() > 99)
    {
        return;
    }
    std::string payloadSizeStr = std::to_string(chunks.size());
    if (chunks.size() < 10)
    {
        payloadSizeStr = "0" + payloadSizeStr;
    }
    for (size_t i = 0; i < chunks.size(); i++)
    {
        std::string chunkNumStr = std::to_string(i + 1);
        if (i + 1 < 10)
        {
            chunkNumStr = "0" + chunkNumStr;
        }
        std::string payload = Acore::StringFormatFmt("_G['{}'].f.p('{}', '{}', '{}', {}, {}, {}, [[{}]]);", cGTN, chunkNumStr, payloadSizeStr, addon, version, cache, comp, chunks[i].c_str());
        if (comp == 1)
        {
            std::string payload = Acore::StringFormatFmt("_G['{}'].f.p('{}', '{}', '{}', {}, {}, {}, [[{}]]);", cGTN, chunkNumStr, payloadSizeStr, addon, version, cache, comp, (std::string(compressedBytes.begin(), compressedBytes.end())));
        }

        SendAddonMessage("ws", payload, CHAT_MSG_WHISPER, player);
    }
}

void WardenInjectMgr::SendPayloadInform(Player* player)
{
    // If the player has any payloads already cached, then they will be loaded immediately
    // Otherwise, full payloads will be requested
    for (const auto& v : wardenLoader.order) {
        const auto& t = wardenLoader.data[v];
        // register all saved variables if there are any
        for (const auto& var : t.savedVars) {
            SendAddonMessage("ws", "_G['" + cGTN + "'].f.r('" + var + "')", CHAT_MSG_WHISPER, player);
        }

        std::string message = Acore::StringFormatFmt("_G['{}'].f.i('{}', {}, {}, {})", cGTN, v, t.version, t.cached, t.compressed);
        //LOG_INFO("module", "Sending payload inform: {}", message);
        SendAddonMessage("ws", message, CHAT_MSG_WHISPER, player);
        //SendLargePayload(player, v, t.version, t.cached, t.compressed, t.payload);
    }
}

void WardenInjectMgr::SendSpecificPayload(Player* player, std::string payloadName)
{
    // If the player has any payloads already cached, then they will be loaded immediately
    // Otherwise, full payloads will be requested
    const WardenLoader::WardenData& data = wardenLoader.data[payloadName];
    if(data.payload.empty())
    {
        LOG_ERROR("module", "Specific Payload not found: {}", payloadName);
        return;
    }

    // register all saved variables if there are any
    for (const auto& var : data.savedVars) {
        SendAddonMessage("ws", "_G['" + cGTN + "'].f.r('" + var + "')", CHAT_MSG_WHISPER, player);
    }

    std::string message = Acore::StringFormatFmt("_G['{}'].f.i('{}', {}, {}, {})", cGTN, payloadName, data.version, data.cached, data.compressed);
    SendAddonMessage("ws", message, CHAT_MSG_WHISPER, player);
    //LOG_INFO("module", "Sending payload specific: {}", message);
    //SendLargePayload(player, payloadName, data.version, data.cached, data.compressed, data.payload);
}

void WardenInjectMgr::SendAddonInjector(Player* player)
{
    // Overwrite reload function
    SendAddonMessage("ws", "local copy,new=_G['ReloadUI'];function new() SendAddonMessage('wc', 'reload', 'WHISPER', UnitName('player')) copy() end _G['ReloadUI'] = new", CHAT_MSG_WHISPER, player);
    SendAddonMessage("ws", "SlashCmdList['RELOAD'] = function() _G['ReloadUI']() end", CHAT_MSG_WHISPER, player);

    // Generate helper functions to load larger datasets
    SendAddonMessage("ws", "_G['" + cGTN + "'] = { f = {}, s = {}, i = {0}, d = " + dbg + "};", CHAT_MSG_WHISPER, player);
    // Load
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.l = function(s, n) local t=_G['" + cGTN + "'].i; forceinsecure(); loadstring(s)(); if(t.d) then print('[WardenLoader]: '..n..' loaded!') end t[1] = t[1]+1; if(t[1] == t[2]) then SendAddonMessage('wc', 'kill', 'WHISPER', UnitName('player')) end end", CHAT_MSG_WHISPER, player);
    // Concatenate
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.c = function(a) local b='' for _,d in ipairs(a) do b=b..d end; return b end", CHAT_MSG_WHISPER, player);
    // Execute
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.e = function(n) local t=_G['" + cGTN + "']; local lt = t.s[n] local fn = t.f.c(lt.ca); _G[n..'payload'] = {v = lt.v, p = fn}; if(lt.co==1) then fn = lualzw.decompress(fn) end t.f.l(fn, n) t.s[n]=nil end", CHAT_MSG_WHISPER, player);
    // Process
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.p = function(a, b, n, v, c, co, s) local t,tc=_G['" + cGTN + "'], _G['" + cGTN + "'].s; if not tc[n] then tc[n] = {['v']=v, ['co']=co, ['c']=c, ['ca']={}} end local lt = tc[n] a=tonumber(a) b=tonumber(b) table.insert(lt.ca, a, s) if a == b and #lt.ca == b then t.f.e(n) end end", CHAT_MSG_WHISPER, player);
    // Inform
    // One potential issue is dependency load order and requirement, this is something I'll have to look into at some point..
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.i = function(n, v, c, co, s) local t=_G['" + cGTN + "']; if not(t.i[2]) then t.i[2] = tonumber(s) end if(c == 1) then RegisterForSave(n..'payload'); local cc = _G[n..'payload'] if(cc) then if(cc.v == v) then local p = cc.p if(co == 1) then p = lualzw.decompress(p) end t.f.l(p, n) return; end end end SendAddonMessage('wc', 'req'..n, 'WHISPER', UnitName('player')) end", CHAT_MSG_WHISPER, player);
    // cache registry
    SendAddonMessage("ws", "_G['" + cGTN + "'].f.r = function(a) RegisterForSave(a); end", CHAT_MSG_WHISPER, player);

    // Sends an inform to the player about the available payloads
    //SendPayloadInform(player);
}

void WardenInjectMgr::PushInitModule(Player* player)
{
    SendAddonInjector(player);
    SendAddonMessage("ws", "SendAddonMessage('wc', 'loaded', 'WHISPER', UnitName('player')); print('[WardenLoader]: Injection successful.')", CHAT_MSG_WHISPER, player);
}

void WardenInjectMgr::InitialInjection(Player* player)
{
    WorldSession* session = player->GetSession();

    if (!session)
    {
        return;
    }

    WardenWin* warden = (WardenWin*)session->GetWarden();

    if (!warden)
    {
        LOG_ERROR("module", "WardenInjectMgr::InitialInjection - Warden could not be found for player {}.", player->GetName());
        return;
    }

    if (!warden->IsInitialized())
    {
        LOG_ERROR("module", "WardenInjectMgr::InitialInjection - Warden is not initialized for player {}.", player->GetName());
        return;
    }

    auto payloadMgr = warden->GetPayloadMgr();
    if (!payloadMgr)
    {
        LOG_ERROR("module", "WardenInjectMgr::InitialInjection - Payload Manager could not be found for player {}.", player->GetName());
        return;
    }

    if (!payloadMgr->GetPayloadById(9601))
    {
        payloadMgr->RegisterPayload(strOne, 9601);
    }

    if (!payloadMgr->GetPayloadById(9602))
    {
        payloadMgr->RegisterPayload(strTwo, 9602);
    }

    // Push to front of queue if NumLuaChecks > 1 as it reverts the order
    bool pushToFront = sWorld->getIntConfig(CONFIG_WARDEN_NUM_LUA_CHECKS) > 1;

    // warden->ForceChecks();
    payloadMgr->QueuePayload(9601, pushToFront);
    // warden->ForceChecks();
    payloadMgr->QueuePayload(9602, pushToFront);
    // warden->ForceChecks();

    ChatHandler(player->GetSession()).PSendSysMessage("[WardenLoader]: Awaiting injection...");
}

void WardenInjectMgr::OnAddonMessageReceived(Player* player, uint32 type, const std::string& header, const std::string& data)
{
    if (type != CHAT_MSG_WHISPER)
    {
        return;
    }

    LOG_INFO("module", "Header: \"{}\" - Data: \"{}\"", header, data);

    if (header != "wc")
    {
        return;
    }

    if (data == "reload")
    {
        // flag the player for re-injection if they reloaded their ui
        InitialInjection(player);
    }
    else if (data == "init")
    {
        LOG_INFO("module", "WardenInjectMgr::OnAddonMessageReceived - Received initialized from player {}.", player->GetName());
        PushInitModule(player);
    }
    else if (data == "loaded")
    {
        // module is loaded and ready to receive data
        //player->SetData("ModuleInit", true);
        LOG_INFO("module", "WardenInjectMgr::OnAddonMessageReceived - Received loaded command from player {}.", player->GetName());
    }
    else if (data == "kill")
    {
        LOG_INFO("module", "WardenInjectMgr::OnAddonMessageReceived - Received kill command from player {}.", player->GetName());
        // kill the initial loader, this is to prevent spoofed addon packets with access to the protected namespace
        // the initial injector can not be used after this point, and the injected helper functions above are the ones that need to be used.
        SendAddonMessage("ws", "false", CHAT_MSG_WHISPER, player);

        // if the below is printed in your chat, then the initial injector was not disabled and you have problems to debug :)
        SendAddonMessage("ws", "print('[WardenLoader]: This message should never show.')", CHAT_MSG_WHISPER, player);
    }
    else if (data.substr(0, 3) == "req")
    {
        std::string addon = data.substr(3);
        LOG_INFO("module", "WardenInjectMgr::OnAddonMessageReceived - Received request for addon {} from player {}.", addon, player->GetName());
        const WardenLoader::WardenData& data = wardenLoader.data[addon];
        if(data.payload.empty())
        {
            LOG_ERROR("module", "Requested Payload not found: {}", addon);
            return;
        }

        SendLargePayload(player, addon, data.version, data.cached, data.compressed, data.payload);
    }

}
