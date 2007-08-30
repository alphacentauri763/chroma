// $Id: clover_term_qdp_w.cc,v 3.16 2007-08-30 17:27:47 edwards Exp $
/*! \file
 *  \brief Clover term linear operator
 *
 *  This particular implementation is specific to a scalar-like
 *  architecture
 */

#include "chromabase.h"
#include "actions/ferm/linop/clover_term_qdp_w.h"
#include "meas/glue/mesfield.h"


namespace Chroma 
{ 

  // Reader/writers
  void read(XMLReader& xml, const string& path, PrimitiveClovTriang& param)
  {
    QDPIO::cerr << __func__ << ": clover term reader not implemented" << endl;
    QDP_abort(1);
  }

  void write(XMLWriter& xml, const string& path, const PrimitiveClovTriang& d)
  {
    xml.openTag(path);

    XMLWriterAPI::AttributeList alist;

    xml.openTag("Diag");
    for(int i=0; i < 2; ++i)
    {
      for(int j=0; j < 2*Nc; ++j)
      {
	alist.clear();
	alist.push_back(XMLWriterAPI::Attribute("block", i));
	alist.push_back(XMLWriterAPI::Attribute("col", j));

	xml.openTag("elem", alist);
	xml << d.diag[i][j];
	xml.closeTag();
      }
    }
    xml.closeTag(); // Diag

    xml.openTag("Offd");
    for(int i=0; i < 2; ++i)
    {
      for(int j=0; j < 2*Nc*Nc-Nc; ++j)
      {
	alist.clear();
	alist.push_back(XMLWriterAPI::Attribute("block", i));
	alist.push_back(XMLWriterAPI::Attribute("col", j));

	xml.openTag("elem", alist);
	xml << d.offd[i][j];
	xml.closeTag();
      }
    }
    xml.closeTag(); // Offd
    xml.closeTag(); // path
  }


  // Empty constructor. Must use create later
  QDPCloverTerm::QDPCloverTerm() {}

  // Now copy
  void QDPCloverTerm::create(Handle< FermState<T,P,Q> > fs,
			     const CloverFermActParams& param_,
			     const QDPCloverTerm& from)
  {
    START_CODE();

    u = fs->getLinks();
    fbc = fs->getFermBC();
    param = param_;
      
    // Sanity check
    if (fbc.operator->() == 0) {
      QDPIO::cerr << "QDPCloverTerm: error: fbc is null" << endl;
      QDP_abort(1);
    }
    
    {
      Real ff = where(param.anisoParam.anisoP, Real(1) / param.anisoParam.xi_0, Real(1));
      param.clovCoeffR *= Real(0.5) * ff;
      param.clovCoeffT *= Real(0.5);
    }
    
    //
    // Yuk. Some bits of knowledge of the dslash term are buried in the 
    // effective mass term. They show up here. If I wanted some more 
    // complicated dslash then this will have to be fixed/adjusted.
    //
    Real diag_mass;
    {
      Real ff = where(param.anisoParam.anisoP, param.anisoParam.nu / param.anisoParam.xi_0, Real(1));
      diag_mass = 1 + (Nd-1)*ff + param.Mass;
    }
    
    
    /* Calculate F(mu,nu) */
    //multi1d<LatticeColorMatrix> f;
    //mesField(f, u);
    //makeClov(f, diag_mass);
    
    choles_done.resize(rb.numSubsets());
    for(int i=0; i < rb.numSubsets(); i++) {
      choles_done[i] = from.choles_done[i];
    }
    
    tr_log_diag_ = from.tr_log_diag_;
    
    tri = from.tri;
    END_CODE();  
  }


  //! Creation routine
  void QDPCloverTerm::create(Handle< FermState<T,P,Q> > fs,
			     const CloverFermActParams& param_)
  {
    START_CODE();

    u = fs->getLinks();
    fbc = fs->getFermBC();
    param = param_;

    // Sanity check
    if (fbc.operator->() == 0)
    {
      QDPIO::cerr << "QDPCloverTerm: error: fbc is null" << endl;
      QDP_abort(1);
    }

    {
      Real ff = where(param.anisoParam.anisoP, Real(1) / param.anisoParam.xi_0, Real(1));
      param.clovCoeffR *= Real(0.5) * ff;
      param.clovCoeffT *= Real(0.5);
    }

    //
    // Yuk. Some bits of knowledge of the dslash term are buried in the 
    // effective mass term. They show up here. If I wanted some more 
    // complicated dslash then this will have to be fixed/adjusted.
    //
    Real diag_mass;
    {
      Real ff = where(param.anisoParam.anisoP, param.anisoParam.nu / param.anisoParam.xi_0, Real(1));
      diag_mass = 1 + (Nd-1)*ff + param.Mass;
    }

    /* Calculate F(mu,nu) */
    multi1d<LatticeColorMatrix> f;
    mesField(f, u);
    makeClov(f, diag_mass);
    
    choles_done.resize(rb.numSubsets());
    for(int i=0; i < rb.numSubsets(); i++) {
      choles_done[i] = false;
    }


    END_CODE();
  }


  /*
   * MAKCLOV 
   *
   *  In this routine, MAKCLOV calculates

   *    1 - (1/4)*sigma(mu,nu) F(mu,nu)

   *  using F from mesfield

   *    F(mu,nu) =  (1/4) sum_p (1/2) [ U_p(x) - U^dag_p(x) ]

   *  using basis of SPPROD and stores in a lower triangular matrix
   *  (no diagonal) plus real diagonal

   *  where
   *    U_1 = u(x,mu)*u(x+mu,nu)*u_dag(x+nu,mu)*u_dag(x,nu)
   *    U_2 = u(x,nu)*u_dag(x-mu+nu,mu)*u_dag(x-mu,nu)*u(x-mu,mu)
   *    U_3 = u_dag(x-mu,mu)*u_dag(x-mu-nu,nu)*u(x-mu-nu,mu)*u(x-nu,nu)
   *    U_4 = u_dag(x-nu,nu)*u(x-nu,mu)*u(x-nu+mu,nu)*u_dag(x,mu)

   *  and

   *         | sigF(1)   sigF(3)     0         0     |
   *  sigF = | sigF(5)  -sigF(1)     0         0     |
   *         |   0         0      -sigF(0)  -sigF(2) |
   *         |   0         0      -sigF(4)   sigF(0) |
   *  where
   *    sigF(i) is a color matrix

   *  sigF(0) = i*(ClovT*E_z + ClovR*B_z)
   *          = i*(ClovT*F(3,2) + ClovR*F(1,0))
   *  sigF(1) = i*(ClovT*E_z - ClovR*B_z)
   *          = i*(ClovT*F(3,2) - ClovR*F(1,0))
   *  sigF(2) = i*(E_+ + B_+)
   *  sigF(3) = i*(E_+ - B_+)
   *  sigF(4) = i*(E_- + B_-)
   *  sigF(5) = i*(E_- - B_-)
   *  i*E_+ = (i*ClovT*E_x - ClovT*E_y)
   *        = (i*ClovT*F(3,0) - ClovT*F(3,1))
   *  i*E_- = (i*ClovT*E_x + ClovT*E_y)
   *        = (i*ClovT*F(3,0) + ClovT*F(3,1))
   *  i*B_+ = (i*ClovR*B_x - ClovR*B_y)
   *        = (i*ClovR*F(2,1) + ClovR*F(2,0))
   *  i*B_- = (i*ClovR*B_x + ClovR*B_y)
   *        = (i*ClovR*F(2,1) - ClovR*F(2,0))

   *  NOTE: I am using  i*F  of the usual F defined by UKQCD, Heatlie et.al.

   *  NOTE: the above definitions assume that the time direction, t_dir,
   *        is 3. In general F(k,j) is multiplied with ClovT if either
   *        k=t_dir or j=t_dir, and with ClovR otherwise.

   *+++
   *  Here are some notes on the origin of this routine. NOTE, ClovCoeff or u0
   *  are not actually used in MAKCLOV.
   *
   *  The clover mass term is suppose to act on a vector like
   *
   *  chi = (1 - (ClovCoeff/u0^3) * kappa/4 * sum_mu sum_nu F(mu,nu)*sigma(mu,nu)) * psi

   *  Definitions used here (NOTE: no "i")
   *   sigma(mu,nu) = gamma(mu)*gamma(nu) - gamma(nu)*gamma(mu)
   *                = 2*gamma(mu)*gamma(nu)   for mu != nu
   *       
   *   chi = sum_mu sum_nu F(mu,nu)*gamma(mu)*gamma(nu)*psi   for mu < nu
   *       = (1/2) * sum_mu sum_nu F(mu,nu)*gamma(mu)*gamma(nu)*psi   for mu != nu
   *       = (1/4) * sum_mu sum_nu F(mu,nu)*sigma(mu,nu)*psi
   *
   *
   * chi = (1 - (ClovCoeff/u0^3) * kappa/4 * sum_mu sum_nu F(mu,nu)*sigma(mu,nu)) * psi
   *     = psi - (ClovCoeff/u0^3) * kappa * chi
   *     == psi - kappa * chi
   *
   *  We have absorbed ClovCoeff/u0^3 into kappa. A u0 was previously absorbed into kappa
   *  for compatibility to ancient conventions. 
   *---

   * Arguments:
   *  \param f         field strength tensor F(cb,mu,nu)        (Read)
   *  \param diag_mass effective mass term                      (Read)
   */
  void QDPCloverTerm::makeClov(const multi1d<LatticeColorMatrix>& f, const Real& diag_mass)
  {
    START_CODE();

    if ( Nd != 4 )
    {
      QDPIO::cerr << __func__ << ": expecting Nd==4" << endl;
      QDP_abort(1);
    }
  
    if ( Ns != 4 )
    {
      QDPIO::cerr << __func__ << ": expecting Ns==4" << endl;
      QDP_abort(1);
    }
  
    LatticeColorMatrix f0 = f[0] * getCloverCoeff(0,1);
    LatticeColorMatrix f1 = f[1] * getCloverCoeff(0,2);
    LatticeColorMatrix f2 = f[2] * getCloverCoeff(0,3);
    LatticeColorMatrix f3 = f[3] * getCloverCoeff(1,2);
    LatticeColorMatrix f4 = f[4] * getCloverCoeff(1,3);
    LatticeColorMatrix f5 = f[5] * getCloverCoeff(2,3);    

    /* Multiply in the appropriate clover coefficient */
    /*
      switch (param.anisoParam.t_dir)
      {
      case 1:
      f0 = f[0] * param.clovCoeffT;
      f1 = f[1] * param.clovCoeffR;
      f2 = f[2] * param.clovCoeffR;
      f3 = f[3] * param.clovCoeffT;
      f4 = f[4] * param.clovCoeffT;
      f5 = f[5] * param.clovCoeffR;
      break;

      case 2:
      f0 = f[0] * param.clovCoeffR;
      f1 = f[1] * param.clovCoeffT;
      f2 = f[2] * param.clovCoeffR;
      f3 = f[3] * param.clovCoeffT;
      f4 = f[4] * param.clovCoeffR;
      f5 = f[5] * param.clovCoeffT;
      break;

      case 3:
      f0 = f[0] * param.clovCoeffR;
      f1 = f[1] * param.clovCoeffR;
      f2 = f[2] * param.clovCoeffT;
      f3 = f[3] * param.clovCoeffR;
      f4 = f[4] * param.clovCoeffT;
      f5 = f[5] * param.clovCoeffT;
      break;

      default:
      QDPIO::cerr << __func__ << ": invalid time direction: t_dir= " 
      << param.anisoParam.t_dir << endl;
      QDP_abort(1);
      }
    */

    const int nodeSites = QDP::Layout::sitesOnNode();

    tri.resize(nodeSites);  // hold local lattice
    
    /*# Construct diagonal */
    for(int site = 0; site < nodeSites; ++site)
    {

      for(int jj = 0; jj < 2; jj++)
      {
	for(int ii = 0; ii < 2*Nc; ii++)
	{
	  tri[site].diag[jj][ii] = diag_mass.elem().elem().elem();
	}
      }
    }



    /* The appropriate clover coeffients are already included in the
       field strengths F(mu,nu)! */
    for(int site = 0; site < nodeSites; ++site)
    {
      RComplex<REAL> E_minus;
      RComplex<REAL> B_minus;
      RComplex<REAL> ctmp_0;
      RComplex<REAL> ctmp_1;
      RScalar<REAL> rtmp_0;
      RScalar<REAL> rtmp_1;
  
      for(int i = 0; i < Nc; ++i)
      {
	/*# diag_L(i,0) = 1 - i*diag(E_z - B_z) */
	/*#             = 1 - i*diag(F(3,2) - F(1,0)) */
	ctmp_0 = f5.elem(site).elem().elem(i,i);
	ctmp_0 -= f0.elem(site).elem().elem(i,i);
	rtmp_0 = imag(ctmp_0);
	tri[site].diag[0][i] += rtmp_0;

	/*# diag_L(i+Nc,0) = 1 + i*diag(E_z - B_z) */
	/*#                = 1 + i*diag(F(3,2) - F(1,0)) */
	tri[site].diag[0][i+Nc] -= rtmp_0;

	/*# diag_L(i,1) = 1 + i*diag(E_z + B_z) */
	/*#             = 1 + i*diag(F(3,2) + F(1,0)) */
	ctmp_1 = f5.elem(site).elem().elem(i,i);
	ctmp_1 += f0.elem(site).elem().elem(i,i);
	rtmp_1 = imag(ctmp_1);
	tri[site].diag[1][i] -= rtmp_1;

	/*# diag_L(i+Nc,1) = 1 - i*diag(E_z + B_z) */
	/*#                = 1 - i*diag(F(3,2) + F(1,0)) */
	tri[site].diag[1][i+Nc] += rtmp_1;
      }

      /*# Construct lower triangular portion */
      /*# Block diagonal terms */
      for(int i = 1; i < Nc; ++i)
      {
	for(int j = 0; j < i; ++j)
	{
	  int elem_ij  = i*(i-1)/2 + j;
	  int elem_tmp = (i+Nc)*(i+Nc-1)/2 + j+Nc;

	  /*# L(i,j,0) = -i*(E_z - B_z)[i,j] */
	  /*#          = -i*(F(3,2) - F(1,0)) */
	  ctmp_0 = f0.elem(site).elem().elem(i,j);
	  ctmp_0 -= f5.elem(site).elem().elem(i,j);
	  tri[site].offd[0][elem_ij] = timesI(ctmp_0);

	  /*# L(i+Nc,j+Nc,0) = +i*(E_z - B_z)[i,j] */
	  /*#                = +i*(F(3,2) - F(1,0)) */
	  tri[site].offd[0][elem_tmp] = -tri[site].offd[0][elem_ij];

	  /*# L(i,j,1) = i*(E_z + B_z)[i,j] */
	  /*#          = i*(F(3,2) + F(1,0)) */
	  ctmp_1 = f5.elem(site).elem().elem(i,j);
	  ctmp_1 += f0.elem(site).elem().elem(i,j);
	  tri[site].offd[1][elem_ij] = timesI(ctmp_1);

	  /*# L(i+Nc,j+Nc,1) = -i*(E_z + B_z)[i,j] */
	  /*#                = -i*(F(3,2) + F(1,0)) */
	  tri[site].offd[1][elem_tmp] = -tri[site].offd[1][elem_ij];
	}
      }
      
      /*# Off-diagonal */
      for(int i = 0; i < Nc; ++i)
      {
	for(int j = 0; j < Nc; ++j)
	{
	  // Flipped index
	  // by swapping i <-> j. In the past i would run slow
	  // and now j runs slow
	  int elem_ij  = (i+Nc)*(i+Nc-1)/2 + j;

	  /*# i*E_- = (i*E_x + E_y) */
	  /*#       = (i*F(3,0) + F(3,1)) */
	  E_minus = timesI(f2.elem(site).elem().elem(i,j));
	  E_minus += f4.elem(site).elem().elem(i,j);

	  /*# i*B_- = (i*B_x + B_y) */
	  /*#       = (i*F(2,1) - F(2,0)) */
	  B_minus = timesI(f3.elem(site).elem().elem(i,j));
	  B_minus -= f1.elem(site).elem().elem(i,j);

	  /*# L(i+Nc,j,0) = -i*(E_- - B_-)  */
	  tri[site].offd[0][elem_ij] = B_minus - E_minus;

	  /*# L(i+Nc,j,1) = +i*(E_- + B_-)  */
	  tri[site].offd[1][elem_ij] = E_minus + B_minus;
	}
      }
	
      
    }
              
#if 0
    {
      XMLFileWriter xml("f.xml");
      push(xml,"f");
      write(xml, "clovT", param.clovCoeffT);
      write(xml, "clovR", param.clovCoeffR);
      write(xml,"f",f);
      pop(xml);
    }
    {
      XMLFileWriter xml("makeclov.xml");
      push(xml,"makeclov");
      write(xml,"tri",tri);
      pop(xml);
    }
#endif

    END_CODE();
  }
  

