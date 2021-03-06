#include <gtest/gtest.h>

#include <Caribou/config.h>
#include <Eigen/Sparse>
#include <SofaCaribou/Algebra/EigenMatrixWrapper.h>

template<int nRows, int nColumns>
using Matrix = Eigen::Matrix<FLOATING_POINT_TYPE, nRows, nColumns>;

template <typename Derived>
void test_core(SofaCaribou::Algebra::EigenMatrixWrapper<Derived> & mm) {
    using Index = typename SofaCaribou::Algebra::EigenMatrixWrapper<Derived>::Index;
    mm.compress();
    // Set-get
    mm.set(30, 30, 200);
    EXPECT_EQ(mm(30,30), 200);

    // Add
    mm.add(30, 30, -100);
    EXPECT_EQ(mm(30, 30), 100);

    // Resize
    mm.resize(50, 50);
    mm.compress();
    EXPECT_EQ(mm.rows(), 50);
    EXPECT_EQ(mm.cols(), 50);
    int nb_not_equal = 0;
    for (Index i=0;i<mm.rows();++i) for (Index j=0;j<mm.cols();++j)
        if (mm(i,j) != 0) nb_not_equal++;

    EXPECT_EQ(nb_not_equal, 0) << "There are " << nb_not_equal << " values that are not equal to zero (and they should).";

    // Clear
    mm.clear();

    // add blocks
    // 3x3 doubles
    mm.add(0, 0, sofa::defaulttype::Mat3x3d(5));
    mm.compress();
    nb_not_equal = 0;
    for (Index i=0;i<3;++i) for (Index j=0;j<3;++j)
        if (mm(i,j) !=5) nb_not_equal++;
    EXPECT_EQ(nb_not_equal, 0) << "There are " << nb_not_equal << " values that are not equal to 5 (and they should).";

    // 3x3 floats
    mm.add(0, 0, sofa::defaulttype::Mat3x3f(5));
    nb_not_equal = 0;
    for (Index i=0;i<3;++i) for (Index j=0;j<3;++j)
        if (mm(i,j) != 10) nb_not_equal++;
    EXPECT_EQ(nb_not_equal, 0) << "There are " << nb_not_equal << " values that are not equal to 10 (and they should).";

    // 2x2 doubles
    mm.add(0, 0, sofa::defaulttype::Mat2x2d(5));
    nb_not_equal = 0;
    for (Index i=0;i<2;++i) for (Index j=0;j<2;++j)
        if (mm(i,j) !=15) nb_not_equal++;
    EXPECT_EQ(nb_not_equal, 0) << "There are " << nb_not_equal << " values that are not equal to 15 (and they should).";

    // 2x2 floats
    mm.add(0, 0, sofa::defaulttype::Mat2x2f(5));
    nb_not_equal = 0;
    for (Index i=0;i<2;++i) for (Index j=0;j<2;++j)
        if (mm(i,j) !=20) nb_not_equal++;
    EXPECT_EQ(nb_not_equal, 0) << "There are " << nb_not_equal << " values that are not equal to 20 (and they should).";

    // Clearing
    for (Index i=0;i<mm.rows();++i) for (Index j=0;j<mm.cols();++j)
        mm.set(i,j,1);

    // Clear row
    mm.clearRow(0);
    nb_not_equal = 0;
    for (Index i=0;i<mm.cols();++i)
        if (mm(0,i) !=0) nb_not_equal++;
    EXPECT_EQ(nb_not_equal, 0) << "There are " << nb_not_equal << " values that are not equal to zero (and they should).";

    // clear middle rows
    mm.clearRows(20, 30);
    nb_not_equal = 0;
    for (Index i=0;i<=10;++i) for (Index j=0;j<mm.cols();++j)
        if (mm(20+i,j) !=0) nb_not_equal++;
    EXPECT_EQ(nb_not_equal, 0) << "There are " << nb_not_equal << " values that are not equal to zero (and they should).";

    // Clear col
    mm.clearCol(0);
    nb_not_equal = 0;
    for (Index i=0;i<mm.rows();++i)
        if (mm(i,0) !=0) nb_not_equal++;
     EXPECT_EQ(nb_not_equal, 0) << "There are " << nb_not_equal << " values that are not equal to zero (and they should).";

    // clear middle cols
    mm.clearCols(20, 30);
    nb_not_equal = 0;
    for (Index i=0;i<mm.rows();++i) for (Index j=0;j<=10;++j)
        if (mm(i, 20+j) !=0) nb_not_equal++;
    EXPECT_EQ(nb_not_equal, 0) << "There are " << nb_not_equal << " values that are not equal to zero (and they should).";
}

