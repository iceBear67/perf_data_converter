// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sample_info_reader.h"

#include <byteswap.h>

#include "compat/test.h"
#include "kernel/perf_event.h"
#include "kernel/perf_internals.h"
#include "test_perf_data.h"
#include "test_utils.h"

namespace quipper {

using testing::PunU32U64;

TEST(SampleInfoReaderTest, ReadSampleEvent) {
  // clang-format off
  uint64_t sample_type =       // * == in sample_id_all
      PERF_SAMPLE_IP |
      PERF_SAMPLE_TID |        // *
      PERF_SAMPLE_TIME |       // *
      PERF_SAMPLE_ADDR |
      PERF_SAMPLE_ID |         // *
      PERF_SAMPLE_STREAM_ID |  // *
      PERF_SAMPLE_CPU |        // *
      PERF_SAMPLE_PERIOD |
      PERF_SAMPLE_WEIGHT |
      PERF_SAMPLE_DATA_SRC |
      PERF_SAMPLE_TRANSACTION |
      PERF_SAMPLE_PHYS_ADDR |
      PERF_SAMPLE_CGROUP |
      PERF_SAMPLE_DATA_PAGE_SIZE |
      PERF_SAMPLE_CODE_PAGE_SIZE;
  // clang-format on
  struct perf_event_attr attr = {0};
  attr.sample_type = sample_type;

  SampleInfoReader reader(attr, false /* read_cross_endian */);

  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415837014 * 1000000000ULL,            // TIME
      0x00007f999c38d15a,                    // ADDR
      2,                                     // ID
      1,                                     // STREAM_ID
      8,                                     // CPU
      10001,                                 // PERIOD
      12345,                                 // WEIGHT
      0x68100142,                            // DATA_SRC
      67890,                                 // TRANSACTIONS
      0x00003f324c43d23b,                    // PHYSICAL_ADDR
      3333,                                  // CGROUP
      4096,                                  // DATA_PAGE_SIZE
      2 * 1024 * 1024,                       // CODE_PAGE_SIZE
  };
  const sample_event sample_event_struct = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = 0,
          .size = sizeof(sample_event) + sizeof(sample_event_array),
      }};

  std::stringstream input;
  input.write(reinterpret_cast<const char*>(&sample_event_struct),
              sizeof(sample_event_struct));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));
  std::string input_string = input.str();
  const event_t& event = *reinterpret_cast<const event_t*>(input_string.data());

  perf_sample sample;
  ASSERT_TRUE(reader.ReadPerfSampleInfo(event, &sample));

  EXPECT_EQ(0xffffffff01234567, sample.ip);
  EXPECT_EQ(0x68d, sample.pid);
  EXPECT_EQ(0x68e, sample.tid);
  EXPECT_EQ(1415837014 * 1000000000ULL, sample.time);
  EXPECT_EQ(0x00007f999c38d15a, sample.addr);
  EXPECT_EQ(2, sample.id);
  EXPECT_EQ(1, sample.stream_id);
  EXPECT_EQ(8, sample.cpu);
  EXPECT_EQ(10001, sample.period);
  EXPECT_EQ(12345, sample.weight.full);
  EXPECT_EQ(0x68100142, sample.data_src);
  EXPECT_EQ(67890, sample.transaction);
  EXPECT_EQ(0x00003f324c43d23b, sample.physical_addr);
  EXPECT_EQ(3333, sample.cgroup);
  EXPECT_EQ(4096, sample.data_page_size);
  EXPECT_EQ(2 * 1024 * 1024, sample.code_page_size);
}

