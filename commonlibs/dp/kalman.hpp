#ifndef DP_KALMAN_HPP
#define DP_KALMAN_HPP
#ifdef _MSC_VER
	#pragma warning(disable:4267)
	#pragma warning(disable:4244)
	#pragma warning(disable:4996)
#endif

#include <vector>
#include <list>
#include <iostream>
#include "commonlibs/dp/matrixinversion.hpp"


namespace commonlibs {
/// \brief Linear Kalman filtering
///	\brief x(k+1) = Ax(k) + noise (Covariance Q)
///	\brief y(k) = Cx(k) + noise (Covariance R)

template<class Tfloat, class Timp>
class dp_kalman
{
private:
	unsigned int u_state, u_observation ;
	typedef commonlibs::Matrix<Tfloat> Tmtx ;
public:
	Tmtx A , C , Q , R , V , x, y ;

public:
	/// \brief deltaT is the sampling period
	/// \param initspd : mph * 10
	/// \param initdis: feet from intersection
	dp_kalman(unsigned int num_state , unsigned int num_observation) :
	  u_state(num_state) , u_observation(num_observation),
	  A(num_state , num_state) , C(num_observation, num_state),
	  Q(num_state , num_state) , R(num_observation, num_observation),
	  x(num_state ,1) ,
	  y(num_observation,1),V(num_state , num_state)
	{
		initKalman() ;
	}
	void initKalman()
	{
		static_cast<Timp *>(this)->initKalman() ;
	}
/*
if initial
  if isempty(u)
    xpred = x;
  else
    xpred = x + B*u;
  end
  Vpred = V;
else
  if isempty(u)
    xpred = A*x;
  else
    xpred = A*x + B*u;
  end
  Vpred = A*V*A' + Q;
end

e = y - C*xpred; % error (innovation)
n = length(e);
ss = length(A);
S = C*Vpred*C' + R;
Sinv = inv(S);
ss = length(V);
loglik = gaussian_prob(e, zeros(1,length(e)), S, 1);
K = Vpred*C'*Sinv; % Kalman gain matrix
% If there is no observation vector, set K = zeros(ss).
xnew = xpred + K*e;
Vnew = (eye(ss) - K*C)*Vpred;
VVnew = (eye(ss) - K*C)*A*V;
*/
	void update_kalman_filtering(
		const Tmtx &xpred ,
		const Tmtx &Vpred ,
		const Tmtx &y_,
		const Tmtx &x_,
		const Tmtx &V_)
	{
		Tmtx e = y_ - Tmtx::multiply(C, xpred) ;
		Tmtx cvp = Tmtx::multiply(C, Vpred) ;
		Tmtx S = Tmtx::multiply(cvp, C.transpose()) + R;
		Tmtx Sinv(u_observation, u_observation);
		bool b = commonlibs::InvertMatrix<Tfloat>(S , Sinv) ;
		if(b)
		{
			Tmtx vct = Tmtx::multiply(Vpred, C.transpose()) ;
			Tmtx K = Tmtx::multiply(vct, Sinv) ;
			x = xpred + Tmtx::multiply(K , e) ;
			Tmtx kc = Tmtx::multiply(K, C) ;
			V = Tmtx::multiply(
				Tmtx::identity(u_state) - kc,
				Vpred) ;
		}
		else
		{
			std::cerr << "Inversion failed" << std::endl ;
		}
	}
	void update_kalman_init(
		const Tmtx &y_,
		const Tmtx &initx_,
		const Tmtx &initV_)
	{
		Tmtx xpred = initx_ ;
		Tmtx Vpred = initV_ ;
		update_kalman_filtering(xpred, Vpred,
			y_, initx_, initV_) ;
	}
	void update_kalman(
		const Tmtx &y_)
	{
		Tmtx xpred = Tmtx::multiply(A , x) ;
		Tmtx av = Tmtx::multiply(A , V) ;
		Tmtx Vpred = Tmtx::multiply(av, A.transpose()) + Q ;
		update_kalman_filtering(xpred, Vpred,
			y_, x, V) ;
	}

};

}

#endif
