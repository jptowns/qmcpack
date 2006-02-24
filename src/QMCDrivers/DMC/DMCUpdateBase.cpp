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
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "QMCDrivers/DMCUpdateBase.h"
#include "ParticleBase/ParticleUtility.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "Message/CommCreate.h"

namespace qmcplusplus { 

  /// Constructor.
  DMCUpdateBase::DMCUpdateBase(ParticleSet& w, TrialWaveFunction& psi, QMCHamiltonian& h,
      RandomGenerator_t& rg): W(w),Psi(psi),H(h), RandomGen(rg)
    { }
  
  /// destructor
  DMCUpdateBase::~DMCUpdateBase() { }

  void DMCUpdateBase::resetRun(BranchEngineType* brancher) {
    branchEngine=brancher;
    NumPtcl = W.getTotalNum();
    deltaR.resize(NumPtcl);
    drift.resize(NumPtcl);

    Tau=brancher->Tau;
    m_oneover2tau = 0.5/Tau;
    m_sqrttau = sqrt(Tau);
  }

  void DMCUpdateBase::resetEtrial(RealType et) {
    branchEngine->E_T=et;
    branchEngine->flush(0);
  }

  void DMCUpdateBase::startBlock() {
    nAccept = 0; 
    nReject=0;
    nAllRejected=0;
    nNodeCrossing=0;
  }
  void DMCUpdateBase::initWalkers(WalkerIter_t it, WalkerIter_t it_end) {
    if(G.size() != NumPtcl) {
      G.resize(NumPtcl);
      dG.resize(NumPtcl);
      L.resize(NumPtcl);
      dL.resize(NumPtcl);
    }
    while(it != it_end) {
      (*it)->DataSet.rewind();
      W.registerData(**it,(*it)->DataSet);
      ValueType logpsi=Psi.registerData(W,(*it)->DataSet);

      RealType vsq = Dot(W.G,W.G);
      RealType scale = ((-1.0+sqrt(1.0+2.0*Tau*vsq))/vsq);
      (*it)->Drift = scale*W.G;

      RealType ene = H.evaluate(W);
      (*it)->resetProperty(logpsi,Psi.getSign(),ene);
      H.saveProperty((*it)->getPropertyBase());
      ++it;
    } 
  }

  void DMCUpdateBase::updateWalkers(WalkerIter_t it, WalkerIter_t it_end) {
    while(it != it_end) {
      Walker_t::Buffer_t& w_buffer((*it)->DataSet);
      w_buffer.rewind();
      W.updateBuffer(**it,w_buffer);
      ValueType logpsi=Psi.updateBuffer(W,w_buffer);
      RealType enew= H.evaluate(W);
      (*it)->resetProperty(logpsi,Psi.getSign(),enew);
      H.saveProperty((*it)->getPropertyBase());
      ValueType vsq = Dot(W.G,W.G);
      ValueType scale = ((-1.0+sqrt(1.0+2.0*Tau*vsq))/vsq);
      (*it)->Drift = scale*W.G;
      ++it;
    }
  }

  void DMCUpdateBase::benchMark(WalkerIter_t it, WalkerIter_t it_end, int ip) {
    char fname[16];
    sprintf(fname,"test.%i",ip);
    ofstream fout(fname,ios::app);
    int i=0;
    while(it != it_end) {
      Walker_t& thisWalker(**it);
      makeGaussRandomWithEngine(deltaR,RandomGen); 
      W.R = m_sqrttau*deltaR+ thisWalker.R;
      W.update();
      ValueType logpsi(Psi.evaluateLog(W));
      RealType e = H.evaluate(W);
      fout <<  i << " " << logpsi << " " << e << endl;
      ++it;++i;
    }
  }

}

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
