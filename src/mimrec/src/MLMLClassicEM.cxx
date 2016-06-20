/*
 * MLMLClassicEM.cxx
 *
 *
 * Copyright (C) by Andreas Zoglauer.
 * All rights reserved.
 *
 *
 * This code implementation is the intellectual property of
 * Andreas Zoglauer.
 *
 * By copying, distributing or modifying the Program (or any work
 * based on the Program) you indicate your acceptance of this statement,
 * and all its terms.
 *
 */


////////////////////////////////////////////////////////////////////////////////
//
// MLMLClassicEM
//
////////////////////////////////////////////////////////////////////////////////


// Include the header:
#include "MLMLClassicEM.h"

// Standard libs:
#include <limits>
using namespace std;

// ROOT libs:
#include "TSystem.h"

// MEGAlib libs:
#include "MStreams.h"


////////////////////////////////////////////////////////////////////////////////


#ifdef ___CINT___
ClassImp(MLMLClassicEM)
#endif


////////////////////////////////////////////////////////////////////////////////


MLMLClassicEM::MLMLClassicEM() : MLMLAlgorithms()
{
  // Construct an instance of MLMLClassicEM
  // BPStorage: class containing the already precomputed PSFs

  m_Vi = 0;
  m_Ej = 0;

  m_InitialLikelihood = 0;
  m_LastLikelihood = 0;
}


////////////////////////////////////////////////////////////////////////////////


MLMLClassicEM::~MLMLClassicEM()
{
  // Destruct this instance of MLMLClassicEM

  delete [] m_Vi; 
  delete [] m_Ej; 
}


////////////////////////////////////////////////////////////////////////////////


bool MLMLClassicEM::SetResponseSlices(vector<MBPData*>& Storage, int NImageBins)
{
  // Set the backprojection storage (PSFs!)

  if (MLMLAlgorithms::SetResponseSlices(Storage, NImageBins) == false) {
    return false;
  }

  // Additionally add the viewport factor and 
  // initialize it with a convolution with a flat image:

  delete [] m_Vi;
  m_Vi = new double[m_NEvents];
  if (m_Vi == 0) {
    merr<<"Out of memory"<<show;
    return false;
  }
  

  for (unsigned int i = 0; i < m_NEvents; i++) m_Vi[i] = 1.0;
  for (unsigned int i = 0; i < m_NEvents; i++) {
    m_Storage[i]->Convolve(m_Vi, i, m_Lj, m_NBins);
  }
  
  // And the expectation
  if (m_Ej != 0) delete [] m_Ej; 
  m_Ej = new double[m_NBins];
  if (m_Ej == 0) {
    merr<<"Out of memory"<<show;
    return false;
  }

  for (unsigned int i = 0; i < m_NBins; i++) m_Ej[i] = 0.0;

  for (unsigned int i = 0; i < m_NEvents; ++i) m_Ri[i] = m_Vi[i];

  /*
  m_BgdT = 0;
  for (int i = 0; i < m_NEvents; ++i) m_BgdT += m_Ri[i];
  m_BgdS = 1;
  */

  return true;
}


////////////////////////////////////////////////////////////////////////////////


bool MLMLClassicEM::DoOneIteration()
{
  // Do only one iteration, i.e. convole and deconvole the data


  // The sum over all image pixel has to be the same,
  // before and after the iteration

  Convolve();
  Deconvolve();

  m_NPerformedIterations++;

  if (m_NPerformedIterations == 1) {
    m_InitialLikelihood = 0;
    for (unsigned int i = 0; i < m_NBins; ++i) m_InitialLikelihood += m_Lj[i];
    m_LastLikelihood = 1;
    m_CurrentLikelihood = m_InitialLikelihood;
  } else {
    m_LastLikelihood = m_CurrentLikelihood;
    m_CurrentLikelihood = 0;
    for (unsigned int i = 0; i < m_NBins; ++i) m_CurrentLikelihood += m_Lj[i];
  }

  return true;
}


////////////////////////////////////////////////////////////////////////////////


