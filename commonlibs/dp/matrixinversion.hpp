 #ifndef INVERT_MATRIX_HPP
 #define INVERT_MATRIX_HPP

 #include <vector>
 #include <cstddef>
 #include <algorithm>
 #include <stdexcept>

 namespace commonlibs {

 /// Minimal dense matrix with value semantics, row-major storage.
 template<class T>
 class Matrix {
 public:
     Matrix() : rows_(0), cols_(0) {}

     Matrix(std::size_t rows, std::size_t cols)
         : rows_(rows), cols_(cols), data_(rows * cols, T{}) {}

     static Matrix identity(std::size_t n)
     {
         Matrix m(n, n);
         for (std::size_t i = 0; i < n; ++i) m(i, i) = T{1};
         return m;
     }

     std::size_t size1() const { return rows_; }
     std::size_t size2() const { return cols_; }

     T& operator()(std::size_t i, std::size_t j)
     {
         return data_[i * cols_ + j];
     }
     const T& operator()(std::size_t i, std::size_t j) const
     {
         return data_[i * cols_ + j];
     }

     Matrix operator+(const Matrix &other) const
     {
         Matrix result(rows_, cols_);
         for (std::size_t k = 0; k < data_.size(); ++k)
             result.data_[k] = data_[k] + other.data_[k];
         return result;
     }

     Matrix operator-(const Matrix &other) const
     {
         Matrix result(rows_, cols_);
         for (std::size_t k = 0; k < data_.size(); ++k)
             result.data_[k] = data_[k] - other.data_[k];
         return result;
     }

     static Matrix multiply(const Matrix &A, const Matrix &B)
     {
         Matrix result(A.rows_, B.cols_);
         for (std::size_t i = 0; i < A.rows_; ++i)
             for (std::size_t k = 0; k < A.cols_; ++k)
                 for (std::size_t j = 0; j < B.cols_; ++j)
                     result(i, j) += A(i, k) * B(k, j);
         return result;
     }

     Matrix transpose() const
     {
         Matrix result(cols_, rows_);
         for (std::size_t i = 0; i < rows_; ++i)
             for (std::size_t j = 0; j < cols_; ++j)
                 result(j, i) = (*this)(i, j);
         return result;
     }

 private:
     std::size_t rows_, cols_;
     std::vector<T> data_;
 };

 /* Matrix inversion using Gauss-Jordan elimination with partial pivoting.
    Returns true on success, false if the matrix is singular. */
 template<class T>
 bool InvertMatrix(const Matrix<T> &input, Matrix<T> &inverse)
 {
     std::size_t n = input.size1();
     if (n == 0 || n != input.size2()) return false;

     // Build augmented matrix [input | I]
     Matrix<T> aug(n, 2 * n);
     for (std::size_t i = 0; i < n; ++i) {
         for (std::size_t j = 0; j < n; ++j)
             aug(i, j) = input(i, j);
         aug(i, n + i) = T{1};
     }

     for (std::size_t col = 0; col < n; ++col) {
         // Find pivot row with largest absolute value in this column
         std::size_t pivot = col;
         T best = aug(col, col) < T{0} ? -aug(col, col) : aug(col, col);
         for (std::size_t row = col + 1; row < n; ++row) {
             T v = aug(row, col) < T{0} ? -aug(row, col) : aug(row, col);
             if (v > best) { best = v; pivot = row; }
         }
         if (best == T{0}) return false; // singular

         // Swap rows
         if (pivot != col)
             for (std::size_t j = 0; j < 2 * n; ++j)
                 std::swap(aug(col, j), aug(pivot, j));

         // Scale pivot row so the diagonal becomes 1
         T scale = aug(col, col);
         for (std::size_t j = 0; j < 2 * n; ++j)
             aug(col, j) /= scale;

         // Eliminate all other rows in this column
         for (std::size_t row = 0; row < n; ++row) {
             if (row == col) continue;
             T factor = aug(row, col);
             if (factor == T{0}) continue;
             for (std::size_t j = 0; j < 2 * n; ++j)
                 aug(row, j) -= factor * aug(col, j);
         }
     }

     // Extract the right half as the inverse
     inverse = Matrix<T>(n, n);
     for (std::size_t i = 0; i < n; ++i)
         for (std::size_t j = 0; j < n; ++j)
             inverse(i, j) = aug(i, n + j);

     return true;
 }

 } // namespace commonlibs
 #endif // INVERT_MATRIX_HPP
