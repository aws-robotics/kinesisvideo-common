^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package kinesis_manager
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

2.0.0 (2019-03-19)
------------------
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
* Contributors: Juan Rodriguez Hortala, M. M, Miaofei, Ross Desmond