TEST(SampleInfoReaderTest, ReadSampleEventWeightStruct) {
  // clang-format off
  uint64_t sample_type =       // * == in sample_id_all
      PERF_SAMPLE_IP |
      PERF_SAMPLE_TID |        // *
      PERF_SAMPLE_TIME |       // *
      PERF_SAMPLE_ADDR |
      PERF_SAMPLE_ID |         // *
      PERF_SAMPLE_STREAM_ID |  // *
      PERF_SAMPLE_CPU |        // *
      PERF_SAMPLE_PERIOD |
      PERF_SAMPLE_DATA_SRC |
      PERF_SAMPLE_TRANSACTION |
      PERF_SAMPLE_PHYS_ADDR |
      PERF_SAMPLE_CGROUP |
      PERF_SAMPLE_DATA_PAGE_SIZE |
      PERF_SAMPLE_CODE_PAGE_SIZE |
      PERF_SAMPLE_WEIGHT_STRUCT;
  // clang-format on
  struct perf_event_attr attr = {0};
  attr.sample_type = sample_type;

  SampleInfoReader reader(attr, false /* read_cross_endian */);
  perf_sample_weight weight_struct;
  weight_struct.var1_dw = 1;
  weight_struct.var2_w = 2;
  weight_struct.var3_w = 3;

  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415837014 * 1000000000ULL,            // TIME
      0x00007f999c38d15a,                    // ADDR
      2,                                     // ID
      1,                                     // STREAM_ID
      8,                                     // CPU
      10001,                                 // PERIOD
      weight_struct.full,                    // WEIGHT_STRUCT
      0x68100142,                            // DATA_SRC
      67890,                                 // TRANSACTIONS
      0x00003f324c43d23b,                    // PHYSICAL_ADDR
      3333,                                  // CGROUP
      4096,                                  // DATA_PAGE_SIZE
      2 * 1024 * 1024,                       // CODE_PAGE_SIZE
  };
  const sample_event sample_event_struct = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = 0,
          .size = sizeof(sample_event) + sizeof(sample_event_array),
      }};

  std::stringstream input;
  input.write(reinterpret_cast<const char*>(&sample_event_struct),
              sizeof(sample_event_struct));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));
  std::string input_string = input.str();
  const event_t& event = *reinterpret_cast<const event_t*>(input_string.data());

  perf_sample sample;
  ASSERT_TRUE(reader.ReadPerfSampleInfo(event, &sample));

  EXPECT_EQ(0xffffffff01234567, sample.ip);
  EXPECT_EQ(0x68d, sample.pid);
  EXPECT_EQ(0x68e, sample.tid);
  EXPECT_EQ(1415837014 * 1000000000ULL, sample.time);
  EXPECT_EQ(0x00007f999c38d15a, sample.addr);
  EXPECT_EQ(2, sample.id);
  EXPECT_EQ(1, sample.stream_id);
  EXPECT_EQ(8, sample.cpu);
  EXPECT_EQ(10001, sample.period);
  EXPECT_EQ(0x68100142, sample.data_src);
  EXPECT_EQ(67890, sample.transaction);
  EXPECT_EQ(0x00003f324c43d23b, sample.physical_addr);
  EXPECT_EQ(3333, sample.cgroup);
  EXPECT_EQ(4096, sample.data_page_size);
  EXPECT_EQ(2 * 1024 * 1024, sample.code_page_size);
  EXPECT_EQ(1, sample.weight.var1_dw);
  EXPECT_EQ(2, sample.weight.var2_w);
  EXPECT_EQ(3, sample.weight.var3_w);
}

TEST(SampleInfoReaderTest, ReadSampleEventCrossEndian) {
  // clang-format off
  uint64_t sample_type =      // * == in sample_id_all
      PERF_SAMPLE_IP |
      PERF_SAMPLE_TID |        // *
      PERF_SAMPLE_TIME |       // *
      PERF_SAMPLE_ADDR |
      PERF_SAMPLE_ID |         // *
      PERF_SAMPLE_STREAM_ID |  // *
      PERF_SAMPLE_CPU |        // *
      PERF_SAMPLE_PERIOD;
  // clang-format on
  struct perf_event_attr attr = {0};
  attr.sample_type = sample_type;

  SampleInfoReader reader(attr, true /* read_cross_endian */);

  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415837014 * 1000000000ULL,            // TIME
      0x00007f999c38d15a,                    // ADDR
      2,                                     // ID
      1,                                     // STREAM_ID
      8,                                     // CPU
      10001,                                 // PERIOD
  };

  const sample_event sample_event_struct = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = 0,
          .size = sizeof(sample_event) + sizeof(sample_event_array),
      }};

  std::stringstream input;
  input.write(reinterpret_cast<const char*>(&sample_event_struct),
              sizeof(sample_event_struct));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));
  std::string input_string = input.str();
  const event_t& event = *reinterpret_cast<const event_t*>(input_string.data());

  perf_sample sample;
  ASSERT_TRUE(reader.ReadPerfSampleInfo(event, &sample));

  EXPECT_EQ(bswap_64(0xffffffff01234567), sample.ip);
  EXPECT_EQ(bswap_32(0x68d), sample.pid);  // 32-bit
  EXPECT_EQ(bswap_32(0x68e), sample.tid);  // 32-bit
  EXPECT_EQ(bswap_64(1415837014 * 1000000000ULL), sample.time);
  EXPECT_EQ(bswap_64(0x00007f999c38d15a), sample.addr);
  EXPECT_EQ(bswap_64(2), sample.id);
  EXPECT_EQ(bswap_64(1), sample.stream_id);
  EXPECT_EQ(bswap_32(8), sample.cpu);  // 32-bit
  EXPECT_EQ(bswap_64(10001), sample.period);
}

