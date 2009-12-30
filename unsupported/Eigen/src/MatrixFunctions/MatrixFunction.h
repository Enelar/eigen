// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2009 Jitse Niesen <jitse@maths.leeds.ac.uk>
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

#ifndef EIGEN_MATRIX_FUNCTION
#define EIGEN_MATRIX_FUNCTION

template <typename Scalar>
struct ei_stem_function
{
  typedef std::complex<typename NumTraits<Scalar>::Real> ComplexScalar;
  typedef ComplexScalar type(ComplexScalar, int);
};

/** \ingroup MatrixFunctions_Module
  *
  * \brief Compute a matrix function.
  *
  * \param[in]  M      argument of matrix function, should be a square matrix.
  * \param[in]  f      an entire function; \c f(x,n) should compute the n-th derivative of f at x.
  * \param[out] result pointer to the matrix in which to store the result, \f$ f(M) \f$.
  *
  * This function computes \f$ f(A) \f$ and stores the result in the
  * matrix pointed to by \p result.
  *
  * %Matrix functions are defined as follows.  Suppose that \f$ f \f$
  * is an entire function (that is, a function on the complex plane
  * that is everywhere complex differentiable).  Then its Taylor
  * series
  * \f[ f(0) + f'(0) x + \frac{f''(0)}{2} x^2 + \frac{f'''(0)}{3!} x^3 + \cdots \f]
  * converges to \f$ f(x) \f$. In this case, we can define the matrix
  * function by the same series:
  * \f[ f(M) = f(0) + f'(0) M + \frac{f''(0)}{2} M^2 + \frac{f'''(0)}{3!} M^3 + \cdots \f]
  *
  * This routine uses the algorithm described in:
  * Philip Davies and Nicholas J. Higham, 
  * "A Schur-Parlett algorithm for computing matrix functions", 
  * <em>SIAM J. %Matrix Anal. Applic.</em>, <b>25</b>:464&ndash;485, 2003.
  *
  * The actual work is done by the MatrixFunction class.
  *
  * Example: The following program checks that
  * \f[ \exp \left[ \begin{array}{ccc} 
  *       0 & \frac14\pi & 0 \\ 
  *       -\frac14\pi & 0 & 0 \\
  *       0 & 0 & 0 
  *     \end{array} \right] = \left[ \begin{array}{ccc}
  *       \frac12\sqrt2 & -\frac12\sqrt2 & 0 \\
  *       \frac12\sqrt2 & \frac12\sqrt2 & 0 \\
  *       0 & 0 & 1
  *     \end{array} \right]. \f]
  * This corresponds to a rotation of \f$ \frac14\pi \f$ radians around
  * the z-axis. This is the same example as used in the documentation
  * of ei_matrix_exponential().
  *
  * Note that the function \c expfn is defined for complex numbers \c x, 
  * even though the matrix \c A is over the reals.
  *
  * \include MatrixFunction.cpp
  * Output: \verbinclude MatrixFunction.out
  */
template <typename Derived>
EIGEN_STRONG_INLINE void ei_matrix_function(const MatrixBase<Derived>& M, 
					    typename ei_stem_function<typename ei_traits<Derived>::Scalar>::type f,
					    typename MatrixBase<Derived>::PlainMatrixType* result);

#include "MatrixFunctionAtomic.h"


/** \ingroup MatrixFunctions_Module 
  * \brief Helper class for computing matrix functions. 
  */
template <typename MatrixType, int IsComplex = NumTraits<typename ei_traits<MatrixType>::Scalar>::IsComplex>
class MatrixFunction
{  
  private:

    typedef typename ei_traits<MatrixType>::Scalar Scalar;
    typedef typename ei_stem_function<Scalar>::type StemFunction;

  public:

    /** \brief Constructor. Computes matrix function. 
      *
      * \param[in]  A      argument of matrix function, should be a square matrix.
      * \param[in]  f      an entire function; \c f(x,n) should compute the n-th derivative of f at x.
      * \param[out] result pointer to the matrix in which to store the result, \f$ f(A) \f$.
      *
      * This function computes \f$ f(A) \f$ and stores the result in
      * the matrix pointed to by \p result.
      *
      * See ei_matrix_function() for details.
      */
    MatrixFunction(const MatrixType& A, StemFunction f, MatrixType* result);
};


