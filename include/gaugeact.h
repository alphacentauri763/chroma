// -*- C++ -*-
// $Id: gaugeact.h,v 1.3 2004-01-02 03:07:06 edwards Exp $

/*! @file
 * @brief Class structure for gauge actions
 */

#ifndef __gaugeactt_h__
#define __gaugeactt_h__

using namespace QDP;

#include "state.h"
#include "gaugebc.h"

//! Abstract base class for gauge actions
/*! @ingroup actions
 *
 * Supports creation and application for gauge actions
 */
class GaugeAction
{
public:
  //! Given links, create the state needed for the linear operators
  /*! 
   * Default version uses a SimpleConnectState 
   */
  virtual const ConnectState* createState(const multi1d<LatticeColorMatrix>& u) const
    {
      multi1d<LatticeColorMatrix> u_tmp = u;
      getGaugeBC().modify(u_tmp);
      return new SimpleConnectState(u_tmp);
    }

  //! Produce a gauge boundary condition object
  virtual const GaugeBC& getGaugeBC() const = 0;

  //! Compute dS/dU
  /*! Default version. Derived class should override this if needed. */
  virtual void dsdu(multi1d<LatticeColorMatrix>& result,
		    Handle<const ConnectState> state) const
    {
      QDPIO::cerr << "GaugeAction::dsdu not implemented" << endl;
      QDP_abort(1);
    }

  //! Virtual destructor to help with cleanup;
  virtual ~GaugeAction() {}
};


//! Concrete empty class for gauge actions
/*! @ingroup actions
 *
 * This class can be used as a trivial action, in cases where we do not know or
 * care what is the original gauge action.
 */
class EmptyGaugeAct
{
public:
  //! Produce a gauge boundary condition object
  const PeriodicGaugeBC& getGaugeBC() const {return gbc;}

  //! Destructor is automatic
  ~EmptyGaugeAct() {}

private:
  PeriodicGaugeBC  gbc;
};


#endif
