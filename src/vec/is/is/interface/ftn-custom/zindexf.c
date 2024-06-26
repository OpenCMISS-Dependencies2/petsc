#include <petsc/private/fortranimpl.h>
#include <petscis.h>
#include <petscviewer.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
  #define isgetindices_                              ISGETINDICES
  #define isrestoreindices_                          ISRESTOREINDICES
  #define isgettotalindices_                         ISGETTOTALINDICES
  #define isrestoretotalindices_                     ISRESTORETOTALINDICES
  #define isgetnonlocalindices_                      ISGETNONLOCALINDICES
  #define isrestorenonlocalindices_                  ISRESTORENONLOCALINDICES
  #define islocaltoglobalmappinggetindices_          ISLOCALTOGLOBALMAPPINGGETINDICES
  #define islocaltoglobalmappingrestoreindices_      ISLOCALTOGLOBALMAPPINGRESTOREINDICES
  #define islocaltoglobalmappinggetblockindices_     ISLOCALTOGLOBALMAPPINGGETBLOCKINDICES
  #define islocaltoglobalmappingrestoreblockindices_ ISLOCALTOGLOBALMAPPINGRESTOREBLOCKINDICES
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
  #define isgetindices_                              isgetindices
  #define isrestoreindices_                          isrestoreindices
  #define isgettotalindices_                         isgettotalindices
  #define isrestoretotalindices_                     isrestoretotalindices
  #define isgetnonlocalindices_                      isgetnonlocalindices
  #define isrestorenonlocalindices_                  isrestorenonlocalindices
  #define islocaltoglobalmappinggetindices_          islocaltoglobalmappinggetindices
  #define islocaltoglobalmappingrestoreindices_      islocaltoglobalmappingrestoreindices
  #define islocaltoglobalmappinggetblockindices_     islocaltoglobalmappinggetblockindices
  #define islocaltoglobalmappingrestoreblockindices_ islocaltoglobalmappingrestoreblockindices
#endif

PETSC_EXTERN void isgetindices_(IS *x, PetscInt *fa, size_t *ia, PetscErrorCode *ierr)
{
  const PetscInt *lx;

  *ierr = ISGetIndices(*x, &lx);
  if (*ierr) return;
  *ia = PetscIntAddressToFortran(fa, (PetscInt *)lx);
}

PETSC_EXTERN void isrestoreindices_(IS *x, PetscInt *fa, size_t *ia, PetscErrorCode *ierr)
{
  const PetscInt *lx = PetscIntAddressFromFortran(fa, *ia);
  *ierr              = ISRestoreIndices(*x, &lx);
}

PETSC_EXTERN void isgettotalindices_(IS *x, PetscInt *fa, size_t *ia, PetscErrorCode *ierr)
{
  const PetscInt *lx;

  *ierr = ISGetTotalIndices(*x, &lx);
  if (*ierr) return;
  *ia = PetscIntAddressToFortran(fa, (PetscInt *)lx);
}

PETSC_EXTERN void isrestoretotalindices_(IS *x, PetscInt *fa, size_t *ia, PetscErrorCode *ierr)
{
  const PetscInt *lx = PetscIntAddressFromFortran(fa, *ia);
  *ierr              = ISRestoreTotalIndices(*x, &lx);
}

PETSC_EXTERN void isgetnonlocalindices_(IS *x, PetscInt *fa, size_t *ia, PetscErrorCode *ierr)
{
  const PetscInt *lx;

  *ierr = ISGetNonlocalIndices(*x, &lx);
  if (*ierr) return;
  *ia = PetscIntAddressToFortran(fa, (PetscInt *)lx);
}

PETSC_EXTERN void isrestorenonlocalindices_(IS *x, PetscInt *fa, size_t *ia, PetscErrorCode *ierr)
{
  const PetscInt *lx = PetscIntAddressFromFortran(fa, *ia);
  *ierr              = ISRestoreNonlocalIndices(*x, &lx);
}

PETSC_EXTERN void islocaltoglobalmappinggetindices_(ISLocalToGlobalMapping *x, PetscInt *fa, size_t *ia, PetscErrorCode *ierr)
{
  const PetscInt *lx;

  *ierr = ISLocalToGlobalMappingGetIndices(*x, &lx);
  if (*ierr) return;
  *ia = PetscIntAddressToFortran(fa, (PetscInt *)lx);
}

PETSC_EXTERN void islocaltoglobalmappingrestoreindices_(ISLocalToGlobalMapping *x, PetscInt *fa, size_t *ia, PetscErrorCode *ierr)
{
  const PetscInt *lx = PetscIntAddressFromFortran(fa, *ia);
  *ierr              = ISLocalToGlobalMappingRestoreIndices(*x, &lx);
}

PETSC_EXTERN void islocaltoglobalmappinggetblockindices_(ISLocalToGlobalMapping *x, PetscInt *fa, size_t *ia, PetscErrorCode *ierr)
{
  const PetscInt *lx;

  *ierr = ISLocalToGlobalMappingGetBlockIndices(*x, &lx);
  if (*ierr) return;
  *ia = PetscIntAddressToFortran(fa, (PetscInt *)lx);
}

PETSC_EXTERN void islocaltoglobalmappingrestoreblockindices_(ISLocalToGlobalMapping *x, PetscInt *fa, size_t *ia, PetscErrorCode *ierr)
{
  const PetscInt *lx = PetscIntAddressFromFortran(fa, *ia);
  *ierr              = ISLocalToGlobalMappingRestoreBlockIndices(*x, &lx);
}
