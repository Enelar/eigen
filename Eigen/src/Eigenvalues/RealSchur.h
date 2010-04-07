// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2008 Gael Guennebaud <g.gael@free.fr>
// Copyright (C) 2010 Jitse Niesen <jitse@maths.leeds.ac.uk>
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

#ifndef EIGEN_REAL_SCHUR_H
#define EIGEN_REAL_SCHUR_H

#include "./HessenbergDecomposition.h"

/** \eigenvalues_module \ingroup Eigenvalues_Module
  * \nonstableyet
  *
  * \class RealSchur
  *
  * \brief Performs a real Schur decomposition of a square matrix
  */
template<typename _MatrixType> class RealSchur
{
  public:
    typedef _MatrixType MatrixType;
    enum {
      RowsAtCompileTime = MatrixType::RowsAtCompileTime,
      ColsAtCompileTime = MatrixType::ColsAtCompileTime,
      Options = MatrixType::Options,
      MaxRowsAtCompileTime = MatrixType::MaxRowsAtCompileTime,
      MaxColsAtCompileTime = MatrixType::MaxColsAtCompileTime
    };
    typedef typename MatrixType::Scalar Scalar;
    typedef std::complex<typename NumTraits<Scalar>::Real> ComplexScalar;
    typedef Matrix<ComplexScalar, ColsAtCompileTime, 1, Options, MaxColsAtCompileTime, 1> EigenvalueType;

    /** \brief Constructor; computes Schur decomposition of given matrix. */
    RealSchur(const MatrixType& matrix)
            : m_matT(matrix.rows(),matrix.cols()),
              m_matU(matrix.rows(),matrix.cols()),
              m_eivalues(matrix.rows()),
              m_isInitialized(false)
    {
      compute(matrix);
    }

    /** \brief Returns the orthogonal matrix in the Schur decomposition. */
    const MatrixType& matrixU() const
    {
      ei_assert(m_isInitialized && "RealSchur is not initialized.");
      return m_matU;
    }

    /** \brief Returns the quasi-triangular matrix in the Schur decomposition. */
    const MatrixType& matrixT() const
    {
      ei_assert(m_isInitialized && "RealSchur is not initialized.");
      return m_matT;
    }
  
    /** \brief Returns vector of eigenvalues. 
      *
      * This function will likely be removed. */
    const EigenvalueType& eigenvalues() const
    {
      ei_assert(m_isInitialized && "RealSchur is not initialized.");
      return m_eivalues;
    }
  
    /** \brief Computes Schur decomposition of given matrix. */
    void compute(const MatrixType& matrix);

  private:
    
    MatrixType m_matT;
    MatrixType m_matU;
    EigenvalueType m_eivalues;
    bool m_isInitialized;

    typedef Matrix<Scalar,3,1> Vector3s;

    Scalar computeNormOfT();
    int findSmallSubdiagEntry(int iu, Scalar norm);
    void splitOffTwoRows(int iu, Scalar exshift);
    void computeShift(int iu, int iter, Scalar& exshift, Vector3s& shiftInfo);
    void initFrancisQRStep(int il, int iu, const Vector3s& shiftInfo, int& im, Vector3s& firstHouseholderVector);
    void performFrancisQRStep(int il, int im, int iu, const Vector3s& firstHouseholderVector, Scalar* workspace);
};


template<typename MatrixType>
void RealSchur<MatrixType>::compute(const MatrixType& matrix)
{
  assert(matrix.cols() == matrix.rows());

  // Step 1. Reduce to Hessenberg form
  // TODO skip Q if skipU = true
  HessenbergDecomposition<MatrixType> hess(matrix);
  m_matT = hess.matrixH();
  m_matU = hess.matrixQ();

  // Step 2. Reduce to real Schur form  
  typedef Matrix<Scalar, ColsAtCompileTime, 1, Options, MaxColsAtCompileTime, 1> ColumnVectorType;
  ColumnVectorType workspaceVector(m_matU.cols());
  Scalar* workspace = &workspaceVector.coeffRef(0);

  // The matrix m_matT is divided in three parts. 
  // Rows 0,...,il-1 are decoupled from the rest because m_matT(il,il-1) is zero. 
  // Rows il,...,iu is the part we are working on (the active window).
  // Rows iu+1,...,end are already brought in triangular form.
  int iu = m_matU.cols() - 1;
  int iter = 0; // iteration count
  Scalar exshift = 0.0; // sum of exceptional shifts
  Scalar norm = computeNormOfT();

  while (iu >= 0)
  {
    int il = findSmallSubdiagEntry(iu, norm);

    // Check for convergence
    if (il == iu) // One root found
    {
      m_matT.coeffRef(iu,iu) = m_matT.coeff(iu,iu) + exshift;
      m_eivalues.coeffRef(iu) = ComplexScalar(m_matT.coeff(iu,iu), 0.0);
      iu--;
      iter = 0;
    }
    else if (il == iu-1) // Two roots found
    {
      splitOffTwoRows(iu, exshift);
      iu -= 2;
      iter = 0;
    }
    else // No convergence yet
    {
      Vector3s firstHouseholderVector, shiftInfo;
      computeShift(iu, iter, exshift, shiftInfo);
      iter = iter + 1;   // (Could check iteration count here.)
      int im;
      initFrancisQRStep(il, iu, shiftInfo, im, firstHouseholderVector);
      performFrancisQRStep(il, im, iu, firstHouseholderVector, workspace);
    }
  } 

  m_isInitialized = true;
}

