-- Globals loaded:
-- mission - DCS mission file object


clientAirframeList = function(csv)
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

clientAirframeList = function()
	local ret = {}
	for coaK,coaV in pairs(mission.coalition) do
		for ctryI,ctryV in ipairs(coaV.country) do
			if ctryV.plane then
				for gpI,gpV in ipairs(ctryV.plane.group) do
					appendGroupData(ret,gpV)
				end
			end
			if ctryV.helicopter then
				for gpI,gpV in ipairs(ctryV.helicopter.group) do
					appendGroupData(ret,gpV)
				end
			end
		end
	end
	return ret
end

------------------------------------------
appendGroupData = function(ret,group)
	for unitI,unitV in ipairs(group.units) do
		if unitV.skill == "Client" or unitV.skill == "Player" then
			ret[unitV.type] = true
		end
	end
end
