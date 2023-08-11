#pragma once
#include <random>

namespace utils
{
	template <typename T>
	T GetRandomNumber(T minValue, T maxValue)
	{
		std::mt19937 engine;
		std::random_device device;
		engine.seed(device());
		std::uniform_int_distribution<T> distribution(minValue, maxValue);
		return distribution(engine);
	}
}
