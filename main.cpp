#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>

//New natives for getting the default value of tunables by their hash
#define INTFUNC "D94071E55F4C9CE"
#define BOOLFUNC "B327CF1B8C2C0EA3"
#define FLOATFUNC "367E5E33E7F0DD1A"

//Tunable global index
#define TUNABLE 262145

//Helper function for defining the output format
std::string formatter(uint32_t hash, uint64_t offset) {
	std::stringstream s;
	s << "{";
	s << "0x" << std::hex << std::uppercase << hash;
	s << ", 0x" << offset << std::dec << std::nouppercase;
	s << "},";
	return s.str();
}

//Splits a string by a substring
void split(const std::string& str, const std::string& splitBy, std::vector<std::string>& tokens) {
	tokens.push_back(str);
	size_t splitAt;
	size_t splitLen = splitBy.size();
	std::string frag;
	while (true) {
		frag = tokens.back();
		splitAt = frag.find(splitBy);
		if (splitAt == std::string::npos)
			break;
		tokens.back() = frag.substr(0, splitAt);
		tokens.push_back(frag.substr(splitAt + splitLen, frag.size() - (splitAt + splitLen)));
	}
}

//Checks if a string exists inside another string
bool contains(std::string& data, const std::string& content) {
	return data.find(content) != std::string::npos;
}

//Replaces a substring in a string with another string
std::string replace(std::string subject, const std::string& search, const std::string& replace) {
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}

//Joaat function for turning hash strings into actual hashes
uint32_t joaat(std::string str) {
	uint32_t hash = 0;
	for (char c : str)
	{
		hash += tolower(c);
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

//Parses a global index
//Examples:
//Global_262145.f_2 becomes 262147 (262145 + 2)
//Global_262145.f_16[1] becomes 262162 (262145 + 16 + 1)
//Global_262145.f_4731[1 /*51*/][2] becomes 266930 (262145 + 4731 + 1 + 1 * 51 + 2)
uint64_t parseGlobalIndex(std::string global) {
	//We only pass the offset after Global_262145, so we just return the global index + 0 when there is no offset
	if (global.empty())
		return TUNABLE;
	uint64_t offset = 0;
	//Repeat this until there are no brackets left since there may be multiple ones
	while (contains(global, "[")) {
		size_t opening = global.find_first_of("[");
		size_t closing = global.find_first_of("]", opening);
		//Get the content inside the brackets
		std::string content = global.substr(opening + 1, closing - opening - 1);
		//If there's no size comment we can just add the array index to our offset
		if (!contains(content, "/*"))
			offset += std::stoull(content);
		else {
			//Otherwise we have a struct which needs to be parsed in a different way
			std::vector<std::string> temp;
			//Split the string by a comment, so that temp[0] is the array index and temp[1] is the size of the struct
			split(content, "/*", temp);
			temp[1] = replace(temp[1], "*/", "");
			//Add 1 + index * size to the offset
			offset += 1 + std::stoull(temp[0]) * std::stoull(temp[1]);
		}
		//Remove the current block from the string to move on
		global = replace(global, "[" + content + "]", "");
	}
	//After parsing the array indices, we can add the f_ offset
	offset += std::stoull(global);
	//Return the base tunable index plus our offset
	return TUNABLE + offset;
}

//Storage for the tunable hashes and their global indices
std::unordered_map<uint32_t, uint64_t> globals;

int main(int argc, char** argv) {
	//We need to pass a decompiled tunable file
	if (argc <= 1) {
		std::cout << "Usage: TunableParser tunable_processing_file.c" << std::endl;
		return 0;
	}
	//Read the file and split it by its lines
	std::fstream file(argv[1]);
	std::string content(std::istreambuf_iterator<char>(file), {});
	std::vector<std::string> lines;
	file.close();
	split(content, "\n", lines);
	bool foundRegistrationFunc = false;
	for (std::string line : lines) {
		//There's nothing to do for us if there's only one char or less in the line
		if (line.size() <= 1)
			continue;
		//Skip all lines until we found the tunable registration function
		if (!foundRegistrationFunc) {
			if (contains(line, "void func_2()"))
				foundRegistrationFunc = true;
			continue;
		}
		if (contains(line, "Global_" + std::to_string(TUNABLE))) {
			bool isFloat = contains(line, FLOATFUNC);
			bool isInt = !isFloat && contains(line, INTFUNC);
			bool isBool = !isFloat && !isInt && contains(line, BOOLFUNC);
			//Check if one of the three tunable natives is present in the line
			if (!isFloat && !isInt && !isBool)
				continue;
			uint32_t hash = -1;
			size_t loceq = line.find_first_of("=");
			//Get the tunable hash by looking for the first bracket which is the native call
			size_t location = line.find_first_of("(", loceq);
			//Look at the first parameter by extracting a substring from the first bracket to the first comma
			std::string sub = line.substr(location + 1, line.find_first_of(",", loceq) - location - 1);
			//If the hash is present as a string, we need to joaat hash it
			if (sub[0] == 'j') {
				sub = replace(sub, "joaat(", "");
				sub = replace(sub, ")", "");
				sub = replace(sub, "\"", "");
				hash = joaat(sub);
			}
			//Otherwise we just store the hash value
			if (hash == -1)
				hash = (uint32_t)std::stoul(sub);
			//Next we parse the global index by using our function above
			std::string globalPart = line.substr(0, loceq - 1);
			//Remove all spaces, the leading global offset and the f_ part before the offset
			globalPart = replace(globalPart, " ", "");
			globalPart = replace(globalPart, "Global_" + std::to_string(TUNABLE), "");
			globalPart = replace(globalPart, ".f_", "");
			uint64_t index = parseGlobalIndex(globalPart);
			//Add the hash and index to the map
			globals.emplace(hash, index);
		}
	}
	//Create an output file and store the content of the unordered map in it
	file = std::fstream();
	file.open("output.txt", std::ios::out);
	for (auto it = globals.begin(); it != globals.end(); it++)
		file << formatter(it->first, it->second) << std::endl;
	file.close();
}
