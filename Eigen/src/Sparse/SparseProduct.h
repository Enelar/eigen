// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2008-2009 Gael Guennebaud <g.gael@free.fr>
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

#ifndef EIGEN_SPARSEPRODUCT_H
#define EIGEN_SPARSEPRODUCT_H

template<typename Lhs, typename Rhs>
struct SparseProductReturnType
{
  typedef typename ei_traits<Lhs>::Scalar Scalar;
  enum {
    LhsRowMajor = ei_traits<Lhs>::Flags & RowMajorBit,
    RhsRowMajor = ei_traits<Rhs>::Flags & RowMajorBit,
    TransposeRhs = (!LhsRowMajor) && RhsRowMajor,
    TransposeLhs = LhsRowMajor && (!RhsRowMajor)
  };

  // FIXME if we transpose let's evaluate to a LinkedVectorMatrix since it is the
  // type of the temporary to perform the transpose op
  typedef typename ei_meta_if<TransposeLhs,
    SparseMatrix<Scalar,0>,
    const typename ei_nested<Lhs,Rhs::RowsAtCompileTime>::type>::ret LhsNested;

  typedef typename ei_meta_if<TransposeRhs,
    SparseMatrix<Scalar,0>,
    const typename ei_nested<Rhs,Lhs::RowsAtCompileTime>::type>::ret RhsNested;

  typedef SparseProduct<LhsNested, RhsNested> Type;
};

template<typename LhsNested, typename RhsNested>
struct ei_traits<SparseProduct<LhsNested, RhsNested> >
{
  // clean the nested types:
  typedef typename ei_cleantype<LhsNested>::type _LhsNested;
  typedef typename ei_cleantype<RhsNested>::type _RhsNested;
  typedef typename _LhsNested::Scalar Scalar;

  enum {
    LhsCoeffReadCost = _LhsNested::CoeffReadCost,
    RhsCoeffReadCost = _RhsNested::CoeffReadCost,
    LhsFlags = _LhsNested::Flags,
    RhsFlags = _RhsNested::Flags,

    RowsAtCompileTime = _LhsNested::RowsAtCompileTime,
    ColsAtCompileTime = _RhsNested::ColsAtCompileTime,
    InnerSize = EIGEN_ENUM_MIN(_LhsNested::ColsAtCompileTime, _RhsNested::RowsAtCompileTime),

    MaxRowsAtCompileTime = _LhsNested::MaxRowsAtCompileTime,
    MaxColsAtCompileTime = _RhsNested::MaxColsAtCompileTime,

    EvalToRowMajor = (RhsFlags & LhsFlags & RowMajorBit),

    RemovedBits = ~(EvalToRowMajor ? 0 : RowMajorBit),

    Flags = (int(LhsFlags | RhsFlags) & HereditaryBits & RemovedBits)
          | EvalBeforeAssigningBit
          | EvalBeforeNestingBit,

    CoeffReadCost = Dynamic
  };

  typedef Sparse StorageType;

  typedef SparseMatrixBase<SparseProduct<LhsNested, RhsNested> > Base;
};

