# --------------------------------------------------------------------

cdef extern from * nogil:

    PetscErrorCode PetscSectionCreate(MPI_Comm, PetscSection*)
    PetscErrorCode PetscSectionClone(PetscSection, PetscSection*)
    PetscErrorCode PetscSectionSetUp(PetscSection)
    PetscErrorCode PetscSectionSetUpBC(PetscSection)
    PetscErrorCode PetscSectionView(PetscSection, PetscViewer)
    PetscErrorCode PetscSectionReset(PetscSection)
    PetscErrorCode PetscSectionDestroy(PetscSection*)

    PetscErrorCode PetscSectionGetNumFields(PetscSection, PetscInt*)
    PetscErrorCode PetscSectionSetNumFields(PetscSection, PetscInt)
    PetscErrorCode PetscSectionGetFieldName(PetscSection, PetscInt, const char*[])
    PetscErrorCode PetscSectionSetFieldName(PetscSection, PetscInt, const char[])
    PetscErrorCode PetscSectionGetFieldComponents(PetscSection, PetscInt, PetscInt*)
    PetscErrorCode PetscSectionSetFieldComponents(PetscSection, PetscInt, PetscInt)
    PetscErrorCode PetscSectionGetChart(PetscSection, PetscInt*, PetscInt*)
    PetscErrorCode PetscSectionSetChart(PetscSection, PetscInt, PetscInt)
    PetscErrorCode PetscSectionGetPermutation(PetscSection, PetscIS*)
    PetscErrorCode PetscSectionSetPermutation(PetscSection, PetscIS)
    PetscErrorCode PetscSectionGetDof(PetscSection, PetscInt, PetscInt*)
    PetscErrorCode PetscSectionSetDof(PetscSection, PetscInt, PetscInt)
    PetscErrorCode PetscSectionAddDof(PetscSection, PetscInt, PetscInt)
    PetscErrorCode PetscSectionGetFieldDof(PetscSection, PetscInt, PetscInt, PetscInt*)
    PetscErrorCode PetscSectionSetFieldDof(PetscSection, PetscInt, PetscInt, PetscInt)
    PetscErrorCode PetscSectionAddFieldDof(PetscSection, PetscInt, PetscInt, PetscInt)
    PetscErrorCode PetscSectionGetConstraintDof(PetscSection, PetscInt, PetscInt*)
    PetscErrorCode PetscSectionSetConstraintDof(PetscSection, PetscInt, PetscInt)
    PetscErrorCode PetscSectionAddConstraintDof(PetscSection, PetscInt, PetscInt)
    PetscErrorCode PetscSectionGetFieldConstraintDof(PetscSection, PetscInt, PetscInt, PetscInt*)
    PetscErrorCode PetscSectionSetFieldConstraintDof(PetscSection, PetscInt, PetscInt, PetscInt)
    PetscErrorCode PetscSectionAddFieldConstraintDof(PetscSection, PetscInt, PetscInt, PetscInt)
    PetscErrorCode PetscSectionGetConstraintIndices(PetscSection, PetscInt, const PetscInt**)
    PetscErrorCode PetscSectionSetConstraintIndices(PetscSection, PetscInt, const PetscInt*)
    PetscErrorCode PetscSectionGetFieldConstraintIndices(PetscSection, PetscInt, PetscInt, const PetscInt**)
    PetscErrorCode PetscSectionSetFieldConstraintIndices(PetscSection, PetscInt, PetscInt, const PetscInt*)
    PetscErrorCode PetscSectionGetMaxDof(PetscSection, PetscInt*)
    PetscErrorCode PetscSectionGetStorageSize(PetscSection, PetscInt*)
    PetscErrorCode PetscSectionGetConstrainedStorageSize(PetscSection, PetscInt*)
    PetscErrorCode PetscSectionGetOffset(PetscSection, PetscInt, PetscInt*)
    PetscErrorCode PetscSectionSetOffset(PetscSection, PetscInt, PetscInt)
    PetscErrorCode PetscSectionGetFieldOffset(PetscSection, PetscInt, PetscInt, PetscInt*)
    PetscErrorCode PetscSectionSetFieldOffset(PetscSection, PetscInt, PetscInt, PetscInt)
    PetscErrorCode PetscSectionGetOffsetRange(PetscSection, PetscInt*, PetscInt*)
    PetscErrorCode PetscSectionCreateGlobalSection(PetscSection, PetscSF, PetscBool, PetscBool, PetscBool, PetscSection*)
    PetscErrorCode PetscSectionCreateSubsection(PetscSection, PetscInt, PetscInt[], PetscSection*)
    PetscErrorCode PetscSectionCreateSubmeshSection(PetscSection, IS, PetscSection*)
