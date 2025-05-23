// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "perf_reader.h"

#include <byteswap.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "base/logging.h"
#include "file_utils.h"
#include "kernel/perf_internals.h"
#include "perf_test_files.h"
#include "test_perf_data.h"
#include "test_utils.h"

namespace quipper {

using PerfBuildID = PerfDataProto_PerfBuildID;
using PerfEvent = PerfDataProto_PerfEvent;
using SampleEvent = PerfDataProto_SampleEvent;
using SampleInfo = PerfDataProto_SampleInfo;

TEST(PerfReaderTest, PipedData_FailIncompleteEventHeader) {
  std::stringstream input;

  // pipe header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  testing::ExamplePerfEventAttrEvent_Hardware(PERF_SAMPLE_IP,
                                              true /*sample_id_all*/)
      .WithConfig(123)
      .WriteTo(&input);

  // PERF_RECORD_HEADER_EVENT_TYPE
  const struct event_type_event event_type = {
      .header =
          {
              .type = PERF_RECORD_HEADER_EVENT_TYPE,
              .misc = 0,
              .size = sizeof(struct event_type_event),
          },
      .event_type = {
          /*event_id*/ 123,
          /*name*/ "cycles",
      },
  };
  input.write(reinterpret_cast<const char*>(&event_type), sizeof(event_type));

  // Incomplete data at the end:
  input << std::string(sizeof(struct perf_event_header) - 1, '\0');

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_FALSE(pr.ReadFromString(input.str()));
}

TEST(PerfReaderTest, PipedData_FailIncompleteEventData) {
  std::stringstream input;

  // pipe header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  testing::ExamplePerfEventAttrEvent_Hardware(PERF_SAMPLE_IP,
                                              true /*sample_id_all*/)
      .WithConfig(456)
      .WriteTo(&input);

  // PERF_RECORD_HEADER_EVENT_TYPE
  const struct event_type_event event_type = {
      .header =
          {
              .type = PERF_RECORD_HEADER_EVENT_TYPE,
              .misc = 0,
              .size = sizeof(struct event_type_event),
          },
      .event_type = {
          /*event_id*/ 456,
          /*name*/ "instructions",
      },
  };
  input.write(reinterpret_cast<const char*>(&event_type), sizeof(event_type));

  // Incomplete data at the end:
  // Header:
  const struct perf_event_header incomplete_event_header = {
      .type = PERF_RECORD_SAMPLE,
      .misc = 0,
      .size = sizeof(perf_event_header) + 10,
  };
  input.write(reinterpret_cast<const char*>(&incomplete_event_header),
              sizeof(incomplete_event_header));
  // Incomplete event:
  input << std::string(3, '\0');

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_FALSE(pr.ReadFromString(input.str()));
}

TEST(PerfReaderTest, PerfEventAttrEvent) {
  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // no data

  // PERF_RECORD_HEADER_ATTR
  testing::ExamplePerfEventAttrEvent_Hardware(PERF_SAMPLE_IP | PERF_SAMPLE_TID,
                                              true /*sample_id_all*/)
      .WithUseClockid(true)
      .WithContextSwitch(true)
      .WithWriteBackward(true)
      .WithNamespaces(true)
      .WithCgroup(true)
      .WriteTo(&input);

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  ASSERT_EQ(1, pr.attrs().size());
  {
    const PerfDataProto::PerfFileAttr& file_attr = pr.attrs().Get(0);
    EXPECT_TRUE(file_attr.has_attr());
    const PerfDataProto::PerfEventAttr& attr = file_attr.attr();
    EXPECT_TRUE(attr.use_clockid());
    EXPECT_TRUE(attr.context_switch());
    EXPECT_TRUE(attr.write_backward());
    EXPECT_TRUE(attr.namespaces());
    EXPECT_TRUE(attr.cgroup());
  }

  ASSERT_EQ(0, pr.event_types().size());
}

TEST(PerfReaderTest, PerfFileAttr) {
  std::stringstream input;

  // header
  testing::ExamplePerfDataFileHeader file_header((0));
  file_header.WithAttrCount(1).WithDataSize(0);
  file_header.WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_IP | PERF_SAMPLE_TID,
                                        true /*sample_id_all*/)
      .WithUseClockid(true)
      .WithContextSwitch(true)
      .WithWriteBackward(true)
      .WithNamespaces(true)
      .WithCgroup(true)
      .WriteTo(&input);

  // no data

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  ASSERT_EQ(1, pr.attrs().size());
  {
    const PerfDataProto::PerfFileAttr& file_attr = pr.attrs().Get(0);
    EXPECT_TRUE(file_attr.has_attr());
    const PerfDataProto::PerfEventAttr& attr = file_attr.attr();
    EXPECT_TRUE(attr.use_clockid());
    EXPECT_TRUE(attr.context_switch());
    EXPECT_TRUE(attr.write_backward());
    EXPECT_TRUE(attr.namespaces());
    EXPECT_TRUE(attr.cgroup());
  }

  ASSERT_EQ(0, pr.event_types().size());
}

TEST(PerfReaderTest, CorruptedFiles) {
  for (const char* test_file :
       perf_test_files::GetCorruptedPerfPipedDataFiles()) {
    std::string input_perf_data = GetTestInputFilePath(test_file);
    LOG(INFO) << "Testing " << input_perf_data;
    ASSERT_TRUE(FileExists(input_perf_data)) << "Test file does not exist!";
    PerfReader pr;
    ASSERT_FALSE(pr.ReadFile(input_perf_data));
  }
}

TEST(PerfReaderTest, ReadsAndWritesPipedModeAuxEvents) {
  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  testing::ExamplePerfEventAttrEvent_Hardware(PERF_SAMPLE_IP,
                                              false /*sample_id_all*/)
      .WriteTo(&input);

  // PERF_RECORD_AUX
  struct aux_event written_aux_event = {
      .header =
          {
              .type = PERF_RECORD_AUX,
              .misc = 0,
              .size = sizeof(struct aux_event),
          },
      .aux_offset = 0x2000,
      .aux_size = 16,
      .flags = PERF_AUX_FLAG_TRUNCATED | PERF_AUX_FLAG_PARTIAL,
  };
  input.write(reinterpret_cast<const char*>(&written_aux_event),
              sizeof(struct aux_event));

  //
  // Parse input.
  //

  PerfReader pr1;
  ASSERT_TRUE(pr1.ReadFromString(input.str()));
  // Write it out and read it in again, the two should have the same data.
  std::vector<char> output_perf_data;
  ASSERT_TRUE(pr1.WriteToVector(&output_perf_data));
  PerfReader pr2;
  ASSERT_TRUE(pr2.ReadFromVector(output_perf_data));

  // Test both versions:
  for (PerfReader* pr : {&pr1, &pr2}) {
    // PERF_RECORD_HEADER_ATTR is added to attr(), not events().
    EXPECT_EQ(1, pr->events().size());

    const PerfEvent& event = pr->events().Get(0);
    EXPECT_EQ(PERF_RECORD_AUX, event.header().type());
    EXPECT_EQ(0x2000, event.aux_event().aux_offset());
    EXPECT_EQ(16, event.aux_event().aux_size());
    EXPECT_TRUE(event.aux_event().is_truncated());
    EXPECT_FALSE(event.aux_event().is_overwrite());
    EXPECT_TRUE(event.aux_event().is_partial());
  }
}

TEST(PerfReaderTest, ReadsAndWritesPipedModeAuxTraceEvents) {
  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  testing::ExamplePerfEventAttrEvent_Hardware(PERF_SAMPLE_IP,
                                              false /*sample_id_all*/)
      .WriteTo(&input);

  // PERF_RECORD_AUXTRACE
  testing::ExampleAuxtraceEvent(9, 0x2000, 7, 3, 0x68d, 4, 0, "/dev/zero")
      .WriteTo(&input);

  //
  // Parse input.
  //

  PerfReader pr1;
  ASSERT_TRUE(pr1.ReadFromString(input.str()));
  // Write it out and read it in again, the two should have the same data.
  std::vector<char> output_perf_data;
  ASSERT_TRUE(pr1.WriteToVector(&output_perf_data));
  PerfReader pr2;
  ASSERT_TRUE(pr2.ReadFromVector(output_perf_data));

  // Test both versions:
  for (PerfReader* pr : {&pr1, &pr2}) {
    // PERF_RECORD_HEADER_ATTR is added to attr(), not events().
    EXPECT_EQ(1, pr->events().size());

    const PerfEvent& event = pr->events().Get(0);
    EXPECT_EQ(PERF_RECORD_AUXTRACE, event.header().type());
    EXPECT_EQ(9, event.auxtrace_event().size());
    EXPECT_EQ(0x2000, event.auxtrace_event().offset());
    EXPECT_EQ(7, event.auxtrace_event().reference());
    EXPECT_EQ(3, event.auxtrace_event().idx());
    EXPECT_EQ(0x68d, event.auxtrace_event().tid());
    EXPECT_EQ(4, event.auxtrace_event().cpu());
    EXPECT_EQ("/dev/zero", event.auxtrace_event().trace_data());
  }
}

TEST(PerfReaderTest, ReadsAndWritesAuxTraceEvents) {
  std::stringstream input;

  // PERF_RECORD_AUXTRACE
  testing::ExampleAuxtraceEvent auxtrace_event(9, 0x2000, 7, 3, 0x68d, 4, 0,
                                               "/dev/zero");
  const size_t data_size =
      auxtrace_event.GetSize() + auxtrace_event.GetTraceSize();

  // header
  testing::ExamplePerfDataFileHeader file_header((0));
  file_header.WithAttrCount(1).WithDataSize(data_size);
  file_header.WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_IP, false /*sample_id_all*/)
      .WriteTo(&input);

  // data
  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  auxtrace_event.WriteTo(&input);

  //
  // Parse input.
  //

  PerfReader pr1;
  ASSERT_TRUE(pr1.ReadFromString(input.str()));
  // Write it out and read it in again, the two should have the same data.
  std::vector<char> output_perf_data;
  ASSERT_TRUE(pr1.WriteToVector(&output_perf_data));
  PerfReader pr2;
  ASSERT_TRUE(pr2.ReadFromVector(output_perf_data));

  // Test both versions:
  for (PerfReader* pr : {&pr1, &pr2}) {
    // PERF_RECORD_HEADER_ATTR is added to attr(), not events().
    EXPECT_EQ(1, pr->events().size());

    const PerfEvent& event = pr->events().Get(0);
    EXPECT_EQ(PERF_RECORD_AUXTRACE, event.header().type());
    EXPECT_EQ(9, event.auxtrace_event().size());
    EXPECT_EQ(0x2000, event.auxtrace_event().offset());
    EXPECT_EQ(7, event.auxtrace_event().reference());
    EXPECT_EQ(3, event.auxtrace_event().idx());
    EXPECT_EQ(0x68d, event.auxtrace_event().tid());
    EXPECT_EQ(4, event.auxtrace_event().cpu());
    EXPECT_EQ("/dev/zero", event.auxtrace_event().trace_data());
  }
}

TEST(PerfReaderTest, FailsToReadAuxTraceEventWithInvalidTraceSize) {
  std::stringstream input;

  // PERF_RECORD_AUXTRACE
  // Auxtrace trace data size > remaining perf.data size and < total perf.data
  // size.
  testing::ExampleAuxtraceEvent auxtrace_event(11, 0x2000, 7, 3, 0x68d, 4, 0,
                                               "/dev/zero");
  const size_t data_size =
      auxtrace_event.GetSize() + auxtrace_event.GetTraceSize();

  // header
  testing::ExamplePerfDataFileHeader file_header((0));
  file_header.WithAttrCount(1).WithDataSize(data_size);
  file_header.WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_IP, false /*sample_id_all*/)
      .WriteTo(&input);

  // data
  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  auxtrace_event.WriteTo(&input);

  //
  // Parse input.
  //

  PerfReader pr1;
  ASSERT_FALSE(pr1.ReadFromString(input.str()));
}

