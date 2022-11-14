#ifndef _FOORT_GEOMETRY_H
#define _FOORT_GEOMETRY_H

#include<string>
#include<array>
#include<algorithm>

///////////////////////////////////////////////////////////////////////////////////////
////// Definitions and some operations with geometric objects
////// Defines Point, tensors with 1-4 indices, and the operator toString for them
////// Also defines basic tensor arithmetic (+, -, *, /)
///////////////////////////////////////////////////////////////////////////////////////


// A real number. (Could be changed to use arbitrary precision in the future.)
using real = double;

/// <summary>
/// CONSTANTS
/// </summary>

// The spacetime dimension
inline constexpr int dimension{ 4 };

inline constexpr real pi{ 3.1415926535 };

// The amount of any coordinate that we shift to calculate derivatives (using central difference)
inline constexpr real DERIVATIVE_hval{ 1e-7 };


/// <summary>
/// TENSOR DEFINITIONS
/// </summary>

// A point in spacetime
using Point = std::array<real, dimension>;

// A point on the ViewScreen
using ScreenPoint = std::array<real, dimension - 2>;

// An index on the ViewScreen (row, column)
using ScreenIndex = std::array<int, dimension - 2>;

// Object with one index
using OneIndex = Point;
// Object with two indices
using TwoIndex = std::array<OneIndex, dimension>;
// Object with three indices
using ThreeIndex = std::array<TwoIndex, dimension>;
// Object with four indices
using FourIndex = std::array<ThreeIndex, dimension>;


/// <summary>
/// PRINTING TENSORS TO STRING
/// </summary>

// General printing function for a tensor; recursively calls the lower rank tensor to print itself
template<typename Tensor>
std::string toString(const Tensor &t)
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

// Provide the base case to print the lowest rank (rank-1) tensor
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

// A ScreenPoint has lower dimension than any spacetime tensor so needs a separate templace specialization
template<>
inline std::string toString(const ScreenPoint& t)
{
	std::string thestr{};
	thestr += "( ";
	for (int i = 0; i < dimension - 3; ++i)
	{
		thestr += std::to_string(t[i]);
		thestr += ", ";
	}
	thestr += std::to_string(t[dimension - 3]);
	thestr += " )";

	return thestr;
}

// Same for ScreenIndex: it needs a template specialization
template<>
inline std::string toString(const ScreenIndex& t)
{
	std::string thestr{};
	thestr += "( ";
	for (int i = 0; i < dimension - 3; ++i)
	{
		thestr += std::to_string(t[i]);
		thestr += ", ";
	}
	thestr += std::to_string(t[dimension - 3]);
	thestr += " )";

	return thestr;
}


/// <summary>
/// TENSOR LINEAR ALGEBRA
/// </summary>
 
// Function to recursively call + on the lower rank tensor (OR the underlying reals, if the tensor is rank 1)
template<typename t>
std::array<t, dimension> operator+(const std::array<t, dimension>& a1, const std::array<t, dimension>& a2)
{
	std::array<t, dimension> temp{ a1 };
	for (int i = 0; i < dimension; ++i)
		temp[i] =temp[i]+a2[i];

	return temp;
}

// Function to recursively call - on the lower rank tensor (OR the underlying reals, if the tensor is rank 1)
template<typename t>
std::array<t, dimension> operator-(const std::array<t, dimension>& a1, const std::array<t, dimension>& a2)
{
	std::array<t, dimension> temp{ a1 };
	for (int i = 0; i < dimension; ++i)
		temp[i] = temp[i] - a2[i];

	return temp;
}

// Add two ScreenPoints
// Not that this is NOT a template specialization of the previous because ScreenPoint is not an array with size dimension
inline ScreenPoint operator+(const ScreenPoint& a1, const ScreenPoint& a2)
{
	ScreenPoint temp{ a1 };
	for (int i = 0; i < dimension - 2; ++i)
		temp[i] = temp[i] + a2[i];

	return temp;
}

// Subtract two ScreenPoints
inline ScreenPoint operator-(const ScreenPoint& a1, const ScreenPoint& a2)
{
	ScreenPoint temp{ a1 };
	for (int i = 0; i < dimension-2; ++i)
		temp[i] = temp[i] - a2[i];

	return temp;
}

// Function to recursively scalar multiply the lower-rank tensors (or the underlying reals for the rank-1 tensor)
template<typename Tensor>
Tensor operator*(const Tensor& t1, real lambda)
{
	Tensor temp{ t1 };
	for (int i = 0; i < dimension; ++i)
		temp[i] = temp[i] * lambda;
	return temp;
}

// Scalar multiplication for a ScreenPoint; this IS a template specialization
template<>
inline ScreenPoint operator*(const ScreenPoint& t1, real lambda)
{
	ScreenPoint temp{ t1 };
	for (int i = 0; i < dimension-2; ++i)
		temp[i] = temp[i] * lambda;
	return temp;
}

// For multiplication with a scalar on the left, call the multiplication on the right defined above
template<typename Tensor>
Tensor operator*(real lambda, const Tensor& t1)
{
	return t1 * lambda;
}

// For division with a scalar, call the multiplication defined above
template<typename Tensor>
Tensor operator/(const Tensor& t1, real lambda)
{
	return t1 * (1 / lambda);
}


#endif