/** \internal Computes and returns vector L1 norm of T */
template<typename MatrixType>
inline typename MatrixType::Scalar RealSchur<MatrixType>::computeNormOfT()
{
  const int size = m_matU.cols();
  // FIXME to be efficient the following would requires a triangular reduxion code
  // Scalar norm = m_matT.upper().cwiseAbs().sum() 
  //               + m_matT.corner(BottomLeft,size-1,size-1).diagonal().cwiseAbs().sum();
  Scalar norm = 0.0;
  for (int j = 0; j < size; ++j)
    norm += m_matT.row(j).segment(std::max(j-1,0), size-std::max(j-1,0)).cwiseAbs().sum();
  return norm;
}

/** \internal Look for single small sub-diagonal element and returns its index */
template<typename MatrixType>
inline int RealSchur<MatrixType>::findSmallSubdiagEntry(int iu, Scalar norm)
{
  int res = iu;
  while (res > 0)
  {
    Scalar s = ei_abs(m_matT.coeff(res-1,res-1)) + ei_abs(m_matT.coeff(res,res));
    if (s == 0.0)
      s = norm;
    if (ei_abs(m_matT.coeff(res,res-1)) < NumTraits<Scalar>::epsilon() * s)
      break;
    res--;
  }
  return res;
}

/** \internal Update T given that rows iu-1 and iu decouple from the rest. */
template<typename MatrixType>
inline void RealSchur<MatrixType>::splitOffTwoRows(int iu, Scalar exshift)
{
  const int size = m_matU.cols();

  // The eigenvalues of the 2x2 matrix [a b; c d] are 
  // trace +/- sqrt(discr/4) where discr = tr^2 - 4*det, tr = a + d, det = ad - bc
  Scalar w = m_matT.coeff(iu,iu-1) * m_matT.coeff(iu-1,iu);
  Scalar p = Scalar(0.5) * (m_matT.coeff(iu-1,iu-1) - m_matT.coeff(iu,iu));
  Scalar q = p * p + w;   // q = tr^2 / 4 - det = discr/4
  Scalar z = ei_sqrt(ei_abs(q));
  m_matT.coeffRef(iu,iu) += exshift;
  m_matT.coeffRef(iu-1,iu-1) += exshift;

  if (q >= 0) // Two real eigenvalues
  {
    PlanarRotation<Scalar> rot;
    if (p >= 0)
      rot.makeGivens(p + z, m_matT.coeff(iu, iu-1));
    else
      rot.makeGivens(p - z, m_matT.coeff(iu, iu-1));

    m_matT.block(0, iu-1, size, size-iu+1).applyOnTheLeft(iu-1, iu, rot.adjoint());
    m_matT.block(0, 0, iu+1, size).applyOnTheRight(iu-1, iu, rot);
    m_matU.applyOnTheRight(iu-1, iu, rot);

    m_eivalues.coeffRef(iu-1) = ComplexScalar(m_matT.coeff(iu-1, iu-1), 0.0);
    m_eivalues.coeffRef(iu)   = ComplexScalar(m_matT.coeff(iu, iu), 0.0);
  }
  else // // Pair of complex conjugate eigenvalues
  {
    m_eivalues.coeffRef(iu-1) = ComplexScalar(m_matT.coeff(iu,iu) + p, z);
    m_eivalues.coeffRef(iu)   = ComplexScalar(m_matT.coeff(iu,iu) + p, -z);
  }
}