TEST(PerfReaderTest, ReadsAndWritesTraceMetadata) {
  std::stringstream input;

  const size_t data_size =
      testing::ExamplePerfSampleEvent_Tracepoint::kEventSize;

  // header
  testing::ExamplePerfDataFileHeader file_header(1 << HEADER_TRACING_DATA);
  file_header.WithAttrCount(1).WithDataSize(data_size);
  file_header.WriteTo(&input);
  const perf_file_header& header = file_header.header();

  // attrs
  CHECK_EQ(header.attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Tracepoint(73).WriteTo(&input);

  // data
  ASSERT_EQ(header.data.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfSampleEvent_Tracepoint().WriteTo(&input);
  ASSERT_EQ(file_header.data_end(), input.tellp());

  // metadata

  const unsigned int metadata_count = 1;

  // HEADER_TRACING_DATA
  testing::ExampleTracingMetadata tracing_metadata(
      file_header.data_end() + metadata_count * sizeof(perf_file_section));

  // write metadata index entries
  tracing_metadata.index_entry().WriteTo(&input);
  // write metadata
  tracing_metadata.data().WriteTo(&input);

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  EXPECT_EQ(tracing_metadata.data().value(), pr.tracing_data());

  // Write it out and read it in again, it should still be good:
  std::vector<char> output_perf_data;
  EXPECT_TRUE(pr.WriteToVector(&output_perf_data));
  EXPECT_TRUE(pr.ReadFromVector(output_perf_data));
  EXPECT_EQ(tracing_metadata.data().value(), pr.tracing_data());
}

TEST(PerfReaderTest, ReadsTracingMetadataEvent) {
  std::stringstream input;

  // pipe header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  const char raw_data[] = "\x17\x08\x44tracing0.5BLAHBLAHBLAH....";
  const std::string trace_metadata(raw_data, sizeof(raw_data) - 1);

  const tracing_data_event trace_event = {
      .header =
          {
              .type = PERF_RECORD_HEADER_TRACING_DATA,
              .misc = 0,
              .size = sizeof(tracing_data_event),
          },
      .size = static_cast<u32>(trace_metadata.size()),
  };

  input.write(reinterpret_cast<const char*>(&trace_event), sizeof(trace_event));
  input.write(trace_metadata.data(), trace_metadata.size());

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));
  EXPECT_EQ(trace_metadata, pr.tracing_data());

  // Write it out and read it in again, tracing_data() should still be correct.
  // NB: It does not get written as an event, but in a metadata section.
  std::vector<char> output_perf_data;
  EXPECT_TRUE(pr.WriteToVector(&output_perf_data));
  EXPECT_TRUE(pr.ReadFromVector(output_perf_data));
  EXPECT_EQ(trace_metadata, pr.tracing_data());
}

TEST(PerfReaderTest, FailsToReadTracingMetadataEventWithInvalidTraceSize) {
  std::stringstream input;

  // pipe header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  const char raw_data[] = "\x17\x08\x44tracing0.5BLAHBLAHBLAH....";
  const std::string trace_metadata(raw_data, sizeof(raw_data) - 1);

  const tracing_data_event trace_event = {
      .header =
          {
              .type = PERF_RECORD_HEADER_TRACING_DATA,
              .misc = 0,
              .size = sizeof(tracing_data_event),
          },
      // Trace metadata size > remaining perf.data size and < total perf.data
      // size.
      .size = static_cast<u32>(trace_metadata.size() + 2),
  };

  input.write(reinterpret_cast<const char*>(&trace_event), sizeof(trace_event));
  input.write(trace_metadata.data(), trace_metadata.size());

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_FALSE(pr.ReadFromString(input.str()));
}

// Regression test for http://crbug.com/484393
TEST(PerfReaderTest, BranchStackMetadataIndexHasZeroSize) {
  std::stringstream input;

  const size_t data_size =
      testing::ExamplePerfSampleEvent_BranchStack::kEventSize;

  // header
  testing::ExamplePerfDataFileHeader file_header(1 << HEADER_BRANCH_STACK);
  file_header.WithAttrCount(1).WithDataSize(data_size);
  file_header.WriteTo(&input);
  const perf_file_header& header = file_header.header();

  // attrs
  CHECK_EQ(header.attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_BRANCH_STACK,
                                        false /*sample_id_all*/)
      .WriteTo(&input);

  // data
  ASSERT_EQ(header.data.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfSampleEvent_BranchStack().WriteTo(&input);
  ASSERT_EQ(file_header.data_end(), input.tellp());

  // metadata

  // HEADER_BRANCH_STACK
  const perf_file_section branch_stack_index = {
      .offset = file_header.data_end_offset(),
      .size = 0,
  };
  input.write(reinterpret_cast<const char*>(&branch_stack_index),
              sizeof(branch_stack_index));

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  // Write it out again.
  // Initialize the buffer to a non-zero sentinel value so that the bytes
  // we are checking were written with zero must have been written.
  const size_t max_predicted_written_size = 1024;
  std::vector<char> output_perf_data(max_predicted_written_size, '\xaa');
  EXPECT_TRUE(pr.WriteToVector(&output_perf_data));
  EXPECT_LE(output_perf_data.size(), max_predicted_written_size)
      << "Bad prediction for written size";

  // Specifically check that the metadata index has zero in the size.
  const auto* output_header =
      reinterpret_cast<struct perf_file_header*>(output_perf_data.data());
  EXPECT_EQ(1 << HEADER_BRANCH_STACK, output_header->adds_features[0])
      << "Expected just a HEADER_BRANCH_STACK feature";
  const size_t metadata_offset =
      output_header->data.offset + output_header->data.size;
  const auto* output_feature_index =
      reinterpret_cast<struct perf_file_section*>(output_perf_data.data() +
                                                  metadata_offset);
  EXPECT_EQ(0, output_feature_index[0].size)
      << "Regression: Expected zero size for the HEADER_BRANCH_STACK feature "
      << "metadata index";
}

// Regression test for http://crbug.com/427767
TEST(PerfReaderTest, CorrectlyReadsPerfEventAttrSize) {
  std::stringstream input;

  // pipe header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  struct old_perf_event_attr {
    __u32 type;
    __u32 size;
    __u64 config;
    // clang-format off
    // union {
    __u64 sample_period;
    //      __u64 sample_freq;
    // };
    // clang-format on
    __u64 sample_type;
    __u64 read_format;
    // Skip the rest of the fields from perf_event_attr to simulate an
    // older, smaller version of the struct.
  };

  struct old_attr_event {
    struct perf_event_header header;
    struct old_perf_event_attr attr;
    u64 id[];
  };

  const old_attr_event attr = {
      .header =
          {
              .type = PERF_RECORD_HEADER_ATTR,
              .misc = 0,
              // A count of 8 ids is carefully selected to make the event exceed
              // 96 bytes (sizeof(perf_event_attr)) so that the test fails
              // instead of crashes with the old code.
              .size = sizeof(old_attr_event) + 8 * sizeof(u64),
          },
      .attr =
          {
              .type = 0,
              .size = sizeof(old_perf_event_attr),
              .config = 0,
              .sample_period = 10000001,
              .sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID |
                             PERF_SAMPLE_TIME | PERF_SAMPLE_ID |
                             PERF_SAMPLE_CPU,
              .read_format = PERF_FORMAT_ID,
          },
  };

  input.write(reinterpret_cast<const char*>(&attr), sizeof(attr));
  for (u64 id : {301, 302, 303, 304, 305, 306, 307, 308})
    input.write(reinterpret_cast<const char*>(&id), sizeof(id));

  // Add some sample events so that there's something to over-read.
  const sample_event sample = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = 0,
          .size = sizeof(perf_event_header) + 5 * sizeof(u64),
      }};
  // We don't care about the contents of the SAMPLE events, except for the ID,
  // which is needed to determine the attr for reading sample info.
  const u64 sample_event_array[] = {
      0,    // IP
      0,    // TID
      0,    // TIME
      308,  // ID
      0,    // CPU
  };

  for (int i = 0; i < 20; i++) {
    input.write(reinterpret_cast<const char*>(&sample), sizeof(sample));
    input.write(reinterpret_cast<const char*>(sample_event_array),
                sizeof(sample_event_array));
  }

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));
  ASSERT_EQ(pr.attrs().size(), 1);
  const auto& actual_attr = pr.attrs().Get(0);
  ASSERT_EQ(8, actual_attr.ids().size());
  EXPECT_EQ(301, actual_attr.ids(0));
  EXPECT_EQ(302, actual_attr.ids(1));
  EXPECT_EQ(303, actual_attr.ids(2));
  EXPECT_EQ(304, actual_attr.ids(3));
}

// Tests all sample info fields. When support for new sample_info fields are
// added to quipper, update this test accordingly.
TEST(PerfReaderTest, ReadsAndWritesSampleEvent) {
  using testing::PunU32U64;

  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  const u64 sample_type =
      PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_TIME | PERF_SAMPLE_ADDR |
      PERF_SAMPLE_READ | PERF_SAMPLE_CALLCHAIN | PERF_SAMPLE_CPU |
      PERF_SAMPLE_ID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_STREAM_ID |
      PERF_SAMPLE_BRANCH_STACK | PERF_SAMPLE_PHYS_ADDR;

  const size_t num_sample_event_bits = 11;
  // not tested:
  // PERF_SAMPLE_RAW |
  testing::ExamplePerfEventAttrEvent_Hardware(sample_type,
                                              true /*sample_id_all*/)
      .WithId(401)
      .WithReadFormat(PERF_FORMAT_TOTAL_TIME_ENABLED |
                      PERF_FORMAT_TOTAL_TIME_RUNNING | PERF_FORMAT_ID)
      .WriteTo(&input);

  // PERF_RECORD_SAMPLE
  const size_t call_chain_size = 6;
  const size_t branch_stack_size = 5;
  const sample_event written_sample_event = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = PERF_RECORD_MISC_KERNEL,
          .size =
              sizeof(struct sample_event) +
              num_sample_event_bits * sizeof(u64) +
              4 * sizeof(
                      u64) +  // Non-grouped read info, see
                              // perf_event_read_format in kernel/perf_event.h.
              call_chain_size * sizeof(u64) +
              branch_stack_size * sizeof(struct branch_entry),
      }};
  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415837014 * 1000000000ULL,            // TIME
      0x00007f999c38d15a,                    // ADDR
      401,                                   // ID
      1,                                     // STREAM_ID
      8,                                     // CPU
      10001,                                 // PERIOD

      // READ
      0x103c5d0,       // value
      0x7f6e8c45a920,  // time_enabled
      0x7ffed1a5e950,  // time_running
      402,             // id

      // clang-format off
      // CALLCHAIN
      6,         // nr
      0x1c1000,  // ips
      0x1c2000,
      0x1c3000,
      0x2c0000,
      0x2c1000,
      0x3c1000,

      // BRANCH_STACK
      5,  // nr
      0x00007f4a313bb8cc, 0x00007f4a313bdb40, 0x02,  // entries
      0x00007f4a30ce4de2, 0x00007f4a313bb8b3, 0x02,  // predicted = 0x2
      0x00007f4a313bb8b0, 0x00007f4a30ce4de0, 0x01,  // mispredict = 0x1
      0x00007f4a30ff45c1, 0x00007f4a313bb8a0, 0x02,
      0x00007f4a30ff49f2, 0x00007f4a30ff45bb, 0x02,

      0x00003f324c43d23b,  // PHYSICAL ADDRESS
      // clang-format on
  };
  ASSERT_EQ(written_sample_event.header.size,
            sizeof(written_sample_event.header) + sizeof(sample_event_array));
  input.write(reinterpret_cast<const char*>(&written_sample_event),
              sizeof(written_sample_event));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));

  //
  // Parse input.
  //

  PerfReader pr1;
  ASSERT_TRUE(pr1.ReadFromString(input.str()));
  // Write it out and read it in again, the two should have the same data.
  std::vector<char> output_perf_data;
  ASSERT_TRUE(pr1.WriteToVector(&output_perf_data));
  PerfReader pr2;
  ASSERT_TRUE(pr2.ReadFromVector(output_perf_data));

  // Test both versions:
  for (PerfReader* pr : {&pr1, &pr2}) {
    // PERF_RECORD_HEADER_ATTR is added to attr(), not events().
    EXPECT_EQ(1, pr->events().size());

    const PerfEvent& event = pr->events().Get(0);
    EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());

    const SampleEvent& sample = event.sample_event();
    EXPECT_EQ(0xffffffff01234567, sample.ip());
    EXPECT_EQ(0x68d, sample.pid());
    EXPECT_EQ(0x68e, sample.tid());
    EXPECT_EQ(1415837014 * 1000000000ULL, sample.sample_time_ns());
    EXPECT_EQ(0x00007f999c38d15a, sample.addr());
    EXPECT_EQ(401, sample.id());
    EXPECT_EQ(1, sample.stream_id());
    EXPECT_EQ(8, sample.cpu());
    EXPECT_EQ(10001, sample.period());
    EXPECT_EQ(0x00003f324c43d23b, sample.physical_addr());

    // Read info
    EXPECT_TRUE(sample.has_read_info());
    EXPECT_EQ(0x7f6e8c45a920, sample.read_info().time_enabled());
    EXPECT_EQ(0x7ffed1a5e950, sample.read_info().time_running());
    ASSERT_EQ(1, sample.read_info().read_value_size());
    EXPECT_EQ(0x103c5d0, sample.read_info().read_value(0).value());
    EXPECT_EQ(402, sample.read_info().read_value(0).id());

    // Callchain.
    ASSERT_EQ(6, sample.callchain_size());
    EXPECT_EQ(0x1c1000, sample.callchain(0));
    EXPECT_EQ(0x1c2000, sample.callchain(1));
    EXPECT_EQ(0x1c3000, sample.callchain(2));
    EXPECT_EQ(0x2c0000, sample.callchain(3));
    EXPECT_EQ(0x2c1000, sample.callchain(4));
    EXPECT_EQ(0x3c1000, sample.callchain(5));

    // Branch stack.
    ASSERT_EQ(5, sample.branch_stack_size());
    EXPECT_EQ(0x00007f4a313bb8cc, sample.branch_stack(0).from_ip());
    EXPECT_EQ(0x00007f4a313bdb40, sample.branch_stack(0).to_ip());
    EXPECT_FALSE(sample.branch_stack(0).mispredicted());
    EXPECT_EQ(0x00007f4a30ce4de2, sample.branch_stack(1).from_ip());
    EXPECT_EQ(0x00007f4a313bb8b3, sample.branch_stack(1).to_ip());
    EXPECT_FALSE(sample.branch_stack(1).mispredicted());
    EXPECT_EQ(0x00007f4a313bb8b0, sample.branch_stack(2).from_ip());
    EXPECT_EQ(0x00007f4a30ce4de0, sample.branch_stack(2).to_ip());
    EXPECT_TRUE(sample.branch_stack(2).mispredicted());
    EXPECT_EQ(0x00007f4a30ff45c1, sample.branch_stack(3).from_ip());
    EXPECT_EQ(0x00007f4a313bb8a0, sample.branch_stack(3).to_ip());
    EXPECT_FALSE(sample.branch_stack(3).mispredicted());
    EXPECT_EQ(0x00007f4a30ff49f2, sample.branch_stack(4).from_ip());
    EXPECT_EQ(0x00007f4a30ff45bb, sample.branch_stack(4).to_ip());
    EXPECT_FALSE(sample.branch_stack(4).mispredicted());
  }
}