TEST(SampleInfoReaderTest, ReadMmapEvent) {
  // clang-format off
  uint64_t sample_type =      // * == in sample_id_all
      PERF_SAMPLE_IP |
      PERF_SAMPLE_TID |        // *
      PERF_SAMPLE_TIME |       // *
      PERF_SAMPLE_ADDR |
      PERF_SAMPLE_ID |         // *
      PERF_SAMPLE_STREAM_ID |  // *
      PERF_SAMPLE_CPU |        // *
      PERF_SAMPLE_PERIOD;
  // clang-format on
  struct perf_event_attr attr = {0};
  attr.sample_type = sample_type;
  attr.sample_id_all = true;

  SampleInfoReader reader(attr, false /* read_cross_endian */);

  // PERF_RECORD_MMAP
  ASSERT_EQ(40, offsetof(struct mmap_event, filename));
  const u64 mmap_sample_id[] = {
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1415911367 * 1000000000ULL,            // TIME
      3,                                     // ID
      2,                                     // STREAM_ID
      9,                                     // CPU
  };
  // clang-format off
  const size_t mmap_event_size =
      offsetof(struct mmap_event, filename) +
      10+6 /* ==16, nearest 64-bit boundary for filename */ +
      sizeof(mmap_sample_id);
  // clang-format on

  const char mmap_filename[10 + 6] = "/dev/zero";
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

  std::stringstream input;
  input.write(reinterpret_cast<const char*>(&written_mmap_event),
              offsetof(struct mmap_event, filename));
  input.write(mmap_filename, 10 + 6);
  input.write(reinterpret_cast<const char*>(mmap_sample_id),
              sizeof(mmap_sample_id));

  std::string input_string = input.str();
  const event_t& event = *reinterpret_cast<const event_t*>(input_string.data());

  perf_sample sample;
  ASSERT_TRUE(reader.ReadPerfSampleInfo(event, &sample));

  EXPECT_EQ(0x68d, sample.pid);
  EXPECT_EQ(0x68e, sample.tid);
  EXPECT_EQ(1415911367 * 1000000000ULL, sample.time);
  EXPECT_EQ(3, sample.id);
  EXPECT_EQ(2, sample.stream_id);
  EXPECT_EQ(9, sample.cpu);
}

TEST(SampleInfoReaderTest, ReadReadInfoAllFields) {
  struct perf_event_attr attr = {0};
  attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_READ;
  attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED |
                     PERF_FORMAT_TOTAL_TIME_RUNNING | PERF_FORMAT_ID |
                     PERF_FORMAT_LOST;

  SampleInfoReader reader(attr, false /* read_cross_endian */);

  // PERF_RECORD_SAMPLE
  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      // From kernel/perf_event.h:
      // struct read_format {
      //   { u64  value;
      //     { u64  time_enabled; } && PERF_FORMAT_TOTAL_TIME_ENABLED
      //     { u64  time_running; } && PERF_FORMAT_TOTAL_TIME_RUNNING
      //     { u64  id;           } && PERF_FORMAT_ID
      //     { u64  lost;         } && PERF_FORMAT_LOST
      //   } && !PERF_FORMAT_GROUP
      // };
      1000000,                     // READ: value
      1415837014 * 1000000000ULL,  // READ: time_enabled
      1234567890 * 1000000000ULL,  // READ: time_running
      0xabcdef,                    // READ: id
      0xfedcba,                    // READ: lost
  };
  sample_event sample_event_struct = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = 0,
          .size = sizeof(sample_event) + sizeof(sample_event_array),
      }};

  std::stringstream input;
  input.write(reinterpret_cast<const char*>(&sample_event_struct),
              sizeof(sample_event_struct));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));
  std::string input_string = input.str();
  const event_t& event = *reinterpret_cast<const event_t*>(input_string.data());

  perf_sample sample;
  ASSERT_TRUE(reader.ReadPerfSampleInfo(event, &sample));

  EXPECT_EQ(0xffffffff01234567, sample.ip);
  EXPECT_EQ(0x68d, sample.pid);
  EXPECT_EQ(0x68e, sample.tid);
  EXPECT_EQ(1415837014 * 1000000000ULL, sample.read.time_enabled);
  EXPECT_EQ(1234567890 * 1000000000ULL, sample.read.time_running);
  EXPECT_EQ(0xabcdef, sample.read.one.id);
  EXPECT_EQ(0xfedcba, sample.read.one.lost);
  EXPECT_EQ(1000000, sample.read.one.value);
}

