// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2009 Ilya Baran <ibaran@mit.edu>
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

#ifndef KDBVH_H_INCLUDED
#define KDBVH_H_INCLUDED

//internal pair class for the BVH--used instead of std::pair because of alignment
template<typename Scalar, int Dim>
struct ei_vector_int_pair
{
EIGEN_MAKE_ALIGNED_OPERATOR_NEW_IF_VECTORIZABLE_FIXED_SIZE(Scalar, Dim)
  typedef Matrix<Scalar, Dim, 1> VectorType;

  ei_vector_int_pair(const VectorType &v, int i) : first(v), second(i) {}

  VectorType first;
  int second;
};

//these templates help the tree initializer get the bounding boxes either from a provided
//iterator range or using ei_bounding_box in a unified way
template<typename Object, typename Volume, typename BoxIter>
struct ei_get_boxes_helper {
  void operator()(const std::vector<Object> &objects, BoxIter boxBegin, BoxIter boxEnd, std::vector<Volume> &outBoxes)
  {
    outBoxes.insert(outBoxes.end(), boxBegin, boxEnd);
    ei_assert(outBoxes.size() == objects.size());
  }
};

template<typename Object, typename Volume>
struct ei_get_boxes_helper<Object, Volume, int> {
  void operator()(const std::vector<Object> &objects, int, int, std::vector<Volume> &outBoxes)
  {
    outBoxes.reserve(objects.size());
    for(int i = 0; i < (int)objects.size(); ++i)
      outBoxes.push_back(ei_bounding_box(objects[i]));
  }
};


/** \class KdBVH
 *  \brief A simple bounding volume hierarchy based on AlignedBox
 *
 *  \param _Scalar The underlying scalar type of the bounding boxes
 *  \param _Dim The dimension of the space in which the hierarchy lives
 *  \param _Object The object type that lives in the hierarchy.  It must have value semantics.  Either ei_bounding_box(_Object) must
 *                 be defined and return an AlignedBox<_Scalar, _Dim> or bounding boxes must be provided to the tree initializer.
 *
 *  This class provides a simple (as opposed to optimized) implementation of a bounding volume hierarchy analogous to a Kd-tree.
 *  Given a sequence of objects, it computes their bounding boxes, constructs a Kd-tree of their centers
 *  and builds a BVH with the structure of that Kd-tree.  When the elements of the tree are too expensive to be copied around,
 *  it is useful for _Object to be a pointer.
 */