TEST(PerfReaderTest, ReadsAndWritesSampleEventMissingTime) {
  using testing::PunU32U64;

  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  const u64 sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_READ | PERF_SAMPLE_ID;
  const size_t num_sample_event_bits = 2;
  testing::ExamplePerfEventAttrEvent_Hardware(sample_type,
                                              true /*sample_id_all*/)
      .WithId(401)
      .WithReadFormat(PERF_FORMAT_ID)
      .WriteTo(&input);
  const sample_event written_sample_event = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = 0,
          .size = sizeof(struct sample_event) +
                  num_sample_event_bits * sizeof(u64) +
                  2 * sizeof(u64),  // Non-grouped read info (without times!).
      }};
  const u64 sample_event_array[] = {
      0xffffffff01234567,  // IP
      401,                 // ID
      // READ
      0x103c5d0,  // value
      402,        // id
  };
  ASSERT_EQ(written_sample_event.header.size,
            sizeof(written_sample_event.header) + sizeof(sample_event_array));
  input.write(reinterpret_cast<const char*>(&written_sample_event),
              sizeof(written_sample_event));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));

  //
  // Parse input.
  //

  PerfReader pr1;
  ASSERT_TRUE(pr1.ReadFromString(input.str()));
  // Write it out and read it in again, the two should have the same data.
  std::vector<char> output_perf_data;
  ASSERT_TRUE(pr1.WriteToVector(&output_perf_data));
  PerfReader pr2;
  ASSERT_TRUE(pr2.ReadFromVector(output_perf_data));

  // Test both versions:
  for (PerfReader* pr : {&pr1, &pr2}) {
    // PERF_RECORD_HEADER_ATTR is added to attr(), not events().
    EXPECT_EQ(1, pr->events().size());

    const PerfEvent& event = pr->events().Get(0);
    EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());

    const SampleEvent& sample = event.sample_event();
    EXPECT_EQ(0xffffffff01234567, sample.ip());
    EXPECT_EQ(401, sample.id());

    // Read info
    EXPECT_TRUE(sample.has_read_info());
    EXPECT_FALSE(sample.read_info().has_time_enabled());
    EXPECT_FALSE(sample.read_info().has_time_running());
    ASSERT_EQ(1, sample.read_info().read_value_size());
    EXPECT_EQ(0x103c5d0, sample.read_info().read_value(0).value());
    EXPECT_EQ(402, sample.read_info().read_value(0).id());
  }
}

TEST(PerfReaderTest, ReadsAndWritesSampleAndSampleIdAll) {
  using testing::PunU32U64;

  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  // clang-format off
  const u64 sample_type =      // * == in sample_id_all
      PERF_SAMPLE_IP |
      PERF_SAMPLE_TID |        // *
      PERF_SAMPLE_TIME |       // *
      PERF_SAMPLE_ADDR |
      PERF_SAMPLE_ID |         // *
      PERF_SAMPLE_STREAM_ID |  // *
      PERF_SAMPLE_CPU |        // *
      PERF_SAMPLE_PERIOD;
  // clang-format on
  const size_t num_sample_event_bits = 8;
  const size_t num_sample_id_bits = 5;
  // not tested:
  // PERF_SAMPLE_READ |
  // PERF_SAMPLE_RAW |
  // PERF_SAMPLE_CALLCHAIN |
  // PERF_SAMPLE_BRANCH_STACK |
  testing::ExamplePerfEventAttrEvent_Hardware(sample_type,
                                              true /*sample_id_all*/)
      .WithId(401)
      .WriteTo(&input);

  // PERF_RECORD_SAMPLE
  const sample_event written_sample_event = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = PERF_RECORD_MISC_KERNEL,
          .size =
              sizeof(struct sample_event) + num_sample_event_bits * sizeof(u64),
      }};
  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415837014 * 1000000000ULL,            // TIME
      0x00007f999c38d15a,                    // ADDR
      401,                                   // ID
      1,                                     // STREAM_ID
      8,                                     // CPU
      10001,                                 // PERIOD
  };
  ASSERT_EQ(written_sample_event.header.size,
            sizeof(written_sample_event.header) + sizeof(sample_event_array));
  input.write(reinterpret_cast<const char*>(&written_sample_event),
              sizeof(written_sample_event));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));

  // PERF_RECORD_MMAP
  ASSERT_EQ(40, offsetof(struct mmap_event, filename));
  // clang-format off
  const size_t mmap_event_size =
      offsetof(struct mmap_event, filename) + 10 +
      6 /* ==16, nearest 64-bit boundary for filename */ +
      num_sample_id_bits * sizeof(u64);
  // clang-format on
  struct mmap_event written_mmap_event = {
      .header =
          {
              .type = PERF_RECORD_MMAP,
              .misc = 0,
              .size = mmap_event_size,
          },
      .pid = 0x68d,
      .tid = 0x68d,
      .start = 0x1d000,
      .len = 0x1000,
      .pgoff = 0,
      // .filename = ..., // written separately
  };
  const char mmap_filename[10 + 6] = "/dev/zero";
  const u64 mmap_sample_id[] = {
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415911367 * 1000000000ULL,            // TIME
      401,                                   // ID
      2,                                     // STREAM_ID
      9,                                     // CPU
  };
  const size_t pre_mmap_offset = input.tellp();
  input.write(reinterpret_cast<const char*>(&written_mmap_event),
              offsetof(struct mmap_event, filename));
  input.write(mmap_filename, 10 + 6);
  input.write(reinterpret_cast<const char*>(mmap_sample_id),
              sizeof(mmap_sample_id));
  const size_t written_mmap_size =
      static_cast<size_t>(input.tellp()) - pre_mmap_offset;
  ASSERT_EQ(written_mmap_event.header.size,
            static_cast<u64>(written_mmap_size));

  //
  // Parse input.
  //

  PerfReader pr1;
  ASSERT_TRUE(pr1.ReadFromString(input.str()));
  // Write it out and read it in again, the two should have the same data.
  std::vector<char> output_perf_data;
  ASSERT_TRUE(pr1.WriteToVector(&output_perf_data));
  PerfReader pr2;
  ASSERT_TRUE(pr2.ReadFromVector(output_perf_data));

  // Test both versions:
  for (PerfReader* pr : {&pr1, &pr2}) {
    // PERF_RECORD_HEADER_ATTR is added to attr(), not events().
    EXPECT_EQ(2, pr->events().size());

    {
      const PerfEvent& event = pr->events().Get(0);
      EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());

      const SampleEvent& sample = event.sample_event();
      EXPECT_EQ(0xffffffff01234567, sample.ip());
      EXPECT_EQ(0x68d, sample.pid());
      EXPECT_EQ(0x68e, sample.tid());
      EXPECT_EQ(1415837014 * 1000000000ULL, sample.sample_time_ns());
      EXPECT_EQ(0x00007f999c38d15a, sample.addr());
      EXPECT_EQ(401, sample.id());
      EXPECT_EQ(1, sample.stream_id());
      EXPECT_EQ(8, sample.cpu());
      EXPECT_EQ(10001, sample.period());
    }

    {
      const PerfEvent& event = pr->events().Get(1);
      EXPECT_EQ(PERF_RECORD_MMAP, event.header().type());

      const SampleInfo& sample = event.mmap_event().sample_info();
      EXPECT_EQ(0x68d, sample.pid());
      EXPECT_EQ(0x68e, sample.tid());
      EXPECT_EQ(1415911367 * 1000000000ULL, sample.sample_time_ns());
      EXPECT_EQ(401, sample.id());
      EXPECT_EQ(2, sample.stream_id());
      EXPECT_EQ(9, sample.cpu());
    }
  }
}

// Test that PERF_SAMPLE_IDENTIFIER is parsed correctly. This field
// is in a different place in PERF_RECORD_SAMPLE events compared to the
// struct sample_id placed at the end of all other events.
TEST(PerfReaderTest, ReadsAndWritesPerfSampleIdentifier) {
  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  testing::ExamplePerfEventAttrEvent_Hardware(
      PERF_SAMPLE_IDENTIFIER | PERF_SAMPLE_IP | PERF_SAMPLE_TID,
      true /*sample_id_all*/)
      .WithIds({0xdeadbeef, 0xf00dbaad})
      .WriteTo(&input);

  // PERF_RECORD_SAMPLE
  const sample_event written_sample_event = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = PERF_RECORD_MISC_KERNEL,
          .size = sizeof(struct sample_event) + 3 * sizeof(u64),
      }};
  const u64 sample_event_array[] = {
      0x00000000deadbeef,  // IDENTIFIER
      0x00007f999c38d15a,  // IP
      0x0000068d0000068d,  // TID (u32 pid, tid)
  };
  ASSERT_EQ(written_sample_event.header.size,
            sizeof(written_sample_event.header) + sizeof(sample_event_array));
  input.write(reinterpret_cast<const char*>(&written_sample_event),
              sizeof(written_sample_event));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));

  // PERF_RECORD_MMAP
  ASSERT_EQ(40, offsetof(struct mmap_event, filename));
  // clang-format off
  const size_t mmap_event_size =
      offsetof(struct mmap_event, filename) + 10 +
      6 /* ==16, nearest 64-bit boundary for filename */ + 2 * sizeof(u64);
  // clang-format on
  struct mmap_event written_mmap_event = {
      .header =
          {
              .type = PERF_RECORD_MMAP,
              .misc = 0,
              .size = mmap_event_size,
          },
      .pid = 0x68d,
      .tid = 0x68d,
      .start = 0x1d000,
      .len = 0x1000,
      .pgoff = 0,
      // .filename = ..., // written separately
  };
  const char mmap_filename[10 + 6] = "/dev/zero";
  const u64 mmap_sample_id[] = {
      // NB: PERF_SAMPLE_IP is not part of sample_id
      0x0000068d0000068d,  // TID (u32 pid, tid)
      0x00000000f00dbaad,  // IDENTIFIER
  };
  const size_t pre_mmap_offset = input.tellp();
  input.write(reinterpret_cast<const char*>(&written_mmap_event),
              offsetof(struct mmap_event, filename));
  input.write(mmap_filename, 10 + 6);
  input.write(reinterpret_cast<const char*>(mmap_sample_id),
              sizeof(mmap_sample_id));
  const size_t written_mmap_size =
      static_cast<size_t>(input.tellp()) - pre_mmap_offset;
  ASSERT_EQ(written_mmap_event.header.size,
            static_cast<u64>(written_mmap_size));

  //
  // Parse input.
  //

  PerfReader pr1;
  ASSERT_TRUE(pr1.ReadFromString(input.str()));
  // Write it out and read it in again, the two should have the same data.
  std::vector<char> output_perf_data;
  ASSERT_TRUE(pr1.WriteToVector(&output_perf_data));
  PerfReader pr2;
  ASSERT_TRUE(pr2.ReadFromVector(output_perf_data));

  // Test both versions:
  for (PerfReader* pr : {&pr1, &pr2}) {
    // PERF_RECORD_HEADER_ATTR is added to attr(), not events().
    EXPECT_EQ(2, pr->events().size());

    const PerfEvent& ip_event = pr->events().Get(0);
    EXPECT_EQ(PERF_RECORD_SAMPLE, ip_event.header().type());
    EXPECT_EQ(0xdeadbeefULL, ip_event.sample_event().id());

    const PerfEvent& mmap_event = pr->events().Get(1);
    EXPECT_EQ(PERF_RECORD_MMAP, mmap_event.header().type());
    EXPECT_EQ(0xf00dbaadULL, mmap_event.mmap_event().sample_info().id());
  }
}

