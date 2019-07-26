^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package kinesis_manager
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

2.0.1 (2019-07-26)
------------------
* Merge pull request `#32 <https://github.com/aws-robotics/kinesisvideo-common/issues/32>`_ from aws-robotics/bump-version-2-0-1
  Bump version
* Bump version
  md5 sum was changed. Bumping version in preparation for release.
* Update md5 checksum for KVS SDK (`#27 <https://github.com/aws-robotics/kinesisvideo-common/issues/27>`_)
* Use standard CMake macros for adding gtest/gmock tests (`#24 <https://github.com/aws-robotics/kinesisvideo-common/issues/24>`_)
  * use the macro in aws_common to find test dependencies for ROS1 or ROS2
  Signed-off-by: Miaofei <miaofei@amazon.com>
  * update travis.yml to be compatible with specifying multiple package names
  Signed-off-by: Miaofei <miaofei@amazon.com>
  * update travis.yml test matrix
  Signed-off-by: Miaofei <miaofei@amazon.com>
  * update PACKAGE_NAMES
  Signed-off-by: Miaofei <miaofei@amazon.com>
* update kvs producer sdk to v1.7.8
* Improve unit tests coverage
  * Add KinesisVideoProducerInterface and
  KinesisVideoStreamInterface in order to use
  mocks for Kinesis Video Streams
  * Resulting in the following overall coverage rate:
  ```
  lines......: 88.8% (906 of 1020 lines)
  functions..: 86.0% (208 of 242 functions)
  branches...: 38.0% (1346 of 3544 branches)
  ```
* Update to use non-legacy ParameterReader API (`#12 <https://github.com/aws-robotics/kinesisvideo-common/issues/12>`_)
* Update to use new ParameterReader API (`#11 <https://github.com/aws-robotics/kinesisvideo-common/issues/11>`_)
  * clean up CMakeFiles
  * Adjusting usage of the ParameterReader due to the updated API taking in a ParameterPath.
  * refactor based on new ParameterPath object design
  * increment major version number in package.xml
* Merge pull request `#5 <https://github.com/aws-robotics/kinesisvideo-common/issues/5>`_ from mm318/master
  Executable permission for script files
* fix permission issues when this repo is zipped
* Contributors: Juan Rodriguez Hortala, M. M, Miaofei, Ross Desmond, Tim Robinson
