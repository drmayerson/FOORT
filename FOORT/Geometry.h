#ifndef _FOORT_GEOMETRY_H
#define _FOORT_GEOMETRY_H

///////////////////////////////////////////////////////////////////////////////////////
////// GEOMETRY.H
////// Definitions and some operations with geometric objects
////// Defines Point, tensors with 1-4 indices, and the operator toString for them
////// Also defines basic tensor arithmetic (+, -, *, /)
////// (No .cpp with implementations; all functions are inline)
///////////////////////////////////////////////////////////////////////////////////////

#include <limits> // for std::numeric_limits
#include <string> // needed for toString(...) to convert tensors to strings
#include <array> // needed to define tensors as fixed-size arrays of real or pixelcoord


// A real number.
// (Could be changed to use arbitrary precision in the future.)
using real = double;


// Note: An unsigned long is guaranteed to be able to hold at least 4 294 967 295 (4.10^10).
// An unsigned int is only guaranteed to be able to hold 65 535, although
// many modern-day implementations will actually make the int 32-bit and so much larger
// 
// This type is used to count geodesics integrated
using largecounter = unsigned long;
// A pixel coordinate: always >=0 and integer; we use the largecounter type
using pixelcoord = largecounter;

// Macro definition of maximum value that can be held in this large counter
#ifndef LARGECOUNTER_MAX
#define LARGECOUNTER_MAX std::numeric_limits<largecounter>::max()
#endif
#ifndef PIXEL_MAX
#define PIXEL_MAX std::numeric_limits<pixelcoord>::max()
#endif


/// <summary>
/// CONSTANTS
/// </summary>

inline constexpr real pi{ 3.1415926535 };

// The spacetime dimension
inline constexpr int dimension{ 4 };


/// <summary>
/// TENSOR DEFINITIONS
/// </summary>

// A point in spacetime
// Note that coordinates are always assumed to be (t, r, theta, phi)
using Point = std::array<real, dimension>;

// A point on the ViewScreen; this does not have a time or radial extent
using ScreenPoint = std::array<real, dimension - 2>;

// An index on the ViewScreen (row, column)
using ScreenIndex = std::array<pixelcoord, dimension - 2>;

// Object with one index has the same structure as a Point
using OneIndex = Point;
// Object with two indices is an array of OneIndex objects
using TwoIndex = std::array<OneIndex, dimension>;
// Object with three indices is an array of TwoIndex objects
using ThreeIndex = std::array<TwoIndex, dimension>;
// Object with four indices is an array of ThreeIndex objects
using FourIndex = std::array<ThreeIndex, dimension>;


/// <summary>
/// PRINTING TENSORS TO STRING
/// </summary>
 
// Base case for single index tensor of unsigned integers (ScreenIndex).
// We do not want toString(ScreenIndex) to convert its entries to reals and use the
// implementation for a single index tensor of reals, because we do not want decimal points in our
// string for the ints!
template<size_t TensorDim>
std::string toString(const std::array<largecounter, TensorDim>& theTensor)
{
	std::string theStr{ "(" }; // no spaces for the innermost brackets
	for (int i = 0; i < TensorDim - 1; ++i)
	{
		// Here we use the std::to_string to convert the real to string
		theStr += std::to_string(theTensor[i]);
		theStr += ", ";
	}
	theStr += std::to_string(theTensor[TensorDim - 1]);
	theStr += ")"; // no spaces for the innermost brackets

	return theStr;
}

// Base case for single index tensor of reals (Point, OneIndex, ScreenPoint)
template<size_t TensorDim>
std::string toString(const std::array<real, TensorDim>& theTensor)
{
	std::string theStr{ "(" }; // no spaces for the innermost brackets

	for (int i = 0; i < TensorDim - 1; ++i)
	{
		// Here we use the std::to_string to convert the real to string
		theStr += std::to_string(theTensor[i]);
		theStr += ", ";
	}
	theStr += std::to_string(theTensor[TensorDim - 1]);
	theStr += ")"; // no spaces for the innermost brackets

	return theStr;
}

// General printing function for a tensor (TwoIndex, ThreeIndex, FourInedex);
// recursively calls the lower rank tensor to print itself
template<typename Tensor, size_t TensorDim>
std::string toString(const std::array<Tensor, TensorDim> &theTensor)
{
	std::string theStr{"( "}; // All but the innermost brackets have an extra space padding the bracket
	
	for (int i = 0; i < TensorDim -1; ++i)
	{
		theStr += toString(theTensor[i]);
		theStr += ", ";
	}
	theStr += toString(theTensor[TensorDim - 1]); // the last element doesn't have a comma after it

	theStr += " )"; // All but the innermost brackets have an extra space padding the bracket

	return theStr;
}


/// <summary>
/// TENSOR ARITHMETIC: addition/subtraction of tensors, scalar multiplication/division
/// </summary>
 
// Function to recursively call + on the lower rank tensor (OR the underlying reals/ints, if the tensor is rank 1)
template<typename t, size_t TensorDim>
std::array<t, TensorDim> operator+(const std::array<t, TensorDim>& a1, const std::array<t, TensorDim>& a2)
{
	std::array<t, TensorDim> temp{ a1 };
	for (int i = 0; i < TensorDim; ++i)
		temp[i] = temp[i] + a2[i];

	return temp;
}

// Function to recursively call - on the lower rank tensor (OR the underlying reals/ints, if the tensor is rank 1)
template<typename t, size_t TensorDim>
std::array<t, TensorDim> operator-(const std::array<t, TensorDim>& a1, const std::array<t, TensorDim>& a2)
{
	std::array<t, TensorDim> temp{ a1 };
	for (int i = 0; i < TensorDim; ++i)
		temp[i] = temp[i] - a2[i];

	return temp;
}

// Function to recursively scalar multiply the lower-rank tensors (OR the underlying reals/ints for the rank-1 tensor)
template<typename t, size_t TensorDim>
std::array<t, TensorDim> operator*(const std::array<t, TensorDim>& t1, real lambda)
{
	std::array<t, TensorDim> temp{ t1 };
	for (int i = 0; i < TensorDim; ++i)
		temp[i] = static_cast<t>(temp[i] * lambda); // static_cast necessary if t is integral type!
	return temp;
}

// For multiplication with a scalar on the left, call the multiplication on the right defined above
template<typename t, size_t TensorDim>
std::array<t, TensorDim> operator*(real lambda, const std::array<t, TensorDim>& t1)
{
	return t1 * lambda;
}

// For division with a scalar, call the multiplication defined above
template<typename t, size_t TensorDim>
std::array<t, TensorDim> operator/(const std::array<t, TensorDim>& t1, real lambda)
{
	return t1 * (1 / lambda);
}


#endif