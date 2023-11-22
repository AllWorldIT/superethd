/*
 * SPDX-FileCopyrightText: 2023 Conarx Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <deque>
#include <mutex>
#include <numeric>
#include <shared_mutex>

namespace accl {

/**
 * @brief Struct to hold the minimum, mean and maximum values of a statistic.
 *
 * @tparam T Type of statistic, this is a numeric type.
 */
template <typename T> struct StatisticResult {
		T min;
		T mean;
		T max;
};

/**
 * @brief Statistic class to keep track of a statistic over a number of values.
 *
 * @tparam T Type of statistic, this is a numeric type.
 */
template <typename T> class Statistic {
	private:
		mutable std::shared_mutex mtx;
		std::deque<T> values;
		const size_t maxSize;

		inline T _max() const;
		inline T _mean() const;
		inline T _min() const;

	public:
		Statistic(size_t size);
		~Statistic();

		void add(T value);

		T max() const;
		T mean() const;
		T min() const;

		void getStatisticResult(StatisticResult<T> &result);
		StatisticResult<T> getStatisticResult() const;
};

/**
 * @brief Get the maximum value in the statistic internally without locking.
 *
 * @tparam T Type of statistic.
 * @return T Maximum value.
 */
template <typename T> T Statistic<T>::_max() const {
	if (values.size() == 0) {
		return T{};
	}
	return *std::max_element(values.begin(), values.end());
}

/**
 * @brief Get the mean value in the statistic internally without locking.
 *
 * @tparam T Type of statistic.
 * @return T Mean value.
 */
template <typename T> T Statistic<T>::_mean() const {
	if (values.size() == 0) {
		return T{};
	}
	return std::accumulate(values.begin(), values.end(), T{}) / values.size();
}

/**
 * @brief Get the minimum value in the statistic internally without locking.
 *
 * @tparam T Type of statistic.
 * @return T Minimum value.
 */
template <typename T> T Statistic<T>::_min() const {
	if (values.size() == 0) {
		return T{};
	}
	return *std::min_element(values.begin(), values.end());
}

/**
 * @brief Construct a new Statistic< T>:: Statistic object
 *
 * @tparam T Type of statistic.
 * @param size Number of values to keep in the statistic.
 */
template <typename T> Statistic<T>::Statistic(size_t size) : maxSize(size) {}

template <typename T> Statistic<T>::~Statistic() = default;

/**
 * @brief Add a value to the statistic.
 *
 * @tparam T Type of statistic.
 * @param value Value to add.
 */
template <typename T> void Statistic<T>::add(T value) {
	std::unique_lock<std::shared_mutex> lock(mtx);

	if (values.size() == maxSize) {
		values.pop_front();
	}
	values.push_back(value);
}

/**
 * @brief Get the maximum value in the statistic.
 *
 * @tparam T Type of statistic.
 * @return T Maximum value.
 */
template <typename T> T Statistic<T>::max() const {
	std::shared_lock<std::shared_mutex> lock(mtx);
	return _max();
}

/**
 * @brief Get the mean value in the statistic.
 *
 * @tparam T Type of statistic.
 * @return T Mean value.
 */
template <typename T> T Statistic<T>::mean() const {
	std::shared_lock<std::shared_mutex> lock(mtx);
	return _mean();
}

/**
 * @brief Get the minimum value in the statistic.
 *
 * @tparam T Type of statistic.
 * @return T Minimum value.
 */
template <typename T> T Statistic<T>::min() const {
	std::shared_lock<std::shared_mutex> lock(mtx);
	return _min();
}

/**
 * @brief Get the minimum, mean and maximum values in the statistic.
 *
 * @tparam T Type of statistic.
 * @return StatisticResult<T> Minimum, mean and maximum values.
 */
template <typename T> void Statistic<T>::getStatisticResult(StatisticResult<T> &result) {
	std::shared_lock<std::shared_mutex> lock(mtx);
	result.min = _min();
	result.mean = _mean();
	result.max = _max();
}

/**
 * @brief Get the minimum, mean and maximum values in the statistic.
 *
 * @tparam T Type of statistic.
 * @return StatisticResult<T> Minimum, mean and maximum values.
 */
template <typename T> StatisticResult<T> Statistic<T>::getStatisticResult() const {
	StatisticResult<T> result;
	getStatisticResult(result);
	return result;
}

} // namespace accl