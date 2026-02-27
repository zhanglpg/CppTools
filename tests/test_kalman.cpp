// NOTE: dp_kalman calls initKalman() from the base-class constructor via CRTP.
// At that point derived-class members are not yet constructed, so initKalman()
// must not read any derived-class member variables — use only constants.

#include "commonlibs/dp/kalman.hpp"
#include <gtest/gtest.h>
#include <cmath>

using Matrix = commonlibs::Matrix<double>;

static constexpr double kEps = 1e-6;

// ---- Concrete Kalman implementations ---------------------------------------

// 1-D constant-position model.
//   State x = [position], A = [1], C = [1]
//   Process noise Q = 0.01, Observation noise R = 1.0
class KalmanConstPos1D
    : public commonlibs::dp_kalman<double, KalmanConstPos1D>
{
public:
    KalmanConstPos1D()
        : dp_kalman<double, KalmanConstPos1D>(1, 1) {}

    void initKalman()
    {
        A(0,0) = 1.0;
        C(0,0) = 1.0;
        Q(0,0) = 0.01;   // constants only — no derived member reads here
        R(0,0) = 1.0;
    }
};

// 2-D constant-velocity model, dt = 1 s.
//   State x = [position, velocity], observation = [position]
class KalmanConstVel2D
    : public commonlibs::dp_kalman<double, KalmanConstVel2D>
{
public:
    KalmanConstVel2D()
        : dp_kalman<double, KalmanConstVel2D>(2, 1) {}

    void initKalman()
    {
        A(0,0) = 1.0; A(0,1) = 1.0;   // dt = 1
        A(1,0) = 0.0; A(1,1) = 1.0;

        C(0,0) = 1.0; C(0,1) = 0.0;

        Q(0,0) = 1e-4; Q(0,1) = 0.0;
        Q(1,0) = 0.0;  Q(1,1) = 1e-4;

        R(0,0) = 1.0;
    }
};

// ---- helpers ---------------------------------------------------------------

static Matrix col1(double v)
{
    Matrix m(1, 1); m(0,0) = v; return m;
}
static Matrix col2(double v0, double v1)
{
    Matrix m(2, 1); m(0,0) = v0; m(1,0) = v1; return m;
}
static Matrix eye(int n)
{
    return Matrix::identity(static_cast<std::size_t>(n));
}

// ---- 1-D tests -------------------------------------------------------------

TEST(Kalman1D, ConstructionDoesNotCrash)
{
    KalmanConstPos1D k;
    SUCCEED();
}

TEST(Kalman1D, InitKalman_MatricesSet)
{
    KalmanConstPos1D k;
    EXPECT_NEAR(1.0,  k.A(0,0), kEps);
    EXPECT_NEAR(1.0,  k.C(0,0), kEps);
    EXPECT_NEAR(0.01, k.Q(0,0), kEps);
    EXPECT_NEAR(1.0,  k.R(0,0), kEps);
}

TEST(Kalman1D, UpdateInit_StateMovesTowardObservation)
{
    KalmanConstPos1D k;
    Matrix initx = col1(0.0);
    Matrix initV = eye(1);

    k.update_kalman_init(col1(10.0), initx, initV);
    // Estimate should move toward the observation (10) from the prior (0)
    EXPECT_GT(k.x(0,0), 0.0);
    EXPECT_LE(k.x(0,0), 10.0 + kEps);
}

TEST(Kalman1D, UpdateKalman_ConvergesOnConstantSignal)
{
    KalmanConstPos1D k;
    Matrix initx = col1(0.0);
    Matrix initV = eye(1);
    k.update_kalman_init(col1(10.0), initx, initV);

    for (int i = 0; i < 200; ++i)
        k.update_kalman(col1(10.0));

    EXPECT_NEAR(10.0, k.x(0,0), 0.2);
}

TEST(Kalman1D, UpdateKalman_TrackStep)
{
    // After a step change from 0 to 20, the estimate should monotonically
    // increase toward 20 over several updates.
    KalmanConstPos1D k;
    k.update_kalman_init(col1(0.0), col1(0.0), eye(1));

    double prev = k.x(0,0);
    for (int i = 0; i < 50; ++i)
        k.update_kalman(col1(20.0));

    EXPECT_GT(k.x(0,0), prev);
    EXPECT_LT(k.x(0,0), 20.0 + 0.2);
}

// ---- 2-D tests -------------------------------------------------------------

TEST(Kalman2D, ConstructionDoesNotCrash)
{
    KalmanConstVel2D k;
    SUCCEED();
}

TEST(Kalman2D, InitKalman_TransitionMatrix)
{
    KalmanConstVel2D k;
    EXPECT_NEAR(1.0, k.A(0,0), kEps);
    EXPECT_NEAR(1.0, k.A(0,1), kEps);   // dt = 1
    EXPECT_NEAR(0.0, k.A(1,0), kEps);
    EXPECT_NEAR(1.0, k.A(1,1), kEps);
}

TEST(Kalman2D, UpdateInit_NoCrash)
{
    KalmanConstVel2D k;
    k.update_kalman_init(col1(0.0), col2(0.0, 1.0), eye(2));
    SUCCEED();
}

TEST(Kalman2D, UpdateKalman_PositionFollowsLinearTrajectory)
{
    // True motion: position = t (velocity = 1 m/s, dt = 1 s)
    // Run 50 steps; expect position estimate close to 50, velocity close to 1.
    KalmanConstVel2D k;
    k.update_kalman_init(col1(0.0), col2(0.0, 1.0), eye(2));

    for (int t = 1; t <= 50; ++t)
        k.update_kalman(col1(static_cast<double>(t)));

    EXPECT_NEAR(50.0, k.x(0,0), 2.0);
    EXPECT_NEAR(1.0,  k.x(1,0), 0.5);
}