TEST(SampleInfoReaderTest, ReadReadInfoOmitTotalTimeFields) {
  struct perf_event_attr attr = {0};
  attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_READ;
  // Omit the PERF_FORMAT_TOTAL_TIME_* fields.
  attr.read_format = PERF_FORMAT_ID;

  SampleInfoReader reader(attr, false /* read_cross_endian */);

  // PERF_RECORD_SAMPLE
  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1000000,                               // READ: value
      0xabcdef,                              // READ: id
  };
  sample_event sample_event_struct = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = 0,
          .size = sizeof(sample_event) + sizeof(sample_event_array),
      }};

  std::stringstream input;
  input.write(reinterpret_cast<const char*>(&sample_event_struct),
              sizeof(sample_event_struct));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));
  std::string input_string = input.str();
  const event_t& event = *reinterpret_cast<const event_t*>(input_string.data());

  perf_sample sample;
  ASSERT_TRUE(reader.ReadPerfSampleInfo(event, &sample));

  EXPECT_EQ(0xffffffff01234567, sample.ip);
  EXPECT_EQ(0x68d, sample.pid);
  EXPECT_EQ(0x68e, sample.tid);
  EXPECT_EQ(0xabcdef, sample.read.one.id);
  EXPECT_EQ(1000000, sample.read.one.value);
}

TEST(SampleInfoReaderTest, ReadReadInfoValueFieldOnly) {
  struct perf_event_attr attr = {0};
  attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_READ;
  // Omit all optional fields. The |value| field still remains.
  attr.read_format = 0;

  SampleInfoReader reader(attr, false /* read_cross_endian */);

  // PERF_RECORD_SAMPLE
  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      1000000,                               // READ: value
  };
  sample_event sample_event_struct = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = 0,
          .size = sizeof(sample_event) + sizeof(sample_event_array),
      }};

  std::stringstream input;
  input.write(reinterpret_cast<const char*>(&sample_event_struct),
              sizeof(sample_event_struct));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));
  std::string input_string = input.str();
  const event_t& event = *reinterpret_cast<const event_t*>(input_string.data());

  perf_sample sample;
  ASSERT_TRUE(reader.ReadPerfSampleInfo(event, &sample));

  EXPECT_EQ(0xffffffff01234567, sample.ip);
  EXPECT_EQ(0x68d, sample.pid);
  EXPECT_EQ(0x68e, sample.tid);
  EXPECT_EQ(1000000, sample.read.one.value);
}

