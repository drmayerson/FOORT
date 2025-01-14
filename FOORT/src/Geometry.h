#ifndef _FOORT_GEOMETRY_H
#define _FOORT_GEOMETRY_H

/**
 * @file Geometry.h
 * @author Daniel R. Mayerson
 * @brief Definitions and some operations with geometric objects.alignas
 * @details Defines Point, tensors with 1-4 indices, and the operator toString for them. Also defines basic tensor arithmetic (+, -, *, /).
 * No .cpp with implementations; all functions are inline.
 * @version 0.1
 * @date 2025-01-14
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <limits>  // for std::numeric_limits
#include <string>  // needed for toString(...) to convert tensors to strings
#include <array>   // needed to define tensors as fixed-size arrays of real or pixelcoord
#include <utility> // needed for std::pair
#include <vector>  // for std::vector

//! A real number (could be changed to use arbitrary precision in the future).
using real = double;

// Note: An unsigned long is guaranteed to be able to hold at least 4 294 967 295 (4.10^10).
// An unsigned int is only guaranteed to be able to hold 65 535, although
// many modern-day implementations will actually make the int 32-bit and so much larger

//! This type is used to count geodesics integrated
using largecounter = unsigned long;
//! A pixel coordinate: always >=0 and integer; we use the largecounter type
using pixelcoord = largecounter;

//! Macro definition of maximum value that can be held in this large counter
#ifndef LARGECOUNTER_MAX
#define LARGECOUNTER_MAX std::numeric_limits<largecounter>::max()
#endif
//! Macro definition of maximum value that can be held in this large counter
#ifndef PIXEL_MAX
#define PIXEL_MAX std::numeric_limits<pixelcoord>::max()
#endif

// CONSTANTS

//! The value of pi
inline constexpr real pi{3.1415926535};

//! The spacetime dimension
inline constexpr int dimension{4};

// TENSOR DEFINITIONS

//! A point in spacetime. Note that coordinates are always assumed to be (t, r, theta, phi)
using Point = std::array<real, dimension>;

//! A point on the ViewScreen; this does not have a time or radial extent
using ScreenPoint = std::array<real, dimension - 2>;

//! An index on the ViewScreen (row, column)
using ScreenIndex = std::array<pixelcoord, dimension - 2>;

//! Object with one index has the same structure as a Point
using OneIndex = Point;
//! Object with two indices is an array of OneIndex objects
using TwoIndex = std::array<OneIndex, dimension>;
//! Object with three indices is an array of TwoIndex objects
using ThreeIndex = std::array<TwoIndex, dimension>;
//! Object with four indices is an array of ThreeIndex objects
using FourIndex = std::array<ThreeIndex, dimension>;

// Definition used to define singularity of arbitrary codimension
//! SingularityCoord: pair of (coordinate number, coordinate value)
using SingularityCoord = std::pair<int, real>;
//! Singularity: a number of SingularityCoords together that define a arbitrary codimension surface/line/point
using Singularity = std::vector<SingularityCoord>;

// PRINTING TENSORS TO STRING

/**
 * @brief Base case for single index tensor of unsigned integers (ScreenIndex).
 * @details We do not want toString(ScreenIndex) to convert its entries to reals and use the
 * implementation for a single index tensor of reals, because we do not want decimal points in our
 * string for the ints!
 * @tparam TensorDim Dimension of the tensor
 * @param theTensor Tensor object
 * @return std::string
 */
