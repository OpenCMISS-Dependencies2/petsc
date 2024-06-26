#include <petsc/private/fortranimpl.h>
#include <petscksp.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
  #define pcasmgetsubksp1_          PCASMGETSUBKSP1
  #define pcasmgetsubksp2_          PCASMGETSUBKSP2
  #define pcasmgetsubksp3_          PCASMGETSUBKSP3
  #define pcasmgetsubksp4_          PCASMGETSUBKSP4
  #define pcasmgetsubksp5_          PCASMGETSUBKSP5
  #define pcasmgetsubksp6_          PCASMGETSUBKSP6
  #define pcasmgetsubksp7_          PCASMGETSUBKSP7
  #define pcasmgetsubksp8_          PCASMGETSUBKSP8
  #define pcasmgetlocalsubmatrices_ PCASMGETLOCALSUBMATRICES
  #define pcasmgetlocalsubdomains_  PCASMGETLOCALSUBDOMAINS
  #define pcasmcreatesubdomains_    PCASMCREATESUBDOMAINS
  #define pcasmdestroysubdomains_   PCASMDESTROYSUBDOMAINS
  #define pcasmcreatesubdomains2d_  PCASMCREATESUBDOMAINS2D
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
  #define pcasmgetsubksp1_          pcasmgetsubksp1
  #define pcasmgetsubksp2_          pcasmgetsubksp2
  #define pcasmgetsubksp3_          pcasmgetsubksp3
  #define pcasmgetsubksp4_          pcasmgetsubksp4
  #define pcasmgetsubksp5_          pcasmgetsubksp5
  #define pcasmgetsubksp6_          pcasmgetsubksp6
  #define pcasmgetsubksp7_          pcasmgetsubksp7
  #define pcasmgetsubksp8_          pcasmgetsubksp8
  #define pcasmgetlocalsubmatrices_ pcasmgetlocalsubmatrices
  #define pcasmgetlocalsubdomains_  pcasmgetlocalsubdomains
  #define pcasmcreatesubdomains_    pcasmcreatesubdomains
  #define pcasmdestroysubdomains_   pcasmdestroysubdomains
  #define pcasmcreatesubdomains2d_  pcasmcreatesubdomains2d
#endif

PETSC_EXTERN void pcasmcreatesubdomains2d_(PetscInt *m, PetscInt *n, PetscInt *M, PetscInt *N, PetscInt *dof, PetscInt *overlap, PetscInt *Nsub, IS *is, IS *isl, int *ierr)
{
  IS *iis, *iisl;
  *ierr = PCASMCreateSubdomains2D(*m, *n, *M, *N, *dof, *overlap, Nsub, &iis, &iisl);
  if (*ierr) return;
  *ierr = PetscMemcpy(is, iis, *Nsub * sizeof(IS));
  if (*ierr) return;
  *ierr = PetscMemcpy(isl, iisl, *Nsub * sizeof(IS));
  if (*ierr) return;
  *ierr = PetscFree(iis);
  if (*ierr) return;
  *ierr = PetscFree(iisl);
}

PETSC_EXTERN void pcasmcreatesubdomains_(Mat *mat, PetscInt *n, IS *subs, PetscErrorCode *ierr)
{
  PetscInt i;
  IS      *insubs;

  *ierr = PCASMCreateSubdomains(*mat, *n, &insubs);
  if (*ierr) return;
  for (i = 0; i < *n; i++) subs[i] = insubs[i];
  *ierr = PetscFree(insubs);
}

PETSC_EXTERN void pcasmdestroysubdomains_(PetscInt *n, IS *subs, IS *isubs, PetscErrorCode *ierr)
{
  PetscInt i;

  for (i = 0; i < *n; i++) {
    *ierr = ISDestroy(&subs[i]);
    if (*ierr) return;
    *ierr = ISDestroy(&isubs[i]);
    if (*ierr) return;
  }
}