  //! Invert
  /*!
   * Computes the inverse of the term on cb using Cholesky
   */
  void QDPCloverTerm::choles(int cb)
  {
    START_CODE();

    // When you are doing the cholesky - also fill out the trace_log_diag piece)
    // chlclovms(tr_log_diag_, cb);
    // Switch to LDL^\dag inversion
    ldagdlinv(tr_log_diag_,cb);
    END_CODE();
  }


  //! Invert
  /*!
   * Computes the inverse of the term on cb using Cholesky
   *
   * \return logarithm of the determinant  
   */
  Double QDPCloverTerm::cholesDet(int cb) const
  {
    START_CODE();

    if( choles_done[cb] == false ) 
    {
      QDPIO::cout << __func__ << ": Error: you have not done the Cholesky.on this operator on this subset" << endl;
      QDPIO::cout << "You sure you should not be asking invclov?" << endl;
      QDP_abort(1);
    }

    END_CODE();

    return sum(tr_log_diag_, rb[cb]);
  }



  /*! An LDL^\dag decomposition and inversion? */
  void QDPCloverTerm::ldagdlinv(LatticeReal& tr_log_diag, int cb)
  {
    START_CODE();

    if ( 2*Nc < 3 )
    {
      QDPIO::cerr << __func__ << ": Matrix is too small" << endl;
      QDP_abort(1);
    }

    // Zero trace log
    tr_log_diag = zero;
    RScalar<REAL> zip=0;

    int N = 2*Nc;

    // Loop through the sites.
    for(int ssite=0; ssite < rb[cb].numSiteTable(); ++ssite) 
    {
      int site = rb[cb].siteTable()[ssite];

      int site_neg_logdet=0;
      // Loop through the blocks on the site.
      for(int block=0; block < 2; block++) { 

	// Triangular storage 
	RScalar<REAL> inv_d[6] QDP_ALIGN16;
	RComplex<REAL> inv_offd[15] QDP_ALIGN16;
	RComplex<REAL> v[6] QDP_ALIGN16;
	RScalar<REAL>  diag_g[6] QDP_ALIGN16;
	// Algorithm 4.1.2 LDL^\dagger Decomposition
	// From Golub, van Loan 3rd ed, page 139
	for(int i=0; i < N; i++) { 
	  inv_d[i] = tri[site].diag[block][i];
	}

	for(int i=0; i < 15; i++) { 
	  inv_offd[i]  =tri[site].offd[block][i];
	}

	for(int j=0; j < N; ++j) { 

	  // Compute v(0:j-1)
	  //
	  // for i=0:j-2
          //   v(i) = A(j,i) A(i,i)
	  // end


	  for(int i=0; i < j; i++) { 
	    int elem_ji = j*(j-1)/2 + i;

	    RComplex<REAL> A_ii = cmplx( inv_d[i], zip );
	    v[i] = A_ii*adj(inv_offd[elem_ji]);
	  }
	  
	  // v(j) = A(j,j) - A(j, 0:j-2) v(0:j-2)
	  //                 ^ This is done with a loop over k ie:
	  //
	  // v(j) = A(j,j) - sum_k A*(j,k) v(k)     k=0...j-2
	  //
	  //      = A(j,j) - sum_k A*(j,k) A(j,k) A(k,k)
	  //      = A(j,j) - sum_k | A(j,k) |^2 A(k,k)
 
	  v[j] = cmplx(inv_d[j],zip);

	  for(int k=0; k < j; k++) { 
	    int elem_jk = j*(j-1)/2 + k;
	    v[j] -= inv_offd[elem_jk]*v[k];
	  }
	  

	  // At this point in time v[j] has to be real, since
	  // A(j,j) is from diag ie real and all | A(j,k) |^2 is real
	  // as is A(k,k)
	  
	  // A(j,j) is the diagonal element - so store it.
	  inv_d[j] = real( v[j] );

	  // Last line of algorithm:
	  // A( j+1 : n, j) = ( A(j+1:n, j) - A(j+1:n, 1:j-1)v(1:k-1) ) / v(j)
	  //
	  // use k as first colon notation and l as second so
	  // 
	  // for k=j+1 < n-1
	  //      A(k,j) = A(k,j) ;
	  //      for l=0 < j-1
	  //         A(k,j) -= A(k, l) v(l)
	  //      end
	  //      A(k,j) /= v(j);
	  //
	  for(int k=j+1; k < N; k++) { 
	    int elem_kj = k*(k-1)/2 + j;
	    for(int l=0; l < j; l++) { 
	      int elem_kl = k*(k-1)/2 + l;
	      inv_offd[elem_kj] -= inv_offd[elem_kl] * v[l];
	    }
	    inv_offd[elem_kj] /= v[j];
	  }
	}

	// Now fix up the inverse
	RScalar<REAL> one;
	one.elem() = (REAL)1;

	for(int i=0; i < N; i++) { 
	  diag_g[i] = one/inv_d[i];
	
	  // Compute the trace log
	  // NB we are always doing trace log | A | 
	  // (because we are always working with actually A^\dagger A
	  //  even in one flavour case where we square root)
	  tr_log_diag.elem(site).elem().elem().elem() += log(fabs(inv_d[i].elem()));
	  // However, it is worth counting just the no of negative logdets
	  // on site
	  if( inv_d[i].elem() < 0 ) { 
	    site_neg_logdet++;
	  }
	}
	// Now we need to invert the L D L^\dagger 
	// We can do this by solving:
	//
	//  L D L^\dagger M^{-1} = 1   
	//
	// This can be done by solving L D X = 1  (X = L^\dagger M^{-1})
	//
	// Then solving L^\dagger M^{-1} = X
	//
	// LD is lower diagonal and so X will also be lower diagonal.
	// LD X = 1 can be solved by forward substitution.
	//
	// Likewise L^\dagger is strictly upper triagonal and so
	// L^\dagger M^{-1} = X can be solved by forward substitution.
	RComplex<REAL> sum;
	for(int k = 0; k < N; ++k)
	{
	  for(int i = 0; i < k; ++i) {
	    zero_rep(v[i]);
	  }
	  
	  /*# Forward substitution */

	  // The first element is the inverse of the diagonal
	  v[k] = cmplx(diag_g[k],zip);
      
	  for(int i = k+1; i < N; ++i) {
	    zero_rep(v[i]);

	    for(int j = k; j < i; ++j) {
	      int elem_ij = i*(i-1)/2+j;	

	      // subtract l_ij*d_j*x_{kj}
	      v[i] -= inv_offd[elem_ij] *inv_d[j]*v[j];

	    }
	
	    // scale out by 1/d_i
	    v[i] *= diag_g[i];
	  }
      
	  /*# Backward substitution */
	  // V[N-1] remains unchanged
	  // Start from V[N-2]

	  for(int i = N-2; (int)i >= (int)k; --i) {
	    for(int j = i+1; j < N; ++j) {
	      int elem_ji = j*(j-1)/2 + i;
	      // Subtract terms of typ (l_ji)*x_kj
	      v[i] -= adj(inv_offd[elem_ji]) * v[j];
	    }
	  }

	  /*# Overwrite column k of invcl.offd */
	  inv_d[k] = real(v[k]);
	  for(int i = k+1; i < N; ++i)
	  {
	    int elem_ik = i*(i-1)/2+k;
	    inv_offd[elem_ik] = v[i];
	  }
	}


	// Overwrite original data
	for(int i=0; i < N; i++) { 
	  tri[site].diag[block][i] = inv_d[i];
	}
	for(int i=0; i < 15; i++) { 
	  tri[site].offd[block][i] = inv_offd[i];
	}
      }

      if( site_neg_logdet != 0 ) { 
	// Report if site has any negative terms. (-ve def)
	std::cout << "WARNING: found " << site_neg_logdet
		  << " negative terms in Clover DET at site: " << site << endl;
      }
    }
    
    // This comes from the days when we used to do Cholesky
    choles_done[cb] = true;
    END_CODE();
  }
 