/** \internal Form shift in shiftInfo, and update exshift if an exceptional shift is performed. */
template<typename MatrixType>
inline void RealSchur<MatrixType>::computeShift(int iu, int iter, Scalar& exshift, Vector3s& shiftInfo)
{
  shiftInfo.coeffRef(0) = m_matT.coeff(iu,iu);
  shiftInfo.coeffRef(1) = m_matT.coeff(iu-1,iu-1);
  shiftInfo.coeffRef(2) = m_matT.coeff(iu,iu-1) * m_matT.coeff(iu-1,iu);

  // Wilkinson's original ad hoc shift
  if (iter == 10)
  {
    exshift += shiftInfo.coeff(0);
    for (int i = 0; i <= iu; ++i)
      m_matT.coeffRef(i,i) -= shiftInfo.coeff(0);
    Scalar s = ei_abs(m_matT.coeff(iu,iu-1)) + ei_abs(m_matT.coeff(iu-1,iu-2));
    shiftInfo.coeffRef(0) = Scalar(0.75) * s;
    shiftInfo.coeffRef(1) = Scalar(0.75) * s;
    shiftInfo.coeffRef(2) = Scalar(-0.4375) * s * s;
  }

  // MATLAB's new ad hoc shift
  if (iter == 30)
  {
    Scalar s = (shiftInfo.coeff(1) - shiftInfo.coeff(0)) / Scalar(2.0);
    s = s * s + shiftInfo.coeff(2);
    if (s > 0)
    {
      s = ei_sqrt(s);
      if (shiftInfo.coeff(1) < shiftInfo.coeff(0))
        s = -s;
      s = s + (shiftInfo.coeff(1) - shiftInfo.coeff(0)) / Scalar(2.0);
      s = shiftInfo.coeff(0) - shiftInfo.coeff(2) / s;
      exshift += s;
      for (int i = 0; i <= iu; ++i)
        m_matT.coeffRef(i,i) -= s;
      shiftInfo.setConstant(Scalar(0.964));
    }
  }
}

/** \internal Compute index im at which Francis QR step starts and the first Householder vector. */
template<typename MatrixType>
inline void RealSchur<MatrixType>::initFrancisQRStep(int il, int iu, const Vector3s& shiftInfo, int& im, Vector3s& firstHouseholderVector)
{
  Scalar p = 0, q = 0, r = 0;

  for (im = iu-2; im >= il; --im)
  {
    Scalar z = m_matT.coeff(im,im);
    r = shiftInfo.coeff(0) - z;
    Scalar s = shiftInfo.coeff(1) - z;
    p = (r * s - shiftInfo.coeff(2)) / m_matT.coeff(im+1,im) + m_matT.coeff(im,im+1);
    q = m_matT.coeff(im+1,im+1) - z - r - s;
    r = m_matT.coeff(im+2,im+1);
    s = ei_abs(p) + ei_abs(q) + ei_abs(r);
    p = p / s;
    q = q / s;
    r = r / s;
    if (im == il) {
      break;
    }
    if (ei_abs(m_matT.coeff(im,im-1)) * (ei_abs(q) + ei_abs(r)) <
      NumTraits<Scalar>::epsilon() * (ei_abs(p) * (ei_abs(m_matT.coeff(im-1,im-1)) + ei_abs(z) +
      ei_abs(m_matT.coeff(im+1,im+1)))))
    {
      break;
    }
  }

  for (int i = im+2; i <= iu; ++i)
  {
    m_matT.coeffRef(i,i-2) = 0.0;
    if (i > im+2)
      m_matT.coeffRef(i,i-3) = 0.0;
  }

  firstHouseholderVector << p, q, r;
}

/** Perform a Francis QR step involving rows il:iu and columns im:iu. */
template<typename MatrixType>
inline void RealSchur<MatrixType>::performFrancisQRStep(int il, int im, int iu, const Vector3s& firstHouseholderVector, Scalar* workspace)
{
  assert(im >= il);
  assert(im <= iu-2);

  const int size = m_matU.cols();

  for (int k = im; k <= iu-2; ++k)
  {
    bool firstIteration = (k == im);

    Vector3s v;
    if (firstIteration)
      v = firstHouseholderVector;
    else
      v = m_matT.template block<3,1>(k,k-1);

    Scalar tau, beta;
    Matrix<Scalar, 2, 1> ess;
    v.makeHouseholder(ess, tau, beta);
    
    if (beta != Scalar(0)) // if v is not zero
    {
      if (firstIteration && k > il)
        m_matT.coeffRef(k,k-1) = -m_matT.coeff(k,k-1);
      else if (!firstIteration)
        m_matT.coeffRef(k,k-1) = beta;

      // These Householder transformations form the O(n^3) part of the algorithm
      m_matT.block(k, k, 3, size-k).applyHouseholderOnTheLeft(ess, tau, workspace);
      m_matT.block(0, k, std::min(iu,k+3) + 1, 3).applyHouseholderOnTheRight(ess, tau, workspace);
      m_matU.block(0, k, size, 3).applyHouseholderOnTheRight(ess, tau, workspace);
    }
  }

  Matrix<Scalar, 2, 1> v = m_matT.template block<2,1>(iu-1, iu-2);
  Scalar tau, beta;
  Matrix<Scalar, 1, 1> ess;
  v.makeHouseholder(ess, tau, beta);

  if (beta != Scalar(0)) // if v is not zero
  {
    m_matT.coeffRef(iu-1, iu-2) = beta;
    m_matT.block(iu-1, iu-1, 2, size-iu+1).applyHouseholderOnTheLeft(ess, tau, workspace);
    m_matT.block(0, iu-1, iu+1, 2).applyHouseholderOnTheRight(ess, tau, workspace);
    m_matU.block(0, iu-1, size, 2).applyHouseholderOnTheRight(ess, tau, workspace);
  }
}

#endif // EIGEN_REAL_SCHUR_H