TEST(SampleInfoReaderTest, ReadReadInfoWithGroups) {
  struct perf_event_attr attr = {0};
  attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_READ;
  // Omit the PERF_FORMAT_TOTAL_TIME_* fields.
  attr.read_format = PERF_FORMAT_ID | PERF_FORMAT_GROUP | PERF_FORMAT_LOST;

  SampleInfoReader reader(attr, false /* read_cross_endian */);

  // PERF_RECORD_SAMPLE
  const u64 sample_event_array[] = {
      0xffffffff01234567,                    // IP
      PunU32U64{.v32 = {0x68d, 0x68e}}.v64,  // TID (u32 pid, tid)
      3,                                     // READ: nr
      1000000,                               // READ: values[0].value
      0xabcdef,                              // READ: values[0].id
      0xfedcba,                              // READ: values[0].lost
      2000000,                               // READ: values[1].value
      0xdecaf0,                              // READ: values[1].id
      0x0faced,                              // READ: values[1].lost
      3000000,                               // READ: values[2].value
      0xbeef00,                              // READ: values[2].id
      0x00feeb,                              // READ: values[2].lost
  };
  sample_event sample_event_struct = {
      .header = {
          .type = PERF_RECORD_SAMPLE,
          .misc = 0,
          .size = sizeof(sample_event) + sizeof(sample_event_array),
      }};

  std::stringstream input;
  input.write(reinterpret_cast<const char*>(&sample_event_struct),
              sizeof(sample_event_struct));
  input.write(reinterpret_cast<const char*>(sample_event_array),
              sizeof(sample_event_array));
  std::string input_string = input.str();
  const event_t& event = *reinterpret_cast<const event_t*>(input_string.data());

  perf_sample sample;
  ASSERT_TRUE(reader.ReadPerfSampleInfo(event, &sample));

  EXPECT_EQ(0xffffffff01234567, sample.ip);
  EXPECT_EQ(0x68d, sample.pid);
  EXPECT_EQ(0x68e, sample.tid);
  EXPECT_EQ(3, sample.read.group.nr);
  ASSERT_NE(static_cast<void*>(NULL), sample.read.group.values);
  EXPECT_EQ(0xabcdef, sample.read.group.values[0].id);
  EXPECT_EQ(0xfedcba, sample.read.group.values[0].lost);
  EXPECT_EQ(1000000, sample.read.group.values[0].value);
  EXPECT_EQ(0xdecaf0, sample.read.group.values[1].id);
  EXPECT_EQ(0x0faced, sample.read.group.values[1].lost);
  EXPECT_EQ(2000000, sample.read.group.values[1].value);
  EXPECT_EQ(0xbeef00, sample.read.group.values[2].id);
  EXPECT_EQ(0x00feeb, sample.read.group.values[2].lost);
  EXPECT_EQ(3000000, sample.read.group.values[2].value);
}

TEST(SampleInfoReaderTest, WriteSampleEventWithZeroBranchStack) {
  // clang-format off
  uint64_t sample_type =
      PERF_SAMPLE_IP |
      PERF_SAMPLE_TID |
      PERF_SAMPLE_TIME |
      PERF_SAMPLE_CPU |
      PERF_SAMPLE_PERIOD |
      PERF_SAMPLE_BRANCH_STACK;
  // clang-format on
  struct perf_event_attr attr = {0};
  attr.sample_type = sample_type;
  SampleInfoReader reader(attr, false /* read_cross_endian */);

  size_t event_size = sizeof(sample_event) + (6 * sizeof(u64));

  malloced_unique_ptr<event_t> event_ptr(CallocMemoryForEvent(event_size));
  event_t* event = event_ptr.get();
  event->header.type = PERF_RECORD_SAMPLE;
  event->header.misc = 0;
  event->header.size = event_size;

  PunU32U64 pid_tid{.v32 = {0x68d, 0x68e}};

  perf_sample sample;
  sample.ip = 0xffffffff01234567;
  sample.pid = pid_tid.v32[0];
  sample.tid = pid_tid.v32[1];
  sample.time = 1415837014 * 1000000000ULL;
  sample.cpu = 8;
  sample.period = 10001;
  sample.no_hw_idx = true;

  ASSERT_TRUE(reader.WritePerfSampleInfo(sample, event));

  size_t offset = GetEventDataSize(*event);
  EXPECT_NE(0, offset);

  uint64_t* array =
      reinterpret_cast<uint64_t*>(event) + offset / sizeof(uint64_t);

  EXPECT_EQ(sample.ip, *array++);
  EXPECT_EQ(pid_tid.v64, *array++);
  EXPECT_EQ(sample.time, *array++);
  EXPECT_EQ(sample.cpu, *array++);
  EXPECT_EQ(sample.period, *array++);
  EXPECT_EQ(0, *array++);  // BRANCH_STACK.nr
}