template<typename LhsNested, typename RhsNested>
class SparseProduct : ei_no_assignment_operator,
  public ei_traits<SparseProduct<LhsNested, RhsNested> >::Base
{
  public:

    EIGEN_GENERIC_PUBLIC_INTERFACE(SparseProduct)

  private:

    typedef typename ei_traits<SparseProduct>::_LhsNested _LhsNested;
    typedef typename ei_traits<SparseProduct>::_RhsNested _RhsNested;

  public:

    template<typename Lhs, typename Rhs>
    EIGEN_STRONG_INLINE SparseProduct(const Lhs& lhs, const Rhs& rhs)
      : m_lhs(lhs), m_rhs(rhs)
    {
      ei_assert(lhs.cols() == rhs.rows());

      enum {
        ProductIsValid = _LhsNested::ColsAtCompileTime==Dynamic
                      || _RhsNested::RowsAtCompileTime==Dynamic
                      || int(_LhsNested::ColsAtCompileTime)==int(_RhsNested::RowsAtCompileTime),
        AreVectors = _LhsNested::IsVectorAtCompileTime && _RhsNested::IsVectorAtCompileTime,
        SameSizes = EIGEN_PREDICATE_SAME_MATRIX_SIZE(_LhsNested,_RhsNested)
      };
      // note to the lost user:
      //    * for a dot product use: v1.dot(v2)
      //    * for a coeff-wise product use: v1.cwise()*v2
      EIGEN_STATIC_ASSERT(ProductIsValid || !(AreVectors && SameSizes),
        INVALID_VECTOR_VECTOR_PRODUCT__IF_YOU_WANTED_A_DOT_OR_COEFF_WISE_PRODUCT_YOU_MUST_USE_THE_EXPLICIT_FUNCTIONS)
      EIGEN_STATIC_ASSERT(ProductIsValid || !(SameSizes && !AreVectors),
        INVALID_MATRIX_PRODUCT__IF_YOU_WANTED_A_COEFF_WISE_PRODUCT_YOU_MUST_USE_THE_EXPLICIT_FUNCTION)
      EIGEN_STATIC_ASSERT(ProductIsValid || SameSizes, INVALID_MATRIX_PRODUCT)
    }

    EIGEN_STRONG_INLINE int rows() const { return m_lhs.rows(); }
    EIGEN_STRONG_INLINE int cols() const { return m_rhs.cols(); }

    EIGEN_STRONG_INLINE const _LhsNested& lhs() const { return m_lhs; }
    EIGEN_STRONG_INLINE const _RhsNested& rhs() const { return m_rhs; }

  protected:
    LhsNested m_lhs;
    RhsNested m_rhs;
};

// perform a pseudo in-place sparse * sparse product assuming all matrices are col major
template<typename Lhs, typename Rhs, typename ResultType>
static void ei_sparse_product_impl(const Lhs& lhs, const Rhs& rhs, ResultType& res)
{
  typedef typename ei_traits<typename ei_cleantype<Lhs>::type>::Scalar Scalar;

  // make sure to call innerSize/outerSize since we fake the storage order.
  int rows = lhs.innerSize();
  int cols = rhs.outerSize();
  //int size = lhs.outerSize();
  ei_assert(lhs.outerSize() == rhs.innerSize());

  // allocate a temporary buffer
  AmbiVector<Scalar> tempVector(rows);

  // estimate the number of non zero entries
  float ratioLhs = float(lhs.nonZeros())/(float(lhs.rows())*float(lhs.cols()));
  float avgNnzPerRhsColumn = float(rhs.nonZeros())/float(cols);
  float ratioRes = std::min(ratioLhs * avgNnzPerRhsColumn, 1.f);

  res.resize(rows, cols);
  res.reserve(int(ratioRes*rows*cols));
  for (int j=0; j<cols; ++j)
  {
    // let's do a more accurate determination of the nnz ratio for the current column j of res
    //float ratioColRes = std::min(ratioLhs * rhs.innerNonZeros(j), 1.f);
    // FIXME find a nice way to get the number of nonzeros of a sub matrix (here an inner vector)
    float ratioColRes = ratioRes;
    tempVector.init(ratioColRes);
    tempVector.setZero();
    for (typename Rhs::InnerIterator rhsIt(rhs, j); rhsIt; ++rhsIt)
    {
      // FIXME should be written like this: tmp += rhsIt.value() * lhs.col(rhsIt.index())
      tempVector.restart();
      Scalar x = rhsIt.value();
      for (typename Lhs::InnerIterator lhsIt(lhs, rhsIt.index()); lhsIt; ++lhsIt)
      {
        tempVector.coeffRef(lhsIt.index()) += lhsIt.value() * x;
      }
    }
    res.startVec(j);
    for (typename AmbiVector<Scalar>::Iterator it(tempVector); it; ++it)
      res.insertBack(j,it.index()) = it.value();
  }
  res.finalize();
}

