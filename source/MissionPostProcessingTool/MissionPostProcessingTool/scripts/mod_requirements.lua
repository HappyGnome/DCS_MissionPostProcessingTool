-- Globals loaded:
-- core - helper methods for mission file/csv io
-- mission - DCS mission file object

--[[
Apply changes to mission file object and return new mission file content
@args: 
	csv - 2 dimensional table of numbers and strings, representing data in an input csv file
	
@return: a string to be the new content in the mission file. Use core.luaObjToLuaString(mission,'mission') to format the return properly.
--]]
applyCsv = function(csv)
	local newReqs = {}
	for i,row in ipairs(csv) do
		if i>1 then --skip header line
			local modName = row[1]
			newReqs[modName] = modName
		end	
	end
	mission.requiredModules = newReqs
	return core.luaObjToLuaString(mission,'mission')
end

--[[
Extract a 2D table of numbers and strings to be saved in a csv file
	
@return: table of tables, each representing a row in the csv file. Keys are ignored and values taken in index order.
--]]
extractCsv = function()
	local ret = {{"Mod Name"}}
	local once = {}
	if mission.requiredModules then
		for modK,modV in pairs(mission.requiredModules) do
			ret[#ret + 1] = {[1] = modK}
		end
	end
	return ret
end

------------------------------------------