TEST(PerfReaderTest, IgnoresEventsOfSkippedTypes) {
  using testing::PunU32U64;

  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  // clang-format off
  const u64 sample_type =      // * == in sample_id_all
      PERF_SAMPLE_IP |
      PERF_SAMPLE_TID |        // *
      PERF_SAMPLE_TIME |       // *
      PERF_SAMPLE_ADDR |
      PERF_SAMPLE_ID |         // *
      PERF_SAMPLE_STREAM_ID |  // *
      PERF_SAMPLE_CPU |        // *
      PERF_SAMPLE_PERIOD;
  // clang-format on
  const size_t num_sample_event_bits = 8;
  const size_t num_sample_id_bits = 5;
  // not tested:
  // PERF_SAMPLE_READ |
  // PERF_SAMPLE_RAW |
  // PERF_SAMPLE_CALLCHAIN |
  // PERF_SAMPLE_BRANCH_STACK |
  testing::ExamplePerfEventAttrEvent_Hardware(sample_type,
                                              true /*sample_id_all*/)
      .WithId(401)
      .WriteTo(&input);

  // PERF_RECORD_SAMPLE
  const sample_event written_sample_event = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = PERF_RECORD_MISC_KERNEL,
          .size =
              sizeof(struct sample_event) + num_sample_event_bits * sizeof(u64),
      }};
  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415837014 * 1000000000ULL,            // TIME
      0x00007f999c38d15a,                    // ADDR
      401,                                   // ID
      1,                                     // STREAM_ID
      8,                                     // CPU
      10001,                                 // PERIOD
  };
  ASSERT_EQ(written_sample_event.header.size,
            sizeof(written_sample_event.header) + sizeof(sample_event_array));
  input.write(reinterpret_cast<const char*>(&written_sample_event),
              sizeof(written_sample_event));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));

  // PERF_RECORD_MMAP
  ASSERT_EQ(40, offsetof(struct mmap_event, filename));
  // clang-format off
  const size_t mmap_event_size =
      offsetof(struct mmap_event, filename) + 10 +
      6 /* ==16, nearest 64-bit boundary for filename */ +
      num_sample_id_bits * sizeof(u64);
  // clang-format on
  struct mmap_event written_mmap_event = {
      .header =
          {
              .type = PERF_RECORD_MMAP,
              .misc = 0,
              .size = mmap_event_size,
          },
      .pid = 0x68d,
      .tid = 0x68d,
      .start = 0x1d000,
      .len = 0x1000,
      .pgoff = 0,
      // .filename = ..., // written separately
  };
  const char mmap_filename[10 + 6] = "/dev/zero";
  const u64 mmap_sample_id[] = {
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415911367 * 1000000000ULL,            // TIME
      401,                                   // ID
      2,                                     // STREAM_ID
      9,                                     // CPU
  };
  const size_t pre_mmap_offset = input.tellp();
  input.write(reinterpret_cast<const char*>(&written_mmap_event),
              offsetof(struct mmap_event, filename));
  input.write(mmap_filename, 10 + 6);
  input.write(reinterpret_cast<const char*>(mmap_sample_id),
              sizeof(mmap_sample_id));
  const size_t written_mmap_size =
      static_cast<size_t>(input.tellp()) - pre_mmap_offset;
  ASSERT_EQ(written_mmap_event.header.size,
            static_cast<u64>(written_mmap_size));

  //
  // Parse input.
  //

  PerfReader pr;
  EXPECT_EQ(0, pr.event_types_to_skip_when_serializing().size());
  pr.SetEventTypesToSkipWhenSerializing({PERF_RECORD_SAMPLE});
  EXPECT_EQ(1, pr.event_types_to_skip_when_serializing().size());

  ASSERT_TRUE(pr.ReadFromString(input.str()));

  EXPECT_EQ(1, pr.events().size());
  const PerfEvent& event = pr.events().Get(0);
  EXPECT_EQ(PERF_RECORD_MMAP, event.header().type());

  const SampleInfo& sample = event.mmap_event().sample_info();
  EXPECT_EQ(0x68d, sample.pid());
  EXPECT_EQ(0x68e, sample.tid());
  EXPECT_EQ(1415911367 * 1000000000ULL, sample.sample_time_ns());
  EXPECT_EQ(401, sample.id());
  EXPECT_EQ(2, sample.stream_id());
  EXPECT_EQ(9, sample.cpu());
}

TEST(PerfReaderTest, InvokesCallbackForSkippedSampleEvents) {
  using testing::PunU32U64;

  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  // clang-format off
  const u64 sample_type =      // * == in sample_id_all
      PERF_SAMPLE_IP |
      PERF_SAMPLE_TID |        // *
      PERF_SAMPLE_TIME |       // *
      PERF_SAMPLE_ADDR |
      PERF_SAMPLE_ID |         // *
      PERF_SAMPLE_STREAM_ID |  // *
      PERF_SAMPLE_CPU |        // *
      PERF_SAMPLE_PERIOD;
  // clang-format on
  const size_t num_sample_event_bits = 8;
  const size_t num_sample_id_bits = 5;
  // not tested:
  // PERF_SAMPLE_READ |
  // PERF_SAMPLE_RAW |
  // PERF_SAMPLE_CALLCHAIN |
  // PERF_SAMPLE_BRANCH_STACK |
  testing::ExamplePerfEventAttrEvent_Hardware(sample_type,
                                              true /*sample_id_all*/)
      .WithId(401)
      .WriteTo(&input);

  // PERF_RECORD_SAMPLE
  const sample_event written_sample_event = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = PERF_RECORD_MISC_KERNEL,
          .size =
              sizeof(struct sample_event) + num_sample_event_bits * sizeof(u64),
      }};
  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415837014 * 1000000000ULL,            // TIME
      0x00007f999c38d15a,                    // ADDR
      401,                                   // ID
      1,                                     // STREAM_ID
      8,                                     // CPU
      10001,                                 // PERIOD
  };
  ASSERT_EQ(written_sample_event.header.size,
            sizeof(written_sample_event.header) + sizeof(sample_event_array));
  input.write(reinterpret_cast<const char*>(&written_sample_event),
              sizeof(written_sample_event));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));

  // PERF_RECORD_MMAP
  ASSERT_EQ(40, offsetof(struct mmap_event, filename));
  // clang-format off
  const size_t mmap_event_size =
      offsetof(struct mmap_event, filename) + 10 +
      6 /* ==16, nearest 64-bit boundary for filename */ +
      num_sample_id_bits * sizeof(u64);
  // clang-format on
  struct mmap_event written_mmap_event = {
      .header =
          {
              .type = PERF_RECORD_MMAP,
              .misc = 0,
              .size = mmap_event_size,
          },
      .pid = 0x68d,
      .tid = 0x68d,
      .start = 0x1d000,
      .len = 0x1000,
      .pgoff = 0,
      // .filename = ..., // written separately
  };
  const char mmap_filename[10 + 6] = "/dev/zero";
  const u64 mmap_sample_id[] = {
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415911367 * 1000000000ULL,            // TIME
      401,                                   // ID
      2,                                     // STREAM_ID
      9,                                     // CPU
  };
  const size_t pre_mmap_offset = input.tellp();
  input.write(reinterpret_cast<const char*>(&written_mmap_event),
              offsetof(struct mmap_event, filename));
  input.write(mmap_filename, 10 + 6);
  input.write(reinterpret_cast<const char*>(mmap_sample_id),
              sizeof(mmap_sample_id));
  const size_t written_mmap_size =
      static_cast<size_t>(input.tellp()) - pre_mmap_offset;
  ASSERT_EQ(written_mmap_event.header.size,
            static_cast<u64>(written_mmap_size));

  //
  // Parse input.
  //

  int callback_invocation_count = 0;
  auto callback =
      [&callback_invocation_count](const PerfDataProto_SampleEvent& sample) {
        ++callback_invocation_count;

        EXPECT_EQ(0xffffffff01234567, sample.ip());
        EXPECT_EQ(0x68d, sample.pid());
        EXPECT_EQ(0x68e, sample.tid());
        EXPECT_EQ(1415837014 * 1000000000ULL, sample.sample_time_ns());
        EXPECT_EQ(0x00007f999c38d15a, sample.addr());
        EXPECT_EQ(401, sample.id());
        EXPECT_EQ(1, sample.stream_id());
        EXPECT_EQ(8, sample.cpu());
        EXPECT_EQ(10001, sample.period());
      };

  PerfReader pr;
  pr.SetEventTypesToSkipWhenSerializing({PERF_RECORD_SAMPLE});
  pr.SetSampleCallback(callback);
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  EXPECT_EQ(1, callback_invocation_count);

  ASSERT_EQ(1, pr.events().size());
  const PerfEvent& event = pr.events().Get(0);
  EXPECT_EQ(PERF_RECORD_MMAP, event.header().type());

  const SampleInfo& sample = event.mmap_event().sample_info();
  EXPECT_EQ(0x68d, sample.pid());
  EXPECT_EQ(0x68e, sample.tid());
  EXPECT_EQ(1415911367 * 1000000000ULL, sample.sample_time_ns());
  EXPECT_EQ(401, sample.id());
  EXPECT_EQ(2, sample.stream_id());
  EXPECT_EQ(9, sample.cpu());
}