  /*! CHLCLOVMS - Cholesky decompose the clover mass term and uses it to
   *              compute  lower(A^-1) = lower((L.L^dag)^-1)
   *              Adapted from Golub and Van Loan, Matrix Computations, 2nd, Sec 4.2.4
   *
   * Arguments:
   *
   * \param DetP         flag whether to compute determinant (Read)
   * \param logdet       logarithm of the determinant        (Write)
   * \param cb           checkerboard of work                (Read)
   */
  void QDPCloverTerm::chlclovms(LatticeReal& tr_log_diag, int cb)
  {
    START_CODE();

    if ( 2*Nc < 3 )
    {
      QDPIO::cerr << __func__ << ": Matrix is too small" << endl;
      QDP_abort(1);
    }
  
    tr_log_diag = zero;
  
    int n = 2*Nc;

    /*# Cholesky decompose  A = L.L^dag */
    /*# NOTE!!: I can store this matrix in  invclov, but will need a */
    /*#   temporary  diag */
    for(int ssite=0; ssite < rb[cb].numSiteTable(); ++ssite) 
    {
      int site = rb[cb].siteTable()[ssite];

      PrimitiveClovTriang  invcl;

      multi1d< RScalar<REAL> > diag_g(n);
      multi1d< RComplex<REAL> > v1(n);
      RComplex<REAL> sum;
      RScalar<REAL> one;
      RScalar<REAL> zero;
      RScalar<REAL> lrtmp;

      one = 1;
      zero = 0;
  
      for(int s = 0; s < 2; ++s)
      {
	int elem_jk = 0;
	int elem_ij;

	for(int j = 0; j <  n; ++j)
	{
	  /*# Multiply clover mass term against basis vector.  */
	  /*# Actually, I need a column of the lower triang matrix clov. */
	  v1[j] = cmplx(tri[site].diag[s][j],zero);
    
	  elem_ij = elem_jk + 2*j;
      
	  for(int i = j+1; i < n; ++i)
	  {
	    v1[i] = tri[site].offd[s][elem_ij];
	    elem_ij += i;
	  }
      
	  /*# Back to cholesky */
	  /*# forward substitute */
	  for(int k = 0; k < j; ++k)
	  {
	    int elem_ik = elem_jk;
	
	    for(int i = j; i < n; ++i)
	    {
	      v1[i] -= adj(invcl.offd[s][elem_jk]) * invcl.offd[s][elem_ik];
	      elem_ik += i;
	    }
	    elem_jk++;
	  }

	  /*# The diagonal is (should be!!) real and positive */
	  diag_g[j] = real(v1[j]);
	  
	  /*#+ */
	  /*# Squeeze in computation of the trace log of the diagonal term */
	  /*#- */
	  if ( diag_g[j].elem() > 0 ) 
          { 
	    lrtmp = log(diag_g[j]);
	  }
	  else
          { 
            // Make sure any node can print this message
	    cerr << "Clover term has negative diagonal element: "
	         << "diag_g[" << j << "]= " << diag_g[j] 
	         << " at site: " << site << endl;
	    QDP_abort(1);
	  }

	  tr_log_diag.elem(site).elem().elem() += lrtmp;
	  
	  diag_g[j] = sqrt(diag_g[j]);
	  diag_g[j] = one / diag_g[j];
      
	  /*# backward substitute */
	  elem_ij = elem_jk + j;
	  for(int i = j+1; i < n; ++i)
	  {
	    invcl.offd[s][elem_ij] = v1[i] * diag_g[j];
	    elem_ij += i;
	  }
	}

	/*# Use forward and back substitution to construct  invcl.offd = lower(A^-1) */
	for(int k = 0; k < n; ++k)
	{
	  for(int i = 0; i < k; ++i)
	    zero_rep(v1[i]);
	  
	  /*# Forward substitution */
	  v1[k] = cmplx(diag_g[k],zero);
      
	  for(int i = k+1; i < n; ++i)
	  {
	    zero_rep(sum);
	    elem_ij = i*(i-1)/2+k;	
	    for(int j = k; j < i; ++j)
	    {
	      sum -= invcl.offd[s][elem_ij] * v1[j];
	      elem_ij++;
	    }
	
	    v1[i] = sum * diag_g[i];
	  }
      
	  /*# Backward substitution */
	  v1[n-1] = v1[n-1] * diag_g[n-1];
     
	  for(int i = n-2; (int)i >= (int)k; --i)
	  {
	    sum = v1[i];
	    
	    int elem_ji = ((i+1)*i)/2+i;
	    for(int j = i+1; j < n; ++j)
	    {
	      sum -= adj(invcl.offd[s][elem_ji]) * v1[j];
	      elem_ji += j;
	    }
	    v1[i] = sum * diag_g[i];
	  }

	  /*# Overwrite column k of invcl.offd */
	  invcl.diag[s][k] = real(v1[k]);

	  int elem_ik = ((k+1)*k)/2+k;
      
	  for(int i = k+1; i < n; ++i)
	  {
	    invcl.offd[s][elem_ik] = v1[i];
	    elem_ik += i;
	  }
	}
      }

      // Overwrite original element
      tri[site] = invcl;
    }

    choles_done[cb] = true;
    END_CODE();
  }


  /**
   * Apply a dslash
   *
   * Performs the operation
   *
   *  chi <-   (L + D + L^dag) . psi
   *
   * where
   *   L       is a lower triangular matrix
   *   D       is the real diagonal. (stored together in type TRIANG)
   *
   * Arguments:
   * \param chi     result                                      (Write)
   * \param psi     source                                      (Read)
   * \param isign   D'^dag or D'  ( MINUS | PLUS ) resp.        (Read)
   * \param cb      Checkerboard of OUTPUT vector               (Read) 
   */
  void QDPCloverTerm::apply(LatticeFermion& chi, const LatticeFermion& psi, 
			    enum PlusMinus isign, int cb) const
  {
    START_CODE();

    if ( Ns != 4 )
    {
      QDPIO::cerr << __func__ << ": CloverTerm::apply requires Ns==4" << endl;
      QDP_abort(1);
    }

    int n = 2*Nc;

