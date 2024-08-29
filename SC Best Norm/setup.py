from setuptools import setup, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        'normalization',
        ['sc_best_norm.cc'],
        extra_compile_args=['-O3','-mavx2'],
        extra_link_args = []
    ),
]

setup(
    name='normalization',
    ext_modules=ext_modules,
    cmdclass={'build_ext': build_ext},
)