TEST(PerfReaderTest, InvokesSampleEventCallback) {
  using testing::PunU32U64;

  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  // clang-format off
  const u64 sample_type =      // * == in sample_id_all
      PERF_SAMPLE_IP |
      PERF_SAMPLE_TID |        // *
      PERF_SAMPLE_TIME |       // *
      PERF_SAMPLE_ADDR |
      PERF_SAMPLE_ID |         // *
      PERF_SAMPLE_STREAM_ID |  // *
      PERF_SAMPLE_CPU |        // *
      PERF_SAMPLE_PERIOD;
  // clang-format on
  const size_t num_sample_event_bits = 8;
  const size_t num_sample_id_bits = 5;
  // not tested:
  // PERF_SAMPLE_READ |
  // PERF_SAMPLE_RAW |
  // PERF_SAMPLE_CALLCHAIN |
  // PERF_SAMPLE_BRANCH_STACK |
  testing::ExamplePerfEventAttrEvent_Hardware(sample_type,
                                              true /*sample_id_all*/)
      .WithId(401)
      .WriteTo(&input);

  // PERF_RECORD_SAMPLE
  const sample_event written_sample_event = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = PERF_RECORD_MISC_KERNEL,
          .size =
              sizeof(struct sample_event) + num_sample_event_bits * sizeof(u64),
      }};
  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415837014 * 1000000000ULL,            // TIME
      0x00007f999c38d15a,                    // ADDR
      401,                                   // ID
      1,                                     // STREAM_ID
      8,                                     // CPU
      10001,                                 // PERIOD
  };
  ASSERT_EQ(written_sample_event.header.size,
            sizeof(written_sample_event.header) + sizeof(sample_event_array));
  input.write(reinterpret_cast<const char*>(&written_sample_event),
              sizeof(written_sample_event));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));

  // PERF_RECORD_MMAP
  ASSERT_EQ(40, offsetof(struct mmap_event, filename));
  // clang-format off
  const size_t mmap_event_size =
      offsetof(struct mmap_event, filename) + 10 +
      6 /* ==16, nearest 64-bit boundary for filename */ +
      num_sample_id_bits * sizeof(u64);
  // clang-format on
  struct mmap_event written_mmap_event = {
      .header =
          {
              .type = PERF_RECORD_MMAP,
              .misc = 0,
              .size = mmap_event_size,
          },
      .pid = 0x68d,
      .tid = 0x68d,
      .start = 0x1d000,
      .len = 0x1000,
      .pgoff = 0,
      // .filename = ..., // written separately
  };
  const char mmap_filename[10 + 6] = "/dev/zero";
  const u64 mmap_sample_id[] = {
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415911367 * 1000000000ULL,            // TIME
      401,                                   // ID
      2,                                     // STREAM_ID
      9,                                     // CPU
  };
  const size_t pre_mmap_offset = input.tellp();
  input.write(reinterpret_cast<const char*>(&written_mmap_event),
              offsetof(struct mmap_event, filename));
  input.write(mmap_filename, 10 + 6);
  input.write(reinterpret_cast<const char*>(mmap_sample_id),
              sizeof(mmap_sample_id));
  const size_t written_mmap_size =
      static_cast<size_t>(input.tellp()) - pre_mmap_offset;
  ASSERT_EQ(written_mmap_event.header.size,
            static_cast<u64>(written_mmap_size));

  //
  // Parse input.
  //

  int callback_invocation_count = 0;
  auto callback =
      [&callback_invocation_count](const PerfDataProto_SampleEvent& sample) {
        ++callback_invocation_count;

        EXPECT_EQ(0xffffffff01234567, sample.ip());
        EXPECT_EQ(0x68d, sample.pid());
        EXPECT_EQ(0x68e, sample.tid());
        EXPECT_EQ(1415837014 * 1000000000ULL, sample.sample_time_ns());
        EXPECT_EQ(0x00007f999c38d15a, sample.addr());
        EXPECT_EQ(401, sample.id());
        EXPECT_EQ(1, sample.stream_id());
        EXPECT_EQ(8, sample.cpu());
        EXPECT_EQ(10001, sample.period());
      };

  PerfReader pr;
  pr.SetSampleCallback(callback);
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  EXPECT_EQ(1, callback_invocation_count);

  // PERF_RECORD_HEADER_ATTR is added to attr(), not events().
  ASSERT_EQ(2, pr.events().size());

  {
    const PerfEvent& event = pr.events().Get(0);
    EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());

    const SampleEvent& sample = event.sample_event();
    EXPECT_EQ(0xffffffff01234567, sample.ip());
    EXPECT_EQ(0x68d, sample.pid());
    EXPECT_EQ(0x68e, sample.tid());
    EXPECT_EQ(1415837014 * 1000000000ULL, sample.sample_time_ns());
    EXPECT_EQ(0x00007f999c38d15a, sample.addr());
    EXPECT_EQ(401, sample.id());
    EXPECT_EQ(1, sample.stream_id());
    EXPECT_EQ(8, sample.cpu());
    EXPECT_EQ(10001, sample.period());
  }

  {
    const PerfEvent& event = pr.events().Get(1);
    EXPECT_EQ(PERF_RECORD_MMAP, event.header().type());

    const SampleInfo& sample = event.mmap_event().sample_info();
    EXPECT_EQ(0x68d, sample.pid());
    EXPECT_EQ(0x68e, sample.tid());
    EXPECT_EQ(1415911367 * 1000000000ULL, sample.sample_time_ns());
    EXPECT_EQ(401, sample.id());
    EXPECT_EQ(2, sample.stream_id());
    EXPECT_EQ(9, sample.cpu());
  }
}

TEST(PerfReaderTest, ReadsAndWritesMmap2Events) {
  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  testing::ExamplePerfEventAttrEvent_Hardware(PERF_SAMPLE_IP,
                                              false /*sample_id_all*/)
      .WriteTo(&input);

  // PERF_RECORD_MMAP2
  ASSERT_EQ(72, offsetof(struct mmap2_event, filename));
  // clang-format off
  const size_t mmap_event_size =
      offsetof(struct mmap2_event, filename) + 10 +
      6; /* ==16, nearest 64-bit boundary for filename */
  // clang-format on
  struct mmap2_event written_mmap_event = {.header =
                                               {
                                                   .type = PERF_RECORD_MMAP2,
                                                   .misc = 0,
                                                   .size = mmap_event_size,
                                               },
                                           .pid = 0x68d,
                                           .tid = 0x68d,
                                           .start = 0x1d000,
                                           .len = 0x1000};
  // Compilers handle unnamed union/struct initializers differently.
  // So it'd be safer to assign following fields after the initialization.
  written_mmap_event.maj = 6;
  written_mmap_event.min = 7;
  written_mmap_event.ino = 8;
  written_mmap_event.ino_generation = 9;

  written_mmap_event.pgoff = 0x2000;
  written_mmap_event.prot = 1 | 2;  // == PROT_READ | PROT_WRITE
  written_mmap_event.flags = 2;     // == MAP_PRIVATE
  const char mmap_filename[10 + 6] = "/dev/zero";
  const size_t pre_mmap_offset = input.tellp();
  input.write(reinterpret_cast<const char*>(&written_mmap_event),
              offsetof(struct mmap2_event, filename));
  input.write(mmap_filename, 10 + 6);
  const size_t written_mmap_size =
      static_cast<size_t>(input.tellp()) - pre_mmap_offset;
  ASSERT_EQ(written_mmap_event.header.size,
            static_cast<u64>(written_mmap_size));

  //
  // Parse input.
  //

  PerfReader pr1;
  ASSERT_TRUE(pr1.ReadFromString(input.str()));
  // Write it out and read it in again, the two should have the same data.
  std::vector<char> output_perf_data;
  ASSERT_TRUE(pr1.WriteToVector(&output_perf_data));
  PerfReader pr2;
  ASSERT_TRUE(pr2.ReadFromVector(output_perf_data));

  // Test both versions:
  for (PerfReader* pr : {&pr1, &pr2}) {
    // PERF_RECORD_HEADER_ATTR is added to attr(), not events().
    EXPECT_EQ(1, pr->events().size());

    const PerfEvent& event = pr->events().Get(0);
    EXPECT_EQ(PERF_RECORD_MMAP2, event.header().type());
    EXPECT_EQ(0x68d, event.mmap_event().pid());
    EXPECT_EQ(0x68d, event.mmap_event().tid());
    EXPECT_EQ(0x1d000, event.mmap_event().start());
    EXPECT_EQ(0x1000, event.mmap_event().len());
    EXPECT_EQ(0x2000, event.mmap_event().pgoff());
    EXPECT_EQ(6, event.mmap_event().maj());
    EXPECT_EQ(7, event.mmap_event().min());
    EXPECT_EQ(8, event.mmap_event().ino());
    EXPECT_EQ(9, event.mmap_event().ino_generation());
    EXPECT_EQ(1 | 2, event.mmap_event().prot());
    EXPECT_EQ(2, event.mmap_event().flags());
  }
}

TEST(PerfReaderTest, ReadCPUTopologyMetadata) {
  std::stringstream input;
  const std::vector<std::string> core_siblings = {"0-7"};
  const std::vector<std::string> thread_siblings = {"0-1", "2-3", "4",
                                                    "5",   "6",   "7"};

  // header
  testing::ExamplePerfDataFileHeader file_header(1 << HEADER_CPU_TOPOLOGY);
  file_header.WithAttrCount(1);
  file_header.WriteTo(&input);
  const perf_file_header& header = file_header.header();

  // attrs
  ASSERT_EQ(header.attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(0, false /*sample_id_all*/)
      .WriteTo(&input);

  // metadata
  size_t metadata_offset = file_header.data_end() + sizeof(perf_file_section);
  testing::ExampleCPUTopologyMetadata cpu_topology_metadata(
      core_siblings.size(), core_siblings, thread_siblings.size(),
      thread_siblings, metadata_offset);
  cpu_topology_metadata.index_entry().WriteTo(&input);
  cpu_topology_metadata.WriteTo(&input);

  //
  // Parse input
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));
  auto cpu_topology = pr.proto().cpu_topology();
  EXPECT_EQ(cpu_topology.core_siblings()[0], "0-7");
  EXPECT_EQ(cpu_topology.thread_siblings()[0], "0-1");
  EXPECT_EQ(cpu_topology.thread_siblings()[5], "7");
}