    const multi1d<int>& tab = rb[cb].siteTable();
    for(int ssite=0; ssite < tab.size(); ++ssite) 
    {
      int site = tab[ssite];

      RComplex<REAL>* cchi = (RComplex<REAL>*)&(chi.elem(site).elem(0).elem(0));
      const RComplex<REAL>* ppsi = (const RComplex<REAL>*)&(psi.elem(site).elem(0).elem(0));


#if 0
#warning "Using unrolled clover term"
      // Rolled version
      for(int i = 0; i < n; ++i)
      {
	cchi[0*n+i] = tri[site].diag[0][i] * ppsi[0*n+i];
	cchi[1*n+i] = tri[site].diag[1][i] * ppsi[1*n+i];
      }

      int kij = 0;  
      for(int i = 0; i < n; ++i)
      {
	for(int j = 0; j < i; j++)
	{
	  cchi[0*n+i] += tri[site].offd[0][kij] * ppsi[0*n+j];
	  cchi[0*n+j] += conj(tri[site].offd[0][kij]) * ppsi[0*n+i];
	  cchi[1*n+i] += tri[site].offd[1][kij] * ppsi[1*n+j];
	  cchi[1*n+j] += conj(tri[site].offd[1][kij]) * ppsi[1*n+i];
	  kij++;
	}
      }
#elif 0
#warning "Using unrolled clover term - version 1"
      // Unrolled version - basically copying the loop structure
      cchi[ 0] = tri[site].diag[0][0] * ppsi[ 0];
      cchi[ 1] = tri[site].diag[0][1] * ppsi[ 1];
      cchi[ 2] = tri[site].diag[0][2] * ppsi[ 2];
      cchi[ 3] = tri[site].diag[0][3] * ppsi[ 3];
      cchi[ 4] = tri[site].diag[0][4] * ppsi[ 4];
      cchi[ 5] = tri[site].diag[0][5] * ppsi[ 5];
      cchi[ 6] = tri[site].diag[1][0] * ppsi[ 6];
      cchi[ 7] = tri[site].diag[1][1] * ppsi[ 7];
      cchi[ 8] = tri[site].diag[1][2] * ppsi[ 8];
      cchi[ 9] = tri[site].diag[1][3] * ppsi[ 9];
      cchi[10] = tri[site].diag[1][4] * ppsi[10];
      cchi[11] = tri[site].diag[1][5] * ppsi[11];

      // cchi[0*n+i] += tri[site].offd[0][kij] * ppsi[0*n+j];
      cchi[ 1] += tri[site].offd[0][ 0] * ppsi[ 0];
      cchi[ 2] += tri[site].offd[0][ 1] * ppsi[ 0];
      cchi[ 2] += tri[site].offd[0][ 2] * ppsi[ 1];
      cchi[ 3] += tri[site].offd[0][ 3] * ppsi[ 0];
      cchi[ 3] += tri[site].offd[0][ 4] * ppsi[ 1];
      cchi[ 3] += tri[site].offd[0][ 5] * ppsi[ 2];
      cchi[ 4] += tri[site].offd[0][ 6] * ppsi[ 0];
      cchi[ 4] += tri[site].offd[0][ 7] * ppsi[ 1];
      cchi[ 4] += tri[site].offd[0][ 8] * ppsi[ 2];
      cchi[ 4] += tri[site].offd[0][ 9] * ppsi[ 3];
      cchi[ 5] += tri[site].offd[0][10] * ppsi[ 0];
      cchi[ 5] += tri[site].offd[0][11] * ppsi[ 1];
      cchi[ 5] += tri[site].offd[0][12] * ppsi[ 2];
      cchi[ 5] += tri[site].offd[0][13] * ppsi[ 3];
      cchi[ 5] += tri[site].offd[0][14] * ppsi[ 4];

      // cchi[0*n+j] += conj(tri[site].offd[0][kij]) * ppsi[0*n+i];
      cchi[ 0] += conj(tri[site].offd[0][ 0]) * ppsi[ 1];
      cchi[ 0] += conj(tri[site].offd[0][ 1]) * ppsi[ 2];
      cchi[ 1] += conj(tri[site].offd[0][ 2]) * ppsi[ 2];
      cchi[ 0] += conj(tri[site].offd[0][ 3]) * ppsi[ 3];
      cchi[ 1] += conj(tri[site].offd[0][ 4]) * ppsi[ 3];
      cchi[ 2] += conj(tri[site].offd[0][ 5]) * ppsi[ 3];
      cchi[ 0] += conj(tri[site].offd[0][ 6]) * ppsi[ 4];
      cchi[ 1] += conj(tri[site].offd[0][ 7]) * ppsi[ 4];
      cchi[ 2] += conj(tri[site].offd[0][ 8]) * ppsi[ 4];
      cchi[ 3] += conj(tri[site].offd[0][ 9]) * ppsi[ 4];
      cchi[ 0] += conj(tri[site].offd[0][10]) * ppsi[ 5];
      cchi[ 1] += conj(tri[site].offd[0][11]) * ppsi[ 5];
      cchi[ 2] += conj(tri[site].offd[0][12]) * ppsi[ 5];
      cchi[ 3] += conj(tri[site].offd[0][13]) * ppsi[ 5];
      cchi[ 4] += conj(tri[site].offd[0][14]) * ppsi[ 5];

      // cchi[1*n+i] += tri[site].offd[1][kij] * ppsi[1*n+j];
      cchi[ 7] += tri[site].offd[1][ 0] * ppsi[ 6];
      cchi[ 8] += tri[site].offd[1][ 1] * ppsi[ 6];
      cchi[ 8] += tri[site].offd[1][ 2] * ppsi[ 7];
      cchi[ 9] += tri[site].offd[1][ 3] * ppsi[ 6];
      cchi[ 9] += tri[site].offd[1][ 4] * ppsi[ 7];
      cchi[ 9] += tri[site].offd[1][ 5] * ppsi[ 8];
      cchi[10] += tri[site].offd[1][ 6] * ppsi[ 6];
      cchi[10] += tri[site].offd[1][ 7] * ppsi[ 7];
      cchi[10] += tri[site].offd[1][ 8] * ppsi[ 8];
      cchi[10] += tri[site].offd[1][ 9] * ppsi[ 9];
      cchi[11] += tri[site].offd[1][10] * ppsi[ 6];
      cchi[11] += tri[site].offd[1][11] * ppsi[ 7];
      cchi[11] += tri[site].offd[1][12] * ppsi[ 8];
      cchi[11] += tri[site].offd[1][13] * ppsi[ 9];
      cchi[11] += tri[site].offd[1][14] * ppsi[10];

      // cchi[1*n+j] += conj(tri[site].offd[1][kij]) * ppsi[1*n+i];
      cchi[ 6] += conj(tri[site].offd[1][ 0]) * ppsi[ 7];
      cchi[ 6] += conj(tri[site].offd[1][ 1]) * ppsi[ 8];
      cchi[ 7] += conj(tri[site].offd[1][ 2]) * ppsi[ 8];
      cchi[ 6] += conj(tri[site].offd[1][ 3]) * ppsi[ 9];
      cchi[ 7] += conj(tri[site].offd[1][ 4]) * ppsi[ 9];
      cchi[ 8] += conj(tri[site].offd[1][ 5]) * ppsi[ 9];
      cchi[ 6] += conj(tri[site].offd[1][ 6]) * ppsi[10];
      cchi[ 7] += conj(tri[site].offd[1][ 7]) * ppsi[10];
      cchi[ 8] += conj(tri[site].offd[1][ 8]) * ppsi[10];
      cchi[ 9] += conj(tri[site].offd[1][ 9]) * ppsi[10];
      cchi[ 6] += conj(tri[site].offd[1][10]) * ppsi[11];
      cchi[ 7] += conj(tri[site].offd[1][11]) * ppsi[11];
      cchi[ 8] += conj(tri[site].offd[1][12]) * ppsi[11];
      cchi[ 9] += conj(tri[site].offd[1][13]) * ppsi[11];
      cchi[10] += conj(tri[site].offd[1][14]) * ppsi[11];
#elif 0
#warning "Using unrolled clover term - version 2"
      // Unrolled version - collect all LHS terms into 1 expression
      cchi[ 0]  =      tri[site].diag[0][ 0]  * ppsi[ 0];
      cchi[ 0] += conj(tri[site].offd[0][ 0]) * ppsi[ 1];
      cchi[ 0] += conj(tri[site].offd[0][ 1]) * ppsi[ 2];
      cchi[ 0] += conj(tri[site].offd[0][ 3]) * ppsi[ 3];
      cchi[ 0] += conj(tri[site].offd[0][ 6]) * ppsi[ 4];
      cchi[ 0] += conj(tri[site].offd[0][10]) * ppsi[ 5];

      cchi[ 1]  = tri[site].diag[0][ 1]  * ppsi[ 1];
      cchi[ 1] += tri[site].offd[0][ 0]  * ppsi[ 0];
      cchi[ 1] += conj(tri[site].offd[0][ 2]) * ppsi[ 2];
      cchi[ 1] += conj(tri[site].offd[0][ 4]) * ppsi[ 3];
      cchi[ 1] += conj(tri[site].offd[0][ 7]) * ppsi[ 4];
      cchi[ 1] += conj(tri[site].offd[0][11]) * ppsi[ 5];

      cchi[ 2]  = tri[site].diag[0][ 2]  * ppsi[ 2];
      cchi[ 2] += tri[site].offd[0][ 1]  * ppsi[ 0];
      cchi[ 2] += tri[site].offd[0][ 2]  * ppsi[ 1];
      cchi[ 2] += conj(tri[site].offd[0][ 5]) * ppsi[ 3];
      cchi[ 2] += conj(tri[site].offd[0][ 8]) * ppsi[ 4];
      cchi[ 2] += conj(tri[site].offd[0][12]) * ppsi[ 5];

      cchi[ 3]  = tri[site].diag[0][ 3]  * ppsi[ 3];
      cchi[ 3] += tri[site].offd[0][ 3]  * ppsi[ 0];
      cchi[ 3] += tri[site].offd[0][ 4]  * ppsi[ 1];
      cchi[ 3] += tri[site].offd[0][ 5]  * ppsi[ 2];
      cchi[ 3] += conj(tri[site].offd[0][ 9]) * ppsi[ 4];
      cchi[ 3] += conj(tri[site].offd[0][13]) * ppsi[ 5];

      cchi[ 4]  = tri[site].diag[0][ 4]  * ppsi[ 4];
      cchi[ 4] += tri[site].offd[0][ 6]  * ppsi[ 0];
      cchi[ 4] += tri[site].offd[0][ 7]  * ppsi[ 1];
      cchi[ 4] += tri[site].offd[0][ 8]  * ppsi[ 2];
      cchi[ 4] += tri[site].offd[0][ 9]  * ppsi[ 3];
      cchi[ 4] += conj(tri[site].offd[0][14]) * ppsi[ 5];

      cchi[ 5]  = tri[site].diag[0][ 5]  * ppsi[ 5];
      cchi[ 5] += tri[site].offd[0][10]  * ppsi[ 0];
      cchi[ 5] += tri[site].offd[0][11]  * ppsi[ 1];
      cchi[ 5] += tri[site].offd[0][12]  * ppsi[ 2];
      cchi[ 5] += tri[site].offd[0][13]  * ppsi[ 3];
      cchi[ 5] += tri[site].offd[0][14]  * ppsi[ 4];

      cchi[ 6]  = tri[site].diag[1][ 0]  * ppsi[ 6];
      cchi[ 6] += conj(tri[site].offd[1][ 0]) * ppsi[ 7];
      cchi[ 6] += conj(tri[site].offd[1][ 1]) * ppsi[ 8];
      cchi[ 6] += conj(tri[site].offd[1][ 3]) * ppsi[ 9];
      cchi[ 6] += conj(tri[site].offd[1][ 6]) * ppsi[10];
      cchi[ 6] += conj(tri[site].offd[1][10]) * ppsi[11];

      cchi[ 7]  = tri[site].diag[1][ 1]  * ppsi[ 7];
      cchi[ 7] += tri[site].offd[1][ 0]  * ppsi[ 6];
      cchi[ 7] += conj(tri[site].offd[1][ 2]) * ppsi[ 8];
      cchi[ 7] += conj(tri[site].offd[1][ 4]) * ppsi[ 9];
      cchi[ 7] += conj(tri[site].offd[1][ 7]) * ppsi[10];
      cchi[ 7] += conj(tri[site].offd[1][11]) * ppsi[11];

      cchi[ 8]  = tri[site].diag[1][ 2]  * ppsi[ 8];
      cchi[ 8] += tri[site].offd[1][ 1]  * ppsi[ 6];
      cchi[ 8] += tri[site].offd[1][ 2]  * ppsi[ 7];
      cchi[ 8] += conj(tri[site].offd[1][ 5]) * ppsi[ 9];
      cchi[ 8] += conj(tri[site].offd[1][ 8]) * ppsi[10];
      cchi[ 8] += conj(tri[site].offd[1][12]) * ppsi[11];

      cchi[ 9]  = tri[site].diag[1][ 3]  * ppsi[ 9];
      cchi[ 9] += tri[site].offd[1][ 3]  * ppsi[ 6];
      cchi[ 9] += tri[site].offd[1][ 4]  * ppsi[ 7];
      cchi[ 9] += tri[site].offd[1][ 5]  * ppsi[ 8];
      cchi[ 9] += conj(tri[site].offd[1][ 9]) * ppsi[10];
      cchi[ 9] += conj(tri[site].offd[1][13]) * ppsi[11];

      cchi[10]  = tri[site].diag[1][ 4]  * ppsi[10];
      cchi[10] += tri[site].offd[1][ 6]  * ppsi[ 6];
      cchi[10] += tri[site].offd[1][ 7]  * ppsi[ 7];
      cchi[10] += tri[site].offd[1][ 8]  * ppsi[ 8];
      cchi[10] += tri[site].offd[1][ 9]  * ppsi[ 9];
      cchi[10] += conj(tri[site].offd[1][14]) * ppsi[11];

      cchi[11]  = tri[site].diag[1][ 5]  * ppsi[11];
      cchi[11] += tri[site].offd[1][10]  * ppsi[ 6];
      cchi[11] += tri[site].offd[1][11]  * ppsi[ 7];
      cchi[11] += tri[site].offd[1][12]  * ppsi[ 8];
      cchi[11] += tri[site].offd[1][13]  * ppsi[ 9];
      cchi[11] += tri[site].offd[1][14]  * ppsi[10];
#elif 1
#warning "Using unrolled clover term - version 3"
      // Unrolled version 3. 
      // Took unrolled version 2 and wrote out in real() and imag() 
      // parts. Rearranged so that all the reals follow each other
      //  in the output so that we can write linearly


      cchi[ 0].real()  = tri[site].diag[0][0].elem()  * ppsi[0].real();
      cchi[ 0].real() += tri[site].offd[0][0].real()  * ppsi[1].real();
      cchi[ 0].real() += tri[site].offd[0][0].imag()  * ppsi[1].imag();
      cchi[ 0].real() += tri[site].offd[0][1].real()  * ppsi[2].real();
      cchi[ 0].real() += tri[site].offd[0][1].imag()  * ppsi[2].imag();
      cchi[ 0].real() += tri[site].offd[0][3].real()  * ppsi[3].real();
      cchi[ 0].real() += tri[site].offd[0][3].imag()  * ppsi[3].imag();
      cchi[ 0].real() += tri[site].offd[0][6].real()  * ppsi[4].real();
      cchi[ 0].real() += tri[site].offd[0][6].imag()  * ppsi[4].imag();
      cchi[ 0].real() += tri[site].offd[0][10].real() * ppsi[5].real();
      cchi[ 0].real() += tri[site].offd[0][10].imag() * ppsi[5].imag();

      cchi[ 0].imag()  = tri[site].diag[0][0].elem()  * ppsi[ 0].imag();
      cchi[ 0].imag() += tri[site].offd[0][0].real()  * ppsi[1].imag();
      cchi[ 0].imag() -= tri[site].offd[0][0].imag()  * ppsi[1].real();
      cchi[ 0].imag() += tri[site].offd[0][3].real()  * ppsi[3].imag();
      cchi[ 0].imag() -= tri[site].offd[0][3].imag()  * ppsi[3].real();
      cchi[ 0].imag() += tri[site].offd[0][1].real()  * ppsi[2].imag();
      cchi[ 0].imag() -= tri[site].offd[0][1].imag()  * ppsi[2].real();
      cchi[ 0].imag() += tri[site].offd[0][6].real()  * ppsi[4].imag();
      cchi[ 0].imag() -= tri[site].offd[0][6].imag()  * ppsi[4].real();
      cchi[ 0].imag() += tri[site].offd[0][10].real() * ppsi[5].imag();
      cchi[ 0].imag() -= tri[site].offd[0][10].imag() * ppsi[5].real();


      cchi[ 1].real()  = tri[site].diag[0][ 1].elem() * ppsi[ 1].real();
      cchi[ 1].real() += tri[site].offd[0][ 0].real() * ppsi[ 0].real();
      cchi[ 1].real() -= tri[site].offd[0][ 0].imag() * ppsi[ 0].imag();
      cchi[ 1].real() += tri[site].offd[0][ 2].real() * ppsi[ 2].real();
      cchi[ 1].real() += tri[site].offd[0][ 2].imag() * ppsi[ 2].imag();
      cchi[ 1].real() += tri[site].offd[0][ 4].real() * ppsi[ 3].real();
      cchi[ 1].real() += tri[site].offd[0][ 4].imag() * ppsi[ 3].imag();
      cchi[ 1].real() += tri[site].offd[0][ 7].real() * ppsi[ 4].real();
      cchi[ 1].real() += tri[site].offd[0][ 7].imag() * ppsi[ 4].imag();
      cchi[ 1].real() += tri[site].offd[0][11].real() * ppsi[ 5].real();
      cchi[ 1].real() += tri[site].offd[0][11].imag() * ppsi[ 5].imag();


      cchi[ 1].imag()  = tri[site].diag[0][ 1].elem() * ppsi[ 1].imag();
      cchi[ 1].imag() += tri[site].offd[0][ 0].real() * ppsi[ 0].imag();
      cchi[ 1].imag() += tri[site].offd[0][ 0].imag() * ppsi[ 0].real();
      cchi[ 1].imag() += tri[site].offd[0][ 2].real() * ppsi[ 2].imag();
      cchi[ 1].imag() -= tri[site].offd[0][ 2].imag() * ppsi[ 2].real();
      cchi[ 1].imag() += tri[site].offd[0][ 4].real() * ppsi[ 3].imag();
      cchi[ 1].imag() -= tri[site].offd[0][ 4].imag() * ppsi[ 3].real();
      cchi[ 1].imag() += tri[site].offd[0][ 7].real() * ppsi[ 4].imag();
      cchi[ 1].imag() -= tri[site].offd[0][ 7].imag() * ppsi[ 4].real();
      cchi[ 1].imag() += tri[site].offd[0][11].real() * ppsi[ 5].imag();
      cchi[ 1].imag() -= tri[site].offd[0][11].imag() * ppsi[ 5].real();


      cchi[ 2].real() = tri[site].diag[0][ 2].elem()  * ppsi[ 2].real();
      cchi[ 2].real() += tri[site].offd[0][ 1].real() * ppsi[ 0].real();
      cchi[ 2].real() -= tri[site].offd[0][ 1].imag() * ppsi[ 0].imag();
      cchi[ 2].real() += tri[site].offd[0][ 2].real() * ppsi[ 1].real();
      cchi[ 2].real() -= tri[site].offd[0][ 2].imag() * ppsi[ 1].imag();
      cchi[ 2].real() += tri[site].offd[0][5].real()  * ppsi[ 3].real();
      cchi[ 2].real() += tri[site].offd[0][5].imag()  * ppsi[ 3].imag();
      cchi[ 2].real() += tri[site].offd[0][8].real()  * ppsi[ 4].real();
      cchi[ 2].real() += tri[site].offd[0][8].imag()  * ppsi[ 4].imag();
      cchi[ 2].real() += tri[site].offd[0][12].real() * ppsi[ 5].real();
      cchi[ 2].real() += tri[site].offd[0][12].imag() * ppsi[ 5].imag();


      cchi[ 2].imag() = tri[site].diag[0][ 2].elem()  * ppsi[ 2].imag();
      cchi[ 2].imag() += tri[site].offd[0][ 1].real() * ppsi[ 0].imag();
      cchi[ 2].imag() += tri[site].offd[0][ 1].imag() * ppsi[ 0].real();
      cchi[ 2].imag() += tri[site].offd[0][ 2].real() * ppsi[ 1].imag();
      cchi[ 2].imag() += tri[site].offd[0][ 2].imag() * ppsi[ 1].real();
      cchi[ 2].imag() += tri[site].offd[0][5].real()  * ppsi[ 3].imag();
      cchi[ 2].imag() -= tri[site].offd[0][5].imag()  * ppsi[ 3].real();
      cchi[ 2].imag() += tri[site].offd[0][8].real()  * ppsi[ 4].imag();
      cchi[ 2].imag() -= tri[site].offd[0][8].imag()  * ppsi[ 4].real();
      cchi[ 2].imag() += tri[site].offd[0][12].real() * ppsi[ 5].imag();
      cchi[ 2].imag() -= tri[site].offd[0][12].imag() * ppsi[ 5].real();


      cchi[ 3].real()  = tri[site].diag[0][ 3].elem() * ppsi[ 3].real();
      cchi[ 3].real() += tri[site].offd[0][ 3].real() * ppsi[ 0].real();
      cchi[ 3].real() -= tri[site].offd[0][ 3].imag() * ppsi[ 0].imag();
      cchi[ 3].real() += tri[site].offd[0][ 4].real() * ppsi[ 1].real();
      cchi[ 3].real() -= tri[site].offd[0][ 4].imag() * ppsi[ 1].imag();
      cchi[ 3].real() += tri[site].offd[0][ 5].real() * ppsi[ 2].real();
      cchi[ 3].real() -= tri[site].offd[0][ 5].imag() * ppsi[ 2].imag();
      cchi[ 3].real() += tri[site].offd[0][ 9].real() * ppsi[ 4].real();
      cchi[ 3].real() += tri[site].offd[0][ 9].imag() * ppsi[ 4].imag();
      cchi[ 3].real() += tri[site].offd[0][13].real() * ppsi[ 5].real();
      cchi[ 3].real() += tri[site].offd[0][13].imag() * ppsi[ 5].imag();


      cchi[ 3].imag()  = tri[site].diag[0][ 3].elem() * ppsi[ 3].imag();
      cchi[ 3].imag() += tri[site].offd[0][ 3].real() * ppsi[ 0].imag();
      cchi[ 3].imag() += tri[site].offd[0][ 3].imag() * ppsi[ 0].real();
      cchi[ 3].imag() += tri[site].offd[0][ 4].real() * ppsi[ 1].imag();
      cchi[ 3].imag() += tri[site].offd[0][ 4].imag() * ppsi[ 1].real();
      cchi[ 3].imag() += tri[site].offd[0][ 5].real() * ppsi[ 2].imag();
      cchi[ 3].imag() += tri[site].offd[0][ 5].imag() * ppsi[ 2].real();
      cchi[ 3].imag() += tri[site].offd[0][ 9].real() * ppsi[ 4].imag();
      cchi[ 3].imag() -= tri[site].offd[0][ 9].imag() * ppsi[ 4].real();
      cchi[ 3].imag() += tri[site].offd[0][13].real() * ppsi[ 5].imag();
      cchi[ 3].imag() -= tri[site].offd[0][13].imag() * ppsi[ 5].real();


      cchi[ 4].real()  = tri[site].diag[0][ 4].elem() * ppsi[ 4].real();
      cchi[ 4].real() += tri[site].offd[0][ 6].real() * ppsi[ 0].real();
      cchi[ 4].real() -= tri[site].offd[0][ 6].imag() * ppsi[ 0].imag();
      cchi[ 4].real() += tri[site].offd[0][ 7].real() * ppsi[ 1].real();
      cchi[ 4].real() -= tri[site].offd[0][ 7].imag() * ppsi[ 1].imag();
      cchi[ 4].real() += tri[site].offd[0][ 8].real() * ppsi[ 2].real();
      cchi[ 4].real() -= tri[site].offd[0][ 8].imag() * ppsi[ 2].imag();
      cchi[ 4].real() += tri[site].offd[0][ 9].real() * ppsi[ 3].real();
      cchi[ 4].real() -= tri[site].offd[0][ 9].imag() * ppsi[ 3].imag();
      cchi[ 4].real() += tri[site].offd[0][14].real() * ppsi[ 5].real();
      cchi[ 4].real() += tri[site].offd[0][14].imag() * ppsi[ 5].imag();


      cchi[ 4].imag()  = tri[site].diag[0][ 4].elem() * ppsi[ 4].imag();
      cchi[ 4].imag() += tri[site].offd[0][ 6].real() * ppsi[ 0].imag();
      cchi[ 4].imag() += tri[site].offd[0][ 6].imag() * ppsi[ 0].real();
      cchi[ 4].imag() += tri[site].offd[0][ 7].real() * ppsi[ 1].imag();
      cchi[ 4].imag() += tri[site].offd[0][ 7].imag() * ppsi[ 1].real();
      cchi[ 4].imag() += tri[site].offd[0][ 8].real() * ppsi[ 2].imag();
      cchi[ 4].imag() += tri[site].offd[0][ 8].imag() * ppsi[ 2].real();
      cchi[ 4].imag() += tri[site].offd[0][ 9].real() * ppsi[ 3].imag();
      cchi[ 4].imag() += tri[site].offd[0][ 9].imag() * ppsi[ 3].real();
      cchi[ 4].imag() += tri[site].offd[0][14].real() * ppsi[ 5].imag();
      cchi[ 4].imag() -= tri[site].offd[0][14].imag() * ppsi[ 5].real();


      cchi[ 5].real()  = tri[site].diag[0][ 5].elem() * ppsi[ 5].real();
      cchi[ 5].real() += tri[site].offd[0][10].real() * ppsi[ 0].real();
      cchi[ 5].real() -= tri[site].offd[0][10].imag() * ppsi[ 0].imag();
      cchi[ 5].real() += tri[site].offd[0][11].real() * ppsi[ 1].real();
      cchi[ 5].real() -= tri[site].offd[0][11].imag() * ppsi[ 1].imag();
      cchi[ 5].real() += tri[site].offd[0][12].real() * ppsi[ 2].real();
      cchi[ 5].real() -= tri[site].offd[0][12].imag() * ppsi[ 2].imag();
      cchi[ 5].real() += tri[site].offd[0][13].real() * ppsi[ 3].real();
      cchi[ 5].real() -= tri[site].offd[0][13].imag() * ppsi[ 3].imag();
      cchi[ 5].real() += tri[site].offd[0][14].real() * ppsi[ 4].real();
      cchi[ 5].real() -= tri[site].offd[0][14].imag() * ppsi[ 4].imag();


      cchi[ 5].imag()  = tri[site].diag[0][ 5].elem() * ppsi[ 5].imag();
      cchi[ 5].imag() += tri[site].offd[0][10].real() * ppsi[ 0].imag();
      cchi[ 5].imag() += tri[site].offd[0][10].imag() * ppsi[ 0].real();
      cchi[ 5].imag() += tri[site].offd[0][11].real() * ppsi[ 1].imag();
      cchi[ 5].imag() += tri[site].offd[0][11].imag() * ppsi[ 1].real();
      cchi[ 5].imag() += tri[site].offd[0][12].real() * ppsi[ 2].imag();
      cchi[ 5].imag() += tri[site].offd[0][12].imag() * ppsi[ 2].real();
      cchi[ 5].imag() += tri[site].offd[0][13].real() * ppsi[ 3].imag();
      cchi[ 5].imag() += tri[site].offd[0][13].imag() * ppsi[ 3].real();
      cchi[ 5].imag() += tri[site].offd[0][14].real() * ppsi[ 4].imag();
      cchi[ 5].imag() += tri[site].offd[0][14].imag() * ppsi[ 4].real();


      cchi[ 6].real()  = tri[site].diag[1][0].elem()  * ppsi[6].real();
      cchi[ 6].real() += tri[site].offd[1][0].real()  * ppsi[7].real();
      cchi[ 6].real() += tri[site].offd[1][0].imag()  * ppsi[7].imag();
      cchi[ 6].real() += tri[site].offd[1][1].real()  * ppsi[8].real();
      cchi[ 6].real() += tri[site].offd[1][1].imag()  * ppsi[8].imag();
      cchi[ 6].real() += tri[site].offd[1][3].real()  * ppsi[9].real();
      cchi[ 6].real() += tri[site].offd[1][3].imag()  * ppsi[9].imag();
      cchi[ 6].real() += tri[site].offd[1][6].real()  * ppsi[10].real();
      cchi[ 6].real() += tri[site].offd[1][6].imag()  * ppsi[10].imag();
      cchi[ 6].real() += tri[site].offd[1][10].real() * ppsi[11].real();
      cchi[ 6].real() += tri[site].offd[1][10].imag() * ppsi[11].imag();

      cchi[ 6].imag()  = tri[site].diag[1][0].elem()  * ppsi[6].imag();
      cchi[ 6].imag() += tri[site].offd[1][0].real()  * ppsi[7].imag();
      cchi[ 6].imag() -= tri[site].offd[1][0].imag()  * ppsi[7].real();
      cchi[ 6].imag() += tri[site].offd[1][1].real()  * ppsi[8].imag();
      cchi[ 6].imag() -= tri[site].offd[1][1].imag()  * ppsi[8].real();
      cchi[ 6].imag() += tri[site].offd[1][3].real()  * ppsi[9].imag();
      cchi[ 6].imag() -= tri[site].offd[1][3].imag()  * ppsi[9].real();
      cchi[ 6].imag() += tri[site].offd[1][6].real()  * ppsi[10].imag();
      cchi[ 6].imag() -= tri[site].offd[1][6].imag()  * ppsi[10].real();
      cchi[ 6].imag() += tri[site].offd[1][10].real() * ppsi[11].imag();
      cchi[ 6].imag() -= tri[site].offd[1][10].imag() * ppsi[11].real();


      cchi[ 7].real()  = tri[site].diag[1][ 1].elem()  * ppsi[ 7].real();
      cchi[ 7].real() += tri[site].offd[1][ 0].real()  * ppsi[ 6].real();
      cchi[ 7].real() -= tri[site].offd[1][ 0].imag()  * ppsi[ 6].imag();
      cchi[ 7].real() += tri[site].offd[1][ 2].real()  * ppsi[ 8].real();
      cchi[ 7].real() += tri[site].offd[1][ 2].imag()  * ppsi[ 8].imag();
      cchi[ 7].real() += tri[site].offd[1][ 4].real()  * ppsi[ 9].real();
      cchi[ 7].real() += tri[site].offd[1][ 4].imag()  * ppsi[ 9].imag();
      cchi[ 7].real() += tri[site].offd[1][ 7].real()  * ppsi[10].real();
      cchi[ 7].real() += tri[site].offd[1][ 7].imag()  * ppsi[10].imag();
      cchi[ 7].real() += tri[site].offd[1][11].real()  * ppsi[11].real();
      cchi[ 7].real() += tri[site].offd[1][11].imag()  * ppsi[11].imag();

      cchi[ 7].imag()  = tri[site].diag[1][ 1].elem()  * ppsi[ 7].imag();
      cchi[ 7].imag() += tri[site].offd[1][ 0].real()  * ppsi[ 6].imag();
      cchi[ 7].imag() += tri[site].offd[1][ 0].imag()  * ppsi[ 6].real();
      cchi[ 7].imag() += tri[site].offd[1][ 2].real()  * ppsi[ 8].imag();
      cchi[ 7].imag() -= tri[site].offd[1][ 2].imag()  * ppsi[ 8].real();
      cchi[ 7].imag() += tri[site].offd[1][ 4].real()  * ppsi[ 9].imag();
      cchi[ 7].imag() -= tri[site].offd[1][ 4].imag()  * ppsi[ 9].real();
      cchi[ 7].imag() += tri[site].offd[1][ 7].real()  * ppsi[10].imag();
      cchi[ 7].imag() -= tri[site].offd[1][ 7].imag()  * ppsi[10].real();
      cchi[ 7].imag() += tri[site].offd[1][11].real()  * ppsi[11].imag();
      cchi[ 7].imag() -= tri[site].offd[1][11].imag()  * ppsi[11].real();


      cchi[ 8].real()  = tri[site].diag[1][ 2].elem()  * ppsi[ 8].real();
      cchi[ 8].real() += tri[site].offd[1][ 1].real()  * ppsi[ 6].real();
      cchi[ 8].real() -= tri[site].offd[1][ 1].imag()  * ppsi[ 6].imag();
      cchi[ 8].real() += tri[site].offd[1][ 2].real()  * ppsi[ 7].real();
      cchi[ 8].real() -= tri[site].offd[1][ 2].imag()  * ppsi[ 7].imag();
      cchi[ 8].real() += tri[site].offd[1][5].real()   * ppsi[ 9].real();
      cchi[ 8].real() += tri[site].offd[1][5].imag()   * ppsi[ 9].imag();
      cchi[ 8].real() += tri[site].offd[1][8].real()   * ppsi[10].real();
      cchi[ 8].real() += tri[site].offd[1][8].imag()   * ppsi[10].imag();
      cchi[ 8].real() += tri[site].offd[1][12].real()  * ppsi[11].real();
      cchi[ 8].real() += tri[site].offd[1][12].imag()  * ppsi[11].imag();

      cchi[ 8].imag() = tri[site].diag[1][ 2].elem()   * ppsi[ 8].imag();
      cchi[ 8].imag() += tri[site].offd[1][ 1].real()  * ppsi[ 6].imag();
      cchi[ 8].imag() += tri[site].offd[1][ 1].imag()  * ppsi[ 6].real();
      cchi[ 8].imag() += tri[site].offd[1][ 2].real()  * ppsi[ 7].imag();
      cchi[ 8].imag() += tri[site].offd[1][ 2].imag()  * ppsi[ 7].real();
      cchi[ 8].imag() += tri[site].offd[1][5].real()   * ppsi[ 9].imag();
      cchi[ 8].imag() -= tri[site].offd[1][5].imag()   * ppsi[ 9].real();
      cchi[ 8].imag() += tri[site].offd[1][8].real()   * ppsi[10].imag();
      cchi[ 8].imag() -= tri[site].offd[1][8].imag()   * ppsi[10].real();
      cchi[ 8].imag() += tri[site].offd[1][12].real()  * ppsi[11].imag();
      cchi[ 8].imag() -= tri[site].offd[1][12].imag()  * ppsi[11].real();


      cchi[ 9].real()  = tri[site].diag[1][ 3].elem()  * ppsi[ 9].real();
      cchi[ 9].real() += tri[site].offd[1][ 3].real()  * ppsi[ 6].real();
      cchi[ 9].real() -= tri[site].offd[1][ 3].imag()  * ppsi[ 6].imag();
      cchi[ 9].real() += tri[site].offd[1][ 4].real()  * ppsi[ 7].real();
      cchi[ 9].real() -= tri[site].offd[1][ 4].imag()  * ppsi[ 7].imag();
      cchi[ 9].real() += tri[site].offd[1][ 5].real()  * ppsi[ 8].real();
      cchi[ 9].real() -= tri[site].offd[1][ 5].imag()  * ppsi[ 8].imag();
      cchi[ 9].real() += tri[site].offd[1][ 9].real()  * ppsi[10].real();
      cchi[ 9].real() += tri[site].offd[1][ 9].imag()  * ppsi[10].imag();
      cchi[ 9].real() += tri[site].offd[1][13].real()  * ppsi[11].real();
      cchi[ 9].real() += tri[site].offd[1][13].imag()  * ppsi[11].imag();

      cchi[ 9].imag()  = tri[site].diag[1][ 3].elem()  * ppsi[ 9].imag();
      cchi[ 9].imag() += tri[site].offd[1][ 3].real()  * ppsi[ 6].imag();
      cchi[ 9].imag() += tri[site].offd[1][ 3].imag()  * ppsi[ 6].real();
      cchi[ 9].imag() += tri[site].offd[1][ 4].real()  * ppsi[ 7].imag();
      cchi[ 9].imag() += tri[site].offd[1][ 4].imag()  * ppsi[ 7].real();
      cchi[ 9].imag() += tri[site].offd[1][ 5].real()  * ppsi[ 8].imag();
      cchi[ 9].imag() += tri[site].offd[1][ 5].imag()  * ppsi[ 8].real();
      cchi[ 9].imag() += tri[site].offd[1][ 9].real()  * ppsi[10].imag();
      cchi[ 9].imag() -= tri[site].offd[1][ 9].imag()  * ppsi[10].real();
      cchi[ 9].imag() += tri[site].offd[1][13].real()  * ppsi[11].imag();
      cchi[ 9].imag() -= tri[site].offd[1][13].imag()  * ppsi[11].real();


      cchi[10].real()  = tri[site].diag[1][ 4].elem()  * ppsi[10].real();
      cchi[10].real() += tri[site].offd[1][ 6].real()  * ppsi[ 6].real();
      cchi[10].real() -= tri[site].offd[1][ 6].imag()  * ppsi[ 6].imag();
      cchi[10].real() += tri[site].offd[1][ 7].real()  * ppsi[ 7].real();
      cchi[10].real() -= tri[site].offd[1][ 7].imag()  * ppsi[ 7].imag();
      cchi[10].real() += tri[site].offd[1][ 8].real()  * ppsi[ 8].real();
      cchi[10].real() -= tri[site].offd[1][ 8].imag()  * ppsi[ 8].imag();
      cchi[10].real() += tri[site].offd[1][ 9].real()  * ppsi[ 9].real();
      cchi[10].real() -= tri[site].offd[1][ 9].imag()  * ppsi[ 9].imag();
      cchi[10].real() += tri[site].offd[1][14].real()  * ppsi[11].real();
      cchi[10].real() += tri[site].offd[1][14].imag()  * ppsi[11].imag();

      cchi[10].imag()  = tri[site].diag[1][ 4].elem()  * ppsi[10].imag();
      cchi[10].imag() += tri[site].offd[1][ 6].real()  * ppsi[ 6].imag();
      cchi[10].imag() += tri[site].offd[1][ 6].imag()  * ppsi[ 6].real();
      cchi[10].imag() += tri[site].offd[1][ 7].real()  * ppsi[ 7].imag();
      cchi[10].imag() += tri[site].offd[1][ 7].imag()  * ppsi[ 7].real();
      cchi[10].imag() += tri[site].offd[1][ 8].real()  * ppsi[ 8].imag();
      cchi[10].imag() += tri[site].offd[1][ 8].imag()  * ppsi[ 8].real();
      cchi[10].imag() += tri[site].offd[1][ 9].real()  * ppsi[ 9].imag();
      cchi[10].imag() += tri[site].offd[1][ 9].imag()  * ppsi[ 9].real();
      cchi[10].imag() += tri[site].offd[1][14].real()  * ppsi[11].imag();
      cchi[10].imag() -= tri[site].offd[1][14].imag()  * ppsi[11].real();


      cchi[11].real()  = tri[site].diag[1][ 5].elem()  * ppsi[11].real();
      cchi[11].real() += tri[site].offd[1][10].real()  * ppsi[ 6].real();
      cchi[11].real() -= tri[site].offd[1][10].imag()  * ppsi[ 6].imag();
      cchi[11].real() += tri[site].offd[1][11].real()  * ppsi[ 7].real();
      cchi[11].real() -= tri[site].offd[1][11].imag()  * ppsi[ 7].imag();
      cchi[11].real() += tri[site].offd[1][12].real()  * ppsi[ 8].real();
      cchi[11].real() -= tri[site].offd[1][12].imag()  * ppsi[ 8].imag();
      cchi[11].real() += tri[site].offd[1][13].real()  * ppsi[ 9].real();
      cchi[11].real() -= tri[site].offd[1][13].imag()  * ppsi[ 9].imag();
      cchi[11].real() += tri[site].offd[1][14].real()  * ppsi[10].real();
      cchi[11].real() -= tri[site].offd[1][14].imag()  * ppsi[10].imag();

      cchi[11].imag()  = tri[site].diag[1][ 5].elem()  * ppsi[11].imag();
      cchi[11].imag() += tri[site].offd[1][10].real()  * ppsi[ 6].imag();
      cchi[11].imag() += tri[site].offd[1][10].imag()  * ppsi[ 6].real();
      cchi[11].imag() += tri[site].offd[1][11].real()  * ppsi[ 7].imag();
      cchi[11].imag() += tri[site].offd[1][11].imag()  * ppsi[ 7].real();
      cchi[11].imag() += tri[site].offd[1][12].real()  * ppsi[ 8].imag();
      cchi[11].imag() += tri[site].offd[1][12].imag()  * ppsi[ 8].real();
      cchi[11].imag() += tri[site].offd[1][13].real()  * ppsi[ 9].imag();
      cchi[11].imag() += tri[site].offd[1][13].imag()  * ppsi[ 9].real();
      cchi[11].imag() += tri[site].offd[1][14].real()  * ppsi[10].imag();
      cchi[11].imag() += tri[site].offd[1][14].imag()  * ppsi[10].real();

#endif

    }