template<typename _Scalar, int _Dim, typename _Object> class KdBVH
{
public:
  enum { Dim = _Dim };
  typedef _Object Object;
  typedef _Scalar Scalar;
  typedef AlignedBox<Scalar, Dim> Volume;
  typedef int Index;
  typedef const int *VolumeIterator; //the iterators are just pointers into the tree's vectors
  typedef const Object *ObjectIterator;

  KdBVH() {}

  /** Given an iterator range over \a Object references, constructs the BVH.  Requires that ei_bounding_box(Object) return a Volume. */
  template<typename Iter> KdBVH(Iter begin, Iter end) { init(begin, end, 0, 0); } //int is recognized by init as not being an iterator type

  /** Given an iterator range over \a Object references and an iterator range over their bounding boxes, constructs the BVH */
  template<typename OIter, typename BIter> KdBVH(OIter begin, OIter end, BIter boxBegin, BIter boxEnd) { init(begin, end, boxBegin, boxEnd); }

  /** Given an iterator range over \a Object references, constructs the BVH, overwriting whatever is in there currently.
    * Requires that ei_bounding_box(Object) return a Volume. */
  template<typename Iter> void init(Iter begin, Iter end) { init(begin, end, 0, 0); }

  /** Given an iterator range over \a Object references and an iterator range over their bounding boxes,
    * constructs the BVH, overwriting whatever is in there currently. */
  template<typename OIter, typename BIter> void init(OIter begin, OIter end, BIter boxBegin, BIter boxEnd)
  {
    objects.clear();
    boxes.clear();
    children.clear();

    objects.insert(objects.end(), begin, end);
    int n = objects.size();

    if(n < 2)
      return; //if we have at most one object, we don't need any internal nodes

    std::vector<Volume> objBoxes;
    std::vector<VIPair> objCenters;

    ei_get_boxes_helper<Object, Volume, BIter>()(objects, boxBegin, boxEnd, objBoxes); //compute the bounding boxes depending on BIter type

    objCenters.reserve(n);
    boxes.reserve(n - 1);
    children.reserve(2 * n - 2);

    for(int i = 0; i < n; ++i)
      objCenters.push_back(VIPair(objBoxes[i].center(), i));

    build(objCenters, 0, n, objBoxes, 0); //the recursive part of the algorithm

    std::vector<Object> tmp(n);
    tmp.swap(objects);
    for(int i = 0; i < n; ++i)
      objects[i] = tmp[objCenters[i].second];
  }

  /** \returns the index of the root of the hierarchy */
  inline Index getRootIndex() const { return (int)boxes.size() - 1; }

  /** Given an \a index of a node, on exit, \a outVBegin and \a outVEnd range over the indices of the volume children of the node
    * and \a outOBegin and \a outOEnd range over the object children of the node */
  EIGEN_STRONG_INLINE void getChildren(Index index, VolumeIterator &outVBegin, VolumeIterator &outVEnd,
                                       ObjectIterator &outOBegin, ObjectIterator &outOEnd) const
  { //inlining this function should open lots of optimization opportunities to the compiler
    if(index < 0) {
      outVBegin = outVEnd;
      if(!objects.empty())
        outOBegin = &(objects[0]);
      outOEnd = outOBegin + objects.size(); //output all objects--necessary when the tree has only one object
      return;
    }

    int numBoxes = boxes.size();

    int idx = index * 2;
    if(children[idx + 1] < numBoxes) { //second index is always bigger
      outVBegin = &(children[idx]);
      outVEnd = outVBegin + 2;
      outOBegin = outOEnd;
    }
    else if(children[idx] >= numBoxes) { //if both children are objects
      outVBegin = outVEnd;
      outOBegin = &(objects[children[idx] - numBoxes]);
      outOEnd = outOBegin + 2;
    } else { //if the first child is a volume and the second is an object
      outVBegin = &(children[idx]);
      outVEnd = outVBegin + 1;
      outOBegin = &(objects[children[idx + 1] - numBoxes]);
      outOEnd = outOBegin + 1;
    }
  }

  /** \returns the bounding box of the node at \a index */
  inline const Volume &getVolume(Index index) const
  {
    return boxes[index];
  }

private:
  typedef ei_vector_int_pair<Scalar, Dim> VIPair;
  typedef Matrix<Scalar, Dim, 1> VectorType;
  struct VectorComparator //compares vectors, or, more specificall, VIPairs along a particular dimension
  {
    VectorComparator(int inDim) : dim(inDim) {}
    inline bool operator()(const VIPair &v1, const VIPair &v2) const { return v1.first[dim] < v2.first[dim]; }
    int dim;
  };

  //Build the part of the tree between objects[from] and objects[to] (not including objects[to]).
  //This routine partitions the objCenters in [from, to) along the dimension dim, recursively constructs
  //the two halves, and adds their parent node.  TODO: a cache-friendlier layout
  void build(std::vector<VIPair> &objCenters, int from, int to, const std::vector<Volume> &objBoxes, int dim)
  {
    ei_assert(to - from > 1);
    if(to - from == 2) {
      boxes.push_back(objBoxes[objCenters[from].second].merged(objBoxes[objCenters[from + 1].second]));
      children.push_back(from + (int)objects.size() - 1); //there are objects.size() - 1 tree nodes
      children.push_back(from + (int)objects.size());
    }
    else if(to - from == 3) {
      int mid = from + 2;
      std::nth_element(objCenters.begin() + from, objCenters.begin() + mid,
                        objCenters.begin() + to, VectorComparator(dim)); //partition
      build(objCenters, from, mid, objBoxes, (dim + 1) % Dim);
      int idx1 = (int)boxes.size() - 1;
      boxes.push_back(boxes[idx1].merged(objBoxes[objCenters[mid].second]));
      children.push_back(idx1);
      children.push_back(mid + (int)objects.size() - 1);
    }
    else {
      int mid = from + (to - from) / 2;
      nth_element(objCenters.begin() + from, objCenters.begin() + mid,
                  objCenters.begin() + to, VectorComparator(dim)); //partition
      build(objCenters, from, mid, objBoxes, (dim + 1) % Dim);
      int idx1 = (int)boxes.size() - 1;
      build(objCenters, mid, to, objBoxes, (dim + 1) % Dim);
      int idx2 = (int)boxes.size() - 1;
      boxes.push_back(boxes[idx1].merged(boxes[idx2]));
      children.push_back(idx1);
      children.push_back(idx2);
    }
  }

  std::vector<int> children; //children of x are children[2x] and children[2x+1], indices bigger than boxes.size() index into objects.
  std::vector<Volume> boxes;
  std::vector<Object> objects;
};

#endif //KDBVH_H_INCLUDED