// Regression test for https://github.com/google/perf_data_converter/issues/143.
TEST(PerfReaderTest, CheckNumSiblingsForCPUTopology) {
  std::stringstream input;
  const std::vector<std::string> core_siblings = {"0-3"};
  const std::vector<std::string> thread_siblings = {"0-1", "2-3"};

  // header
  testing::ExamplePerfDataFileHeader file_header(1 << HEADER_CPU_TOPOLOGY);
  file_header.WithAttrCount(1);
  file_header.WriteTo(&input);
  const perf_file_header& header = file_header.header();

  // attrs
  ASSERT_EQ(header.attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(0, false /*sample_id_all*/)
      .WriteTo(&input);
  size_t metadata_offset = file_header.data_end() + sizeof(perf_file_section);
  testing::ExampleCPUTopologyMetadata cpu_topology_metadata(
      core_siblings.size(), core_siblings, 1000, thread_siblings,
      metadata_offset);
  cpu_topology_metadata.index_entry().WriteTo(&input);
  cpu_topology_metadata.WriteTo(&input);

  // Read input should fail
  PerfReader pr;
  EXPECT_FALSE(pr.ReadFromString(input.str()));
}

TEST(PerfReaderTest, ReadsAndWritesHybridTopologyMetadata) {
  std::stringstream input;
  const std::vector<std::string> pmu_names = {"cpu_core", "cpu_atom"};
  const std::vector<std::string> cpus = {"0-1", "2-5"};

  // header
  testing::ExamplePerfDataFileHeader file_header(1 << HEADER_HYBRID_TOPOLOGY);
  file_header.WithAttrCount(1);
  file_header.WriteTo(&input);
  const perf_file_header& header = file_header.header();

  // attrs
  ASSERT_EQ(header.attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(0, false /*sample_id_all*/)
      .WriteTo(&input);

  // metadata
  size_t metadata_offset = file_header.data_end() + sizeof(perf_file_section);
  testing::ExampleHybridTopologyMetadata hybrid_topology_metadata(
      pmu_names, cpus, metadata_offset);
  hybrid_topology_metadata.index_entry().WriteTo(&input);
  hybrid_topology_metadata.WriteTo(&input);

  //
  // Parse input
  //

  PerfReader pr;
  std::vector<uint32_t> want_cpu_cores = {0, 1};
  std::vector<uint32_t> want_cpu_atoms = {2, 3, 4, 5};
  ASSERT_TRUE(pr.ReadFromString(input.str()));
  auto hybrid_topology = pr.proto().hybrid_topology();
  EXPECT_EQ(hybrid_topology[0].pmu_name(), "cpu_core");
  EXPECT_EQ(hybrid_topology[0].cpus(), "0-1");
  EXPECT_EQ(hybrid_topology[0].cpu_list().size(), want_cpu_cores.size());
  for (int i = 0; i < want_cpu_cores.size(); ++i) {
    EXPECT_EQ(hybrid_topology[0].cpu_list(i), want_cpu_cores[i]);
  }
  EXPECT_EQ(hybrid_topology[1].pmu_name(), "cpu_atom");
  EXPECT_EQ(hybrid_topology[1].cpus(), "2-5");
  EXPECT_EQ(hybrid_topology[1].cpu_list().size(), want_cpu_atoms.size());
  for (int i = 0; i < want_cpu_atoms.size(); ++i) {
    EXPECT_EQ(hybrid_topology[1].cpu_list(i), want_cpu_atoms[i]);
  }
}

// Regression test for http://crbug.com/493533
TEST(PerfReaderTest, ReadsAllAvailableMetadataTypes) {
  std::stringstream input;

  const uint32_t features = (1 << HEADER_HOSTNAME) | (1 << HEADER_OSRELEASE) |
                            (1 << HEADER_VERSION) | (1 << HEADER_ARCH) |
                            (1 << HEADER_LAST_FEATURE);

  // header
  testing::ExamplePerfDataFileHeader file_header(features);
  file_header.WithAttrCount(1);
  file_header.WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(0, false /*sample_id_all*/)
      .WriteTo(&input);

  // metadata

  size_t metadata_offset =
      file_header.data_end() + 5 * sizeof(perf_file_section);

  // HEADER_HOSTNAME
  testing::ExampleStringMetadata hostname_metadata("hostname", metadata_offset);
  metadata_offset += hostname_metadata.size();

  // HEADER_OSRELEASE
  testing::ExampleStringMetadata osrelease_metadata("osrelease",
                                                    metadata_offset);
  metadata_offset += osrelease_metadata.size();

  // HEADER_VERSION
  testing::ExampleStringMetadata version_metadata("version", metadata_offset);
  metadata_offset += version_metadata.size();

  // HEADER_ARCH
  testing::ExampleStringMetadata arch_metadata("arch", metadata_offset);
  metadata_offset += arch_metadata.size();

  // HEADER_LAST_FEATURE -- this is just a dummy metadata that will be skipped
  // over. In practice, there will not actually be a metadata entry of type
  // HEADER_LAST_FEATURE. But use because it will never become a supported
  // metadata type.
  testing::ExampleStringMetadata last_feature("*unsupported*", metadata_offset);
  metadata_offset += last_feature.size();

  hostname_metadata.index_entry().WriteTo(&input);
  osrelease_metadata.index_entry().WriteTo(&input);
  version_metadata.index_entry().WriteTo(&input);
  arch_metadata.index_entry().WriteTo(&input);
  last_feature.index_entry().WriteTo(&input);

  hostname_metadata.WriteTo(&input);
  osrelease_metadata.WriteTo(&input);
  version_metadata.WriteTo(&input);
  arch_metadata.WriteTo(&input);
  last_feature.WriteTo(&input);

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  // The dummy metadata should not have prevented the reading of the other
  // metadata.
  const auto& string_metadata = pr.string_metadata();
  EXPECT_EQ("hostname", string_metadata.hostname().value());
  EXPECT_EQ("osrelease", string_metadata.kernel_version().value());
  EXPECT_EQ("version", string_metadata.perf_version().value());
  EXPECT_EQ("arch", string_metadata.architecture().value());
}

TEST(PerfReaderTest, AttrsWithDifferentSampleTypes) {
  std::stringstream input;

  // PERF_RECORD_SAMPLE
  testing::ExamplePerfSampleEvent sample_event_1(
      testing::SampleInfo().Id(51).Ip(0x00000000002c100a).Tid(1002));
  // PERF_RECORD_SAMPLE
  testing::ExamplePerfSampleEvent sample_event_2(testing::SampleInfo()
                                                     .Id(61)
                                                     .Ip(0x00000000002c100b)
                                                     .Tid(1002)
                                                     .Time(1000006));
  // PERF_RECORD_SAMPLE
  testing::ExamplePerfSampleEvent sample_event_3(
      testing::SampleInfo().Id(52).Ip(0x00000000002c100c).Tid(1002));

  const size_t data_size = sample_event_1.GetSize() + sample_event_2.GetSize() +
                           sample_event_3.GetSize();
  const uint32_t features = 0;

  // header
  testing::ExamplePerfDataFileHeader file_header(features);
  file_header.WithAttrIdsCount(3).WithAttrCount(2).WithDataSize(data_size);
  file_header.WriteTo(&input);

  // attr ids
  testing::AttrIdsSection attr_ids(input.tellp());
  const auto id_section_1 = attr_ids.AddIds({51, 52});
  const auto id_section_2 = attr_ids.AddIds({61});
  attr_ids.WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(
      PERF_SAMPLE_IDENTIFIER | PERF_SAMPLE_IP | PERF_SAMPLE_TID,
      true /*sample_id_all*/)
      .WithIds(id_section_1)
      .WriteTo(&input);
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_IDENTIFIER |
                                            PERF_SAMPLE_IP | PERF_SAMPLE_TID |
                                            PERF_SAMPLE_TIME,
                                        true /*sample_id_all*/)
      .WithIds(id_section_2)
      .WriteTo(&input);

  // data

  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  sample_event_1.WriteTo(&input);
  sample_event_2.WriteTo(&input);
  sample_event_3.WriteTo(&input);
  ASSERT_EQ(file_header.header().data.offset + data_size,
            static_cast<u64>(input.tellp()));

  // no metadata

  //
  // Parse input.
  //
  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  // Make sure the attr ids were read correctly.
  ASSERT_EQ(2, pr.attrs().size());
  ASSERT_EQ(2, pr.attrs().Get(0).ids().size());
  EXPECT_EQ(51, pr.attrs().Get(0).ids(0));
  EXPECT_EQ(52, pr.attrs().Get(0).ids(1));
  ASSERT_EQ(1, pr.attrs().Get(1).ids().size());
  EXPECT_EQ(61, pr.attrs().Get(1).ids(0));

  // Verify events were read properly.
  ASSERT_EQ(3, pr.events().size());
  {
    const PerfEvent& event = pr.events().Get(0);
    EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());

    const SampleEvent& sample = event.sample_event();
    EXPECT_EQ(51, sample.id());
    EXPECT_EQ(0x00000000002c100a, sample.ip());
    EXPECT_EQ(1002, sample.tid());
    // This event doesn't have a timestamp.
    EXPECT_FALSE(sample.has_sample_time_ns());
  }
  {
    const PerfEvent& event = pr.events().Get(1);
    EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());

    const SampleEvent& sample = event.sample_event();
    EXPECT_EQ(61, sample.id());
    EXPECT_EQ(0x00000000002c100b, sample.ip());
    EXPECT_EQ(1002, sample.tid());
    EXPECT_EQ(1000006, sample.sample_time_ns());
  }
  {
    const PerfEvent& event = pr.events().Get(2);
    EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());

    const SampleEvent& sample = event.sample_event();
    EXPECT_EQ(52, sample.id());
    EXPECT_EQ(0x00000000002c100c, sample.ip());
    EXPECT_EQ(1002, sample.tid());
    // This event doesn't have a timestamp.
    EXPECT_FALSE(sample.has_sample_time_ns());
  }
}

// Neither PERF_SAMPLE_ID nor PERF_SAMPLE_IDENTIFIER are set. We should
// fall back to using the first attr when looking for the sample_format.
TEST(PerfReaderTest, NoSampleIdField) {
  std::stringstream input;

  // PERF_RECORD_SAMPLE
  testing::ExamplePerfSampleEvent sample_event(
      testing::SampleInfo().Ip(0x00000000002c100a).Tid(1002));

  const size_t data_size = sample_event.GetSize();
  const uint32_t features = 0;

  // header
  testing::ExamplePerfDataFileHeader file_header(features);
  file_header.WithAttrCount(1).WithDataSize(data_size);
  file_header.WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_IP | PERF_SAMPLE_TID,
                                        false /*sample_id_all*/)
      .WithConfig(456)
      .WriteTo(&input);

  // data

  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  sample_event.WriteTo(&input);
  ASSERT_EQ(file_header.header().data.offset + data_size,
            static_cast<u64>(input.tellp()));

  // no metadata

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  // Make sure the attr was recorded properly.
  ASSERT_EQ(1, pr.attrs().size());
  EXPECT_EQ(456, pr.attrs().Get(0).attr().config());

  // Verify subsequent sample event was read properly.
  ASSERT_EQ(1, pr.events().size());
  const PerfEvent& event = pr.events().Get(0);
  EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());
  EXPECT_EQ(data_size, event.header().size());

  EXPECT_EQ(0x00000000002c100a, event.sample_event().ip());
  EXPECT_EQ(1002, event.sample_event().tid());
}

// When sample_id_all == false, non-sample events should not look for sample_id.
TEST(PerfReaderTest, MMapEventWithInvalidSampleInfoData) {
  std::stringstream input;

  // PERF_RECORD_SAMPLE
  testing::ExamplePerfSampleEvent sample_event(
      testing::SampleInfo().Ip(0x00000000002c100a).Tid(1002).Id(48));

  // PERF_RECORD_MMAP
  testing::ExampleMmapEvent mmap_event(
      1001, 0x1c1000, 0x1000, 0, "/usr/lib/foo.so",
      // Write a sample_info even though we shouldn't (sample_id_all==false)
      // Use a bogus ID: if we look at the sample_id and look for an attr, it
      // should return an error.
      testing::SampleInfo().Tid(1001).Id(666));

  const size_t data_size = sample_event.GetSize() + mmap_event.GetSize();
  const uint32_t features = 0;

  // header
  testing::ExamplePerfDataFileHeader file_header(features);
  file_header.WithAttrIdsCount(1).WithAttrCount(1).WithDataSize(data_size);
  file_header.WriteTo(&input);

  // attr IDs
  testing::AttrIdsSection attr_ids(input.tellp());
  const auto id_section = attr_ids.AddId(48);
  attr_ids.WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(
      PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_ID,
      false /*sample_id_all*/)
      .WithIds(id_section)
      .WriteTo(&input);

  // data

  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  sample_event.WriteTo(&input);
  mmap_event.WriteTo(&input);
  ASSERT_EQ(file_header.header().data.offset + data_size,
            static_cast<u64>(input.tellp()));

  // no metadata

  //
  // Parse input.
  //

  PerfReader pr;
  // PerfReader should error as the PERF_RECORD_MMAP event has invalid size
  // because the event includes sample info data when sample_id.all is false.
  EXPECT_FALSE(pr.ReadFromString(input.str()));
}

// Regression test for http://crbug.com/496441
TEST(PerfReaderTest, LargePerfEventAttr) {
  std::stringstream input;

  const size_t attr_size = sizeof(perf_event_attr) + sizeof(u64);
  testing::ExamplePerfSampleEvent sample_event(
      testing::SampleInfo().Ip(0x00000000002c100a).Tid(1002));
  const size_t data_size = sample_event.GetSize();

  // header
  testing::ExamplePerfDataFileHeader file_header(0);
  file_header.WithCustomPerfEventAttrSize(attr_size)
      .WithAttrCount(1)
      .WithDataSize(data_size);
  file_header.WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_IP | PERF_SAMPLE_TID,
                                        false /*sample_id_all*/)
      .WithAttrSize(attr_size)
      .WithConfig(456)
      .WriteTo(&input);

  // data

  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  sample_event.WriteTo(&input);
  ASSERT_EQ(file_header.header().data.offset + data_size,
            static_cast<u64>(input.tellp()));

  // no metadata

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  // Make sure the attr was recorded properly.
  EXPECT_EQ(1, pr.attrs().size());
  EXPECT_EQ(456, pr.attrs().Get(0).attr().config());

  // Verify subsequent sample event was read properly.
  ASSERT_EQ(1, pr.events().size());

  const PerfEvent& event = pr.events().Get(0);
  EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());
  EXPECT_EQ(data_size, event.header().size());

  const SampleEvent& sample_info = event.sample_event();
  EXPECT_EQ(0x00000000002c100a, sample_info.ip());
  EXPECT_EQ(1002, sample_info.tid());
}

// Regression test for http://crbug.com/496441
TEST(PerfReaderTest, LargePerfEventAttrPiped) {
  std::stringstream input;

  // pipe header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  testing::ExamplePerfEventAttrEvent_Hardware(PERF_SAMPLE_IP | PERF_SAMPLE_TID,
                                              true /*sample_id_all*/)
      .WithAttrSize(sizeof(perf_event_attr) + sizeof(u64))
      .WithConfig(123)
      .WriteTo(&input);

  // PERF_RECORD_HEADER_EVENT_TYPE
  const struct event_type_event event_type = {
      .header =
          {
              .type = PERF_RECORD_HEADER_EVENT_TYPE,
              .misc = 0,
              .size = sizeof(struct event_type_event),
          },
      .event_type = {
          /*event_id*/ 123,
          /*name*/ "cycles",
      },
  };
  input.write(reinterpret_cast<const char*>(&event_type), sizeof(event_type));

  testing::ExamplePerfSampleEvent sample_event(
      testing::SampleInfo().Ip(0x00000000002c100a).Tid(1002));
  sample_event.WriteTo(&input);

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  // Make sure the attr was recorded properly.
  EXPECT_EQ(1, pr.attrs().size());
  EXPECT_EQ(123, pr.attrs().Get(0).attr().config());
  ASSERT_EQ(1, pr.event_types().size());
  EXPECT_EQ("cycles", pr.event_types().Get(0).name());

  // Verify subsequent sample event was read properly.
  ASSERT_EQ(1, pr.events().size());

  const PerfEvent& event = pr.events().Get(0);
  EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());
  EXPECT_EQ(sample_event.GetSize(), event.header().size());

  const SampleEvent& sample_info = event.sample_event();
  EXPECT_EQ(0x00000000002c100a, sample_info.ip());
  EXPECT_EQ(1002, sample_info.tid());
}