template<typename Lhs, typename Rhs, typename ResultType,
  int LhsStorageOrder = ei_traits<Lhs>::Flags&RowMajorBit,
  int RhsStorageOrder = ei_traits<Rhs>::Flags&RowMajorBit,
  int ResStorageOrder = ei_traits<ResultType>::Flags&RowMajorBit>
struct ei_sparse_product_selector;

template<typename Lhs, typename Rhs, typename ResultType>
struct ei_sparse_product_selector<Lhs,Rhs,ResultType,ColMajor,ColMajor,ColMajor>
{
  typedef typename ei_traits<typename ei_cleantype<Lhs>::type>::Scalar Scalar;

  static void run(const Lhs& lhs, const Rhs& rhs, ResultType& res)
  {
    typename ei_cleantype<ResultType>::type _res(res.rows(), res.cols());
    ei_sparse_product_impl<Lhs,Rhs,ResultType>(lhs, rhs, _res);
    res.swap(_res);
  }
};

template<typename Lhs, typename Rhs, typename ResultType>
struct ei_sparse_product_selector<Lhs,Rhs,ResultType,ColMajor,ColMajor,RowMajor>
{
  static void run(const Lhs& lhs, const Rhs& rhs, ResultType& res)
  {
    // we need a col-major matrix to hold the result
    typedef SparseMatrix<typename ResultType::Scalar> SparseTemporaryType;
    SparseTemporaryType _res(res.rows(), res.cols());
    ei_sparse_product_impl<Lhs,Rhs,SparseTemporaryType>(lhs, rhs, _res);
    res = _res;
  }
};

template<typename Lhs, typename Rhs, typename ResultType>
struct ei_sparse_product_selector<Lhs,Rhs,ResultType,RowMajor,RowMajor,RowMajor>
{
  static void run(const Lhs& lhs, const Rhs& rhs, ResultType& res)
  {
    // let's transpose the product to get a column x column product
    typename ei_cleantype<ResultType>::type _res(res.rows(), res.cols());
    ei_sparse_product_impl<Rhs,Lhs,ResultType>(rhs, lhs, _res);
    res.swap(_res);
  }
};

template<typename Lhs, typename Rhs, typename ResultType>
struct ei_sparse_product_selector<Lhs,Rhs,ResultType,RowMajor,RowMajor,ColMajor>
{
  static void run(const Lhs& lhs, const Rhs& rhs, ResultType& res)
  {
    // let's transpose the product to get a column x column product
    typedef SparseMatrix<typename ResultType::Scalar> SparseTemporaryType;
    SparseTemporaryType _res(res.cols(), res.rows());
    ei_sparse_product_impl<Rhs,Lhs,SparseTemporaryType>(rhs, lhs, _res);
    res = _res.transpose();
  }
};

// NOTE the 2 others cases (col row *) must never occurs since they are caught
// by ProductReturnType which transform it to (col col *) by evaluating rhs.


// sparse = sparse * sparse
template<typename Derived>
template<typename Lhs, typename Rhs>
inline Derived& SparseMatrixBase<Derived>::operator=(const SparseProduct<Lhs,Rhs>& product)
{
  ei_sparse_product_selector<
    typename ei_cleantype<Lhs>::type,
    typename ei_cleantype<Rhs>::type,
    Derived>::run(product.lhs(),product.rhs(),derived());
  return derived();
}


template<typename Lhs, typename Rhs>
struct ei_traits<SparseTimeDenseProduct<Lhs,Rhs> >
 : ei_traits<ProductBase<SparseTimeDenseProduct<Lhs,Rhs>, Lhs, Rhs> >
{
  typedef Dense StorageType;
};

