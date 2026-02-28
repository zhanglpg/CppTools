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

// ============================================================================
// Matrix class arithmetic and structural operations
// ============================================================================

TEST(Matrix, DefaultConstructor_ZeroSize)
{
    Matrix m;
    EXPECT_EQ(0u, m.size1());
    EXPECT_EQ(0u, m.size2());
}

TEST(Matrix, ConstructorZeroInitializes)
{
    Matrix m(2, 3);
    EXPECT_EQ(2u, m.size1());
    EXPECT_EQ(3u, m.size2());
    for (std::size_t i = 0; i < 2; ++i)
        for (std::size_t j = 0; j < 3; ++j)
            EXPECT_EQ(0.0, m(i, j));
}

TEST(Matrix, AdditionElementWise)
{
    Matrix A(2, 2), B(2, 2);
    A(0,0)=1; A(0,1)=2; A(1,0)=3; A(1,1)=4;
    B(0,0)=5; B(0,1)=6; B(1,0)=7; B(1,1)=8;

    Matrix C = A + B;
    EXPECT_EQ(2u, C.size1());
    EXPECT_EQ(2u, C.size2());
    EXPECT_NEAR( 6.0, C(0,0), kEps);
    EXPECT_NEAR( 8.0, C(0,1), kEps);
    EXPECT_NEAR(10.0, C(1,0), kEps);
    EXPECT_NEAR(12.0, C(1,1), kEps);
}

TEST(Matrix, SubtractionElementWise)
{
    Matrix A(2, 2), B(2, 2);
    A(0,0)=9; A(0,1)=7; A(1,0)=5; A(1,1)=3;
    B(0,0)=1; B(0,1)=2; B(1,0)=3; B(1,1)=4;

    Matrix C = A - B;
    EXPECT_NEAR(8.0, C(0,0), kEps);
    EXPECT_NEAR(5.0, C(0,1), kEps);
    EXPECT_NEAR(2.0, C(1,0), kEps);
    EXPECT_NEAR(-1.0, C(1,1), kEps);
}

TEST(Matrix, AdditionWithIdentityGivesShifted)
{
    Matrix A(3, 3);
    A(0,0)=1; A(1,1)=2; A(2,2)=3;
    Matrix I = Matrix::identity(3);
    Matrix result = A + I;
    EXPECT_NEAR(2.0, result(0,0), kEps);
    EXPECT_NEAR(3.0, result(1,1), kEps);
    EXPECT_NEAR(4.0, result(2,2), kEps);
    EXPECT_NEAR(0.0, result(0,1), kEps);
}

TEST(Matrix, Transpose_Square)
{
    Matrix A(3, 3);
    A(0,0)=1; A(0,1)=2; A(0,2)=3;
    A(1,0)=4; A(1,1)=5; A(1,2)=6;
    A(2,0)=7; A(2,1)=8; A(2,2)=9;

    Matrix T = A.transpose();
    EXPECT_EQ(3u, T.size1());
    EXPECT_EQ(3u, T.size2());
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j)
            EXPECT_NEAR(A(i,j), T(j,i), kEps);
}

TEST(Matrix, Transpose_Rectangular_2x3_Gives_3x2)
{
    Matrix A(2, 3);
    A(0,0)=1; A(0,1)=2; A(0,2)=3;
    A(1,0)=4; A(1,1)=5; A(1,2)=6;

    Matrix T = A.transpose();
    EXPECT_EQ(3u, T.size1());
    EXPECT_EQ(2u, T.size2());
    EXPECT_NEAR(1.0, T(0,0), kEps); EXPECT_NEAR(4.0, T(0,1), kEps);
    EXPECT_NEAR(2.0, T(1,0), kEps); EXPECT_NEAR(5.0, T(1,1), kEps);
    EXPECT_NEAR(3.0, T(2,0), kEps); EXPECT_NEAR(6.0, T(2,1), kEps);
}

TEST(Matrix, Transpose_DoubleTranspose_IsOriginal)
{
    Matrix A(2, 3);
    A(0,0)=1; A(0,1)=2; A(0,2)=3;
    A(1,0)=4; A(1,1)=5; A(1,2)=6;
    Matrix TT = A.transpose().transpose();
    EXPECT_EQ(2u, TT.size1());
    EXPECT_EQ(3u, TT.size2());
    for (std::size_t i = 0; i < 2; ++i)
        for (std::size_t j = 0; j < 3; ++j)
            EXPECT_NEAR(A(i,j), TT(i,j), kEps);
}

