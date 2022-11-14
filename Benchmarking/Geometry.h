#ifndef _FOORT_BENCH_GEOMETRY_H
#define _FOORT_BENCH_GEOMETRY_H

#include<string>
#include<array>
#include<algorithm>

#include<functional>

// The spacetime dimension
inline constexpr int dimension{ 4 };

// A real number. Leaving the door open for using arbitrary precision in the future.
using real = double;

// A point in spacetime
using Point = std::array<real, dimension>;

// Define objects with one index (e.g. vector), two indices (e.g. metric),
// three indices (e.g. Christoffel), four indices (e.g. Riemann)

// Object with one index
using OneIndex = Point;
// Object with two indices
using TwoIndex = std::array<OneIndex, dimension>;
// Object with three indices
using ThreeIndex = std::array<TwoIndex, dimension>;
// Object with four indices
using FourIndex = std::array<ThreeIndex, dimension>;


/// <summary>
/// Printing geometric objects
/// </summary>

template<typename Tensor>
std::string toString(const Tensor& t)
{
	std::string thestr{};
	thestr += "( ";
	for (int i = 0; i < dimension - 1; ++i)
	{
		thestr += toString(t[i]);
		thestr += ", ";
	}
	thestr += toString(t[dimension - 1]);

	thestr += " )";

	return thestr;
}

template<>
inline std::string toString(const Point& t)
{
	std::string thestr{};
	thestr += "( ";
	for (int i = 0; i < dimension - 1; ++i)
	{
		thestr += std::to_string(t[i]);
		thestr += ", ";
	}
	thestr += std::to_string(t[dimension - 1]);
	thestr += " )";

	return thestr;
}



/// <summary>
/// Linear operations with tensors
/// </summary>


template<typename T>
std::array<T, dimension>& operator+(const std::array<T, dimension>& a1, const std::array<T, dimension>& a2)
{
	std::array<T, dimension> temp{ a1 };
	std::transform(std::begin(temp), std::end(temp), begin(a2), std::begin(a1), std::plus<real>());

	return temp;
}

template<typename T>
std::array<T, dimension>& operator-(const std::array<T, dimension>& a1, const std::array<T, dimension>& a2)
{
	std::array<T, dimension> temp{ a1 };
	std::transform(std::begin(temp), std::end(temp), begin(a2), std::begin(a1), std::minus<real>());

	return temp;
}

//template<typename Tensor>
//Tensor operator+(const Tensor& t1, const Tensor& t2)
//{
//	Tensor temp{ t1 };
//	for (int i = 0; i < dimension; ++i)
//		temp[i] = temp[i] + t2[i];
//	return temp;
//}
//
//template<typename Tensor>
//Tensor operator-(const Tensor& t1, const Tensor& t2)
//{
//	Tensor temp{ t1 };
//	for (int i = 0; i < dimension; ++i)
//		temp[i] = temp[i] - t2[i];
//	return temp;
//}

template<typename Tensor>
Tensor operator*(const Tensor& t1, real lambda)
{
	Tensor temp{ t1 };
	for (int i = 0; i < dimension; ++i)
		temp[i] = temp[i] * lambda;
	return temp;
}

template<typename Tensor>
Tensor operator*(real lambda, const Tensor& t1)
{
	return t1 * lambda;
}

template<typename Tensor>
Tensor operator/(const Tensor& t1, real lambda)
{
	Tensor temp{ t1 };
	for (int i = 0; i < dimension; ++i)
		temp[i] = temp[i] / lambda;
	return temp;
}


#endif
