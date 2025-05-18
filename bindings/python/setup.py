import multiprocessing
import os
import subprocess
import sys
from pathlib import Path

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir: str = ""):
        super().__init__(name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuildCommand(build_ext):
    def run(self):
        for ext in self.extensions:
            self.build_cmake(ext)

    def build_cmake(self, ext: CMakeExtension):
        extdir = Path(self.get_ext_fullpath(ext.name)).parent.resolve()

        build_temp = Path(self.build_temp)
        build_temp.mkdir(parents=True, exist_ok=True)

        cmake_args = [
            "-DPYBUILD=ON",
            f"-DSETUPTOOLS_EXTDIR={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
        ]
        cfg = "Debug" if self.debug else "Release"
        cmake_args += [f"-DCMAKE_BUILD_TYPE={cfg}"]

        subprocess.check_call(["cmake", ext.sourcedir] + cmake_args, cwd=build_temp)
        subprocess.check_call(
            [
                "cmake",
                "--build",
                ".",
                "--target",
                "ailoy_py",
                "--parallel",
                f"{multiprocessing.cpu_count()}",
                "--config",
                cfg,
            ],
            cwd=build_temp,
        )


setup(
    name="ailoy",
    version="0.0.1",
    packages=["ailoy"],
    package_dir={"": "."},
    ext_modules=[CMakeExtension("ailoy.ailoy_py", "../../")],
    cmdclass={
        "build_ext": CMakeBuildCommand,
    },
    zip_safe=False,
)
