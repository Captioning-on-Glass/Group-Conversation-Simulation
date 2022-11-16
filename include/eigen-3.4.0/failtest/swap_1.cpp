#include "eigen-3.4.0/Eigen/Core"

using namespace Eigen;

int main()
{
  VectorXf a(10), b(10);
#ifdef EIGEN_SHOULD_FAIL_TO_BUILD
  const DenseBase<VectorXf> &ac(a);
#else
  DenseBase<VectorXf> &ac(a);
#endif
  b.swap(ac);
}
