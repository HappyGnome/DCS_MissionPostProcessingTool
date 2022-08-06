#include <iostream>

#include "CsvFileTools.h"
#include "MizFileTools.h"


bool CSVTest1() {
    bool ok = true;
    ok &= CsvFileTools::EscapeCSVEntry("") == "";
    ok &= CsvFileTools::EscapeCSVEntry("\"hahaha\" ") == "\"\"\"hahaha\"\" \"";
    ok &= CsvFileTools::EscapeCSVEntry(",hahaha\" \n") == "\",hahaha\"\" \"";
    ok &= CsvFileTools::EscapeCSVEntry("1234") == "1234";
    return ok;
}

bool CSVTest2() {
    bool ok = true;
   /* ok &= CsvFileTools::SplitCSVLine("") == std::vector<std::string>{""};
    ok &= CsvFileTools::SplitCSVLine("\"\"\"hahaha\"\" \"") == std::vector<std::string>{"\"hahaha\" "};
    ok &= CsvFileTools::SplitCSVLine("\",hahaha\"\" \"") == std::vector<std::string>{",hahaha\" "};
    ok &= CsvFileTools::SplitCSVLine("\",hahaha\"\" \",1234,\"hi,\"") == std::vector<std::string>{",hahaha\" ","1234","hi,"};
    ok &= CsvFileTools::SplitCSVLine("1234") == std::vector<std::string>{"1234"};*/
    return ok;
}

int main() {
    CSVTest1();
    CSVTest2();
    return 1;
}