void MLMLClassicEM::Convolve()
{
  // Do the convolution:
  // y_i_bar = Sum_j (t_ij l_j)/v_i + rs*r_i

  // This is very simple for the classic algorithm:

  for (unsigned int i = 0; i < m_NEvents; i++) m_Yi[i] = 0.0;
  for (unsigned int i = 0; i < m_NEvents; i++) {
    // All the convolution-work is done within the MBPImage... classes, 
    // called by m_BPStorage->GetResponseSlice(i): Sum_j (t_ij l_j)
    m_Storage[i]->Convolve(m_Yi, i, m_Lj, m_NBins);

    // Normalize and add background
    if (m_Vi[i] != 0) {
      //cout<<"m_Yi["<<i<<"]: "<<m_Yi[i]/m_Vi[i] + m_BgdS*m_Ri[i]<<" = "<<m_Yi[i]/m_Vi[i]<<" + "<<m_BgdS*m_Ri[i]<<endl;
      // m_Yi[i] = m_Yi[i]/m_Vi[i] + m_BgdS*m_Ri[i];
      // m_Yi[i] = (m_Yi[i] + m_BgdS*m_Ri[i])/m_Vi[i];  // 18.1.01
      m_Yi[i] = m_Yi[i]/m_Vi[i];
    } else {
      //Warning("void MLMLClassicEM::Convolve(double *Ci)",
      //"m_V[i] = 0 not allowed --> The event is not within the image so useless...");
    }

    if (m_EnableGUIInteractions == true && i%1000 == 0) {
      gSystem->ProcessEvents();
    }
  }
  // For improved deconvolution, invert the Yi
  for (unsigned int i = 0; i < m_NEvents; i++) {
    if (m_Yi[i] == 0) {
      cout<<"LM-ML-EM classic: We have an empty event. Eliminating event "<<i<<"..."<<endl;
      m_InvYi[i] = 0.0;
    } else {
      m_InvYi[i] = 1.0/m_Yi[i];
    }
  }
}


////////////////////////////////////////////////////////////////////////////////


void MLMLClassicEM::Deconvolve()
{
  // Do the de-convolution:
  // In other words: compute the correction image
  //             or: compute the expectation
  // e_j = Sum_i t_ij / y_i_bar for the image pixels
  

  for (unsigned int i = 0; i < m_NBins; i++) m_Ej[i] = 0.0;
  for (unsigned int i = 0; i < m_NEvents; i++) {
    // All the deconvolution-work is done within the MBPImage... classes, 
    // called by m_BPStorage->GetResponseSlice(i)
    m_Storage[i]->Deconvolve(m_Ej, m_InvYi, i);

    if (m_EnableGUIInteractions == true && i%1000 == 0) {
      gSystem->ProcessEvents();
    }
  }

  // Correct the image:
  for (unsigned int i = 0; i < m_NBins; i++) {
    m_Lj[i] *= m_Ej[i]/m_Sj[i];
  }

//   // Now deconvolve the background scaling factor: 
//   double B_corr = 0;
//   for (i = 0; i < m_NEvents; i++) {
//     if (m_Yi[i] != 0) {
//       B_corr += m_Ri[i]/m_Yi[i];
//     }
//     //cout<<B_corr<<"!"<<m_Ri[i]<<"!"<<m_Yi[i]<<endl;
//   }
  //m_BgdS *= B_corr/m_BgdT;   
  //cout<<"Background scaling factor: "<<m_BgdS<<"!"<<B_corr<<"!"<<m_BgdT<<endl;

  return;
}


////////////////////////////////////////////////////////////////////////////////


double* MLMLClassicEM::GetInitialImage()
{
  // The initial image is nothing but a deconvolution with a flat image

  double* D = new double[m_NEvents];

  for (unsigned int i = 0; i < m_NEvents; i++) D[i] = 1.0;
  for (unsigned int i = 0; i < m_NEvents; i++) {
    m_Storage[i]->Deconvolve(m_Ej, D, i);
  }
    
  delete [] D;

  return m_Ej;
}



// MLMLClassicEM.cxx: the end...
////////////////////////////////////////////////////////////////////////////////