// Regression test for http://crbug.com/496441
TEST(PerfReaderTest, SmallPerfEventAttr) {
  std::stringstream input;

  const size_t attr_size = sizeof(perf_event_attr) - sizeof(u64);
  testing::ExamplePerfSampleEvent sample_event(
      testing::SampleInfo().Ip(0x00000000002c100a).Tid(1002));
  const size_t data_size = sample_event.GetSize();

  // header
  testing::ExamplePerfDataFileHeader file_header(0);
  file_header.WithCustomPerfEventAttrSize(attr_size)
      .WithAttrCount(1)
      .WithDataSize(data_size);
  file_header.WriteTo(&input);

  // attrs
  CHECK_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_IP | PERF_SAMPLE_TID,
                                        false /*sample_id_all*/)
      .WithAttrSize(attr_size)
      .WithConfig(456)
      .WriteTo(&input);

  // data

  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  sample_event.WriteTo(&input);
  ASSERT_EQ(file_header.header().data.offset + data_size,
            static_cast<u64>(input.tellp()));

  // no metadata

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  // Make sure the attr was recorded properly.
  EXPECT_EQ(1, pr.attrs().size());
  EXPECT_EQ(456, pr.attrs().Get(0).attr().config());

  // Verify subsequent sample event was read properly.
  ASSERT_EQ(1, pr.events().size());

  const PerfEvent& event = pr.events().Get(0);
  EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());
  EXPECT_EQ(data_size, event.header().size());

  const SampleEvent& sample_info = event.sample_event();
  EXPECT_EQ(0x00000000002c100a, sample_info.ip());
  EXPECT_EQ(1002, sample_info.tid());
}

// Regression test for http://crbug.com/496441
TEST(PerfReaderTest, SmallPerfEventAttrPiped) {
  std::stringstream input;

  // pipe header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  testing::ExamplePerfEventAttrEvent_Hardware(PERF_SAMPLE_IP | PERF_SAMPLE_TID,
                                              true /*sample_id_all*/)
      .WithAttrSize(sizeof(perf_event_attr) - sizeof(u64))
      .WithConfig(123)
      .WriteTo(&input);

  // PERF_RECORD_HEADER_EVENT_TYPE
  const struct event_type_event event_type = {
      .header =
          {
              .type = PERF_RECORD_HEADER_EVENT_TYPE,
              .misc = 0,
              .size = sizeof(struct event_type_event),
          },
      .event_type = {
          /*event_id*/ 123,
          /*name*/ "cycles",
      },
  };
  input.write(reinterpret_cast<const char*>(&event_type), sizeof(event_type));

  testing::ExamplePerfSampleEvent sample_event(
      testing::SampleInfo().Ip(0x00000000002c100a).Tid(1002));
  sample_event.WriteTo(&input);

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  // Make sure the attr was recorded properly.
  EXPECT_EQ(1, pr.attrs().size());
  EXPECT_EQ(123, pr.attrs().Get(0).attr().config());
  ASSERT_EQ(1, pr.event_types().size());
  EXPECT_EQ("cycles", pr.event_types().Get(0).name());

  // Verify subsequent sample event was read properly.
  ASSERT_EQ(1, pr.events().size());

  const PerfEvent& event = pr.events().Get(0);
  EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());
  EXPECT_EQ(sample_event.GetSize(), event.header().size());

  const SampleEvent& sample_info = event.sample_event();
  EXPECT_EQ(0x00000000002c100a, sample_info.ip());
  EXPECT_EQ(1002, sample_info.tid());
}

TEST(PerfReaderTest, CrossEndianAttrs) {
  for (bool is_cross_endian : {true, false}) {
    LOG(INFO) << "Testing with cross endianness = " << is_cross_endian;

    std::stringstream input;

    // header
    const uint32_t features = 0;
    testing::ExamplePerfDataFileHeader file_header(features);
    file_header.WithAttrCount(3)
        .WithCrossEndianness(is_cross_endian)
        .WriteTo(&input);

    // attrs
    CHECK_EQ(file_header.header().attrs.offset,
             static_cast<u64>(input.tellp()));
    // Provide two attrs with different sample_id_all values to test the
    // correctness of byte swapping of the bit fields.
    testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_IP | PERF_SAMPLE_TID,
                                          true /*sample_id_all*/)
        .WithConfig(123)
        .WithCrossEndianness(is_cross_endian)
        .WriteTo(&input);
    testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_IP | PERF_SAMPLE_TID,
                                          true /*sample_id_all*/)
        .WithConfig(456)
        .WithCrossEndianness(is_cross_endian)
        .WriteTo(&input);
    testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_IP | PERF_SAMPLE_TID,
                                          false /*sample_id_all*/)
        .WithConfig(456)
        .WithCrossEndianness(is_cross_endian)
        .WriteTo(&input);

    // No data.
    // No metadata.

    // Read data.

    PerfReader pr;
    ASSERT_TRUE(pr.ReadFromString(input.str()));

    // Make sure the attr was recorded properly.
    EXPECT_EQ(3, pr.attrs().size());

    const auto& attr0 = pr.attrs().Get(0).attr();
    EXPECT_EQ(123, attr0.config());
    EXPECT_EQ(1, attr0.sample_period());
    EXPECT_EQ(PERF_SAMPLE_IP | PERF_SAMPLE_TID, attr0.sample_type());
    EXPECT_TRUE(attr0.sample_id_all());
    EXPECT_EQ(2, attr0.precise_ip());

    const auto& attr1 = pr.attrs().Get(1).attr();
    EXPECT_EQ(456, attr1.config());
    EXPECT_EQ(1, attr1.sample_period());
    EXPECT_EQ(PERF_SAMPLE_IP | PERF_SAMPLE_TID, attr1.sample_type());
    EXPECT_TRUE(attr1.sample_id_all());
    EXPECT_EQ(2, attr1.precise_ip());

    const auto& attr2 = pr.attrs().Get(2).attr();
    EXPECT_EQ(456, attr2.config());
    EXPECT_EQ(1, attr2.sample_period());
    EXPECT_EQ(PERF_SAMPLE_IP | PERF_SAMPLE_TID, attr2.sample_type());
    EXPECT_FALSE(attr2.sample_id_all());
    EXPECT_EQ(2, attr2.precise_ip());
  }
}

TEST(PerfReaderTest, CrossEndianNormalPerfData) {
  // data
  // Do this before header to compute the total data size.
  std::stringstream input_data;
  std::vector<u8> build_id{0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x12, 0x34};
  testing::ExampleMmapEvent(
      1234, 0x0000000000810000, 0x10000, 0x2000, "/usr/lib/foo.so",
      testing::SampleInfo().Tid(bswap_32(1234), bswap_32(1235)))
      .WithCrossEndianness(true)
      .WriteTo(&input_data);
  testing::ExampleForkEvent(
      1236, 1234, 1237, 1235, 30ULL * 1000000000,
      testing::SampleInfo().Tid(bswap_32(1236), bswap_32(1237)))
      .WithCrossEndianness(true)
      .WriteTo(&input_data);
  testing::ExamplePerfSampleEvent(testing::SampleInfo()
                                      .Ip(bswap_64(0x0000000000810100))
                                      .Tid(bswap_32(1234), bswap_32(1235)))
      .WithCrossEndianness(true)
      .WriteTo(&input_data);
  testing::ExamplePerfSampleEvent(testing::SampleInfo()
                                      .Ip(bswap_64(0x000000000081ff00))
                                      .Tid(bswap_32(1236), bswap_32(1237)))
      .WithCrossEndianness(true)
      .WriteTo(&input_data);
  testing::ExampleMmap2Event(
      1234, 1235, 0x0000000000c00000, 0x10000, 0x1000, "/usr/lib/bar.so",
      testing::SampleInfo().Tid(bswap_32(1234), bswap_32(1235)))
      .WithDeviceInfo(8, 1, 9876)
      .WithCrossEndianness(true)
      .WriteTo(&input_data);
  testing::ExampleMmap2Event(
      1234, 1235, 0x0000000000d00000, 0x20000, 0, "/usr/lib/baz.so",
      testing::SampleInfo().Tid(bswap_32(1234), bswap_32(1235)))
      .WithMisc(PERF_RECORD_MISC_MMAP_BUILD_ID)
      .WithBuildId(build_id.data(), build_id.size())
      .WithCrossEndianness(true)
      .WriteTo(&input_data);

  std::stringstream input;

  // header
  const size_t data_size = input_data.str().size();
  const uint32_t features = (1 << HEADER_HOSTNAME) | (1 << HEADER_OSRELEASE);
  testing::ExamplePerfDataFileHeader file_header(features);
  file_header.WithAttrCount(1)
      .WithDataSize(data_size)
      .WithCrossEndianness(true)
      .WriteTo(&input);

  // attrs
  CHECK_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_IP | PERF_SAMPLE_TID,
                                        true /*sample_id_all*/)
      .WithConfig(456)
      .WithCrossEndianness(true)
      .WriteTo(&input);

  // Write data.

  u64 data_offset = file_header.header().data.offset;
  ASSERT_EQ(data_offset, static_cast<u64>(input.tellp()));
  input << input_data.str();
  ASSERT_EQ(data_offset + data_size, static_cast<u64>(input.tellp()));

  // metadata
  size_t metadata_offset =
      file_header.data_end() + 2 * sizeof(perf_file_section);

  // HEADER_HOSTNAME
  testing::ExampleStringMetadata hostname_metadata("hostname", metadata_offset);
  hostname_metadata.WithCrossEndianness(true);
  metadata_offset += hostname_metadata.size();

  // HEADER_OSRELEASE
  testing::ExampleStringMetadata osrelease_metadata("osrelease",
                                                    metadata_offset);
  osrelease_metadata.WithCrossEndianness(true);
  metadata_offset += osrelease_metadata.size();

  hostname_metadata.index_entry().WriteTo(&input);
  osrelease_metadata.index_entry().WriteTo(&input);

  hostname_metadata.WriteTo(&input);
  osrelease_metadata.WriteTo(&input);

  //
  // Parse input.
  //

  PerfReader pr;
  ASSERT_TRUE(pr.ReadFromString(input.str()));

  // Make sure the attr was recorded properly.
  EXPECT_EQ(1, pr.attrs().size());
  EXPECT_EQ(456, pr.attrs().Get(0).attr().config());
  EXPECT_TRUE(pr.attrs().Get(0).attr().sample_id_all());

  // Verify perf events.
  ASSERT_EQ(6, pr.events().size());

  {
    const PerfEvent& event = pr.events().Get(0);
    EXPECT_EQ(PERF_RECORD_MMAP, event.header().type());
    EXPECT_EQ(1234, event.mmap_event().pid());
    EXPECT_EQ(1234, event.mmap_event().tid());
    EXPECT_EQ(std::string("/usr/lib/foo.so"), event.mmap_event().filename());
    EXPECT_EQ(0x0000000000810000, event.mmap_event().start());
    EXPECT_EQ(0x10000, event.mmap_event().len());
    EXPECT_EQ(0x2000, event.mmap_event().pgoff());
  }

  {
    const PerfEvent& event = pr.events().Get(1);
    EXPECT_EQ(PERF_RECORD_FORK, event.header().type());
    EXPECT_EQ(1236, event.fork_event().pid());
    EXPECT_EQ(1234, event.fork_event().ppid());
    EXPECT_EQ(1237, event.fork_event().tid());
    EXPECT_EQ(1235, event.fork_event().ptid());
    EXPECT_EQ(30ULL * 1000000000, event.fork_event().fork_time_ns());
  }

  {
    const PerfEvent& event = pr.events().Get(2);
    EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());

    const SampleEvent& sample_info = event.sample_event();
    EXPECT_EQ(0x0000000000810100, sample_info.ip());
    EXPECT_EQ(1234, sample_info.pid());
    EXPECT_EQ(1235, sample_info.tid());
  }

  {
    const PerfEvent& event = pr.events().Get(3);
    EXPECT_EQ(PERF_RECORD_SAMPLE, event.header().type());

    const SampleEvent& sample_info = event.sample_event();
    EXPECT_EQ(0x000000000081ff00, sample_info.ip());
    EXPECT_EQ(1236, sample_info.pid());
    EXPECT_EQ(1237, sample_info.tid());
  }

  {
    const PerfEvent& event = pr.events().Get(4);
    EXPECT_EQ(PERF_RECORD_MMAP2, event.header().type());
    EXPECT_EQ(0, event.header().misc());
    EXPECT_EQ(1234, event.mmap_event().pid());
    EXPECT_EQ(1235, event.mmap_event().tid());
    EXPECT_EQ(std::string("/usr/lib/bar.so"), event.mmap_event().filename());
    EXPECT_EQ(0x0000000000c00000, event.mmap_event().start());
    EXPECT_EQ(0x10000, event.mmap_event().len());
    EXPECT_EQ(0x1000, event.mmap_event().pgoff());
    EXPECT_EQ(8, event.mmap_event().maj());
    EXPECT_EQ(1, event.mmap_event().min());
    EXPECT_EQ(9876, event.mmap_event().ino());
  }

  {
    const PerfEvent& event = pr.events().Get(5);
    EXPECT_EQ(PERF_RECORD_MMAP2, event.header().type());
    EXPECT_EQ(PERF_RECORD_MISC_MMAP_BUILD_ID, event.header().misc());
    EXPECT_EQ(1234, event.mmap_event().pid());
    EXPECT_EQ(1235, event.mmap_event().tid());
    EXPECT_EQ(std::string("/usr/lib/baz.so"), event.mmap_event().filename());
    EXPECT_EQ(0x0000000000d00000, event.mmap_event().start());
    EXPECT_EQ(0x20000, event.mmap_event().len());
    EXPECT_EQ(0, event.mmap_event().pgoff());
    // below fields should be zero when a build-id is given
    EXPECT_EQ(0, event.mmap_event().maj());
    EXPECT_EQ(0, event.mmap_event().min());
    EXPECT_EQ(0, event.mmap_event().ino());
  }

  // Verify perf build id.
  EXPECT_EQ(1, pr.build_ids().size());

  {
    const PerfBuildID& buildId = pr.build_ids().Get(0);
    EXPECT_EQ(std::string("/usr/lib/baz.so"), buildId.filename());
    EXPECT_EQ(std::string(build_id.begin(), build_id.end()),
              buildId.build_id_hash());
  }
}