template<typename Lhs, typename Rhs>
class SparseTimeDenseProduct
  : public ProductBase<SparseTimeDenseProduct<Lhs,Rhs>, Lhs, Rhs>
{
  public:
    EIGEN_PRODUCT_PUBLIC_INTERFACE(SparseTimeDenseProduct)

    SparseTimeDenseProduct(const Lhs& lhs, const Rhs& rhs) : Base(lhs,rhs)
    {}

    template<typename Dest> void scaleAndAddTo(Dest& dest, Scalar alpha) const
    {
      typedef typename ei_cleantype<Lhs>::type _Lhs;
      typedef typename ei_cleantype<Rhs>::type _Rhs;
      typedef typename _Lhs::InnerIterator LhsInnerIterator;
      enum { LhsIsRowMajor = (_Lhs::Flags&RowMajorBit)==RowMajorBit };
      for(int j=0; j<m_lhs.outerSize(); ++j)
      {
        typename Rhs::Scalar rhs_j = alpha * m_rhs.coeff(j,0);
        Block<Dest,1,Dest::ColsAtCompileTime> dest_j(dest.row(LhsIsRowMajor ? j : 0));
        for(LhsInnerIterator it(m_lhs,j); it ;++it)
        {
          if(LhsIsRowMajor)                   dest_j += (alpha*it.value()) * m_rhs.row(it.index());
          else if(Rhs::ColsAtCompileTime==1)  dest.coeffRef(it.index()) += it.value() * rhs_j;
          else                                dest.row(it.index()) += (alpha*it.value()) * m_rhs.row(j);
        }
      }
    }

  private:
    SparseTimeDenseProduct& operator=(const SparseTimeDenseProduct&);
};


// dense = dense * sparse
template<typename Lhs, typename Rhs>
struct ei_traits<DenseTimeSparseProduct<Lhs,Rhs> >
 : ei_traits<ProductBase<DenseTimeSparseProduct<Lhs,Rhs>, Lhs, Rhs> >
{
  typedef Dense StorageType;
};

template<typename Lhs, typename Rhs>
class DenseTimeSparseProduct
  : public ProductBase<DenseTimeSparseProduct<Lhs,Rhs>, Lhs, Rhs>
{
  public:
    EIGEN_PRODUCT_PUBLIC_INTERFACE(DenseTimeSparseProduct)

    DenseTimeSparseProduct(const Lhs& lhs, const Rhs& rhs) : Base(lhs,rhs)
    {}

    template<typename Dest> void scaleAndAddTo(Dest& dest, Scalar alpha) const
    {
      typedef typename ei_cleantype<Rhs>::type _Rhs;
      typedef typename _Rhs::InnerIterator RhsInnerIterator;
      enum { RhsIsRowMajor = (_Rhs::Flags&RowMajorBit)==RowMajorBit };
      for(int j=0; j<m_rhs.outerSize(); ++j)
        for(RhsInnerIterator i(m_rhs,j); i; ++i)
          dest.col(RhsIsRowMajor ? i.index() : j) += (alpha*i.value()) * m_lhs.col(RhsIsRowMajor ? j : i.index());
    }

  private:
    DenseTimeSparseProduct& operator=(const DenseTimeSparseProduct&);
};

// sparse * sparse
template<typename Derived>
template<typename OtherDerived>
EIGEN_STRONG_INLINE const typename SparseProductReturnType<Derived,OtherDerived>::Type
SparseMatrixBase<Derived>::operator*(const SparseMatrixBase<OtherDerived> &other) const
{
  return typename SparseProductReturnType<Derived,OtherDerived>::Type(derived(), other.derived());
}

// sparse * dense
template<typename Derived>
template<typename OtherDerived>
EIGEN_STRONG_INLINE const SparseTimeDenseProduct<Derived,OtherDerived>
SparseMatrixBase<Derived>::operator*(const MatrixBase<OtherDerived> &other) const
{
  return SparseTimeDenseProduct<Derived,OtherDerived>(derived(), other.derived());
}

#endif // EIGEN_SPARSEPRODUCT_H
