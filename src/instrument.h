/*
 * Copyright (C) - 2018 - Jimmy Aguilar Mena <kratsbinovish@gmail.com>
 *                        Guido Giuntoli <gagiuntoli@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#ifndef INSTRUMENTPP_ACTIVE

#define INSTRUMENTPP_CONSTRUCT
#define INSTRUMENTPP_START
#define INSTRUMENTPP_CUSTOM(strname)

#else // INSTRUMENTPP_ACTIVE

#define INSTRUMENTPP_CONSTRUCT Instrument<> __global_intrumenter__;
#define INSTRUMENTPP_START Instrument<>::InstrumentFunction __timer__(__FUNCTION__);
#define INSTRUMENTPP_CUSTOM(strname) Instrument<>::InstrumentFunction __custom__(strname);

#include <chrono>
#include <cmath>
#include <numeric>
#include <unordered_map>
#include <vector>
#include <string>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <cassert>

/*!
   This is a stats struct with basic statistics about the input vector.

   The constructor receives a vector of some numerable and initializes
   the internal members stored as const.
*/
template <typename T>
struct stats {
	const size_t count;
	const T total, min, max;
	const double mean, stdev;

	static_assert(std::is_arithmetic<T>::value, "Not an arithmetic type");

	stats(const std::vector<T> &in)
	: count(in.size()),
	  total(std::accumulate(in.begin(), in.end(), 0.0)),
	  min(*std::min_element(in.begin(), in.end())),
	  max(*std::max_element(in.begin(), in.end())),
	  mean((double)total / count),
	  stdev(devest(in, mean))
	{
	}

	static double devest(const std::vector<T> &in, const double mean)
	{
		double out = 0;
		for (const T &x : in) {
			const double tmp = (double)x - mean;
			out += tmp * tmp;
		}

		return std::sqrt(out / in.size());
	}

};

/**
   Main instrument class constructed globally in the main initialization.

  The class is supposed to be created and destroyed only one time (like
  a singleton) by the macro.
*/
template <int LIMIT=1<<30>
class Instrument {
	typedef std::vector<uint64_t> timevect;

private:
	//! This is static to assert that this class behaves like a singleton.
	static uint64_t initialTime;
	static std::unordered_map<std::string, timevect> times;

	static inline uint64_t take_time_stamp() {
		return uint64_t(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	}

public:

	/** Inner class to instrument functions and scopes based on RAII */
	class InstrumentFunction {
		const std::string funct_;
		const uint64_t start_time_;

	public:
		InstrumentFunction(const std::string &funct)
		: funct_(funct), start_time_(take_time_stamp())
		{
			// Check that the global instrumenter was already
			// initialized before this call.  Otherwise the destructor
			// will fail, but detecting here is earlier.
			assert(Instrument::initialTime > 0);
		}

		~InstrumentFunction() {
			const uint64_t elapsed = (take_time_stamp() - start_time_) * 1E-3;
			Instrument::times[funct_].push_back(elapsed);
		}
	};

	Instrument()
	{
		assert(initialTime == 0);
		initialTime = take_time_stamp();
	}

	~Instrument() {

		const uint64_t elapsed = (take_time_stamp() - initialTime) * 1E-3;

		std::cout << "# Final execution report: total time = " << elapsed << std::endl;
		std::cout << std::setw(6) << std::left << "#No"
		<< std::setw(25) << "function"
		<< std::setw(8) << std::right << "calls"
		<< std::setw(16) << "total time"
		<< std::setw(16) << "percent"
		<< std::setw(16) << "mean"
		<< std::setw(16) << "min"
		<< std::setw(16) << "max"
		<< std::setw(16) << "stdev"
		<< std::setw(16) << "relative"
		<< std::endl;

		std::cout.precision(2);
		std::cout << std::fixed;

		size_t cont = 0;

		for (auto const &f : times) {
			stats st(f.second);

			std::cout << std::setw(6) << std::left << cont++
			<< std::setw(25) << f.first
			<< std::setw(8) << std::right << st.count
			<< std::setw(16) << st.total
			<< std::setw(16) << st.total * 100. / elapsed
			<< std::setw(16) << st.mean
			<< std::setw(16) << st.min
			<< std::setw(16) << st.max
			<< std::setw(16) << st.stdev
			<< std::setw(16) << st.stdev * 100 / st.mean
			<< std::endl;
		}
	}
};

template <int LIMIT>
uint64_t Instrument<LIMIT>::initialTime(0);

template <int LIMIT>
std::unordered_map<std::string, Instrument<>::timevect> Instrument<LIMIT>::times;

#endif // INSTRUMENTPP_ACTIVE
