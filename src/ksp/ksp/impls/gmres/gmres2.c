/*$Id: gmres2.c,v 1.35 2001/08/06 21:16:44 bsmith Exp $*/
#include "src/ksp/ksp/impls/gmres/gmresp.h"       /*I  "petscksp.h"  I*/

#undef __FUNCT__  
#define __FUNCT__ "KSPGMRESSetOrthogonalization" 
/*@C
   KSPGMRESSetOrthogonalization - Sets the orthogonalization routine used by GMRES and FGMRES.

   Collective on KSP

   Input Parameters:
+  ksp - iterative context obtained from KSPCreate
-  fcn - orthogonalization function

   Calling Sequence of function:
$   errorcode = int fcn(KSP ksp,int it);
$   it is one minus the number of GMRES iterations since last restart;
$    i.e. the size of Krylov space minus one

   Notes:
   Two orthogonalization routines are predefined, including

   KSPGMRESModifiedGramSchmidtOrthogonalization()

   KSPGMRESClassicalGramSchmidtOrthogonalization() - Default. Use KSPGMRESSetCGSRefinementType() to determine if 
     iterative refinement is used to increase stability. 


   Options Database Keys:

+  -ksp_gmres_classicalgramschmidt - Activates KSPGMRESClassicalGramSchmidtOrthogonalization() (default)
-  -ksp_gmres_modifiedgramschmidt - Activates KSPGMRESModifiedGramSchmidtOrthogonalization()

   Level: intermediate

.keywords: KSP, GMRES, set, orthogonalization, Gram-Schmidt, iterative refinement

.seealso: KSPGMRESSetRestart(), KSPGMRESSetPreAllocateVectors(), KSPGMRESSetCGSRefinementType(),
          KSPGMRESModifiedGramSchmidtOrthogonalization(), KSPGMRESClassicalGramSchmidtOrthogonalization()
@*/
int KSPGMRESSetOrthogonalization(KSP ksp,int (*fcn)(KSP,int))
{
  int ierr,(*f)(KSP,int (*)(KSP,int));

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  ierr = PetscObjectQueryFunction((PetscObject)ksp,"KSPGMRESSetOrthogonalization_C",(void (**)(void))&f);CHKERRQ(ierr);
  if (f) {
    ierr = (*f)(ksp,fcn);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
