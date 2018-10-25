/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include <aws/core/Aws.h>
#include <aws_common/sdk_utils/aws_error.h>
#include <gtest/gtest.h>
#include <kinesis-video-producer/KinesisVideoProducer.h>
#include <kinesis-video-producer/Logger.h>
#include <kinesis_manager/common.h>
#include <kinesis_manager/kinesis_stream_manager.h>
#include <kinesis_manager/stream_definition_provider.h>

LOGGER_TAG("aws.kinesis.kinesis_manager_unittest");

using namespace std;
using namespace Aws;
using namespace Aws::Kinesis;

/**
 * Parameter reader that sets the output using provided std::mapS.
 */
class TestParameterReader : public ParameterReaderInterface
{
public:
  TestParameterReader(map<string, int> int_map, map<string, bool> bool_map,
                      map<string, string> string_map, map<string, map<string, string>> map_map)
  : int_map_(int_map), bool_map_(bool_map), string_map_(string_map), map_map_(map_map)
  {
  }
  TestParameterReader(string test_prefix)
  {
    int_map_ = {
      {test_prefix + "retention_period", 2},
      {test_prefix + "streaming_type", 0},
      {test_prefix + "max_latency", 0},
      {test_prefix + "fragment_duration", 2},
      {test_prefix + "timecode_scale", 1},
      {test_prefix + "nal_adaptation_flags",
       NAL_ADAPTATION_ANNEXB_NALS | NAL_ADAPTATION_ANNEXB_CPD_NALS},
      {test_prefix + "frame_rate", 24},
      {test_prefix + "avg_bandwidth_bps", 4 * 1024 * 1024},
      {test_prefix + "buffer_duration", 120},
      {test_prefix + "replay_duration", 40},
      {test_prefix + "connection_staleness", 30},
    };
    bool_map_ = {
      {test_prefix + "key_frame_fragmentation", true}, {test_prefix + "frame_timecodes", true},
      {test_prefix + "absolute_fragment_time", true},  {test_prefix + "fragment_acks", true},
      {test_prefix + "restart_on_error", true},        {test_prefix + "recalculate_metrics", true},
    };
    string_map_ = {
      {test_prefix + "stream_name", "testStream"},   {test_prefix + "kms_key_id", ""},
      {test_prefix + "content_type", "video/h264"},  {test_prefix + "codec_id", "V_MPEG4/ISO/AVC"},
      {test_prefix + "track_name", "kinesis_video"},
    };
    map_map_ = {
      {test_prefix + "tags", {{"someKey", "someValue"}}},
    };
  }
  AwsError ReadInt(const char * name, int & out) const
  {
    AwsError result = AWS_ERR_NOT_FOUND;
    if (int_map_.count(name) > 0) {
      out = int_map_.at(name);
      result = AWS_ERR_OK;
    }
    return result;
  }
  AwsError ReadBool(const char * name, bool & out) const
  {
    AwsError result = AWS_ERR_NOT_FOUND;
    if (bool_map_.count(name) > 0) {
      out = bool_map_.at(name);
      result = AWS_ERR_OK;
    }
    return result;
  }
  AwsError ReadStdString(const char * name, string & out) const
  {
    AwsError result = AWS_ERR_NOT_FOUND;
    if (string_map_.count(name) > 0) {
      out = string_map_.at(name);
      result = AWS_ERR_OK;
    }
    return result;
  }
  AwsError ReadString(const char * name, Aws::String & out) const { return AWS_ERR_EMPTY; }
  AwsError ReadMap(const char * name, map<string, string> & out) const
  {
    AwsError result = AWS_ERR_NOT_FOUND;
    if (map_map_.count(name) > 0) {
      out = map_map_.at(name);
      result = AWS_ERR_OK;
    }
    return result;
  }
  AwsError ReadList(const char * name, std::vector<std::string> & out) const
  {
    return AWS_ERR_EMPTY;
  }
  AwsError ReadDouble(const char * name, double & out) const { return AWS_ERR_EMPTY; }

