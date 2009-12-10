// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2008-2009 Gael Guennebaud <g.gael@free.fr>
// Copyright (C) 2006-2008 Benoit Jacob <jacob.benoit.1@gmail.com>
//
// Eigen is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// Alternatively, you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// Eigen is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License and a copy of the GNU General Public License along with
// Eigen. If not, see <http://www.gnu.org/licenses/>.

// This file is a base class plugin containing matrix specifics coefficient wise functions.

/** \returns an expression of the Schur product (coefficient wise product) of *this and \a other
  *
  * Example: \include MatrixBase_cwiseProduct.cpp
  * Output: \verbinclude MatrixBase_cwiseProduct.out
  *
  * \sa class CwiseBinaryOp, cwiseAbs2
  */

#define EIGEN_CWISE_PRODUCT_RETURN_TYPE \
    CwiseBinaryOp< \
      ei_scalar_product_op< \
        typename ei_scalar_product_traits< \
          typename ei_traits<Derived>::Scalar, \
          typename ei_traits<OtherDerived>::Scalar \
        >::ReturnType \
      >, \
      Derived, \
      OtherDerived \
    >

template<typename OtherDerived>
EIGEN_STRONG_INLINE const EIGEN_CWISE_PRODUCT_RETURN_TYPE
cwiseProduct(const EIGEN_CURRENT_STORAGE_BASE_CLASS<OtherDerived> &other) const
{
  return EIGEN_CWISE_PRODUCT_RETURN_TYPE(derived(), other.derived());
}

#undef EIGEN_CWISE_PRODUCT_RETURN_TYPE

/** \returns an expression of the coefficient-wise == operator of *this and \a other
  *
  * \warning this performs an exact comparison, which is generally a bad idea with floating-point types.
  * In order to check for equality between two vectors or matrices with floating-point coefficients, it is
  * generally a far better idea to use a fuzzy comparison as provided by MatrixBase::isApprox() and
  * MatrixBase::isMuchSmallerThan().
  *
  * Example: \include MatrixBase_cwiseEqual.cpp
  * Output: \verbinclude MatrixBase_cwiseEqual.out
  *
  * \sa MatrixBase::cwiseNotEqual(), MatrixBase::isApprox(), MatrixBase::isMuchSmallerThan()
  */
template<typename OtherDerived>
inline const CwiseBinaryOp<std::equal_to<Scalar>, Derived, OtherDerived>
cwiseEqual(const EIGEN_CURRENT_STORAGE_BASE_CLASS<OtherDerived> &other) const
{
  return CwiseBinaryOp<std::equal_to<Scalar>, Derived, OtherDerived>(derived(), other.derived());
}

/** \returns an expression of the coefficient-wise != operator of *this and \a other
  *
  * \warning this performs an exact comparison, which is generally a bad idea with floating-point types.
  * In order to check for equality between two vectors or matrices with floating-point coefficients, it is
  * generally a far better idea to use a fuzzy comparison as provided by MatrixBase::isApprox() and
  * MatrixBase::isMuchSmallerThan().
  *
  * Example: \include MatrixBase_cwiseNotEqual.cpp
  * Output: \verbinclude MatrixBase_cwiseNotEqual.out
  *
  * \sa MatrixBase::cwiseEqual(), MatrixBase::isApprox(), MatrixBase::isMuchSmallerThan()
  */
template<typename OtherDerived>
inline const CwiseBinaryOp<std::not_equal_to<Scalar>, Derived, OtherDerived>
cwiseNotEqual(const EIGEN_CURRENT_STORAGE_BASE_CLASS<OtherDerived> &other) const
{
  return CwiseBinaryOp<std::not_equal_to<Scalar>, Derived, OtherDerived>(derived(), other.derived());
}

/** \returns an expression of the coefficient-wise min of *this and \a other
  *
  * Example: \include MatrixBase_cwiseMin.cpp
  * Output: \verbinclude MatrixBase_cwiseMin.out
  *
  * \sa class CwiseBinaryOp, max()
  */
template<typename OtherDerived>
EIGEN_STRONG_INLINE const CwiseBinaryOp<ei_scalar_min_op<Scalar>, Derived, OtherDerived>
cwiseMin(const EIGEN_CURRENT_STORAGE_BASE_CLASS<OtherDerived> &other) const
{
  return CwiseBinaryOp<ei_scalar_min_op<Scalar>, Derived, OtherDerived>(derived(), other.derived());
}

/** \returns an expression of the coefficient-wise max of *this and \a other
  *
  * Example: \include MatrixBase_cwiseMax.cpp
  * Output: \verbinclude MatrixBase_cwiseMax.out
  *
  * \sa class CwiseBinaryOp, min()
  */
template<typename OtherDerived>
EIGEN_STRONG_INLINE const CwiseBinaryOp<ei_scalar_max_op<Scalar>, Derived, OtherDerived>
cwiseMax(const EIGEN_CURRENT_STORAGE_BASE_CLASS<OtherDerived> &other) const
{
  return CwiseBinaryOp<ei_scalar_max_op<Scalar>, Derived, OtherDerived>(derived(), other.derived());
}

/** \returns an expression of the coefficient-wise quotient of *this and \a other
  *
  * Example: \include MatrixBase_cwiseQuotient.cpp
  * Output: \verbinclude MatrixBase_cwiseQuotient.out
  *
  * \sa class CwiseBinaryOp, cwiseProduct(), cwiseInverse()
  */
template<typename OtherDerived>
EIGEN_STRONG_INLINE const CwiseBinaryOp<ei_scalar_quotient_op<Scalar>, Derived, OtherDerived>
cwiseQuotient(const EIGEN_CURRENT_STORAGE_BASE_CLASS<OtherDerived> &other) const
{
  return CwiseBinaryOp<ei_scalar_quotient_op<Scalar>, Derived, OtherDerived>(derived(), other.derived());
}