PETSC_EXTERN void pcasmgetsubksp1_(PC *pc, PetscInt *n_local, PetscInt *first_local, KSP *ksp, PetscErrorCode *ierr)
{
  KSP     *tksp;
  PetscInt i, nloc;
  CHKFORTRANNULLINTEGER(n_local);
  CHKFORTRANNULLINTEGER(first_local);
  CHKFORTRANNULLOBJECT(ksp);
  *ierr = PCASMGetSubKSP(*pc, &nloc, first_local, &tksp);
  if (n_local) *n_local = nloc;
  if (ksp) {
    for (i = 0; i < nloc; i++) ksp[i] = tksp[i];
  }
}

PETSC_EXTERN void pcasmgetsubksp2_(PC *pc, PetscInt *n_local, PetscInt *first_local, KSP *ksp, PetscErrorCode *ierr)
{
  pcasmgetsubksp1_(pc, n_local, first_local, ksp, ierr);
}

PETSC_EXTERN void pcasmgetsubksp3_(PC *pc, PetscInt *n_local, PetscInt *first_local, KSP *ksp, PetscErrorCode *ierr)
{
  pcasmgetsubksp1_(pc, n_local, first_local, ksp, ierr);
}

PETSC_EXTERN void pcasmgetsubksp4_(PC *pc, PetscInt *n_local, PetscInt *first_local, KSP *ksp, PetscErrorCode *ierr)
{
  pcasmgetsubksp1_(pc, n_local, first_local, ksp, ierr);
}

PETSC_EXTERN void pcasmgetsubksp5_(PC *pc, PetscInt *n_local, PetscInt *first_local, KSP *ksp, PetscErrorCode *ierr)
{
  pcasmgetsubksp1_(pc, n_local, first_local, ksp, ierr);
}

PETSC_EXTERN void pcasmgetsubksp6_(PC *pc, PetscInt *n_local, PetscInt *first_local, KSP *ksp, PetscErrorCode *ierr)
{
  pcasmgetsubksp1_(pc, n_local, first_local, ksp, ierr);
}

PETSC_EXTERN void pcasmgetsubksp7_(PC *pc, PetscInt *n_local, PetscInt *first_local, KSP *ksp, PetscErrorCode *ierr)
{
  pcasmgetsubksp1_(pc, n_local, first_local, ksp, ierr);
}

PETSC_EXTERN void pcasmgetsubksp8_(PC *pc, PetscInt *n_local, PetscInt *first_local, KSP *ksp, PetscErrorCode *ierr)
{
  pcasmgetsubksp1_(pc, n_local, first_local, ksp, ierr);
}

PETSC_EXTERN void pcasmgetlocalsubmatrices_(PC *pc, PetscInt *n, Mat *mat, PetscErrorCode *ierr)
{
  PetscInt nloc, i;
  Mat     *tmat;
  CHKFORTRANNULLOBJECT(mat);
  CHKFORTRANNULLINTEGER(n);
  *ierr = PCASMGetLocalSubmatrices(*pc, &nloc, &tmat);
  if (n) *n = nloc;
  if (mat) {
    for (i = 0; i < nloc; i++) mat[i] = tmat[i];
  }
}
PETSC_EXTERN void pcasmgetlocalsubdomains_(PC *pc, PetscInt *n, IS *is, IS *is_local, PetscErrorCode *ierr)
{
  PetscInt nloc, i;
  IS      *tis, *tis_local;
  CHKFORTRANNULLOBJECT(is);
  CHKFORTRANNULLOBJECT(is_local);
  CHKFORTRANNULLINTEGER(n);
  *ierr = PCASMGetLocalSubdomains(*pc, &nloc, &tis, &tis_local);
  if (n) *n = nloc;
  if (is) {
    for (i = 0; i < nloc; i++) is[i] = tis[i];
  }
  if (is_local && tis_local) {
    for (i = 0; i < nloc; i++) is_local[i] = tis_local[i];
  }
}