  map<string, int> int_map_;
  map<string, bool> bool_map_;
  map<string, string> string_map_;
  map<string, map<string, string>> map_map_;
};

/**
 * Tests stream definitions for equivalence.
 * @param stream1
 * @param stream2
 * @return true if the streams are equivalent, false otherwise.
 */
static bool are_streams_equivalent(unique_ptr<StreamDefinition> stream1,
                                   unique_ptr<StreamDefinition> stream2)
{
  bool result = true;
  StreamInfo stream1_info = stream1->getStreamInfo();
  StreamInfo stream2_info = stream2->getStreamInfo();
  /**
   * Compare complex structures first
   */
  if (stream1_info.streamCaps.codecPrivateDataSize !=
      stream2_info.streamCaps.codecPrivateDataSize) {
    return false;
  } else {
    result &= (0 == memcmp((void *)&(stream1_info.streamCaps.codecPrivateData),
                           (void *)&(stream2_info.streamCaps.codecPrivateData),
                           stream1_info.streamCaps.codecPrivateDataSize));
  }
  if (stream1_info.tagCount != stream2_info.tagCount) {
    return false;
  } else {
    for (int tag_idx = 0; tag_idx < stream1_info.tagCount; tag_idx++) {
      result &= (stream1_info.tags[tag_idx].version == stream2_info.tags[tag_idx].version);
      result &= (0 == strncmp(stream1_info.tags[tag_idx].name, stream2_info.tags[tag_idx].name,
                              MAX_TAG_NAME_LEN));
      result &= (0 == strncmp(stream1_info.tags[tag_idx].value, stream2_info.tags[tag_idx].value,
                              MAX_TAG_VALUE_LEN));
    }
  }
  /**
   * Zero out pointers contained within the structs and use memcmp.
   */
  stream2_info.streamCaps.codecPrivateData = nullptr;
  stream1_info.streamCaps.codecPrivateData = nullptr;
  stream1_info.tags = nullptr;
  stream2_info.tags = nullptr;
  result &= (0 == memcmp((void *)&(stream1_info), (void *)&(stream2_info), sizeof(stream1_info)));
  return result;
}

/**
 * Tests that GetCodecPrivateData successfully reads and decodes the given base64-encoded buffer.
 */
TEST(StreamDefinitionProviderSuite, getCodecPrivateDataTest)
{
  string test_prefix = "some/test/prefix";
  Aws::Kinesis::StreamDefinitionProvider stream_definition_provider;

  string decoded_string = "hello world";
  string encoded_string = "aGVsbG8gd29ybGQ=";
  map<string, int> int_map = {};
  map<string, bool> bool_map = {};
  map<string, string> tags;
  map<string, map<string, string>> map_map = {};
  map<string, string> string_map = {
    {test_prefix + "codecPrivateData", encoded_string},
  };
  TestParameterReader parameter_reader(int_map, bool_map, string_map, map_map);

  PBYTE codec_private_data;
  uint32_t codec_private_data_size;
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_SUCCEEDED(stream_definition_provider.GetCodecPrivateData(
    test_prefix.c_str(), parameter_reader, &codec_private_data, &codec_private_data_size)));
  ASSERT_EQ(decoded_string.length(), codec_private_data_size);
  ASSERT_TRUE(0 == strncmp(decoded_string.c_str(), (const char *)codec_private_data,
                           codec_private_data_size));

  /* Invalid input tests */
  KinesisManagerStatus status = stream_definition_provider.GetCodecPrivateData(
    nullptr, parameter_reader, &codec_private_data, &codec_private_data_size);
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_FAILED(status) &&
              KINESIS_MANAGER_STATUS_INVALID_INPUT == status);
  status = stream_definition_provider.GetCodecPrivateData(test_prefix.c_str(), parameter_reader,
                                                          nullptr, &codec_private_data_size);
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_FAILED(status) &&
              KINESIS_MANAGER_STATUS_INVALID_INPUT == status);
  status = stream_definition_provider.GetCodecPrivateData(test_prefix.c_str(), parameter_reader,
                                                          &codec_private_data, nullptr);
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_FAILED(status) &&
              KINESIS_MANAGER_STATUS_INVALID_INPUT == status);

  /* Empty input */
  string_map = {};
  TestParameterReader empty_parameter_reader(int_map, bool_map, string_map, map_map);
  codec_private_data = nullptr;
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_SUCCEEDED(stream_definition_provider.GetCodecPrivateData(
                test_prefix.c_str(), empty_parameter_reader, &codec_private_data,
                &codec_private_data_size)) &&
              !codec_private_data);

  /* Dependency failure */
  string_map = {
    {test_prefix + "codecPrivateData", "1"},
  };
  TestParameterReader parameter_reader_with_invalid_values(int_map, bool_map, string_map, map_map);
  status = stream_definition_provider.GetCodecPrivateData(
    test_prefix.c_str(), parameter_reader_with_invalid_values, &codec_private_data,
    &codec_private_data_size);
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_FAILED(status) &&
              KINESIS_MANAGER_STATUS_BASE64DECODE_FAILED == status);
}