    getFermBC().modifyF(chi, QDP::rb[cb]);

    END_CODE();
  }

  /**
   * Apply a dslash
   *
   * Performs the operation
   *
   *  chi <-   (L + D + L^dag) . psi
   *
   * where
   *   L       is a lower triangular matrix
   *   D       is the real diagonal. (stored together in type TRIANG)
   *
   * Arguments:
   * \param chi     result                                      (Write)
   * \param psi     source                                      (Read)
   * \param isign   D'^dag or D'  ( MINUS | PLUS ) resp.        (Read)
   * \param cb      Checkerboard of OUTPUT vector               (Read) 
   */
  void QDPCloverTerm::applySite(LatticeFermion& chi, const LatticeFermion& psi, 
			    enum PlusMinus isign, int site) const
  {
    START_CODE();

    if ( Ns != 4 )
    {
      QDPIO::cerr << __func__ << ": CloverTerm::applySite requires Ns==4" << endl;
      QDP_abort(1);
    }

    int n = 2*Nc;

    RComplex<REAL>* cchi = (RComplex<REAL>*)&(chi.elem(site).elem(0).elem(0));
    const RComplex<REAL>* ppsi = (const RComplex<REAL>*)&(psi.elem(site).elem(0).elem(0));


    cchi[ 0] = tri[site].diag[0][ 0]  * ppsi[ 0]
      +   conj(tri[site].offd[0][ 0]) * ppsi[ 1]
      +   conj(tri[site].offd[0][ 1]) * ppsi[ 2]
      +   conj(tri[site].offd[0][ 3]) * ppsi[ 3]
      +   conj(tri[site].offd[0][ 6]) * ppsi[ 4]
      +   conj(tri[site].offd[0][10]) * ppsi[ 5];
    