TEST(Algebra, DenseMatrixByCopy) {
    using EigenDense = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>;
    EigenDense m(100,100);

    using EigenMatrixWrapper = SofaCaribou::Algebra::EigenMatrixWrapper<EigenDense>;
    EigenMatrixWrapper mm(m);
    EXPECT_EQ(mm.rows(), 100);
    EXPECT_EQ(mm.cols(), 100);

    // Testing the core functionalities
    test_core(mm);

    // Set - should not change the values of the initial Eigen matrix since the wrapper made a copy
    m(30, 30) = 100;
    mm.set(30, 30, 200);
    EXPECT_EQ(mm(30, 30), 200);
    EXPECT_EQ(m(30, 30), 100);
}

TEST(Algebra, DenseMatrixByReference) {
    using EigenDense = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>;
    using EigenWrapper = SofaCaribou::Algebra::EigenMatrixWrapper<EigenDense &>;
    EigenDense m(100,100);

    EigenWrapper mm(m);

    EXPECT_EQ(mm.rows(), 100);
    EXPECT_EQ(mm.cols(), 100);

    // Testing the core functionalities
    test_core(mm);

    // Set - should change the values of the initial Eigen matrix since the wrapper has a reference.
    m(30, 30) = 100;
    mm.set(30, 30, 200);
    EXPECT_EQ(mm(30, 30), 200);
    EXPECT_EQ(m(30, 30), 200);
}

TEST(Algebra, SparseMatrixByCopy) {
    using EigenSparse = Eigen::SparseMatrix<double>;
    using EigenWrapper = SofaCaribou::Algebra::EigenMatrixWrapper<EigenSparse>;

    const size_t N = 100;
    EigenSparse m(N, N);
    EigenWrapper mm(m);
    EXPECT_EQ(mm.rows(), N);
    EXPECT_EQ(mm.cols(), N);

    ASSERT_DEBUG_DEATH(mm(30,30), "Accessing an element on an uninitialized matrix.");
    mm.compress();

    // Testing the core functionalities
    test_core(mm);

    // Set - should not change the values of the initial Eigen matrix since the wrapper made a copy
    m.coeffRef(30,30) = 100;
    mm.set(30, 30, 200);
    EXPECT_EQ(mm(30, 30), 200);
    EXPECT_EQ(m.coeff(30, 30), 100);
}

TEST(Algebra, SparseMatrixByReference) {
    using EigenSparse = Eigen::SparseMatrix<double>;
    using EigenWrapper = SofaCaribou::Algebra::EigenMatrixWrapper<EigenSparse &>;

    const size_t N = 100;
    EigenSparse m(N, N);
    EigenWrapper mm(m);
    EXPECT_EQ(mm.rows(), N);
    EXPECT_EQ(mm.cols(), N);

    // Testing the core functionalities
    test_core(mm);

    // Set - should change the values of the initial Eigen matrix since the wrapper has a reference.
    m.coeffRef(30,30) = 100;
    mm.set(30, 30, 200);
    EXPECT_EQ(mm(30, 30), 200);
    EXPECT_EQ(m.coeff(30, 30), 200);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
