core = {}

core.luaStringEscape = function(input)
	local output = string.gsub(input,'\\','\\\\')
	output = string.gsub(output,'\n','\\n')
	output = string.gsub(output,'\r','\\r')
	output = string.gsub(output,'\v','\\v')
	output = string.gsub(output,'\t','\\t')
	output = string.gsub(output,'\f','\\f')
	output = string.gsub(output,'\a','\\a')
	output = string.gsub(output,'\b','\\b')
	output = string.gsub(output,'\'','\\\'')
	output = string.gsub(output,'\"','\\\"')
	return output
end

core.luaObjToLuaString = function(t,name,indent)
	if name == nil then name = "" end
	if indent == nil then indent = "" end
	
	local res = indent .. name 
	if name ~= "" then
		res = res .. " = "
	end
	if type(t) == "table" then
		local newIndent = indent .. "    "
		res = res .. "\n"..indent .. "{\n";
		for k,v in pairs(t) do
			local key = ""
			if type(k) == "number" then
				key =  "["..tostring(k).."]"
			elseif type(k) == "string" then
				key =  "[\"".. core.luaStringEscape(k).."\"]"
			end
			res = res .. core.luaObjToLuaString(v,key,newIndent)
		end
		if indent ~= "" then
			res = res .. indent .. "}, -- end of " .. name .. "\n"
		else
			res = res .. "} -- end of " .. name .. "\n"
		end
	elseif type(t) == "number" or type(t) == "boolean" then
		res = res .. tostring(t) .. ",\n"
	elseif type(t) == "string" then
		res = res .. "\"".. core.luaStringEscape(t).."\",\n"
	else 
		res = res .. type(t)
	end
	
	return res
end