    cchi[ 1] = tri[site].diag[0][ 1]  * ppsi[ 1]
      +        tri[site].offd[0][ 0]  * ppsi[ 0]
      +   conj(tri[site].offd[0][ 2]) * ppsi[ 2]
      +   conj(tri[site].offd[0][ 4]) * ppsi[ 3]
      +   conj(tri[site].offd[0][ 7]) * ppsi[ 4]
      +   conj(tri[site].offd[0][11]) * ppsi[ 5];
    
    cchi[ 2] = tri[site].diag[0][ 2]  * ppsi[ 2]
      +        tri[site].offd[0][ 1]  * ppsi[ 0]
      +        tri[site].offd[0][ 2]  * ppsi[ 1]
      +   conj(tri[site].offd[0][ 5]) * ppsi[ 3]
      +   conj(tri[site].offd[0][ 8]) * ppsi[ 4]
      +   conj(tri[site].offd[0][12]) * ppsi[ 5];
    
    cchi[ 3] = tri[site].diag[0][ 3]  * ppsi[ 3]
      +        tri[site].offd[0][ 3]  * ppsi[ 0]
      +        tri[site].offd[0][ 4]  * ppsi[ 1]
      +        tri[site].offd[0][ 5]  * ppsi[ 2]
      +   conj(tri[site].offd[0][ 9]) * ppsi[ 4]
      +   conj(tri[site].offd[0][13]) * ppsi[ 5];
    
