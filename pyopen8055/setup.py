from setuptools import setup

setup(name='pyopen8055',
      description='Open8055 and Velleman K8055(N) access module',
      url='https://github.com/wieck/open8055',
      version='0.1',
      author='Jan Wieck',
      author_email='jan@wi3ck.info',
      license='BSD',
      install_requires = [
          "libusb1",
      ],
      packages = ['pyopen8055'],
      )
