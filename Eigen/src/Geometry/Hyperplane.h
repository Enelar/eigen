// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2008 Gael Guennebaud <g.gael@free.fr>
// Copyright (C) 2008 Benoit Jacob <jacob@math.jussieu.fr>
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

#ifndef EIGEN_HYPERPLANE_H
#define EIGEN_HYPERPLANE_H

/** \geometry_module \ingroup GeometryModule
  *
  * \class ParametrizedLine
  *
  * \brief A parametrized line
  *
  * \param _Scalar the scalar type, i.e., the type of the coefficients
  * \param _AmbientDim the dimension of the ambient space, can be a compile time value or Dynamic.
  *             Notice that the dimension of the hyperplane is _AmbientDim-1.
  */
template <typename _Scalar, int _AmbientDim>
class ParametrizedLine
{
  public:

    enum { AmbientDimAtCompileTime = _AmbientDim };
    typedef _Scalar Scalar;
    typedef typename NumTraits<Scalar>::Real RealScalar;
    typedef Matrix<Scalar,AmbientDimAtCompileTime,1> VectorType;

    ParametrizedLine(const VectorType& origin, const VectorType& direction)
      : m_origin(origin), m_direction(direction) {}
    explicit ParametrizedLine(const Hyperplane<_Scalar, _AmbientDim>& hyperplane);

    ~ParametrizedLine() {}

    const VectorType& origin() const { return m_origin; }
    VectorType& origin() { return m_origin; }

    const VectorType& direction() const { return m_direction; }
    VectorType& direction() { return m_direction; }

    Scalar intersection(const Hyperplane<_Scalar, _AmbientDim>& hyperplane);

  protected:

    VectorType m_origin, m_direction;
};

/** \geometry_module \ingroup GeometryModule
  *
  * \class Hyperplane
  *
  * \brief A hyperplane
  *
  * A hyperplane is an affine subspace of dimension n-1 in a space of dimension n.
  * For example, a hyperplane in a plane is a line; a hyperplane in 3-space is a plane.
  *
  * \param _Scalar the scalar type, i.e., the type of the coefficients
  * \param _AmbientDim the dimension of the ambient space, can be a compile time value or Dynamic.
  *             Notice that the dimension of the hyperplane is _AmbientDim-1.
  *
  * This class represents an hyperplane as the zero set of the implicit equation
  * \f$ n \cdot x + d = 0 \f$ where \f$ n \f$ is a unit normal vector of the plane (linear part)
  * and \f$ d \f$ is the distance (offset) to the origin.
  */
template <typename _Scalar, int _AmbientDim>
class Hyperplane
{
  public:

    enum { AmbientDimAtCompileTime = _AmbientDim };
    typedef _Scalar Scalar;
    typedef typename NumTraits<Scalar>::Real RealScalar;
    typedef Matrix<Scalar,AmbientDimAtCompileTime,1> VectorType;
    typedef Matrix<Scalar,AmbientDimAtCompileTime==Dynamic
                          ? Dynamic
                          : AmbientDimAtCompileTime+1,1> Coefficients;
    typedef Block<Coefficients,AmbientDimAtCompileTime,1> NormalReturnType;

    /** Default constructor without initialization */
    inline Hyperplane(int _dim = AmbientDimAtCompileTime) : m_coeffs(_dim+1) {}
    
    /** Construct a plane from its normal \a n and a point \a e onto the plane.
      * \warning the vector normal is assumed to be normalized.
      */
    inline Hyperplane(const VectorType& n, const VectorType e)
      : m_coeffs(n.size()+1)
    {
      normal() = n;
      offset() = -e.dot(n);
    }
    
    /** Constructs a plane from its normal \a n and distance to the origin \a d.
      * \warning the vector normal is assumed to be normalized.
      */
    inline Hyperplane(const VectorType& n, Scalar d)
      : m_coeffs(n.size()+1)
    {
      normal() = n;
      offset() = d;
    }

    /** Constructs a hyperplane passing through the two points. If the dimension of the ambient space
      * is greater than 2, then there isn't uniqueness, so an arbitrary choice is made.
      */
    static inline Hyperplane Through(const VectorType& p0, const VectorType& p1)
    {
      Hyperplane result(p0.size());
      result.normal() = (p1 - p0).unitOrthogonal();
      result.offset() = -result.normal().dot(p0);
      return result;
    }

    /** Constructs a hyperplane passing through the three points. The dimension of the ambient space
      * is required to be exactly 3.
      */
    static inline Hyperplane Through(const VectorType& p0, const VectorType& p1, const VectorType& p2)
    {
      EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(VectorType, 3);
      Hyperplane result(p0.size());
      result.normal() = (p2 - p0).cross(p1 - p0).normalized();
      result.offset() = -result.normal().dot(p0);
      return result;
    }

    Hyperplane(const ParametrizedLine<Scalar, AmbientDimAtCompileTime>& parametrized)
    {
      normal() = parametrized.direction().unitOrthogonal();
      offset() = -normal().dot(parametrized.origin());
    }
    
    ~Hyperplane() {}

    /** \returns the dimension in which the plane holds */
    inline int dim() const { return AmbientDimAtCompileTime==Dynamic ? m_coeffs.size()-1 : AmbientDimAtCompileTime; }
    
    /** normalizes \c *this */
    void normalize(void)
    {
      m_coeffs /= normal().norm();
    }
    