/** \ingroup MatrixFunctions_Module 
  * \brief Partial specialization of MatrixFunction for real matrices.
  * \internal
  */
template <typename MatrixType>
class MatrixFunction<MatrixType, 0>
{  
  private:

    typedef ei_traits<MatrixType> Traits;
    typedef typename Traits::Scalar Scalar;
    static const int Rows = Traits::RowsAtCompileTime;
    static const int Cols = Traits::ColsAtCompileTime;
    static const int Options = MatrixType::Options;
    static const int MaxRows = Traits::MaxRowsAtCompileTime;
    static const int MaxCols = Traits::MaxColsAtCompileTime;

    typedef std::complex<Scalar> ComplexScalar;
    typedef Matrix<ComplexScalar, Rows, Cols, Options, MaxRows, MaxCols> ComplexMatrix;
    typedef typename ei_stem_function<Scalar>::type StemFunction;

  public:

    /** \brief Constructor. Computes matrix function. 
      *
      * \param[in]  A      argument of matrix function, should be a square matrix.
      * \param[in]  f      an entire function; \c f(x,n) should compute the n-th derivative of f at x.
      * \param[out] result pointer to the matrix in which to store the result, \f$ f(A) \f$.
      *
      * This function converts the real matrix \c A to a complex matrix,
      * uses MatrixFunction<MatrixType,1> and then converts the result back to
      * a real matrix.
      */
    MatrixFunction(const MatrixType& A, StemFunction f, MatrixType* result) 
    {
      ComplexMatrix CA = A.template cast<ComplexScalar>();
      ComplexMatrix Cresult;
      MatrixFunction<ComplexMatrix>(CA, f, &Cresult);
      *result = Cresult.real();
    }
};

      
/** \ingroup MatrixFunctions_Module 
  * \brief Partial specialization of MatrixFunction for complex matrices 
  * \internal
  */
template <typename MatrixType>
class MatrixFunction<MatrixType, 1>
{
  private:

    typedef ei_traits<MatrixType> Traits;
    typedef typename Traits::Scalar Scalar;
    static const int RowsAtCompileTime = Traits::RowsAtCompileTime;
    static const int ColsAtCompileTime = Traits::ColsAtCompileTime;
    static const int Options = MatrixType::Options;
    typedef typename NumTraits<Scalar>::Real RealScalar;
    typedef typename ei_stem_function<Scalar>::type StemFunction;
    typedef Matrix<Scalar, Traits::RowsAtCompileTime, 1> VectorType;
    typedef Matrix<int, Traits::RowsAtCompileTime, 1> IntVectorType;
    typedef std::list<Scalar> listOfScalars;
    typedef std::list<listOfScalars> listOfLists;
    typedef Matrix<Scalar, Dynamic, Dynamic, Options, RowsAtCompileTime, ColsAtCompileTime> DynMatrixType;

  public:

    /** \brief Constructor. Computes matrix function. 
      *
      * \param[in]  A      argument of matrix function, should be a square matrix.
      * \param[in]  f      an entire function; \c f(x,n) should compute the n-th derivative of f at x.
      * \param[out] result pointer to the matrix in which to store the result, \f$ f(A) \f$.
      */
    MatrixFunction(const MatrixType& A, StemFunction f, MatrixType* result);

  private:

    // Prevent copying
    MatrixFunction(const MatrixFunction&);
    MatrixFunction& operator=(const MatrixFunction&);

    void separateBlocksInSchur(MatrixType& T, MatrixType& U, VectorXi& blockSize);
    void permuteSchur(const IntVectorType& permutation, MatrixType& T, MatrixType& U);
    void swapEntriesInSchur(int index, MatrixType& T, MatrixType& U);
    void computeTriangular(const MatrixType& T, MatrixType& result, const VectorXi& blockSize);
    void computeBlockAtomic(const MatrixType& T, MatrixType& result, const VectorXi& blockSize);
    DynMatrixType solveTriangularSylvester(const DynMatrixType& A, const DynMatrixType& B, const DynMatrixType& C);
    void divideInBlocks(const VectorType& v, listOfLists* result);
    void constructPermutation(const VectorType& diag, const listOfLists& blocks, 
			      VectorXi& blockSize, IntVectorType& permutation);