template <size_t TensorDim>
std::string toString(const std::array<largecounter, TensorDim> &theTensor)
{
	std::string theStr{"("}; // no spaces for the innermost brackets
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

/**
 * @brief Base case for single index tensor of reals (Point, OneIndex, ScreenPoint).
 *
 * @tparam TensorDim Dimension of the tensor
 * @param theTensor Tensor object
 * @return std::string
 */
template <size_t TensorDim>
std::string toString(const std::array<real, TensorDim> &theTensor)
{
	std::string theStr{"("}; // no spaces for the innermost brackets

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

/**
 * @brief General case for tensors with more than one index (TwoIndex, ThreeIndex, FourIndex).
 * @details This function recursively calls the lower rank tensor to print itself.
 * @tparam Tensor The type of the tensor
 * @tparam TensorDim Dimension of the tensor
 * @param theTensor Tensor object
 * @return std::string
 */
template <typename Tensor, size_t TensorDim>
std::string toString(const std::array<Tensor, TensorDim> &theTensor)
{
	std::string theStr{"( "}; // All but the innermost brackets have an extra space padding the bracket

	for (int i = 0; i < TensorDim - 1; ++i)
	{
		theStr += toString(theTensor[i]);
		theStr += ", ";
	}
	theStr += toString(theTensor[TensorDim - 1]); // the last element doesn't have a comma after it

	theStr += " )"; // All but the innermost brackets have an extra space padding the bracket

	return theStr;
}

// TENSOR ARITHMETIC: addition/subtraction of tensors, scalar multiplication/division

/**
 * @brief Function to recursively call + on the lower rank tensor (OR the underlying reals/ints, if the tensor is rank 1)
 *
 * @tparam t data type of the tensor
 * @tparam TensorDim Dimension of the tensor
 * @param a1 Tensor 1
 * @param a2 Tensor 2
 * @return std::array<t, TensorDim> Tensor sum of a1 and a2
 */
template <typename t, size_t TensorDim>
std::array<t, TensorDim> operator+(const std::array<t, TensorDim> &a1, const std::array<t, TensorDim> &a2)
{
	std::array<t, TensorDim> temp{a1};
	for (int i = 0; i < TensorDim; ++i)
		temp[i] = temp[i] + a2[i];

	return temp;
}

/**
 * @brief Function to recursively call - on the lower rank tensor (OR the underlying reals/ints, if the tensor is rank 1)
 *
 * @tparam t data type of the tensor
 * @tparam TensorDim Dimension of the tensor
 * @param a1 Tensor 1
 * @param a2 Tensor 2
 * @return std::array<t, TensorDim> Tensor difference of a1 and a2
 */
template <typename t, size_t TensorDim>
std::array<t, TensorDim> operator-(const std::array<t, TensorDim> &a1, const std::array<t, TensorDim> &a2)
{
	std::array<t, TensorDim> temp{a1};
	for (int i = 0; i < TensorDim; ++i)
		temp[i] = temp[i] - a2[i];

	return temp;
}

/**
 * @brief Function to recursively scalar multiply the lower-rank tensors (OR the underlying reals/ints for the rank-1 tensor)
 *
 * @tparam t Type of the tensor
 * @tparam TensorDim Dimension of the tensor
 * @param t1 Tensor
 * @param lambda Scalar
 * @return std::array<t, TensorDim> Scalar product of t1 and lambda
 */
template <typename t, size_t TensorDim>
std::array<t, TensorDim> operator*(const std::array<t, TensorDim> &t1, real lambda)
{
	std::array<t, TensorDim> temp{t1};
	for (int i = 0; i < TensorDim; ++i)
		temp[i] = static_cast<t>(temp[i] * lambda); // static_cast necessary if t is integral type!
	return temp;
}

/**
 * @brief Function to recursively scalar multiply the lower-rank tensors (OR the underlying reals/ints for the rank-1 tensor)
 * @details This function calls the right multiplication operator.
 * @tparam t Type of the tensor
 * @tparam TensorDim Dimension of the tensor
 * @param lambda Scalar
 * @param t1 Tensor
 * @return std::array<t, TensorDim> Scalar product of lambda and t1
 */
template <typename t, size_t TensorDim>
std::array<t, TensorDim> operator*(real lambda, const std::array<t, TensorDim> &t1)
{
	return t1 * lambda;
}

/**
 * @brief Function to recursively scalar divide the lower-rank tensors (OR the underlying reals/ints for the rank-1 tensor)
 * @details This function calls the right multiplication operator.
 * @tparam t Type of the tensor
 * @tparam TensorDim Dimension of the tensor
 * @param t1 Tensor
 * @param lambda Scalar
 * @return std::array<t, TensorDim> Scalar division of t1 by lambda
 */
template <typename t, size_t TensorDim>
std::array<t, TensorDim> operator/(const std::array<t, TensorDim> &t1, real lambda)
{
	return t1 * (1 / lambda);
}

#endif
