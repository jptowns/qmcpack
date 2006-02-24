//////////////////////////////////////////////////////////////////
// (c) Copyright 2003- by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Jeongnim Kim
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//   Tel:    217-244-6319 (NCSA) 217-333-3324 (MCC)
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//   Department of Physics, Ohio State University
//   Ohio Supercomputer Center
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "QMCDrivers/DMC/DMCUpdatePbyP.h"
#include "Particle/MCWalkerConfiguration.h"
#include "Particle/DistanceTable.h"
#include "Particle/HDFWalkerIO.h"
#include "ParticleBase/ParticleUtility.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "Message/CommCreate.h"

namespace qmcplusplus { 

  /// Constructor.
  DMCUpdatePbyPWithRejection::DMCUpdatePbyPWithRejection(ParticleSet& w, TrialWaveFunction& psi, QMCHamiltonian& h,
      RandomGenerator_t& rg): DMCUpdateBase(w,psi,h,rg)
    { }
  
  /// destructor
  DMCUpdatePbyPWithRejection::~DMCUpdatePbyPWithRejection() { }


  /** advance all the walkers with killnode==no
   * @param nat number of particles to move
   * 
   * When killnode==no, any move resulting in node-crossing is treated
   * as a normal rejection.
   */
  void DMCUpdatePbyPWithRejection::advanceWalkers(WalkerIter_t it, WalkerIter_t it_end) {
    int item=0;
    while(it != it_end) {
      //MCWalkerConfiguration::WalkerData_t& w_buffer = *(W.DataSet[iwalker]);
      Walker_t& thisWalker(**it);
      Walker_t::Buffer_t& w_buffer(thisWalker.DataSet);

      //thisWalker.Weight = 1.0e0;
      //thisWalker.Multiplicity=1.0e0;
      //save old local energy
      ValueType eold(thisWalker.Properties(LOCALENERGY));
      ValueType emixed(eold), enew(eold);

      W.R = thisWalker.R;
      w_buffer.rewind();
      W.copyFromBuffer(w_buffer);
      Psi.copyFromBuffer(W,w_buffer);

      //create a 3N-Dimensional Gaussian with variance=1
      makeGaussRandom(deltaR);
      bool notcrossed(true);
      int nAcceptTemp(0);
      int nRejectTemp(0);
      int iat=0;

      RealType rr_proposed=0.0;
      RealType rr_accepted=0.0;
      while(iat<NumPtcl) {//particle-by-particle move
        PosType dr(m_sqrttau*deltaR[iat]+thisWalker.Drift[iat]);
        PosType newpos(W.makeMove(iat,dr));

        RealType ratio=Psi.ratio(W,iat,dG,dL);

        RealType rr=dot(dr,dr);
        rr_proposed+=rr;

        if(ratio < 0.0) {//node is crossed reject the move
          ++nRejectTemp;
          ++nNodeCrossing;
          Psi.restore(iat);
        } else {
          G = W.G+dG;
          RealType logGf = -0.5*dot(deltaR[iat],deltaR[iat]);
          RealType scale=Tau;
          //ValueType vsq = Dot(G,G);
          //ValueType scale = ((-1.0+sqrt(1.0+2.0*Tau*vsq))/vsq);
          dr = thisWalker.R[iat]-newpos-scale*G[iat]; 
          RealType logGb = -m_oneover2tau*dot(dr,dr);
          RealType prob = std::min(1.0,ratio*ratio*exp(logGb-logGf));
          if(RandomGen() < prob) { 
            ++nAcceptTemp;
            W.acceptMove(iat);
            Psi.update2(W,iat);
            W.G = G;
            W.L += dL;
            thisWalker.Drift = scale*G;
            rr_accepted+=rr;
          } else {
            ++nRejectTemp; 
            Psi.restore(iat);
          }
        } 

        ++iat;
      }

      if(nAcceptTemp>0) {//need to overwrite the walker properties
        thisWalker.R = W.R;
        w_buffer.rewind();
        W.copyToBuffer(w_buffer);
        ValueType psi = Psi.evaluate(W,w_buffer);
        enew= H.evaluate(W);
        thisWalker.resetProperty(log(abs(psi)),psi,enew);
        H.saveProperty(thisWalker.getPropertyBase());
        emixed = (eold+enew)*0.5e0;
      } else {
        thisWalker.Age++;
        ++nAllRejected;
        rr_accepted=0.0;
      }

      //branchEngine->setWeight(thisWalker,Tau*rr_accepted/rr_proposed,emixed,RandomGen());
      thisWalker.Weight *= branchEngine->branchGF(Tau*rr_accepted/rr_proposed,emixed,0.0);
      
      nAccept += nAcceptTemp;
      nReject += nRejectTemp;
      ++it;
    }
  }
}

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