    static const RealScalar separation() { return static_cast<RealScalar>(0.01); }
    StemFunction *m_f;
};

template <typename MatrixType>
MatrixFunction<MatrixType,1>::MatrixFunction(const MatrixType& A, StemFunction f, MatrixType* result) :
  m_f(f)
{
  if (A.rows() == 1) {
    result->resize(1,1);
    (*result)(0,0) = f(A(0,0), 0);
  } else {
    const ComplexSchur<MatrixType> schurOfA(A);  
    MatrixType T = schurOfA.matrixT();
    MatrixType U = schurOfA.matrixU();
    VectorXi blockSize;
    separateBlocksInSchur(T, U, blockSize);
    MatrixType fT;
    computeTriangular(T, fT, blockSize);
    *result = U * fT * U.adjoint();
  }
}

template <typename MatrixType>
void MatrixFunction<MatrixType,1>::separateBlocksInSchur(MatrixType& T, MatrixType& U, VectorXi& blockSize)
{
  const VectorType d = T.diagonal();
  listOfLists blocks;
  divideInBlocks(d, &blocks);

  IntVectorType permutation;
  constructPermutation(d, blocks, blockSize, permutation);
  permuteSchur(permutation, T, U);
}

template <typename MatrixType>
void MatrixFunction<MatrixType,1>::permuteSchur(const IntVectorType& permutation, MatrixType& T, MatrixType& U)
{
  IntVectorType p = permutation;
  for (int i = 0; i < p.rows() - 1; i++) {
    int j;
    for (j = i; j < p.rows(); j++) {
      if (p(j) == i) break;
    }
    ei_assert(p(j) == i);
    for (int k = j-1; k >= i; k--) {
      swapEntriesInSchur(k, T, U);
      std::swap(p.coeffRef(k), p.coeffRef(k+1));
    }
  }
}

// swap T(index, index) and T(index+1, index+1)
template <typename MatrixType>
void MatrixFunction<MatrixType,1>::swapEntriesInSchur(int index, MatrixType& T, MatrixType& U)
{
  PlanarRotation<Scalar> rotation;
  rotation.makeGivens(T(index, index+1), T(index+1, index+1) - T(index, index));
  T.applyOnTheLeft(index, index+1, rotation.adjoint());
  T.applyOnTheRight(index, index+1, rotation);
  U.applyOnTheRight(index, index+1, rotation);
}  

template <typename MatrixType>
void MatrixFunction<MatrixType,1>::computeTriangular(const MatrixType& T, MatrixType& result, const VectorXi& blockSize)
{ 
  MatrixType expT;
  ei_matrix_exponential(T, &expT);
  computeBlockAtomic(T, result, blockSize);
  VectorXi blockStart(blockSize.rows());
  blockStart(0) = 0;
  for (int i = 1; i < blockSize.rows(); i++) {
    blockStart(i) = blockStart(i-1) + blockSize(i-1);
  }
  for (int diagIndex = 1; diagIndex < blockSize.rows(); diagIndex++) {
    for (int blockIndex = 0; blockIndex < blockSize.rows() - diagIndex; blockIndex++) {
      // compute (blockIndex, blockIndex+diagIndex) block
      DynMatrixType A = T.block(blockStart(blockIndex), blockStart(blockIndex), blockSize(blockIndex), blockSize(blockIndex));
      DynMatrixType B = -T.block(blockStart(blockIndex+diagIndex), blockStart(blockIndex+diagIndex), blockSize(blockIndex+diagIndex), blockSize(blockIndex+diagIndex));
      DynMatrixType C = result.block(blockStart(blockIndex), blockStart(blockIndex), blockSize(blockIndex), blockSize(blockIndex)) * T.block(blockStart(blockIndex), blockStart(blockIndex+diagIndex), blockSize(blockIndex), blockSize(blockIndex+diagIndex));
      C -= T.block(blockStart(blockIndex), blockStart(blockIndex+diagIndex), blockSize(blockIndex), blockSize(blockIndex+diagIndex)) * result.block(blockStart(blockIndex+diagIndex), blockStart(blockIndex+diagIndex), blockSize(blockIndex+diagIndex), blockSize(blockIndex+diagIndex));
      for (int k = blockIndex + 1; k < blockIndex + diagIndex; k++) {
	C += result.block(blockStart(blockIndex), blockStart(k), blockSize(blockIndex), blockSize(k)) * T.block(blockStart(k), blockStart(blockIndex+diagIndex), blockSize(k), blockSize(blockIndex+diagIndex));
	C -= T.block(blockStart(blockIndex), blockStart(k), blockSize(blockIndex), blockSize(k)) * result.block(blockStart(k), blockStart(blockIndex+diagIndex), blockSize(k), blockSize(blockIndex+diagIndex));
      }
      result.block(blockStart(blockIndex), blockStart(blockIndex+diagIndex), blockSize(blockIndex), blockSize(blockIndex+diagIndex)) = solveTriangularSylvester(A, B, C);
    }
  }
}

