// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2009 Gael Guennebaud <g.gael@free.fr>
// Copyright (C) 2009 Benoit Jacob <jacob.benoit.1@gmail.com>
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

#ifndef EIGEN_RETURNBYVALUE_H
#define EIGEN_RETURNBYVALUE_H

/** \class ReturnByValue
  *
  */
template<typename Derived>
struct ei_traits<ReturnByValue<Derived> >
  : public ei_traits<typename ei_traits<Derived>::ReturnMatrixType>
{
  enum {
    Flags = ei_traits<typename ei_traits<Derived>::ReturnMatrixType>::Flags | EvalBeforeNestingBit
  };
};

/* The ReturnByValue object doesn't even have a coeff() method.
 * So the only way that nesting it in an expression can work, is by evaluating it into a plain matrix.
 * So ei_nested always gives the plain return matrix type.
 */
template<typename Derived,int n,typename PlainMatrixType>
struct ei_nested<ReturnByValue<Derived>, n, PlainMatrixType>
{
  typedef typename ei_traits<Derived>::ReturnMatrixType type;
};

template<typename Derived>
  class ReturnByValue : public MatrixBase<ReturnByValue<Derived> >
{
  public:
    EIGEN_GENERIC_PUBLIC_INTERFACE(ReturnByValue)
    typedef typename ei_traits<Derived>::ReturnMatrixType ReturnMatrixType;
    template<typename Dest>
    inline void evalTo(Dest& dst) const
    { static_cast<const Derived* const>(this)->evalTo(dst); }
    inline int rows() const { return static_cast<const Derived* const>(this)->rows(); }
    inline int cols() const { return static_cast<const Derived* const>(this)->cols(); }
};

template<typename Derived>
template<typename OtherDerived>
Derived& MatrixBase<Derived>::operator=(const ReturnByValue<OtherDerived>& other)
{
  // Here we evaluate to a temporary matrix tmp, which we then copy. The main purpose
  // of this is to limit the number of instantiations of the template method evalTo<Destination>():
  // we only instantiate for PlainMatrixType.
  // Notice that this behaviour is specific to this operator in MatrixBase. The corresponding operator in class Matrix
  // does not evaluate into a temporary first.
  // TODO find a way to avoid evaluating into a temporary in the cases that matter. At least Block<> matters
  // for the implementation of blocked algorithms.
  // Should we:
  //  - try a trick like for the products, where the destination is abstracted as an array with stride?
  //  - or just add an operator in class Block, so we get a separate instantiation there (bad) but at least not more
  //    than that, and at least that's easy to make work?
  //  - or, since here we're talking about a compromise between code size and performance, let the user choose?
  //    Not obvious: many users will never find out about this feature, and it's hard to find a good API.
  PlainMatrixType tmp;
  other.evalTo(tmp);
  return derived() = tmp;
}

#endif // EIGEN_RETURNBYVALUE_H
