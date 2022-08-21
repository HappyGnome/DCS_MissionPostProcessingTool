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
	for i,row in ipairs(csv) do
		if i>1 then --skip header line
			local ctryName = row[1]
			local typeName = row[2]
			local radioNumber = row[3]
			local channels = {}
			for j,v in ipairs(row) do
				if j>3 then
					channels[#channels + 1] = v
				end
			end
			applyRadioChannelsRow(ctryName,typeName,radioNumber, channels)
		end
	end
	return core.luaObjToLuaString(mission,'mission')
end

--[[
Extract a 2D table of numbers and strings to be saved in a csv file
	
@return: table of tables, each representing a row in the csv file. Keys are ignored and values taken in index order.
--]]
extractCsv = function()
	local ret = {{"Country","Type","Radio #","Preset 1","Preset 2","..."}}
	local once = {}
	for coaK,coaV in pairs(mission.coalition) do
		for ctryI,ctryV in ipairs(coaV.country) do
			if ctryV.plane and ctryV.plane.group then
				for gpI,gpV in ipairs(ctryV.plane.group) do
					appendRadioChannelsRow(ret,gpV, ctryV.name, once)
				end
			end
			if ctryV.helicopter and ctryV.helicopter.group then
				for gpI,gpV in ipairs(ctryV.helicopter.group) do
					appendRadioChannelsRow(ret,gpV, ctryV.name, once)
				end
			end
		end
	end
	return ret
end

------------------------------------------
appendRadioChannelsRow = function(csvData,group, ctryName, once)
	if group.units then 
		for unitI,unitV in ipairs(group.units) do
			if unitV.skill == "Client" or unitV.skill == "Player" and unitV.Radio then
				for radioI,radioV in ipairs(unitV.Radio) do
					local row = {ctryName,unitV.type,radioI}
					
					if not once[row[1]] then once[row[1]] ={} end
					if not once[row[1]][row[2]] then once[row[1]][row[2]] ={} end
					
					if not once[row[1]][row[2]][row[3]] and radioV.channels then
						for channelI,channelV in ipairs(radioV.channels) do
							row[#row + 1] = channelV
						end
						csvData[#csvData + 1] = row;
						once[row[1]][row[2]][row[3]] = true
					end
				end
			end
		end
	end
end

applyRadioChannelsRow = function(ctryName,typeName,radioNumber, channels)
	
	for coaK,coaV in pairs(mission.coalition) do
		for ctryI,ctryV in ipairs(coaV.country) do
			if ctryV.name == ctryName then
				if ctryV.plane and ctryV.plane.group then
					for gpI,gpV in ipairs(ctryV.plane.group) do
						applyRadioChannelsRowToGroup(gpV,typeName,radioNumber, channels)
					end
				end
				if ctryV.helicopter and ctryV.helicopter.group then
					for gpI,gpV in ipairs(ctryV.helicopter.group) do
						applyRadioChannelsRowToGroup(gpV,typeName,radioNumber, channels)
					end
				end
			end
		end
	end	
end

applyRadioChannelsRowToGroup = function(group,typeName,radioNumber, channels)
	if group.units then 
		for unitI,unitV in ipairs(group.units) do
			if unitV.type == typeName and (unitV.skill == "Client" or unitV.skill == "Player") then
				for i,v in ipairs(channels) do
					unitV.Radio[radioNumber].channels[i] = v
				end			
			end
		end
	end
end