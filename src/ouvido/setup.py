from setuptools import find_packages, setup

package_name = 'ouvido'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=[
        'setuptools',
        'vosk',
        'sounddevice',
        #'rapidfuzz',
        'pyyaml',
    ],
    zip_safe=True,
    maintainer='natan',
    maintainer_email='natanssousassiva@gmail.com',
    description='Nó ROS2 usando Vosk para reconhecimento de fala',
    license='Apache-2.0',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
             'ouvido = ouvido.ouvido_node:main',
        ],
    },
)