    /** \returns the signed distance between the plane \c *this and a point \a p.
      */
    inline Scalar signedDistance(const VectorType& p) const { return p.dot(normal()) + offset(); }

    /** \returns the absolute distance between the plane \c *this and a point \a p.
      */
    inline Scalar absDistance(const VectorType& p) const { return ei_abs(signedDistance(p)); }
    
    /** \returns the projection of a point \a p onto the plane \c *this.
      */

    inline VectorType projection(const VectorType& p) const { return p - signedDistance(p) * normal(); }

    /** \returns a constant reference to the unit normal vector of the plane, which corresponds
      * to the linear part of the implicit equation.
      */
    inline const NormalReturnType normal() const { return NormalReturnType(m_coeffs,0,0,dim(),1); }

    /** \returns a non-constant reference to the unit normal vector of the plane, which corresponds
      * to the linear part of the implicit equation.
      */
    inline NormalReturnType normal() { return NormalReturnType(m_coeffs,0,0,dim(),1); }

    /** \returns the distance to the origin, which is also the "constant term" of the implicit equation
      * \warning the vector normal is assumed to be normalized.
      */
    inline const Scalar& offset() const { return m_coeffs.coeff(dim()); }

    /** \returns a non-constant reference to the distance to the origin, which is also the constant part
      * of the implicit equation */
    inline Scalar& offset() { return m_coeffs(dim()); }
    
    /** \returns a constant reference to the coefficients c_i of the plane equation:
      * \f$ c_0*x_0 + ... + c_{d-1}*x_{d-1} + c_d = 0 \f$
      */
    inline const Coefficients& coeffs() const { return m_coeffs; }

    /** \returns a non-constant reference to the coefficients c_i of the plane equation:
      * \f$ c_0*x_0 + ... + c_{d-1}*x_{d-1} + c_d = 0 \f$
      */
    inline Coefficients& coeffs() { return m_coeffs; }

    /** \returns the intersection of *this with \a other.
      *
      * \warning The ambient space must be a plane, i.e. have dimension 2, so that *this and \a other are lines.
      *
      * \note If \a other is approximately parallel to *this, this method will return any point on *this.
      */
    VectorType intersection(const Hyperplane& other)
    {
      EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(VectorType, 2);
      Scalar det = coeffs().coeff(0) * other.coeffs().coeff(1) - coeffs().coeff(1) * other.coeffs().coeff(0);
      // since the line equations ax+by=c are normalized with a^2+b^2=1, the following tests
      // whether the two lines are approximately parallel.
      if(ei_isMuchSmallerThan(det, Scalar(1)))
      {   // special case where the two lines are approximately parallel. Pick any point on the first line.
          if(ei_abs(coeffs().coeff(1))>ei_abs(coeffs().coeff(0)))
              return VectorType(coeffs().coeff(1), -coeffs().coeff(2)/coeffs().coeff(1)-coeffs().coeff(0));
          else
              return VectorType(-coeffs().coeff(2)/coeffs().coeff(0)-coeffs().coeff(1), coeffs().coeff(0));
      }
      else
      {   // general case
          Scalar invdet = Scalar(1) / det;
          return VectorType(invdet*(coeffs().coeff(1)*other.coeffs().coeff(2)-other.coeffs().coeff(1)*coeffs().coeff(2)),
                            invdet*(other.coeffs().coeff(0)*coeffs().coeff(2)-coeffs().coeff(0)*other.coeffs().coeff(2)));
      }
    }
    
    template<typename XprType>
    inline Hyperplane& transform(const MatrixBase<XprType>& mat, TransformTraits traits = GenericAffine)
    {
      if (traits==GenericAffine)
        normal() = mat.inverse().transpose() * normal();
      else if (traits==NoShear)
        normal() = (mat.colwise().norm2().cwise().inverse().eval().asDiagonal()
                    * mat.transpose()).transpose() * normal();
      else if (traits==NoScaling)
        normal() = mat * normal();
      else
      {
        ei_assert("invalid traits value in Hyperplane::transform()");
      }
      return *this;
    }

    inline Hyperplane& transform(const Transform<Scalar,AmbientDimAtCompileTime>& t,
                                 TransformTraits traits = GenericAffine)
    {
      transform(t.linear(), traits);
      offset() -= t.translation().dot(normal());
      return *this;
    }

protected:

    Coefficients m_coeffs;
};

/** Construct a parametrized line from a 2D hyperplane
  *
  * \warning the ambient space must have dimension 2 such that the hyperplane actually describes a line
  */
template <typename _Scalar, int _AmbientDim>
ParametrizedLine<_Scalar, _AmbientDim>::ParametrizedLine(const Hyperplane<_Scalar, _AmbientDim>& hyperplane)
{
  EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(VectorType, 2);
  direction() = hyperplane.normal().unitOrthogonal();
  origin() = -hyperplane.normal()*hyperplane.offset();
}

/** \returns the parameter value of the intersection between *this and the given hyperplane
  */
template <typename _Scalar, int _AmbientDim>
inline _Scalar ParametrizedLine<_Scalar, _AmbientDim>::intersection(const Hyperplane<_Scalar, _AmbientDim>& hyperplane)
{
  return -(hyperplane.offset()+origin().dot(hyperplane.normal()))
          /(direction().dot(hyperplane.normal()));
}

#endif // EIGEN_HYPERPLANE_H