/** \brief Solve a triangular Sylvester equation AX + XB = C 
  *
  * \param[in]  A  the matrix A; should be square and upper triangular
  * \param[in]  B  the matrix B; should be square and upper triangular
  * \param[in]  C  the matrix C; should have correct size.
  *
  * \returns the solution X.
  *
  * If A is m-by-m and B is n-by-n, then both C and X are m-by-n. 
  * The (i,j)-th component of the Sylvester equation is
  * \f[ 
  *     \sum_{k=i}^m A_{ik} X_{kj} + \sum_{k=1}^j X_{ik} B_{kj} = C_{ij}. 
  * \f]
  * This can be re-arranged to yield:
  * \f[ 
  *     X_{ij} = \frac{1}{A_{ii} + B_{jj}} \Bigl( C_{ij}
  *     - \sum_{k=i+1}^m A_{ik} X_{kj} - \sum_{k=1}^{j-1} X_{ik} B_{kj} \Bigr).
  * \f]
  * It is assumed that A and B are such that the numerator is never
  * zero (otherwise the Sylvester equation does not have a unique
  * solution). In that case, these equations can be evaluated in the
  * order \f$ i=m,\ldots,1 \f$ and \f$ j=1,\ldots,n \f$.
  */
template <typename MatrixType>
typename MatrixFunction<MatrixType,1>::DynMatrixType MatrixFunction<MatrixType,1>::solveTriangularSylvester(
  const DynMatrixType& A, 
  const DynMatrixType& B, 
  const DynMatrixType& C)
{
  ei_assert(A.rows() == A.cols());
  ei_assert(A.isUpperTriangular());
  ei_assert(B.rows() == B.cols());
  ei_assert(B.isUpperTriangular());
  ei_assert(C.rows() == A.rows());
  ei_assert(C.cols() == B.rows());

  int m = A.rows();
  int n = B.rows();
  DynMatrixType X(m, n);

  for (int i = m - 1; i >= 0; --i) {
    for (int j = 0; j < n; ++j) {

      // Compute AX = \sum_{k=i+1}^m A_{ik} X_{kj}
      Scalar AX;
      if (i == m - 1) {
	AX = 0; 
      } else {
	Matrix<Scalar,1,1> AXmatrix = A.row(i).end(m-1-i) * X.col(j).end(m-1-i);
	AX = AXmatrix(0,0);
      }

      // Compute XB = \sum_{k=1}^{j-1} X_{ik} B_{kj}
      Scalar XB;
      if (j == 0) {
	XB = 0; 
      } else {
	Matrix<Scalar,1,1> XBmatrix = X.row(i).start(j) * B.col(j).start(j);
	XB = XBmatrix(0,0);
      }

      X(i,j) = (C(i,j) - AX - XB) / (A(i,i) + B(j,j));
    }
  }
  return X;
}