/**
 * Tests that GetStreamDefinition returns the expected StreamDefinition object by comparing it to a
 * manually created StreamDefinition.
 */
TEST(StreamDefinitionProviderSuite, getStreamDefinitionTest)
{
  string test_prefix = "some/test/prefix";
  Aws::Kinesis::StreamDefinitionProvider stream_definition_provider;

  TestParameterReader parameter_reader = TestParameterReader(test_prefix);
  map<string, string> string_map = parameter_reader.string_map_;
  map<string, bool> bool_map = parameter_reader.bool_map_;
  map<string, int> int_map = parameter_reader.int_map_;
  map<string, map<string, string>> map_map = parameter_reader.map_map_;

  unique_ptr<StreamDefinition> generated_stream_definition =
    stream_definition_provider.GetStreamDefinition(test_prefix.c_str(), parameter_reader, nullptr,
                                                   0);
  auto equivalent_stream_definition = make_unique<StreamDefinition>(
    string_map[test_prefix + "stream_name"], hours(int_map[test_prefix + "retention_period"]),
    &map_map[test_prefix + "tags"], string_map[test_prefix + "kms_key_id"],
    static_cast<STREAMING_TYPE>(int_map[test_prefix + "streaming_type"]),
    string_map[test_prefix + "content_type"], milliseconds(int_map[test_prefix + "max_latency"]),
    seconds(int_map[test_prefix + "fragment_duration"]),
    milliseconds(int_map[test_prefix + "timecode_scale"]),
    bool_map[test_prefix + "key_frame_fragmentation"], bool_map[test_prefix + "frame_timecodes"],
    bool_map[test_prefix + "absolute_fragment_time"], bool_map[test_prefix + "fragment_acks"],
    bool_map[test_prefix + "restart_on_error"], bool_map[test_prefix + "recalculate_metrics"],
    static_cast<NAL_ADAPTATION_FLAGS>(int_map[test_prefix + "nal_adaptation_flags"]),
    int_map[test_prefix + "frame_rate"], int_map[test_prefix + "avg_bandwidth_bps"],
    seconds(int_map[test_prefix + "buffer_duration"]),
    seconds(int_map[test_prefix + "replay_duration"]),
    seconds(int_map[test_prefix + "connection_staleness"]), string_map[test_prefix + "codec_id"],
    string_map[test_prefix + "track_name"], nullptr, 0);
  ASSERT_TRUE(
    are_streams_equivalent(move(equivalent_stream_definition), move(generated_stream_definition)));

  auto different_stream_definition = make_unique<StreamDefinition>(
    string_map[test_prefix + "stream_name"], hours(int_map[test_prefix + "retention_period"]),
    &map_map[test_prefix + "tags"], string_map[test_prefix + "kms_key_id"],
    static_cast<STREAMING_TYPE>(int_map[test_prefix + "streaming_type"]),
    string_map[test_prefix + "content_type"], milliseconds(int_map[test_prefix + "max_latency"]),
    seconds(int_map[test_prefix + "fragment_duration"]),
    milliseconds(int_map[test_prefix + "timecode_scale"]),
    bool_map[test_prefix + "key_frame_fragmentation"], bool_map[test_prefix + "frame_timecodes"],
    bool_map[test_prefix + "absolute_fragment_time"], bool_map[test_prefix + "fragment_acks"],
    bool_map[test_prefix + "restart_on_error"], bool_map[test_prefix + "recalculate_metrics"],
    static_cast<NAL_ADAPTATION_FLAGS>(int_map[test_prefix + "nal_adaptation_flags"]), 4914918,
    int_map[test_prefix + "avg_bandwidth_bps"], seconds(int_map[test_prefix + "buffer_duration"]),
    seconds(int_map[test_prefix + "replay_duration"]),
    seconds(int_map[test_prefix + "connection_staleness"]), string_map[test_prefix + "codec_id"],
    string_map[test_prefix + "track_name"], nullptr, 0);
  generated_stream_definition = stream_definition_provider.GetStreamDefinition(
    test_prefix.c_str(), parameter_reader, nullptr, 0);
  ASSERT_FALSE(
    are_streams_equivalent(move(different_stream_definition), move(generated_stream_definition)));

  /* Invalid input tests */
  generated_stream_definition =
    stream_definition_provider.GetStreamDefinition(nullptr, parameter_reader, nullptr, 0);
  ASSERT_FALSE(generated_stream_definition);
  generated_stream_definition = stream_definition_provider.GetStreamDefinition(
    test_prefix.c_str(), parameter_reader, nullptr, 100);
  ASSERT_FALSE(generated_stream_definition);
}

