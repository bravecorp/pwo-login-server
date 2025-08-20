/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef UTILS_TOOLS_H
#define UTILS_TOOLS_H

#include <random>
#include <regex>
#include <any>

#include <boost/algorithm/string.hpp>

std::string transformToSHA1(const std::string& input);

uint32_t adlerChecksum(const uint8_t* data, size_t len);

int64_t OTSYS_TIME();

std::string demangleName(const char* mangledName);

template<typename T>
T anyCast(std::vector<std::any>& vec, int index, T defaultValue = T())
{
	try {
		return std::any_cast<T>(vec[index]);
	} catch (...) {
		return defaultValue;
	}
};

#endif