TEST(Matrix, Transpose_Identity_IsIdentity)
{
    Matrix I = Matrix::identity(4);
    Matrix T = I.transpose();
    expect_near_matrix(T, I);
}

TEST(Matrix, Multiply_NonSquare)
{
    // 2x3 * 3x2 = 2x2
    Matrix A(2, 3), B(3, 2);
    A(0,0)=1; A(0,1)=2; A(0,2)=3;
    A(1,0)=4; A(1,1)=5; A(1,2)=6;
    B(0,0)=7;  B(0,1)=8;
    B(1,0)=9;  B(1,1)=10;
    B(2,0)=11; B(2,1)=12;

    Matrix C = Matrix::multiply(A, B);
    EXPECT_EQ(2u, C.size1());
    EXPECT_EQ(2u, C.size2());
    // Row 0: [1*7+2*9+3*11, 1*8+2*10+3*12] = [58, 64]
    EXPECT_NEAR( 58.0, C(0,0), kEps);
    EXPECT_NEAR( 64.0, C(0,1), kEps);
    // Row 1: [4*7+5*9+6*11, 4*8+5*10+6*12] = [139, 154]
    EXPECT_NEAR(139.0, C(1,0), kEps);
    EXPECT_NEAR(154.0, C(1,1), kEps);
}

// ============================================================================
// Numerical robustness
// ============================================================================

TEST(MatrixInversion, IllConditioned_HilbertMatrix3x3)
{
    // Hilbert matrix: H(i,j) = 1/(i+j+1).  Notoriously ill-conditioned.
    Matrix H(3, 3);
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j)
            H(i, j) = 1.0 / (i + j + 1);

    Matrix inv(3, 3);
    ASSERT_TRUE(commonlibs::InvertMatrix<double>(H, inv));

    Matrix product = multiply(H, inv);
    expect_near_matrix(product, Matrix::identity(3), 1e-6);
}

TEST(MatrixInversion, LargeValues_DiagonalMatrix)
{
    Matrix A(2, 2);
    A(0,0) = 1e10; A(0,1) = 0;
    A(1,0) = 0;    A(1,1) = 1e10;

    Matrix inv(2, 2);
    EXPECT_TRUE(commonlibs::InvertMatrix<double>(A, inv));
    EXPECT_NEAR(1e-10, inv(0,0), 1e-19);
    EXPECT_NEAR(1e-10, inv(1,1), 1e-19);
    EXPECT_NEAR(0.0,   inv(0,1), kEps);
}

TEST(MatrixInversion, SmallValues_DiagonalMatrix)
{
    Matrix A(2, 2);
    A(0,0) = 1e-10; A(0,1) = 0;
    A(1,0) = 0;     A(1,1) = 1e-10;

    Matrix inv(2, 2);
    EXPECT_TRUE(commonlibs::InvertMatrix<double>(A, inv));
    EXPECT_NEAR(1e10, inv(0,0), 1.0);
    EXPECT_NEAR(1e10, inv(1,1), 1.0);
}

TEST(MatrixInversion, PermutationMatrix3x3)
{
    // Permutation [[0,1,0],[0,0,1],[1,0,0]] — its inverse is its transpose
    Matrix P(3, 3);
    P(0,1) = 1; P(1,2) = 1; P(2,0) = 1;

    Matrix inv(3, 3);
    ASSERT_TRUE(commonlibs::InvertMatrix<double>(P, inv));
    expect_near_matrix(inv, P.transpose());
}

TEST(MatrixInversion, SymmetricPositiveDefinite)
{
    // A = [[4,2],[2,3]] — SPD, eigenvalues > 0
    Matrix A(2, 2);
    A(0,0) = 4; A(0,1) = 2;
    A(1,0) = 2; A(1,1) = 3;

    Matrix inv(2, 2);
    ASSERT_TRUE(commonlibs::InvertMatrix<double>(A, inv));

    Matrix product = multiply(A, inv);
    expect_near_matrix(product, Matrix::identity(2), 1e-10);
}