/**
 * Initializes the video producer and generates a basic stream definition.
 */
unique_ptr<StreamDefinition> DefaultProducerSetup(
  Aws::Kinesis::KinesisStreamManager & stream_manager, string region, string test_prefix)
{
#ifdef PLATFORM_TESTING_ACCESS_KEY
  setenv("AWS_ACCESS_KEY_ID", PLATFORM_TESTING_ACCESS_KEY, 1);
#endif
#ifdef PLATFORM_TESTING_SECRET_KEY
  setenv("AWS_SECRET_ACCESS_KEY", PLATFORM_TESTING_SECRET_KEY, 1);
#endif
  stream_manager.InitializeVideoProducer(region);

  Aws::Kinesis::StreamDefinitionProvider stream_definition_provider;
  TestParameterReader parameter_reader = TestParameterReader(test_prefix);
  unique_ptr<StreamDefinition> stream_definition = stream_definition_provider.GetStreamDefinition(
    test_prefix.c_str(), parameter_reader, nullptr, 0);
  return move(stream_definition);
}

/**
 * Tests the InitializeVideoProducer function.
 */
TEST(KinesisStreamManagerSuite, videoInitializationTest)
{
  string test_prefix = "some/test/prefix";
  Aws::Kinesis::KinesisStreamManager stream_manager;

  KinesisManagerStatus status = stream_manager.InitializeVideoProducer("us-west-2");
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_SUCCEEDED(status));
  ASSERT_TRUE(stream_manager.get_video_producer());

  /* Duplicate initialization */
  KinesisVideoProducer * video_producer = stream_manager.get_video_producer();
  status = stream_manager.InitializeVideoProducer("us-west-2");
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_FAILED(status) &&
              KINESIS_MANAGER_STATUS_VIDEO_PRODUCER_ALREADY_INITIALIZED == status);
  KinesisVideoProducer * video_producer_post_call = stream_manager.get_video_producer();
  ASSERT_EQ(video_producer, video_producer_post_call);
}