TEST(SampleInfoReaderTest, WriteSampleEventWithReadInfo) {
  // clang-format off
  uint64_t sample_type =
      PERF_SAMPLE_IP |
      PERF_SAMPLE_TID |
      PERF_SAMPLE_TIME |
      PERF_SAMPLE_CPU |
      PERF_SAMPLE_PERIOD |
      PERF_SAMPLE_READ;
  // clang-format on
  struct perf_event_attr attr = {0};
  attr.sample_type = sample_type;
  attr.read_format = PERF_FORMAT_ID | PERF_FORMAT_LOST;
  SampleInfoReader reader(attr, false /* read_cross_endian */);

  size_t event_size = sizeof(sample_event) + (8 * sizeof(u64));

  malloced_unique_ptr<event_t> event_ptr(CallocMemoryForEvent(event_size));
  event_t* event = event_ptr.get();
  event->header.type = PERF_RECORD_SAMPLE;
  event->header.misc = 0;
  event->header.size = event_size;

  PunU32U64 pid_tid{.v32 = {0x68d, 0x68e}};

  perf_sample sample;
  sample.ip = 0xffffffff01234567;
  sample.pid = pid_tid.v32[0];
  sample.tid = pid_tid.v32[1];
  sample.time = 1415837014 * 1000000000ULL;
  sample.cpu = 8;
  sample.period = 10001;
  sample.read.one.value = 1000000;
  sample.read.one.id = 0xabcdef;
  sample.read.one.lost = 0xfedcba;

  ASSERT_TRUE(reader.WritePerfSampleInfo(sample, event));

  size_t offset = GetEventDataSize(*event);
  EXPECT_NE(0, offset);

  uint64_t* array =
      reinterpret_cast<uint64_t*>(event) + offset / sizeof(uint64_t);

  EXPECT_EQ(sample.ip, *array++);
  EXPECT_EQ(pid_tid.v64, *array++);
  EXPECT_EQ(sample.time, *array++);
  EXPECT_EQ(sample.cpu, *array++);
  EXPECT_EQ(sample.period, *array++);
  EXPECT_EQ(sample.read.one.value, *array++);  // READ.value
  EXPECT_EQ(sample.read.one.id, *array++);     // READ.id
  EXPECT_EQ(sample.read.one.lost, *array++);   // READ.lost
}

TEST(SampleInfoReaderTest, WriteSampleEventWithReadInfoWithGroups) {
  // clang-format off
  uint64_t sample_type =
      PERF_SAMPLE_IP |
      PERF_SAMPLE_TID |
      PERF_SAMPLE_TIME |
      PERF_SAMPLE_CPU |
      PERF_SAMPLE_PERIOD |
      PERF_SAMPLE_READ;
  // clang-format on
  struct perf_event_attr attr = {0};
  attr.sample_type = sample_type;
  attr.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID | PERF_FORMAT_LOST;
  SampleInfoReader reader(attr, false /* read_cross_endian */);

  size_t event_size = sizeof(sample_event) + (12 * sizeof(u64));

  malloced_unique_ptr<event_t> event_ptr(CallocMemoryForEvent(event_size));
  event_t* event = event_ptr.get();
  event->header.type = PERF_RECORD_SAMPLE;
  event->header.misc = 0;
  event->header.size = event_size;

  PunU32U64 pid_tid{.v32 = {0x68d, 0x68e}};

  perf_sample sample;
  sample.ip = 0xffffffff01234567;
  sample.pid = pid_tid.v32[0];
  sample.tid = pid_tid.v32[1];
  sample.time = 1415837014 * 1000000000ULL;
  sample.cpu = 8;
  sample.period = 10001;
  sample.read.group.nr = 2;
  sample.read.group.values = new sample_read_value[2];
  sample.read.group.values[0].value = 1000000;
  sample.read.group.values[0].id = 0xabcdef;
  sample.read.group.values[0].lost = 0xfedcba;
  sample.read.group.values[1].value = 2000000;
  sample.read.group.values[1].id = 0xdecaf0;
  sample.read.group.values[1].lost = 0x0faced;

  ASSERT_TRUE(reader.WritePerfSampleInfo(sample, event));

  size_t offset = GetEventDataSize(*event);
  EXPECT_NE(0, offset);

  uint64_t* array =
      reinterpret_cast<uint64_t*>(event) + offset / sizeof(uint64_t);

  EXPECT_EQ(sample.ip, *array++);
  EXPECT_EQ(pid_tid.v64, *array++);
  EXPECT_EQ(sample.time, *array++);
  EXPECT_EQ(sample.cpu, *array++);
  EXPECT_EQ(sample.period, *array++);
  EXPECT_EQ(sample.read.group.nr, *array++);               // READ.group.nr
  EXPECT_EQ(sample.read.group.values[0].value, *array++);  // READ.value
  EXPECT_EQ(sample.read.group.values[0].id, *array++);     // READ.id
  EXPECT_EQ(sample.read.group.values[0].lost, *array++);   // READ.lost
  EXPECT_EQ(sample.read.group.values[1].value, *array++);  // READ.value
  EXPECT_EQ(sample.read.group.values[1].id, *array++);     // READ.id
  EXPECT_EQ(sample.read.group.values[1].lost, *array++);   // READ.lost
}

}  // namespace quipper