// does not touch irrelevant parts of T
template <typename MatrixType>
void MatrixFunction<MatrixType,1>::computeBlockAtomic(const MatrixType& T, MatrixType& result, const VectorXi& blockSize)
{ 
  int blockStart = 0;
  result.resize(T.rows(), T.cols());
  result.setZero();
  MatrixFunctionAtomic<DynMatrixType> mfa(m_f);
  for (int i = 0; i < blockSize.rows(); i++) {
    result.block(blockStart, blockStart, blockSize(i), blockSize(i))
      = mfa.compute(T.block(blockStart, blockStart, blockSize(i), blockSize(i)));
    blockStart += blockSize(i);
  }
}

template <typename Scalar>
typename std::list<std::list<Scalar> >::iterator ei_find_in_list_of_lists(typename std::list<std::list<Scalar> >& ll, Scalar x)
{
  typename std::list<Scalar>::iterator j;
  for (typename std::list<std::list<Scalar> >::iterator i = ll.begin(); i != ll.end(); i++) {
    j = std::find(i->begin(), i->end(), x);
    if (j != i->end())
      return i;
  }
  return ll.end();
}

// Alg 4.1
template <typename MatrixType>
void MatrixFunction<MatrixType,1>::divideInBlocks(const VectorType& v, listOfLists* result)
{
  const int n = v.rows();
  for (int i=0; i<n; i++) {
    // Find set containing v(i), adding a new set if necessary
    typename listOfLists::iterator qi = ei_find_in_list_of_lists(*result, v(i));
    if (qi == result->end()) {
      listOfScalars l;
      l.push_back(v(i));
      result->push_back(l);
      qi = result->end();
      qi--;
    }
    // Look for other element to add to the set
    for (int j=i+1; j<n; j++) {
      if (ei_abs(v(j) - v(i)) <= separation() && std::find(qi->begin(), qi->end(), v(j)) == qi->end()) {
	typename listOfLists::iterator qj = ei_find_in_list_of_lists(*result, v(j));
	if (qj == result->end()) {
	  qi->push_back(v(j));
	} else {
	  qi->insert(qi->end(), qj->begin(), qj->end());
	  result->erase(qj);
	}
      }
    }
  }
}

// Construct permutation P, such that P(D) has eigenvalues clustered together
template <typename MatrixType>
void MatrixFunction<MatrixType,1>::constructPermutation(const VectorType& diag, const listOfLists& blocks, 
							VectorXi& blockSize, IntVectorType& permutation)
{
  const int n = diag.rows();
  const int numBlocks = blocks.size();

  // For every block in blocks, mark and count the entries in diag that
  // appear in that block
  blockSize.setZero(numBlocks);
  IntVectorType entryToBlock(n);
  int blockIndex = 0;
  for (typename listOfLists::const_iterator block = blocks.begin(); block != blocks.end(); block++) {
    for (int i = 0; i < diag.rows(); i++) {
      if (std::find(block->begin(), block->end(), diag(i)) != block->end()) {
	blockSize[blockIndex]++;
	entryToBlock[i] = blockIndex;
      }
    }
    blockIndex++;
  }

  // Compute index of first entry in every block as the sum of sizes
  // of all the preceding blocks
  VectorXi indexNextEntry(numBlocks);
  indexNextEntry[0] = 0;
  for (blockIndex = 1; blockIndex < numBlocks; blockIndex++) {
    indexNextEntry[blockIndex] = indexNextEntry[blockIndex-1] + blockSize[blockIndex-1];
  }
      
  // Construct permutation 
  permutation.resize(n);
  for (int i = 0; i < n; i++) {
    int block = entryToBlock[i];
    permutation[i] = indexNextEntry[block];
    indexNextEntry[block]++;
  }
}  

template <typename Derived>
EIGEN_STRONG_INLINE void ei_matrix_function(const MatrixBase<Derived>& M, 
					    typename ei_stem_function<typename ei_traits<Derived>::Scalar>::type f,
					    typename MatrixBase<Derived>::PlainMatrixType* result)
{
  ei_assert(M.rows() == M.cols());
  MatrixFunction<typename MatrixBase<Derived>::PlainMatrixType>(M, f, result);
}

#endif // EIGEN_MATRIX_FUNCTION