TEST(PerfReaderTest, MetadataMaskInitialized) {
  // The metadata mask is actually an array of uint64_t's. The accessors/mutator
  // in PerfReader depend on it being initialized.
  PerfReader reader;
  ASSERT_EQ(1U, reader.proto().metadata_mask().size());
  EXPECT_EQ(0U, reader.metadata_mask());
}

TEST(PerfReaderTest, UnsupportedPerfEvent) {
  std::stringstream input;
  // Do this before header to compute the total data size.
  struct perf_event_header event = {
      .type = PERF_RECORD_MAX,
      .misc = 0,
      .size = sizeof(struct perf_event_header),
  };

  const size_t data_size = sizeof(event);

  // header
  testing::ExamplePerfDataFileHeader file_header(0);
  file_header.WithAttrCount(1).WithDataSize(data_size).WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_TID, true /*sample_id_all*/)
      .WriteTo(&input);

  // data
  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  input.write(reinterpret_cast<const char*>(&event), data_size);
  ASSERT_EQ(file_header.header().data.offset + data_size,
            static_cast<u64>(input.tellp()));
  // no metadata

  //
  // Parse input.
  //

  PerfReader pr;
  EXPECT_TRUE(pr.ReadFromString(input.str()));

  // Make sure the attr was recorded properly.
  EXPECT_EQ(1, pr.attrs().size());
  EXPECT_TRUE(pr.attrs().Get(0).attr().sample_id_all());

  // Verify perf events.
  ASSERT_EQ(0, pr.events().size());
}

TEST(PerfReaderTest, MMapEventWithZeroEventSize) {
  // Do this before header to compute the total data size.
  std::stringstream input;

  // PERF_RECORD_MMAP
  testing::ExampleMmapEvent mmap_event(1001, 0x1c1000, 0x1000, 0,
                                       "/usr/lib/foo.so",
                                       testing::SampleInfo().Tid(1001));

  const size_t data_size = mmap_event.GetSize();

  // header
  testing::ExamplePerfDataFileHeader file_header(0);
  file_header.WithAttrCount(1).WithDataSize(data_size).WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_TID, true /*sample_id_all*/)
      .WriteTo(&input);

  // data
  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  mmap_event.WriteToWithEventSize(&input, 0);
  ASSERT_EQ(file_header.header().data.offset + data_size,
            static_cast<u64>(input.tellp()));
  // no metadata

  //
  // Parse input.
  //

  PerfReader pr;
  EXPECT_FALSE(pr.ReadFromString(input.str()));
}

TEST(PerfReaderTest, PipedMMapEventWithZeroEventSize) {
  // Do this before header to compute the total data size.
  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  testing::ExamplePerfEventAttrEvent_Hardware(PERF_SAMPLE_TID,
                                              true /*sample_id_all*/)
      .WriteTo(&input);

  // PERF_RECORD_MMAP
  testing::ExampleMmapEvent(1001, 0x1c1000, 0x1000, 0, "/usr/lib/foo.so",
                            testing::SampleInfo().Tid(1001))
      .WriteToWithEventSize(&input, 0);

  //
  // Parse input.
  //

  PerfReader pr;
  EXPECT_FALSE(pr.ReadFromString(input.str()));
}

TEST(PerfReaderTest, MMapEventWithZeroEventDataSize) {
  // Do this before header to compute the total data size.
  std::stringstream input;

  // PERF_RECORD_MMAP
  testing::ExampleMmapEvent mmap_event(1001, 0x1c1000, 0x1000, 0,
                                       "/usr/lib/foo.so",
                                       testing::SampleInfo().Tid(1001));

  const size_t data_size = mmap_event.GetSize();

  // header
  testing::ExamplePerfDataFileHeader file_header(0);
  file_header.WithAttrCount(1).WithDataSize(data_size).WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_TID, true /*sample_id_all*/)
      .WriteTo(&input);

  // data
  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  mmap_event.WriteToWithEventSize(&input, sizeof(struct perf_event_header));
  ASSERT_EQ(file_header.header().data.offset + data_size,
            static_cast<u64>(input.tellp()));
  // no metadata

  //
  // Parse input.
  //

  PerfReader pr;
  EXPECT_FALSE(pr.ReadFromString(input.str()));
}

TEST(PerfReaderTest, PipedMMapEventWithZeroEventDataSize) {
  // Do this before header to compute the total data size.
  std::stringstream input;

  // header
  testing::ExamplePipedPerfDataFileHeader().WriteTo(&input);

  // data

  // PERF_RECORD_HEADER_ATTR
  testing::ExamplePerfEventAttrEvent_Hardware(PERF_SAMPLE_TID,
                                              true /*sample_id_all*/)
      .WriteTo(&input);

  // PERF_RECORD_MMAP
  testing::ExampleMmapEvent(1001, 0x1c1000, 0x1000, 0, "/usr/lib/foo.so",
                            testing::SampleInfo().Tid(1001))
      .WriteToWithEventSize(&input, sizeof(struct perf_event_header));

  //
  // Parse input.
  //

  PerfReader pr;
  EXPECT_FALSE(pr.ReadFromString(input.str()));
}

TEST(PerfReaderTest, ReadSkipsIncompleteMMapEvent) {
  // Do this before header to compute the total data size.
  std::stringstream input;

  // PERF_RECORD_MMAP2
  testing::ExampleMmap2Event mmap2_event(1002, 0x2c1000, 0x2000, 0x3000,
                                         "/usr/lib/bar.so",
                                         testing::SampleInfo().Tid(1002));
  mmap2_event.WithMisc(PERF_RECORD_MISC_PROC_MAP_PARSE_TIMEOUT);

  const size_t data_size = mmap2_event.GetSize();

  // header
  testing::ExamplePerfDataFileHeader file_header(0);
  file_header.WithAttrCount(1).WithDataSize(data_size).WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_TID, true /*sample_id_all*/)
      .WriteTo(&input);

  // data
  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  mmap2_event.WriteTo(&input);
  ASSERT_EQ(file_header.header().data.offset + data_size,
            static_cast<u64>(input.tellp()));
  // no metadata

  //
  // Parse input.
  //

  PerfReader pr;
  EXPECT_TRUE(pr.ReadFromString(input.str()));

  // Verify the mmap event got skipped.
  ASSERT_EQ(0, pr.events().size());
}

TEST(PerfReaderTest, ReadSkipsInvalidKernelMMapEventFromUserspaceProfile) {
  // Do this before header to compute the total data size.
  std::stringstream input;

  // PERF_RECORD_MMAP
  testing::ExampleMmapEvent mmap_event(
      0, 0x0, 0x0, 0x0, "[kernel.kallsyms]_text", testing::SampleInfo().Tid(0));
  mmap_event.WithMisc(PERF_RECORD_MISC_KERNEL);

  const size_t data_size = mmap_event.GetSize();

  // header
  testing::ExamplePerfDataFileHeader file_header(0);
  file_header.WithAttrCount(1).WithDataSize(data_size).WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_TID, true /*sample_id_all*/)
      .WithExcludeKernel(true)
      .WriteTo(&input);

  // data
  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  mmap_event.WriteTo(&input);
  ASSERT_EQ(file_header.header().data.offset + data_size,
            static_cast<u64>(input.tellp()));
  // no metadata

  //
  // Parse input.
  //

  PerfReader pr;
  EXPECT_TRUE(pr.ReadFromString(input.str()));

  // Verify the mmap event got skipped.
  ASSERT_EQ(0, pr.events().size());
}

TEST(PerfReaderTest, MMap2EventWithBuildId) {
  std::stringstream input;
  // check whether it can handle NUL byte in the build-id
  std::vector<u8> build_id = {0x0,  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                              0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

  // PERF_RECORD_MMAP2
  testing::ExampleMmap2Event mmap_event(1001, 0x1c1000, 0x1000, 0,
                                        "/usr/lib/foo.so",
                                        testing::SampleInfo().Tid(1001));
  mmap_event.WithMisc(PERF_RECORD_MISC_MMAP_BUILD_ID);
  mmap_event.WithDeviceInfo(8, 9, 10);  // dummy
  mmap_event.WithBuildId(build_id.data(), build_id.size());

  const size_t data_size = mmap_event.GetSize();

  // header
  testing::ExamplePerfDataFileHeader file_header(0);
  file_header.WithAttrCount(1).WithDataSize(data_size).WriteTo(&input);

  // attrs
  ASSERT_EQ(file_header.header().attrs.offset, static_cast<u64>(input.tellp()));
  testing::ExamplePerfFileAttr_Hardware(PERF_SAMPLE_TID, true /*sample_id_all*/)
      .WriteTo(&input);

  // data
  ASSERT_EQ(file_header.header().data.offset, static_cast<u64>(input.tellp()));
  mmap_event.WriteTo(&input);
  ASSERT_EQ(file_header.header().data.offset + data_size,
            static_cast<u64>(input.tellp()));
  // no metadata

  //
  // Parse input.
  //

  PerfReader pr;
  EXPECT_TRUE(pr.ReadFromString(input.str()));

  // processing MMAP2 w/ build-id should create a build-id record
  ASSERT_EQ(pr.build_ids().size(), 1);
  const auto& build_id2 = pr.build_ids().at(0);

  ASSERT_EQ(build_id.size(), build_id2.build_id_hash().size());
  ASSERT_EQ(memcmp(build_id.data(), build_id2.build_id_hash().c_str(),
                   build_id.size()),
            0);

  ASSERT_EQ(pr.events().size(), 1);
  ASSERT_TRUE(pr.events().at(0).header().misc() &
              PERF_RECORD_MISC_MMAP_BUILD_ID);

  const auto& mmap2 = pr.events().at(0).mmap_event();
  // it clears all device info when build-id is set
  ASSERT_EQ(mmap2.maj(), 0);
  ASSERT_EQ(mmap2.min(), 0);
  ASSERT_EQ(mmap2.ino(), 0);
  ASSERT_EQ(mmap2.ino_generation(), 0);
}

}  // namespace quipper
