#pragma once
#ifndef __STRING_HELPER_H__

namespace str_helpers {

	bool TryAToF(const char* a, float& out);

	bool TryAToI(const char* a, int& out);

	bool TryAToB(const char* a, int& out);

	bool TryAToNil(const char* a);

	std::string BToS(int b);

	std::string NilToS();
}

#endif
