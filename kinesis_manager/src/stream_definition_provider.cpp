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
#include <aws_common/sdk_utils/client_configuration_provider.h>
#include <com/amazonaws/kinesis/video/utils/Include.h>
#include <kinesis_manager/stream_definition_provider.h>

#define STREAM_DEFINITION_MAX_CODEC_PRIVATE_DATA_SIZE (1024)

using namespace com::amazonaws::kinesis::video;

namespace Aws {
namespace Kinesis {

KinesisManagerStatus StreamDefinitionProvider::GetCodecPrivateData(
  const char * c_prefix, const ParameterReaderInterface & reader, PBYTE * out_codec_private_data,
  uint32_t * out_codec_private_data_size) const
{
  if (nullptr == c_prefix || nullptr == out_codec_private_data ||
      nullptr == out_codec_private_data_size) {
    return KINESIS_MANAGER_STATUS_INVALID_INPUT;
  }
  std::string prefix(c_prefix);
  std::string b64_encoded_codec_private_data;
  reader.ReadStdString((prefix + "codecPrivateData").c_str(), b64_encoded_codec_private_data);
  if (!b64_encoded_codec_private_data.empty()) {
    uint8_t temp_codec_data[STREAM_DEFINITION_MAX_CODEC_PRIVATE_DATA_SIZE] = {0};
    uint32_t decoded_buffer_size = sizeof(temp_codec_data);
    if (STATUS_SUCCESS != base64Decode(const_cast<char *>(b64_encoded_codec_private_data.c_str()),
                                       temp_codec_data, &decoded_buffer_size)) {
      return KINESIS_MANAGER_STATUS_BASE64DECODE_FAILED;
    }
    PBYTE codec_private_data = (PBYTE)malloc(decoded_buffer_size);
    if (nullptr == codec_private_data) {
      return KINESIS_MANAGER_STATUS_MALLOC_FAILED;
    }
    memset(codec_private_data, 0, decoded_buffer_size);
    memcpy(codec_private_data, temp_codec_data, decoded_buffer_size);
    *out_codec_private_data = codec_private_data;
    *out_codec_private_data_size = decoded_buffer_size;
  }
  return KINESIS_MANAGER_STATUS_SUCCESS;
}

unique_ptr<StreamDefinition> StreamDefinitionProvider::GetStreamDefinition(
  const char * c_prefix, const ParameterReaderInterface & reader, const PBYTE codec_private_data,
  uint32_t codec_private_data_size) const
{
  if (nullptr == c_prefix || (nullptr == codec_private_data && 0 != codec_private_data_size)) {
    return unique_ptr<StreamDefinition>{};
  }
  std::string prefix(c_prefix);

  std::string stream_name = "default";
  reader.ReadStdString((prefix + "stream_name").c_str(), stream_name);

  map<string, string> tags;
  reader.ReadMap((prefix + "tags").c_str(), tags);

  int retention_period = 2;
  reader.ReadInt((prefix + "retention_period").c_str(), retention_period);

  std::string kms_key_id;
  reader.ReadStdString((prefix + "kms_key_id").c_str(), kms_key_id);

  int streaming_type_id = 0;
  reader.ReadInt((prefix + "streaming_type").c_str(), streaming_type_id);
  STREAMING_TYPE streaming_type = static_cast<STREAMING_TYPE>(streaming_type_id);

  std::string content_type = "video/h264";
  reader.ReadStdString((prefix + "content_type").c_str(), content_type);

  int max_latency = 0;
  reader.ReadInt((prefix + "max_latency").c_str(), max_latency);

  int fragment_duration = 2;
  reader.ReadInt((prefix + "fragment_duration").c_str(), fragment_duration);

  int timecode_scale = 1;
  reader.ReadInt((prefix + "timecode_scale").c_str(), timecode_scale);

  bool key_frame_fragmentation = true;
  reader.ReadBool((prefix + "key_frame_fragmentation").c_str(), key_frame_fragmentation);

  bool frame_timecodes = true;
  reader.ReadBool((prefix + "frame_timecodes").c_str(), frame_timecodes);

  bool absolute_fragment_time = true;
  reader.ReadBool((prefix + "absolute_fragment_time").c_str(), absolute_fragment_time);

  bool fragment_acks = true;
  reader.ReadBool((prefix + "fragment_acks").c_str(), fragment_acks);

  bool restart_on_error = true;
  reader.ReadBool((prefix + "restart_on_error").c_str(), restart_on_error);

  bool recalculate_metrics = true;
  reader.ReadBool((prefix + "recalculate_metrics").c_str(), recalculate_metrics);

  int nal_adaptation_flag_id = NAL_ADAPTATION_ANNEXB_NALS | NAL_ADAPTATION_ANNEXB_CPD_NALS;
  reader.ReadInt((prefix + "nal_adaptation_flags").c_str(), nal_adaptation_flag_id);
  NAL_ADAPTATION_FLAGS nal_adaptation_flags =
    static_cast<NAL_ADAPTATION_FLAGS>(nal_adaptation_flag_id);

  int frame_rate = 24;
  reader.ReadInt((prefix + "frame_rate").c_str(), frame_rate);

  int avg_bandwidth_bps = 4 * 1024 * 1024;
  reader.ReadInt((prefix + "avg_bandwidth_bps").c_str(), avg_bandwidth_bps);

  int buffer_duration = 120;
  reader.ReadInt((prefix + "buffer_duration").c_str(), buffer_duration);

  int replay_duration = 40;
  reader.ReadInt((prefix + "replay_duration").c_str(), replay_duration);

  int connection_staleness = 30;
  reader.ReadInt((prefix + "connection_staleness").c_str(), connection_staleness);

  std::string codec_id = "V_MPEG4/ISO/AVC";
  reader.ReadStdString((prefix + "codec_id").c_str(), codec_id);

  std::string track_name = "kinesis_video";
  reader.ReadStdString((prefix + "track_name").c_str(), track_name);

  auto stream_definition = make_unique<StreamDefinition>(
    stream_name, hours(retention_period), &tags, kms_key_id, streaming_type, content_type,
    milliseconds(max_latency), seconds(fragment_duration), milliseconds(timecode_scale),
    key_frame_fragmentation, frame_timecodes, absolute_fragment_time, fragment_acks,
    restart_on_error, recalculate_metrics, nal_adaptation_flags, frame_rate, avg_bandwidth_bps,
    seconds(buffer_duration), seconds(replay_duration), seconds(connection_staleness), codec_id,
    track_name, codec_private_data, codec_private_data_size);
  return stream_definition;
}
}  // namespace Kinesis
}  // namespace Aws
