#include "commonlibs/dp/matrixinversion.hpp"
#include <gtest/gtest.h>
#include <cmath>

using Matrix = commonlibs::Matrix<double>;

// Tolerance for floating-point comparisons
static constexpr double kEps = 1e-9;

// Build the product A * B and return it
static Matrix multiply(const Matrix &A, const Matrix &B)
{
    return Matrix::multiply(A, B);
}

// Check that |x - expected| < eps for every element
static void expect_near_matrix(const Matrix &result,
                                const Matrix &expected,
                                double eps = kEps)
{
    ASSERT_EQ(result.size1(), expected.size1());
    ASSERT_EQ(result.size2(), expected.size2());
    for (std::size_t i = 0; i < result.size1(); ++i)
        for (std::size_t j = 0; j < result.size2(); ++j)
            EXPECT_NEAR(result(i,j), expected(i,j), eps)
                << "at (" << i << "," << j << ")";
}

// ---- identity matrix -------------------------------------------------------

TEST(MatrixInversion, Identity2x2)
{
    Matrix I = Matrix::identity(2);
    Matrix inv(2, 2);
    EXPECT_TRUE(commonlibs::InvertMatrix<double>(I, inv));
    expect_near_matrix(inv, I);
}

TEST(MatrixInversion, Identity3x3)
{
    Matrix I = Matrix::identity(3);
    Matrix inv(3, 3);
    EXPECT_TRUE(commonlibs::InvertMatrix<double>(I, inv));
    expect_near_matrix(inv, I);
}

// ---- known inverse ---------------------------------------------------------

TEST(MatrixInversion, Simple2x2)
{
    // [[1,2],[3,4]]  → inverse = [[-2, 1],[1.5,-0.5]]
    Matrix A(2, 2);
    A(0,0)=1; A(0,1)=2;
    A(1,0)=3; A(1,1)=4;

    Matrix inv(2, 2);
    EXPECT_TRUE(commonlibs::InvertMatrix<double>(A, inv));

    Matrix expected(2, 2);
    expected(0,0)=-2.0;  expected(0,1)= 1.0;
    expected(1,0)= 1.5;  expected(1,1)=-0.5;
    expect_near_matrix(inv, expected);
}

TEST(MatrixInversion, DiagonalMatrix)
{
    // diag(2,4,8) → inv = diag(0.5, 0.25, 0.125)
    Matrix D(3, 3);  // zero-initialized
    D(0,0) = 2; D(1,1) = 4; D(2,2) = 8;

    Matrix inv(3, 3);
    EXPECT_TRUE(commonlibs::InvertMatrix<double>(D, inv));

    Matrix expected(3, 3);  // zero-initialized
    expected(0,0) = 0.5; expected(1,1) = 0.25; expected(2,2) = 0.125;
    expect_near_matrix(inv, expected);
}

// ---- A * inv(A) == I -------------------------------------------------------

TEST(MatrixInversion, ProductIsIdentity_3x3)
{
    Matrix A(3, 3);
    A(0,0)=1; A(0,1)=2; A(0,2)=3;
    A(1,0)=0; A(1,1)=1; A(1,2)=4;
    A(2,0)=5; A(2,1)=6; A(2,2)=0;

    Matrix inv(3, 3);
    ASSERT_TRUE(commonlibs::InvertMatrix<double>(A, inv));

    Matrix product = multiply(A, inv);
    Matrix I = Matrix::identity(3);
    expect_near_matrix(product, I, 1e-10);
}

TEST(MatrixInversion, ProductIsIdentity_4x4)
{
    // Upper-triangular with all-1 diagonal
    Matrix A = Matrix::identity(4);
    A(0,1) = 1; A(0,2) = 2; A(0,3) = 3;
    A(1,2) = 4; A(1,3) = 5;
    A(2,3) = 6;

    Matrix inv(4, 4);
    ASSERT_TRUE(commonlibs::InvertMatrix<double>(A, inv));

    Matrix product = multiply(A, inv);
    expect_near_matrix(product, Matrix::identity(4), 1e-10);
}

// ---- singular matrix -------------------------------------------------------

TEST(MatrixInversion, SingularMatrix_ReturnsFalse)
{
    // Row 2 = 2 * Row 1 → determinant == 0
    Matrix A(2, 2);
    A(0,0) = 1; A(0,1) = 2;
    A(1,0) = 2; A(1,1) = 4;

    Matrix inv(2, 2);
    EXPECT_FALSE(commonlibs::InvertMatrix<double>(A, inv));
}

TEST(MatrixInversion, AllZeroMatrix_ReturnsFalse)
{
    Matrix A(3, 3);  // zero-initialized
    Matrix inv(3, 3);
    EXPECT_FALSE(commonlibs::InvertMatrix<double>(A, inv));
}

// ---- scalar (1x1) matrix ---------------------------------------------------

TEST(MatrixInversion, Scalar1x1)
{
    Matrix A(1, 1);
    A(0,0) = 5.0;
    Matrix inv(1, 1);
    EXPECT_TRUE(commonlibs::InvertMatrix<double>(A, inv));
    EXPECT_NEAR(0.2, inv(0,0), kEps);
}
