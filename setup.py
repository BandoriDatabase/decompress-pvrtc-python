from distutils.core import setup, Extension

setup(
    ext_modules=[Extension("pvrtcdecompress", ["decompress.c", "fortseige_pvr.c", "etcdec.c"])],
)
