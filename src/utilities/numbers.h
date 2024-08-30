// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// This file is copied from c++20 standard library.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#ifndef SRC_UTILITIES_NUMBERS_H_
#define SRC_UTILITIES_NUMBERS_H_

#include <type_traits>

// General template declarations
template <class T>
inline constexpr T e_v = T();
template <class T>
inline constexpr T log2e_v = T();
template <class T>
inline constexpr T log10e_v = T();
template <class T>
inline constexpr T pi_v = T();
template <class T>
inline constexpr T inv_pi_v = T();
template <class T>
inline constexpr T inv_sqrtpi_v = T();
template <class T>
inline constexpr T ln2_v = T();
template <class T>
inline constexpr T ln10_v = T();
template <class T>
inline constexpr T sqrt2_v = T();
template <class T>
inline constexpr T sqrt3_v = T();
template <class T>
inline constexpr T inv_sqrt3_v = T();
template <class T>
inline constexpr T egamma_v = T();
template <class T>
inline constexpr T phi_v = T();

// Specializations for floating-point types
template <>
inline constexpr float e_v<float> = 2.718281828459045235360287471352662497757f;
template <>
inline constexpr float log2e_v<float> =
    1.442695040888963407359924681001892137426f;
template <>
inline constexpr float log10e_v<float> =
    0.434294481903251827651128918916605082294f;
template <>
inline constexpr float pi_v<float> = 3.141592653589793238462643383279502884197f;
template <>
inline constexpr float inv_pi_v<float> =
    0.318309886183790671537767526745028724068f;
template <>
inline constexpr float inv_sqrtpi_v<float> =
    0.564189583547756286948079451560772585844f;
template <>
inline constexpr float ln2_v<float> =
    0.693147180559945309417232121458176568075f;
template <>
inline constexpr float ln10_v<float> =
    2.302585092994045684017991454684364207601f;
template <>
inline constexpr float sqrt2_v<float> =
    1.414213562373095048801688724209698078570f;
template <>
inline constexpr float sqrt3_v<float> =
    1.732050807568877293527446341505872366943f;
template <>
inline constexpr float inv_sqrt3_v<float> =
    0.577350269189625764509148780501957455647f;
template <>
inline constexpr float egamma_v<float> =
    0.577215664901532860606512090082402431042f;
template <>
inline constexpr float phi_v<float> =
    1.618033988749894848204586834365638117720f;

template <>
inline constexpr double e_v<double> = 2.718281828459045235360287471352662497757;
template <>
inline constexpr double log2e_v<double> =
    1.442695040888963407359924681001892137426;
template <>
inline constexpr double log10e_v<double> =
    0.434294481903251827651128918916605082294;
template <>
inline constexpr double pi_v<double> =
    3.141592653589793238462643383279502884197;
template <>
inline constexpr double inv_pi_v<double> =
    0.318309886183790671537767526745028724068;
template <>
inline constexpr double inv_sqrtpi_v<double> =
    0.564189583547756286948079451560772585844;
template <>
inline constexpr double ln2_v<double> =
    0.693147180559945309417232121458176568075;
template <>
inline constexpr double ln10_v<double> =
    2.302585092994045684017991454684364207601;
template <>
inline constexpr double sqrt2_v<double> =
    1.414213562373095048801688724209698078570;
template <>
inline constexpr double sqrt3_v<double> =
    1.732050807568877293527446341505872366943;
template <>
inline constexpr double inv_sqrt3_v<double> =
    0.577350269189625764509148780501957455647;
template <>
inline constexpr double egamma_v<double> =
    0.577215664901532860606512090082402431042;
template <>
inline constexpr double phi_v<double> =
    1.618033988749894848204586834365638117720;

template <>
inline constexpr long double e_v<long double> =
    2.718281828459045235360287471352662497757L;
template <>
inline constexpr long double log2e_v<long double> =
    1.442695040888963407359924681001892137426L;
template <>
inline constexpr long double log10e_v<long double> =
    0.434294481903251827651128918916605082294L;
template <>
inline constexpr long double pi_v<long double> =
    3.141592653589793238462643383279502884197L;
template <>
inline constexpr long double inv_pi_v<long double> =
    0.318309886183790671537767526745028724068L;
template <>
inline constexpr long double inv_sqrtpi_v<long double> =
    0.564189583547756286948079451560772585844L;
template <>
inline constexpr long double ln2_v<long double> =
    0.693147180559945309417232121458176568075L;
template <>
inline constexpr long double ln10_v<long double> =
    2.302585092994045684017991454684364207601L;
template <>
inline constexpr long double sqrt2_v<long double> =
    1.414213562373095048801688724209698078570L;
template <>
inline constexpr long double sqrt3_v<long double> =
    1.732050807568877293527446341505872366943L;
template <>
inline constexpr long double inv_sqrt3_v<long double> =
    0.577350269189625764509148780501957455647L;
template <>
inline constexpr long double egamma_v<long double> =
    0.577215664901532860606512090082402431042L;
template <>
inline constexpr long double phi_v<long double> =
    1.618033988749894848204586834365638117720L;

// Convenience constants for 'double' type
inline constexpr double e = e_v<double>;
inline constexpr double log2e = log2e_v<double>;
inline constexpr double log10e = log10e_v<double>;
inline constexpr double pi = pi_v<double>;
inline constexpr double inv_pi = inv_pi_v<double>;
inline constexpr double inv_sqrtpi = inv_sqrtpi_v<double>;
inline constexpr double ln2 = ln2_v<double>;
inline constexpr double ln10 = ln10_v<double>;
inline constexpr double sqrt2 = sqrt2_v<double>;
inline constexpr double sqrt3 = sqrt3_v<double>;
inline constexpr double inv_sqrt3 = inv_sqrt3_v<double>;
inline constexpr double egamma = egamma_v<double>;
inline constexpr double phi = phi_v<double>;

#endif