    cchi[ 4] = tri[site].diag[0][ 4]  * ppsi[ 4]
      +        tri[site].offd[0][ 6]  * ppsi[ 0]
      +        tri[site].offd[0][ 7]  * ppsi[ 1]
      +        tri[site].offd[0][ 8]  * ppsi[ 2]
      +        tri[site].offd[0][ 9]  * ppsi[ 3]
      +   conj(tri[site].offd[0][14]) * ppsi[ 5];
    
    cchi[ 5] = tri[site].diag[0][ 5]  * ppsi[ 5]
      +        tri[site].offd[0][10]  * ppsi[ 0]
      +        tri[site].offd[0][11]  * ppsi[ 1]
      +        tri[site].offd[0][12]  * ppsi[ 2]
      +        tri[site].offd[0][13]  * ppsi[ 3]
      +        tri[site].offd[0][14]  * ppsi[ 4];
    
    cchi[ 6] = tri[site].diag[1][ 0]  * ppsi[ 6]
      +   conj(tri[site].offd[1][ 0]) * ppsi[ 7]
      +   conj(tri[site].offd[1][ 1]) * ppsi[ 8]
      +   conj(tri[site].offd[1][ 3]) * ppsi[ 9]
      +   conj(tri[site].offd[1][ 6]) * ppsi[10]
      +   conj(tri[site].offd[1][10]) * ppsi[11];
    
    cchi[ 7] = tri[site].diag[1][ 1]  * ppsi[ 7]
      +        tri[site].offd[1][ 0]  * ppsi[ 6]
      +   conj(tri[site].offd[1][ 2]) * ppsi[ 8]
      +   conj(tri[site].offd[1][ 4]) * ppsi[ 9]
      +   conj(tri[site].offd[1][ 7]) * ppsi[10]
      +   conj(tri[site].offd[1][11]) * ppsi[11];
    
    cchi[ 8] = tri[site].diag[1][ 2]  * ppsi[ 8]
      +        tri[site].offd[1][ 1]  * ppsi[ 6]
      +        tri[site].offd[1][ 2]  * ppsi[ 7]
      +   conj(tri[site].offd[1][ 5]) * ppsi[ 9]
      +   conj(tri[site].offd[1][ 8]) * ppsi[10]
      +   conj(tri[site].offd[1][12]) * ppsi[11];
    
    cchi[ 9] = tri[site].diag[1][ 3]  * ppsi[ 9]
      +        tri[site].offd[1][ 3]  * ppsi[ 6]
      +        tri[site].offd[1][ 4]  * ppsi[ 7]
      +        tri[site].offd[1][ 5]  * ppsi[ 8]
      +   conj(tri[site].offd[1][ 9]) * ppsi[10]
      +   conj(tri[site].offd[1][13]) * ppsi[11];
    
    cchi[10] = tri[site].diag[1][ 4]  * ppsi[10]
      +        tri[site].offd[1][ 6]  * ppsi[ 6]
      +        tri[site].offd[1][ 7]  * ppsi[ 7]
      +        tri[site].offd[1][ 8]  * ppsi[ 8]
      +        tri[site].offd[1][ 9]  * ppsi[ 9]
      +   conj(tri[site].offd[1][14]) * ppsi[11];
    
    cchi[11] = tri[site].diag[1][ 5]  * ppsi[11]
      +        tri[site].offd[1][10]  * ppsi[ 6]
      +        tri[site].offd[1][11]  * ppsi[ 7]
      +        tri[site].offd[1][12]  * ppsi[ 8]
      +        tri[site].offd[1][13]  * ppsi[ 9]
      +        tri[site].offd[1][14]  * ppsi[10];


    END_CODE();
  }




