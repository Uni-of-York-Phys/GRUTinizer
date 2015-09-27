#ifndef GROOTFUNCTIONS__H
#define GROOTFUNCTIONS__H

#include "TMath.h"

namespace GRootFunctions {

  Double_t PolyBg(Double_t *dim, Double_t *par,Int_t order);
  Double_t StepBG(Double_t *dim, Double_t *par);
  Double_t StepFunction(Double_t *dim, Double_t *par);
  Double_t PhotoPeak(Double_t *dim, Double_t *par);
  Double_t PhotoPeakBG(Double_t *dim, Double_t *par);
  Double_t Gaus(Double_t *dim, Double_t *par);
  Double_t SkewedGaus(Double_t *dim, Double_t *par);


}


#endif
