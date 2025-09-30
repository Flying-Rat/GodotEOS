// Copyright Epic Games, Inc. All Rights Reserved.

#include "StringUtils.h"
#include <algorithm>
#include <string>
#include <vector>
#include <cassert>

std::wstring FStringUtils::ToUpper(const std::wstring & Src)
{
	std::wstring Dest(Src);
	std::transform(Src.begin(), Src.end(), Dest.begin(), ::toupper);
	return std::move(Dest);
}

std::wstring FStringUtils::Widen(const std::string& Utf8)
{
	// Simple conversion for ASCII strings - for full UTF-8 support, would need utf8 library
	std::wstring Result;
	Result.reserve(Utf8.size());
	for (char c : Utf8) {
		Result += static_cast<wchar_t>(c);
	}
	return Result;
}

std::string FStringUtils::Narrow(const std::wstring& Str)
{
	// Simple conversion for ASCII strings - for full UTF-8 support, would need utf8 library
	std::string Result;
	Result.reserve(Str.size());
	for (wchar_t c : Str) {
		if (c <= 127) {
			Result += static_cast<char>(c);
		} else {
			Result += '?'; // Placeholder for non-ASCII
		}
	}
	return Result;
}

std::vector<std::wstring> FStringUtils::Split(const std::wstring& Str, const wchar_t Delimiter)
{
	std::vector<std::wstring> Words;

	std::size_t Current, Previous = 0;
	Current = Str.find(Delimiter);
	while (Current != std::string::npos) {
		Words.push_back(Str.substr(Previous, Current - Previous));
		Previous = Current + 1;
		Current = Str.find(Delimiter, Previous);
	}
	Words.push_back(Str.substr(Previous, Current - Previous));

	return Words;
}