  //! TRIACNTR 
  /*! 
   * \ingroup linop
   *
   *  Calculates
   *     Tr_D ( Gamma_mat L )
   *
   * This routine is specific to Wilson fermions!
   * 
   *  the trace over the Dirac indices for one of the 16 Gamma matrices
   *  and a hermitian color x spin matrix A, stored as a block diagonal
   *  complex lower triangular matrix L and a real diagonal diag_L.

   *  Here 0 <= mat <= 15 and
   *  if mat = mat_1 + mat_2 * 2 + mat_3 * 4 + mat_4 * 8
   *
   *  Gamma(mat) = gamma(1)^(mat_1) * gamma(2)^(mat_2) * gamma(3)^(mat_3)
   *             * gamma(4)^(mat_4)
   *
   *  Further, in basis for the Gamma matrices used, A is of the form
   *
   *      | A_0 |  0  |
   *  A = | --------- |
   *      |  0  | A_1 |
   *
   *
   * Arguments:
   *
   *  \param B         the resulting SU(N) color matrix	  (Write) 
   *  \param clov      clover term                        (Read) 
   *  \param mat       label of the Gamma matrix          (Read)
   */
  void QDPCloverTerm::triacntr(LatticeColorMatrix& B, int mat, int cb) const
  {
    START_CODE();

    B = zero;

    if ( mat < 0  ||  mat > 15 )
    {
      QDPIO::cerr << __func__ << ": Gamma out of range: mat = " << mat << endl;
      QDP_abort(1);
    }
  
    switch( mat )
    {
    case 0:
      /*# gamma(   0)   1  0  0  0            # ( 0000 )  --> 0 */
      /*#               0  1  0  0 */
      /*#               0  0  1  0 */
      /*#               0  0  0  1 */
      /*# From diagonal part */
      for(int ssite=0; ssite < rb[cb].numSiteTable(); ++ssite) 
      {
	int site = rb[cb].siteTable()[ssite];

	RComplex<REAL> lctmp0;
	RScalar<REAL> lr_zero0;
	RScalar<REAL> lrtmp0;
  
	lr_zero0 = 0;
  
	for(int i0 = 0; i0 < Nc; ++i0)
	{
	  lrtmp0 = tri[site].diag[0][i0];
	  lrtmp0 += tri[site].diag[0][i0+Nc];
	  lrtmp0 += tri[site].diag[1][i0];
	  lrtmp0 += tri[site].diag[1][i0+Nc];
	  B.elem(site).elem().elem(i0,i0) = cmplx(lrtmp0,lr_zero0);
	}

	/*# From lower triangular portion */
	int elem_ij0 = 0;
	for(int i0 = 1; i0 < Nc; ++i0)
	{
	  int elem_ijb0 = (i0+Nc)*(i0+Nc-1)/2 + Nc;

	  for(int j0 = 0; j0 < i0; ++j0)
	  {
	    lctmp0 = tri[site].offd[0][elem_ij0];
	    lctmp0 += tri[site].offd[0][elem_ijb0];
	    lctmp0 += tri[site].offd[1][elem_ij0];
	    lctmp0 += tri[site].offd[1][elem_ijb0];

	    B.elem(site).elem().elem(j0,i0) = lctmp0;
	    B.elem(site).elem().elem(i0,j0) = adj(lctmp0);


	    elem_ij0++;
	    elem_ijb0++;
	  }
	}
      }
      break;

    case 3:
      /*# gamma(  12)  -i  0  0  0            # ( 0011 )  --> 3 */
      /*#               0  i  0  0 */
      /*#               0  0 -i  0 */
      /*#               0  0  0  i */
      /*# From diagonal part */
      for(int ssite=0; ssite < rb[cb].numSiteTable(); ++ssite) 
      {
	int site = rb[cb].siteTable()[ssite];

	RComplex<REAL> lctmp3;
	RScalar<REAL> lr_zero3;
	RScalar<REAL> lrtmp3;
                          
	lr_zero3 = 0;
  
	for(int i3 = 0; i3 < Nc; ++i3)
	{
	  lrtmp3 = tri[site].diag[0][i3+Nc];
	  lrtmp3 -= tri[site].diag[0][i3];
	  lrtmp3 -= tri[site].diag[1][i3];
	  lrtmp3 += tri[site].diag[1][i3+Nc];
	  B.elem(site).elem().elem(i3,i3) = cmplx(lr_zero3,lrtmp3);
	}
	
	/*# From lower triangular portion */
	int elem_ij3 = 0;
	for(int i3 = 1; i3 < Nc; ++i3)
	{
	  int elem_ijb3 = (i3+Nc)*(i3+Nc-1)/2 + Nc;

	  for(int j3 = 0; j3 < i3; ++j3)
	  {
	    lctmp3 = tri[site].offd[0][elem_ijb3];
	    lctmp3 -= tri[site].offd[0][elem_ij3];
	    lctmp3 -= tri[site].offd[1][elem_ij3];
	    lctmp3 += tri[site].offd[1][elem_ijb3];

	    B.elem(site).elem().elem(j3,i3) = timesI(adj(lctmp3));
	    B.elem(site).elem().elem(i3,j3) = timesI(lctmp3);
	    
	    elem_ij3++;
	    elem_ijb3++;
	  }
	}
      }
      break;

    case 5:
      /*# gamma(  13)   0 -1  0  0            # ( 0101 )  --> 5 */
      /*#               1  0  0  0 */
      /*#               0  0  0 -1 */
      /*#               0  0  1  0 */
      for(int ssite=0; ssite < rb[cb].numSiteTable(); ++ssite) 
      {
	int site = rb[cb].siteTable()[ssite];

	RComplex<REAL> lctmp5;
	RScalar<REAL> lrtmp5;
                          
	for(int i5 = 0; i5 < Nc; ++i5)
	{
	  int elem_ij5 = (i5+Nc)*(i5+Nc-1)/2;

	  for(int j5 = 0; j5 < Nc; ++j5)
	  {
	    int elem_ji5 = (j5+Nc)*(j5+Nc-1)/2 + i5;

	  
	    lctmp5 = adj(tri[site].offd[0][elem_ji5]);
	    lctmp5 -= tri[site].offd[0][elem_ij5];
	    lctmp5 += adj(tri[site].offd[1][elem_ji5]);
	    lctmp5 -= tri[site].offd[1][elem_ij5];


	    B.elem(site).elem().elem(i5,j5) = lctmp5;

	    elem_ij5++;
	  }
	}
      }
      break;

    case 6:
      /*# gamma(  23)   0 -i  0  0            # ( 0110 )  --> 6 */
      /*#              -i  0  0  0 */
      /*#               0  0  0 -i */
      /*#               0  0 -i  0 */
      for(int ssite=0; ssite < rb[cb].numSiteTable(); ++ssite) 
      {
	int site = rb[cb].siteTable()[ssite];

	RComplex<REAL> lctmp6;
	RScalar<REAL> lrtmp6;
                          
	for(int i6 = 0; i6 < Nc; ++i6)
	{
	  int elem_ij6 = (i6+Nc)*(i6+Nc-1)/2;

	  for(int j6 = 0; j6 < Nc; ++j6)
	  {
	    int elem_ji6 = (j6+Nc)*(j6+Nc-1)/2 + i6;

	    lctmp6 = adj(tri[site].offd[0][elem_ji6]);
	    lctmp6 += tri[site].offd[0][elem_ij6];
	    lctmp6 += adj(tri[site].offd[1][elem_ji6]);
	    lctmp6 += tri[site].offd[1][elem_ij6];

	    B.elem(site).elem().elem(i6,j6) = timesMinusI(lctmp6);

	    elem_ij6++;
	  }
	}
      }
      break;

    case 9:
      /*# gamma(  14)   0  i  0  0            # ( 1001 )  --> 9 */
      /*#               i  0  0  0 */
      /*#               0  0  0 -i */
      /*#               0  0 -i  0 */
      for(int ssite=0; ssite < rb[cb].numSiteTable(); ++ssite) 
      {
	int site = rb[cb].siteTable()[ssite];

	RComplex<REAL> lctmp9;
	RScalar<REAL> lrtmp9;
                          
	for(int i9 = 0; i9 < Nc; ++i9)
	{
	  int elem_ij9 = (i9+Nc)*(i9+Nc-1)/2;

	  for(int j9 = 0; j9 < Nc; ++j9)
	  {
	    int elem_ji9 = (j9+Nc)*(j9+Nc-1)/2 + i9;

	    lctmp9 = adj(tri[site].offd[0][elem_ji9]);
	    lctmp9 += tri[site].offd[0][elem_ij9];
	    lctmp9 -= adj(tri[site].offd[1][elem_ji9]);
	    lctmp9 -= tri[site].offd[1][elem_ij9];

	    B.elem(site).elem().elem(i9,j9) = timesI(lctmp9);

	    elem_ij9++;
	  }
	}
      }
      break;

    case 10:
      /*# gamma(  24)   0 -1  0  0            # ( 1010 )  --> 10 */
      /*#               1  0  0  0 */
      /*#               0  0  0  1 */
      /*#               0  0 -1  0 */
      for(int ssite=0; ssite < rb[cb].numSiteTable(); ++ssite) 
      {
	int site = rb[cb].siteTable()[ssite];

	RComplex<REAL> lctmp10;
	RScalar<REAL> lrtmp10;
                          
	for(int i10 = 0; i10 < Nc; ++i10)
	{
	  int elem_ij10 = (i10+Nc)*(i10+Nc-1)/2;
	
	  for(int j10 = 0; j10 < Nc; ++j10)
	  {
	    int elem_ji10 = (j10+Nc)*(j10+Nc-1)/2 + i10;

	    lctmp10 = adj(tri[site].offd[0][elem_ji10]);
	    lctmp10 -= tri[site].offd[0][elem_ij10];
	    lctmp10 -= adj(tri[site].offd[1][elem_ji10]);
	    lctmp10 += tri[site].offd[1][elem_ij10];

	    B.elem(site).elem().elem(i10,j10) = lctmp10;

	    elem_ij10++;
	  }
	}
      }
      break;
    
    case 12:
      /*# gamma(  34)   i  0  0  0            # ( 1100 )  --> 12 */
      /*#               0 -i  0  0 */
      /*#               0  0 -i  0 */
      /*#               0  0  0  i */
      /*# From diagonal part */
      for(int ssite=0; ssite < rb[cb].numSiteTable(); ++ssite) 
      {
	int site = rb[cb].siteTable()[ssite];

	RComplex<REAL> lctmp12;
	RScalar<REAL> lr_zero12;
	RScalar<REAL> lrtmp12;
                          
	lr_zero12 = 0;
  
	for(int i12 = 0; i12 < Nc; ++i12)
	{
	  lrtmp12 = tri[site].diag[0][i12];
	  lrtmp12 -= tri[site].diag[0][i12+Nc];
	  lrtmp12 -= tri[site].diag[1][i12];
	  lrtmp12 += tri[site].diag[1][i12+Nc];
	  B.elem(site).elem().elem(i12,i12) = cmplx(lr_zero12,lrtmp12);
	}
    
	/*# From lower triangular portion */
	int elem_ij12 = 0;
	for(int i12 = 1; i12 < Nc; ++i12)
	{
	  int elem_ijb12 = (i12+Nc)*(i12+Nc-1)/2 + Nc;

	  for(int j12 = 0; j12 < i12; ++j12)
	  {
	    lctmp12 = tri[site].offd[0][elem_ij12];
	    lctmp12 -= tri[site].offd[0][elem_ijb12];
	    lctmp12 -= tri[site].offd[1][elem_ij12];
	    lctmp12 += tri[site].offd[1][elem_ijb12];
	
	    B.elem(site).elem().elem(i12,j12) = timesI(lctmp12);
	    B.elem(site).elem().elem(j12,i12) = timesI(adj(lctmp12));
	
	    elem_ij12++;
	    elem_ijb12++;
	  }
	}
      }
      break;
    
    default:
      QDPIO::cout << __func__ << ": invalid Gamma matrix int" << endl;
      QDP_abort(1);
    }

    END_CODE();
  }

  //! Returns the appropriate clover coefficient for indices mu and nu
  Real QDPCloverTerm::getCloverCoeff(int mu, int nu) const 
  { 
    START_CODE();

    if( param.anisoParam.anisoP ) 
    { 
      if (mu==param.anisoParam.t_dir || nu == param.anisoParam.t_dir) { 
	return param.clovCoeffT;
      }
      else { 
	// Otherwise return the spatial coeff
	return param.clovCoeffR;
      }
    }
    else { 
      // If there is no anisotropy just return the spatial one, it will
      // be the same as the temporal one
      return param.clovCoeffR; 
    } 
    
    END_CODE();
  }

}

