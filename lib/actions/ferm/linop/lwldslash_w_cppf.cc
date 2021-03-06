/*! \file
 *  \brief Wilson Dslash linear operator
 */

#include "chromabase.h"
#include "actions/ferm/linop/lwldslash_w_cppf.h"
#include "cpp_dslash.h"
#include "cpp_dslash_qdp_packer.h"


using namespace CPlusPlusWilsonDslash;

namespace Chroma 
{ 

  //! Initialization routine
  void CPPWilsonDslashF::init()
  {
    // Initialize using the total problem size -- this is a handle so will be destroyed/refcounted as needed.

    D = new Dslash<float>(Layout::lattSize().slice(),
			  Layout::QDPXX_getSiteCoords,
			  Layout::QDPXX_getLinearSiteIndex,
			  Layout::QDPXX_nodeNumber);
  }


  //! Empty constructor
  CPPWilsonDslashF::CPPWilsonDslashF()
#ifndef CHROMA_STATIC_PACKED_GAUGE
  	  : packed_gauge(nullptr)
#endif
  {

	 init();
  }
  
  //! Full constructor
  CPPWilsonDslashF::CPPWilsonDslashF(Handle< FermState<T,P,Q> > state)
#ifndef CHROMA_STATIC_PACKED_GAUGE
  	  : packed_gauge(nullptr)
#endif
  {

    init();
    create(state);
  }
  
  //! Full constructor with anisotropy
  CPPWilsonDslashF::CPPWilsonDslashF(Handle< FermState<T,P,Q> > state,
				   const AnisoParam_t& aniso_) 
#ifndef CHROMA_STATIC_PACKED_GAUGE
  	  : packed_gauge(nullptr)
#endif
  {

    init();
    create(state, aniso_);
  }

  //! Full constructor with general coefficients
  CPPWilsonDslashF::CPPWilsonDslashF(Handle< FermState<T,P,Q> > state,
				   const multi1d<Real>& coeffs_)
#ifndef CHROMA_STATIC_PACKED_GAUGE
  	  : packed_gauge(nullptr)
#endif
  {

    init();
    create(state, coeffs_);
  }

  //! Creation routine
  void CPPWilsonDslashF::create(Handle< FermState<T,P,Q> > state)
  {
    multi1d<Real> cf(Nd);
    cf = 1.0;
    create(state, cf);
  }

  //! Creation routine with anisotropy
  void CPPWilsonDslashF::create(Handle< FermState<T,P,Q> > state,
			       const AnisoParam_t& anisoParam) 
  {
    START_CODE();

    create(state, makeFermCoeffs(anisoParam));

    END_CODE();
  }

  //! Full constructor with general coefficients
  void CPPWilsonDslashF::create(Handle< FermState<T,P,Q> > state,
			       const multi1d<Real>& coeffs_)
  {
    START_CODE();

    // Save a copy of the aniso params original fields and with aniso folded in
    coeffs = coeffs_;

    // Save a copy of the fermbc
    fbc = state->getFermBC();

    // Sanity check
    if (fbc.operator->() == 0)
    {
      QDPIO::cerr << "CPPWilsonDslashF: error: fbc is null" << std::endl;
      QDP_abort(1);
    }

    // Fold in anisotropy
    multi1d<LatticeColorMatrixF> u = state->getLinks();
  
    // Rescale the u fields by the anisotropy
    for(int mu=0; mu < u.size(); ++mu)
    {
      u[mu] *= coeffs[mu];
    }

    // Pack the gauge fields
    //packed_gauge.resize( Nd * Layout::sitesOnNode() );
    // Allocate as a pointer -- In the case of static allocation
    // Always allocate if null. If the packed gauge is static, it will not be null after the first allocation.
    if ( packed_gauge == nullptr ) {
    	packed_gauge = (PrimitiveSU3MatrixF *)QDP::Allocator::theQDPAllocator::Instance().allocate( Layout::sitesOnNode()*Nd*sizeof(PrimitiveSU3MatrixF),
    			QDP::Allocator::DEFAULT);
    	if( packed_gauge == nullptr ) {
    		QDPIO::cout << "Failed to allocate packed gauge in CPP_Dslash " <<std::endl;
    		QDP_abort(1);
    	}
    }

#if 0
    QDPIO::cout << "Done " << std::endl << std::flush;

    QDPIO::cout << "Calling pack_gauge_field..." << std::flush;
#endif

    qdp_pack_gauge(u, packed_gauge);
  
#if 0
    QDPIO::cout << "Done" << std::endl << std::flush;
#endif

    END_CODE();
  }


  CPPWilsonDslashF::~CPPWilsonDslashF() 
  {
    START_CODE();

    // Handle will free the operator.

    // If we are not using static packed gauges, we need to free here
   // otherwise we never free.
#ifndef CHROMA_STATIC_PACKED_GAUGE
    QDP::Allocator::theQDPAllocator::Instance().free(packed_gauge);
#endif

    END_CODE();
  }

  //! General Wilson-Dirac dslash
  /*! \ingroup linop
   * Wilson dslash
   *
   * Arguments:
   *
   *  \param chi	      Result				                (Write)
   *  \param psi	      Pseudofermion field				(Read)
   *  \param isign      D'^dag or D' ( MINUS | PLUS ) resp.		(Read)
   *  \param cb	      Checkerboard of OUTPUT std::vector			(Read) 
   */
  void
  CPPWilsonDslashF::apply (T& chi, const T& psi, 
			  enum PlusMinus isign, int cb) const
  {
    START_CODE();

    /* Pass the right parities. 
     *
     * SZIN standard is that cb is cb of INPUT 
     * Chroma standard is that the cb is cb of OUTPUT
     *
     * Need to invert cb for SZIN style SSE call 
   
     *
     *
     * Pass all the fermion and all the gauge pieces
     *
     * NOTE: this breaks usage from SZIN. However, Chroma and SSE dslash can handle 
     * odd subgrid lattice sizes, whereas SZIN cannot. Thus, I must pass all fermion
     * cbs to support such flexibility.
     *
     */
    int source_cb = 1 - cb;
    int target_cb = cb;
    int cbsites = QDP::Layout::sitesOnNode()/2;

    (*D)((float *)&(chi.elem(all.start()).elem(0).elem(0).real()),	
	 (float *)&(psi.elem(all.start()).elem(0).elem(0).real()),
	 (float *)&(packed_gauge[0]),
	 isign, 
	 source_cb);
    //    sse_su3dslash_wilson((SSEREAL *)&(packed_gauge[0]),
    //			 (SSEREAL *)&(psi.elem(0).elem(0).elem(0).real()),
    //			 (SSEREAL *)&(chi.elem(0).elem(0).elem(0).real()),
    //			 (int)isign, source_cb);
  

    getFermBC().modifyF(chi, QDP::rb[cb]);

    END_CODE();
  }

} // End Namespace Chroma