#ifdef BUILD_AWS_TESTING
// the following tests perform AWS API calls and require user confiugration
// to enable them run: colcon build --cmake-args -DBUILD_AWS_TESTING=1

/**
 * Tests the InitializeVideoStream function. This will attempt to create and load a test stream in
 * the test account.
 */
TEST(KinesisStreamManagerSuite, streamInitializationTest)
{
  Aws::Kinesis::KinesisStreamManager stream_manager;
  /* Before calling InitializeVideoProducer */
  KinesisManagerStatus status =
    stream_manager.InitializeVideoStream(move(unique_ptr<StreamDefinition>()));
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_FAILED(status) &&
              KINESIS_MANAGER_STATUS_VIDEO_PRODUCER_NOT_INITIALIZED == status);

  ASSERT_FALSE(stream_manager.get_video_producer());
  unique_ptr<StreamDefinition> stream_definition =
    DefaultProducerSetup(stream_manager, string("us-west-2"), string("stream/test"));
  ASSERT_TRUE(stream_manager.get_video_producer());

  /* Video producer has been created but the stream definition is empty. */
  status = stream_manager.InitializeVideoStream(unique_ptr<StreamDefinition>{});
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_FAILED(status) &&
              KINESIS_MANAGER_STATUS_INVALID_INPUT == status);

  status = stream_manager.InitializeVideoStream(move(stream_definition));
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_SUCCEEDED(status));
}

/**
 * Tests the PutFrame function. This will load the test stream and attempt to transmit a dummy frame
 * to it.
 */
TEST(KinesisStreamManagerSuite, putFrameTest)
{
  Aws::Kinesis::KinesisStreamManager stream_manager;
  Frame frame;
  string stream_name("testStream");
  /* Before calling InitializeVideoProducer */
  KinesisManagerStatus status = stream_manager.PutFrame(stream_name, frame);
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_FAILED(status) &&
              KINESIS_MANAGER_STATUS_VIDEO_PRODUCER_NOT_INITIALIZED == status);

  /* Stream name not found (i.e. before calling InitializeVideoStream) */
  unique_ptr<StreamDefinition> stream_definition =
    DefaultProducerSetup(stream_manager, string("us-west-2"), string("frame/test"));
  status = stream_manager.PutFrame(string(stream_name), frame);
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_FAILED(status) &&
              KINESIS_MANAGER_STATUS_PUTFRAME_STREAM_NOT_FOUND == status);

  status = stream_manager.InitializeVideoStream(move(stream_definition));
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_SUCCEEDED(status));

  /* Invalid frame */
  frame.size = 0;
  status = stream_manager.PutFrame(stream_name, frame);
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_FAILED(status) &&
              KINESIS_MANAGER_STATUS_PUTFRAME_FAILED == status);

  /* Valid (but dummy) frame */
  frame.size = 4;
  std::vector<uint8_t> bytes = {0x00, 0x01, 0x02, 0x03};
  frame.frameData = reinterpret_cast<PBYTE>((void *)(bytes.data()));
  frame.duration = 5000000;
  frame.index = 1;
  UINT64 timestamp = 0;
  timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count() /
              DEFAULT_TIME_UNIT_IN_NANOS;
  frame.decodingTs = timestamp;
  frame.presentationTs = timestamp;
  frame.flags = (FRAME_FLAGS)0;

  status = stream_manager.PutFrame(stream_name, frame);
  ASSERT_TRUE(KINESIS_MANAGER_STATUS_SUCCEEDED(status));
}
#endif

int main(int argc, char ** argv)
{
  LOG_CONFIGURE_STDOUT("ERROR");
  Aws::SDKOptions options;
  Aws::InitAPI(options);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
