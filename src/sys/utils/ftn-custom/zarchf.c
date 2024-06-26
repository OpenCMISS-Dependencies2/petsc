#include <petsc/private/fortranimpl.h>
#include <petscsys.h>
#if defined(PETSC_HAVE_FORTRAN_CAPS)
  #define petscgetarchtype_ PETSCGETARCHTYPE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
  #define petscgetarchtype_ petscgetarchtype
#endif

PETSC_EXTERN void petscgetarchtype_(char *str, PetscErrorCode *ierr, PETSC_FORTRAN_CHARLEN_T len)
{
  char  *tstr;
  size_t tlen;
  tstr  = str;
  tlen  = len; /* int to size_t */
  *ierr = PetscGetArchType(tstr, tlen);
  FIXRETURNCHAR(PETSC_TRUE, str, len);
}
