from types import ModuleType


def ImportPETSc(arch: str | None = ...) -> ModuleType: ...
def getPathArchPETSc(arch: str | None = ...) -> tuple[str, str]: ...


def Import(pkg: str, name: str, path: str, arch: str) -> ModuleType: ...
def getPathArch(path: str, arch: str, rcvar: str = ..., rcfile: str = ...) -> tuple[str, str]: ...
def getInitArgs(args: str | list[str] | None) -> list[str]: ...
