// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "test_perf_data.h"

#include <stddef.h>

#include <algorithm>
#include <ostream>  
#include <vector>

#include "base/logging.h"
#include "kernel/perf_event.h"
#include "kernel/perf_internals.h"
#include "perf_data_utils.h"

namespace quipper {
namespace testing {

namespace {

// Write extra bytes to an output stream.
void WriteExtraBytes(size_t size, std::ostream* out) {
  std::vector<char> padding(size);
  out->write(padding.data(), size);
}

u8 ReverseByte(u8 x) {
  x = (x & 0xf0) >> 4 | (x & 0x0f) << 4;  // exchange nibbles
  x = (x & 0xcc) >> 2 | (x & 0x33) << 2;  // exchange pairs
  x = (x & 0xaa) >> 1 | (x & 0x55) << 1;  // exchange neighbors
  return x;
}

void SwapBitfieldOfBits(u8* field, size_t len) {
  for (size_t i = 0; i < len; i++) {
    field[i] = ReverseByte(field[i]);
  }
}

}  // namespace

ExamplePerfDataFileHeader::ExamplePerfDataFileHeader(
    const unsigned long features) {  
  CHECK_EQ(112U, sizeof(perf_file_attr)) << "perf_file_attr has changed size";
  header_ = {
      .magic = kPerfMagic,
      .size = 104,
      .attr_size = sizeof(struct perf_file_attr),
      .attrs = {.offset = 104, .size = 0},
      .data = {.offset = 104, .size = 0},
      .event_types = {0},
      .adds_features = {features, 0, 0, 0},
  };
}

ExamplePerfDataFileHeader& ExamplePerfDataFileHeader::WithAttrIdsCount(
    size_t n) {
  attr_ids_count_ = n;
  UpdateSectionOffsets();
  return *this;
}

ExamplePerfDataFileHeader& ExamplePerfDataFileHeader::WithAttrCount(size_t n) {
  header_.attrs.size = n * header_.attr_size;
  UpdateSectionOffsets();
  return *this;
}

ExamplePerfDataFileHeader& ExamplePerfDataFileHeader::WithDataSize(size_t sz) {
  header_.data.size = sz;
  UpdateSectionOffsets();
  return *this;
}

ExamplePerfDataFileHeader&
ExamplePerfDataFileHeader::WithCustomPerfEventAttrSize(size_t sz) {
  size_t n_attrs = header_.attrs.size / header_.attr_size;
  // Calculate sizeof(perf_file_attr) given the custom sizeof(perf_event_attr)
  header_.attr_size = sz + sizeof(perf_file_section);
  // Re-calculate the attrs section size and update offsets.
  return WithAttrCount(n_attrs);
}

void ExamplePerfDataFileHeader::UpdateSectionOffsets() {
  u64 offset = header_.size;
  offset += attr_ids_count_ * sizeof(u64);
  header_.attrs.offset = offset;
  offset += header_.attrs.size;
  header_.data.offset = offset;
  offset += header_.data.size;
  CHECK_EQ(data_end_offset(), offset);  // aka, the metadata offset.
}

void ExamplePerfDataFileHeader::WriteTo(std::ostream* out) const {
  struct perf_file_header local_header = {
      .magic = MaybeSwap64(header_.magic),
      .size = MaybeSwap64(header_.size),
      .attr_size = MaybeSwap64(header_.attr_size),
      .attrs = {.offset = MaybeSwap64(header_.attrs.offset),
                .size = MaybeSwap64(header_.attrs.size)},
      .data = {.offset = MaybeSwap64(header_.data.offset),
               .size = MaybeSwap64(header_.data.size)},
      .event_types = {.offset = MaybeSwap64(header_.event_types.offset),
                      .size = MaybeSwap64(header_.event_types.size)},
      .adds_features = {0},
  };
  // Copy over the features bits manually since the byte swapping is more
  // complicated.
  // Add parentheses around sizeof(uint64_t) to quell -Wsizeof-array-div
  for (size_t i = 0; i < sizeof(header_.adds_features) / (sizeof(uint64_t));
       ++i) {
    reinterpret_cast<uint64_t*>(local_header.adds_features)[i] = MaybeSwap64(
        reinterpret_cast<const uint64_t*>(header_.adds_features)[i]);
  }

  out->write(reinterpret_cast<const char*>(&local_header),
             sizeof(local_header));
  // Use original header values that weren't endian-swapped.
  CHECK_EQ(static_cast<u64>(out->tellp()), header_.size);
}

void ExamplePipedPerfDataFileHeader::WriteTo(std::ostream* out) const {
  const perf_pipe_file_header header = {
      .magic = kPerfMagic,
      .size = 16,
  };
  out->write(reinterpret_cast<const char*>(&header), sizeof(header));
  CHECK_EQ(static_cast<u64>(out->tellp()), header.size);
}

void ExamplePerfEventAttrEvent_Hardware::WriteTo(std::ostream* out) const {
  // Due to the unnamed union fields (eg, sample_period), this structure can't
  // be initialized with designated initializers.
  perf_event_attr attr = {};
  attr.type = PERF_TYPE_HARDWARE;
  attr.size = attr_size_;
  attr.config = config_;
  attr.sample_period = 100001;
  attr.sample_type = sample_type_;
  attr.read_format = read_format_;
  attr.sample_id_all = sample_id_all_;
  attr.use_clockid = use_clockid_;
  attr.context_switch = context_switch_;
  attr.write_backward = write_backward_;
  attr.namespaces = namespaces_;
  attr.cgroup = cgroup_;
  attr.ksymbol = ksymbol_;

  const size_t event_size = sizeof(perf_event_header) + attr.size +
                            ids_.size() * sizeof(decltype(ids_)::value_type);

  const perf_event_header header = {
      .type = PERF_RECORD_HEADER_ATTR,
      .misc = 0,
      .size = static_cast<u16>(event_size),
  };

  out->write(reinterpret_cast<const char*>(&header), sizeof(header));
  out->write(reinterpret_cast<const char*>(&attr),
             std::min(sizeof(attr), static_cast<size_t>(attr_size_)));
  if (sizeof(attr) < attr_size_)
    WriteExtraBytes(attr_size_ - sizeof(attr), out);
  out->write(reinterpret_cast<const char*>(ids_.data()),
             ids_.size() * sizeof(decltype(ids_)::value_type));
}

void AttrIdsSection::WriteTo(std::ostream* out) const {
  out->write(reinterpret_cast<const char*>(ids_.data()),
             ids_.size() * sizeof(decltype(ids_)::value_type));
}

void ExamplePerfFileAttr_Hardware::WriteTo(std::ostream* out) const {
  // Due to the unnamed union fields (eg, sample_period), this structure can't
  // be initialized with designated initializers.
  perf_event_attr attr = {0};
  attr.type = MaybeSwap32(PERF_TYPE_HARDWARE);
  attr.size = MaybeSwap32(attr_size_);
  attr.config = MaybeSwap64(config_);
  attr.sample_period = MaybeSwap64(1);
  attr.sample_type = MaybeSwap64(sample_type_);
  // Bit fields.
  attr.sample_id_all = sample_id_all_;
  attr.precise_ip = 2;  // For testing a bit field that is more than one bit.
  attr.exclude_kernel = exclude_kernel_;
  attr.use_clockid = use_clockid_;
  attr.context_switch = context_switch_;
  attr.write_backward = write_backward_;
  attr.namespaces = namespaces_;
  attr.cgroup = cgroup_;
  attr.ksymbol = ksymbol_;

  if (is_cross_endian()) {
    // The order of operations here is for native-to-cross-endian conversion.
    // Contrast with similar code in PerfReader for cross-endian-to-native
    // conversion, which performs these swap operations in reverse order.
    const auto tmp = attr.precise_ip;
    attr.precise_ip = (tmp & 0x2) >> 1 | (tmp & 0x1) << 1;

    auto* const bitfield_start = &attr.read_format + 1;
    SwapBitfieldOfBits(reinterpret_cast<u8*>(bitfield_start), sizeof(u64));
  }

  // perf_event_attr can be of a size other than the static struct size. Thus we
  // cannot simply statically create a perf_file_attr (which contains a
  // perf_event_attr and a perf_file_section). Instead, create and write each
  // component separately.
  out->write(reinterpret_cast<const char*>(&attr),
             std::min(sizeof(attr), static_cast<size_t>(attr_size_)));
  if (sizeof(attr) < attr_size_)
    WriteExtraBytes(attr_size_ - sizeof(attr), out);

  perf_file_section ids_section;
  ids_section.offset = MaybeSwap64(ids_section_.offset);
  ids_section.size = MaybeSwap64(ids_section_.size);
  out->write(reinterpret_cast<const char*>(&ids_section), sizeof(ids_section));
}

void ExamplePerfFileAttr_Tracepoint::WriteTo(std::ostream* out) const {
  // Due to the unnamed union fields (eg, sample_period), this structure can't
  // be initialized with designated initializers.
  perf_event_attr attr = {};
  // See kernel src: tools/perf/util/evsel.c perf_evsel__newtp()
  attr.type = PERF_TYPE_TRACEPOINT;
  attr.size = sizeof(perf_event_attr);
  attr.config = tracepoint_event_id_;
  attr.sample_period = 1;
  attr.sample_type = (PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_TIME |
                      PERF_SAMPLE_CPU | PERF_SAMPLE_PERIOD | PERF_SAMPLE_RAW);

  const perf_file_attr file_attr = {
      .attr = attr,
      .ids = {.offset = 104, .size = 0},
  };
  out->write(reinterpret_cast<const char*>(&file_attr), sizeof(file_attr));
}

size_t ExampleMmapEvent::GetSize() const {
  return offsetof(struct mmap_event, filename) +
         GetUint64AlignedStringLength(filename_.size()) +
         sample_id_.size();  // sample_id_all
}

void ExampleMmapEvent::WriteTo(std::ostream* out) const {
  WriteToWithEventSize(out, GetSize());
}

void ExampleMmapEvent::WriteToWithEventSize(std::ostream* out,
                                            u16 event_size) const {
  struct mmap_event event = {
      .header =
          {
              .type = MaybeSwap32(PERF_RECORD_MMAP),
              .misc = misc_,
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .pid = MaybeSwap32(pid_),
      .tid = MaybeSwap32(pid_),
      .start = MaybeSwap64(start_),
      .len = MaybeSwap64(len_),
      .pgoff = MaybeSwap64(pgoff_),
      // .filename = ..., // written separately
  };

  const size_t pre_mmap_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(&event),
             offsetof(struct mmap_event, filename));
  const size_t filename_aligned_length =
      GetUint64AlignedStringLength(filename_.size());
  *out << filename_
       << std::string(filename_aligned_length - filename_.size(), '\0');
  out->write(sample_id_.data(), sample_id_.size());
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_mmap_offset;

  // GetSize() returns the actual event size.
  CHECK_EQ(GetSize(), static_cast<u64>(written_event_size));
}

size_t ExampleMmap2Event::GetSize() const {
  return offsetof(struct mmap2_event, filename) +
         GetUint64AlignedStringLength(filename_.size()) +
         sample_id_.size();  // sample_id_all
}

void ExampleMmap2Event::WriteTo(std::ostream* out) const {
  const size_t filename_aligned_length =
      GetUint64AlignedStringLength(filename_.size());
  const size_t event_size = GetSize();

  struct mmap2_event event = {
      .header =
          {
              .type = MaybeSwap32(PERF_RECORD_MMAP2),
              .misc = MaybeSwap16(misc_),
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .pid = MaybeSwap32(pid_),
      .tid = MaybeSwap32(tid_),
      .start = MaybeSwap64(start_),
      .len = MaybeSwap64(len_),
      .pgoff = MaybeSwap64(pgoff_),
      // union, .prot,.flags,.filename = ..., written separately
  };

  // Compilers handle unnamed union/struct initializers differently.
  // So it'd be safer to assign them after the initialization.
  event.maj = MaybeSwap32(maj_);
  event.min = MaybeSwap32(min_);
  event.ino = MaybeSwap64(ino_);
  event.ino_generation = MaybeSwap64(9);

  if (misc_ & PERF_RECORD_MISC_MMAP_BUILD_ID) {
    memcpy(event.build_id, build_id_, build_id_size_);
    if (build_id_size_ < sizeof(event.build_id))
      memset(&event.build_id[build_id_size_], 0,
             sizeof(event.build_id) - build_id_size_);
    event.build_id_size = build_id_size_;
    event.__reserved1 = 0;
    event.__reserved2 = 0;
  }

  event.prot = MaybeSwap32(prot_);
  event.flags = MaybeSwap32(flags_);

  const size_t pre_mmap_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(&event),
             offsetof(struct mmap2_event, filename));
  *out << filename_
       << std::string(filename_aligned_length - filename_.size(), '\0');
  out->write(sample_id_.data(), sample_id_.size());
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_mmap_offset;
  CHECK_EQ(GetSize(), static_cast<u64>(written_event_size));
}

void ExampleForkExitEvent::WriteTo(std::ostream* out) const {
  const size_t event_size = sizeof(struct fork_event) + sample_id_.size();

  struct fork_event event = {
      .header =
          {
              .type = MaybeSwap32(type_),
              .misc = 0,
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .pid = MaybeSwap32(pid_),
      .ppid = MaybeSwap32(ppid_),
      .tid = MaybeSwap32(tid_),
      .ptid = MaybeSwap32(ptid_),
      .time = MaybeSwap64(time_),
  };

  const size_t pre_event_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(&event), sizeof(event));
  out->write(sample_id_.data(), sample_id_.size());
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_event_offset;
  CHECK_EQ(MaybeSwap16(event.header.size),
           static_cast<u64>(written_event_size));
}

void FinishedRoundEvent::WriteTo(std::ostream* out) const {
  const perf_event_header event = {
      .type = PERF_RECORD_FINISHED_ROUND,
      .misc = 0,
      .size = sizeof(struct perf_event_header),
  };
  out->write(reinterpret_cast<const char*>(&event), sizeof(event));
}

size_t ExamplePerfSampleEvent::GetSize() const {
  return sizeof(struct sample_event) + sample_info_.size();
}

void ExamplePerfSampleEvent::WriteTo(std::ostream* out) const {
  const sample_event event = {
      .header = {
          .type = MaybeSwap32(PERF_RECORD_SAMPLE),
          .misc = MaybeSwap16(PERF_RECORD_MISC_USER),
          .size = MaybeSwap16(static_cast<u16>(GetSize())),
      }};
  out->write(reinterpret_cast<const char*>(&event), sizeof(event));
  out->write(sample_info_.data(), sample_info_.size());
}

ExamplePerfSampleEvent_BranchStack::ExamplePerfSampleEvent_BranchStack()
    : ExamplePerfSampleEvent(
          SampleInfo()
              .BranchStack_nr(16)
              .BranchStack_lbr(0x00007f4a313bb8cc, 0x00007f4a313bdb40, 0x02)
              .BranchStack_lbr(0x00007f4a30ce4de2, 0x00007f4a313bb8b3, 0x02)
              .BranchStack_lbr(0x00007f4a313bb8b0, 0x00007f4a30ce4de0, 0x01)
              .BranchStack_lbr(0x00007f4a30ff45c1, 0x00007f4a313bb8a0, 0x02)
              .BranchStack_lbr(0x00007f4a30ff49f2, 0x00007f4a30ff45bb, 0x02)
              .BranchStack_lbr(0x00007f4a30ff4a98, 0x00007f4a30ff49ed, 0x02)
              .BranchStack_lbr(0x00007f4a30ff4a7c, 0x00007f4a30ff4a91, 0x02)
              .BranchStack_lbr(0x00007f4a30ff4a34, 0x00007f4a30ff4a46, 0x02)
              .BranchStack_lbr(0x00007f4a30ff4c22, 0x00007f4a30ff4a0e, 0x02)
              .BranchStack_lbr(0x00007f4a30ff4bb3, 0x00007f4a30ff4c1b, 0x01)
              .BranchStack_lbr(0x00007f4a30ff4a09, 0x00007f4a30ff4b60, 0x02)
              .BranchStack_lbr(0x00007f4a30ff49e8, 0x00007f4a30ff4a00, 0x02)
              .BranchStack_lbr(0x00007f4a30ff42db, 0x00007f4a30ff49e0, 0x02)
              .BranchStack_lbr(0x00007f4a30ff42bb, 0x00007f4a30ff42d4, 0x02)
              .BranchStack_lbr(0x00007f4a333bf88b, 0x00007f4a30ff42ac, 0x02)
              .BranchStack_lbr(0x00007f4a333bf853, 0x00007f4a333bf885, 0x02)) {}

// Event size matching the event produced above
const size_t ExamplePerfSampleEvent_BranchStack::kEventSize =
    (1 /*perf_event_header*/ + 1 /*nr*/ + 16 * 3 /*lbr*/) * sizeof(u64);

void ExamplePerfSampleEvent_Tracepoint::WriteTo(std::ostream* out) const {
  const sample_event event = {.header = {
                                  .type = PERF_RECORD_SAMPLE,
                                  .misc = PERF_RECORD_MISC_USER,
                                  .size = 0x0078,
                              }};
  const u64 sample_event_array[] = {
      0x00007f999c38d15a,  // IP
      0x0000068d0000068d,  // TID (u32 pid, tid)
      0x0001e0211cbab7b9,  // TIME
      0x0000000000000000,  // CPU
      0x0000000000000001,  // PERIOD
      0x0000004900000044,  // RAW (u32 size = 0x44 = 68 = 4 + 8*sizeof(u64))
      0x000000090000068d,  //  .
      0x0000000000000000,  //  .
      0x0000100000000000,  //  .
      0x0000000300000000,  //  .
      0x0000002200000000,  //  .
      0xffffffff00000000,  //  .
      0x0000000000000000,  //  .
      0x0000000000000000,  //  .
  };
  CHECK_EQ(event.header.size,
           sizeof(event.header) + sizeof(sample_event_array));
  out->write(reinterpret_cast<const char*>(&event), sizeof(event));
  out->write(reinterpret_cast<const char*>(sample_event_array),
             sizeof(sample_event_array));
}

// Event size matching the event produced above
const size_t ExamplePerfSampleEvent_Tracepoint::kEventSize =
    (1 /*perf_event_header*/ + 14 /*sample array*/) * sizeof(u64);

void ExampleStringMetadata::WriteTo(std::ostream* out) const {
  const perf_file_section& index_entry = index_entry_.index_entry_;
  CHECK_EQ(static_cast<u64>(out->tellp()), index_entry.offset);
  const u32 data_size_value = MaybeSwap32(data_.size());
  out->write(reinterpret_cast<const char*>(&data_size_value),
             sizeof(data_size_value));
  out->write(data_.data(), data_.size());

  CHECK_EQ(static_cast<u64>(out->tellp()), index_entry.offset + size());
}

void ExampleStringMetadataEvent::WriteTo(std::ostream* out) const {
  const size_t initial_position = out->tellp();

  const u32 data_size = data_.size();
  const perf_event_header header = {
      .type = type_,
      .misc = 0,
      .size =
          static_cast<u16>(sizeof(header) + sizeof(data_size) + data_.size()),
  };
  out->write(reinterpret_cast<const char*>(&header), sizeof(header));

  out->write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
  out->write(reinterpret_cast<const char*>(data_.data()), data_.size());

  CHECK_EQ(static_cast<u64>(out->tellp()), initial_position + header.size);
}

static const char kTraceMetadataValue[] =
    "\x17\x08\x44tracing0.5BLAHBLAHBLAH....";

const std::string ExampleTracingMetadata::Data::kTraceMetadata(
    kTraceMetadataValue, sizeof(kTraceMetadataValue) - 1);

void ExampleTracingMetadata::Data::WriteTo(std::ostream* out) const {
  const perf_file_section& index_entry = parent_->index_entry_.index_entry_;
  CHECK_EQ(static_cast<u64>(out->tellp()), index_entry.offset);
  out->write(kTraceMetadata.data(), kTraceMetadata.size());
  CHECK_EQ(static_cast<u64>(out->tellp()),
           index_entry.offset + index_entry.size);
}

void ExampleCPUTopologyMetadata::WriteTo(std::ostream* out) const {
  const perf_file_section& index_entry = index_entry_.index_entry_;
  CHECK_EQ(static_cast<u64>(out->tellp()), index_entry.offset);

  size_t offset = index_entry.offset;
  const u32 num_core_siblings_value = MaybeSwap32(num_core_siblings_);
  out->write(reinterpret_cast<const char*>(&num_core_siblings_value),
             sizeof(num_core_siblings_value));
  offset += sizeof(u32);  // num_core_siblings
  for (const auto& sibling : core_siblings_) {
    ExampleStringMetadata cs(sibling, offset);
    cs.WriteTo(out);
    offset += cs.size();
  }
  const u32 num_thread_siblings_value = MaybeSwap32(num_thread_siblings_);
  out->write(reinterpret_cast<const char*>(&num_thread_siblings_value),
             sizeof(num_thread_siblings_value));
  offset += sizeof(u32);  // num_thread_siblings
  for (int i = 0; i < thread_siblings_.size(); ++i) {
    ExampleStringMetadata ts(thread_siblings_[i], offset);
    ts.WriteTo(out);
    offset += ts.size();
  }
  CHECK_EQ(static_cast<u64>(out->tellp()),
           index_entry.offset + IndexEntrySize());
}

void ExampleHybridTopologyMetadata::WriteTo(std::ostream* out) const {
  const perf_file_section& index_entry = index_entry_.index_entry_;
  CHECK_EQ(static_cast<u64>(out->tellp()), index_entry.offset);

  size_t offset = index_entry.offset;
  const u32 num_records = MaybeSwap32(records_.size());
  out->write(reinterpret_cast<const char*>(&num_records), sizeof(num_records));
  offset += sizeof(u32);  // nr
  for (const auto& record : records_) {
    ExampleStringMetadata pmu_name(record.first, offset);
    pmu_name.WriteTo(out);
    offset += pmu_name.size();
    ExampleStringMetadata cpus(record.second, offset);
    cpus.WriteTo(out);
    offset += cpus.size();
  }
  CHECK_EQ(static_cast<u64>(out->tellp()),
           index_entry.offset + IndexEntrySize());
}

size_t ExampleAuxtraceEvent::GetSize() const {
  return sizeof(struct auxtrace_event);
}

size_t ExampleAuxtraceEvent::GetTraceSize() const { return trace_data_.size(); }

void ExampleAuxtraceEvent::WriteTo(std::ostream* out) const {
  const size_t event_size = GetSize();

  struct auxtrace_event event = {
      .header =
          {
              .type = MaybeSwap32(PERF_RECORD_AUXTRACE),
              .misc = 0,
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .size = MaybeSwap64(size_),
      .offset = MaybeSwap64(offset_),
      .reference = MaybeSwap64(reference_),
      .idx = MaybeSwap32(idx_),
      .tid = MaybeSwap32(tid_),
      .cpu = MaybeSwap32(cpu_),
      .reserved__ = MaybeSwap32(reserved_),
  };

  out->write(reinterpret_cast<const char*>(&event), event_size);
  out->write(trace_data_.data(), trace_data_.size());
}

size_t ExampleContextSwitchEvent::GetSize() const {
  if (type_ == PERF_RECORD_SWITCH) {
    return sizeof(struct context_switch_event) -
           sizeof(context_switch_event::next_prev_pid) -
           sizeof(context_switch_event::next_prev_tid) +
           sample_id_.size();  // Sample info fields are present for this event.
  }
  return sizeof(struct context_switch_event) +
         sample_id_.size();  // Sample info fields are present for this event.
}

void ExampleContextSwitchEvent::WriteTo(std::ostream* out) const {
  const size_t event_size = GetSize();
  u16 misc_ = (is_out_ ? PERF_RECORD_MISC_SWITCH_OUT : 0);
  struct context_switch_event event = {
      .header =
          {
              .type = MaybeSwap32(type_),
              .misc = misc_,
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .next_prev_pid = next_prev_pid_,
      .next_prev_tid = next_prev_tid_,
  };

  const size_t pre_context_switch_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(&event),
             event_size - sample_id_.size());
  out->write(sample_id_.data(), sample_id_.size());
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_context_switch_offset;
  CHECK_EQ(event_size, static_cast<u64>(written_event_size));
}

size_t ExampleNamespacesEvent::GetSize() const {
  return sizeof(struct namespaces_event) +
         link_info_.size() * sizeof(struct perf_ns_link_info) +
         sample_id_.size();  // Sample info fields are present for this event.
}

void ExampleNamespacesEvent::WriteTo(std::ostream* out) const {
  const size_t event_size = GetSize();
  malloced_unique_ptr<namespaces_event> event(
      reinterpret_cast<struct namespaces_event*>(
          calloc(1, event_size - sample_id_.size())));
  event->header.type = MaybeSwap32(PERF_RECORD_NAMESPACES);
  event->header.misc = 0;
  event->header.size = MaybeSwap16(static_cast<u16>(event_size));
  event->pid = pid_;
  event->tid = tid_;
  event->nr_namespaces = link_info_.size();

  for (u64 i = 0; i < link_info_.size(); ++i) {
    event->link_info[i] = link_info_[i];
  }
  const size_t pre_namespaces_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(event.get()),
             event_size - sample_id_.size());
  out->write(sample_id_.data(), sample_id_.size());
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_namespaces_offset;
  CHECK_EQ(event_size, static_cast<u64>(written_event_size));
}

size_t ExampleAuxtraceInfoEvent::GetSize() const {
  return sizeof(struct auxtrace_info_event) + priv_.size() * sizeof(u64);
}

void ExampleAuxtraceInfoEvent::WriteTo(std::ostream* out) const {
  const size_t event_size = GetSize();
  malloced_unique_ptr<auxtrace_info_event> event(
      reinterpret_cast<struct auxtrace_info_event*>(calloc(1, event_size)));
  event->header.type = MaybeSwap32(PERF_RECORD_AUXTRACE_INFO),
  event->header.misc = 0,
  event->header.size = MaybeSwap16(static_cast<u16>(event_size)),
  event->type = MaybeSwap32(type_);
  for (u64 i = 0; i < priv_.size(); ++i) {
    event->priv[i] = MaybeSwap64(priv_[i]);
  }

  const size_t pre_auxtrace_info_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(event.get()), event_size);
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_auxtrace_info_offset;
  CHECK_EQ(event_size, static_cast<u64>(written_event_size));
}

size_t ExampleAuxtraceErrorEvent::GetSize() const {
  return offsetof(struct auxtrace_error_event, msg) +
         GetUint64AlignedStringLength(msg_.size());
}

void ExampleAuxtraceErrorEvent::WriteTo(std::ostream* out) const {
  const size_t event_size = GetSize();
  const size_t msg_aligned_length = GetUint64AlignedStringLength(msg_.size());
  struct auxtrace_error_event event = {
      .header =
          {
              .type = MaybeSwap32(PERF_RECORD_AUXTRACE_ERROR),
              .misc = 0,
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .type = MaybeSwap32(type_),
      .code = MaybeSwap32(code_),
      .cpu = MaybeSwap32(cpu_),
      .pid = MaybeSwap32(pid_),
      .tid = MaybeSwap32(tid_),
      .reserved__ = 0,
      .ip = MaybeSwap64(ip_),
      // .msg = msg_,  // written separately
  };

  const size_t pre_auxtrace_error_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(&event),
             offsetof(struct auxtrace_error_event, msg));
  *out << msg_ << std::string(msg_aligned_length - msg_.size(), '\0');
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_auxtrace_error_offset;
  CHECK_EQ(event_size, static_cast<u64>(written_event_size));
}

size_t ExampleIdIndexEvent::GetSize() const {
  return offsetof(struct id_index_event, entries) +
         entries_.size() * sizeof(struct id_index_event_entry);
}

void ExampleIdIndexEvent::WriteTo(std::ostream* out) const {
  const size_t event_size = GetSize();
  malloced_unique_ptr<id_index_event> event(
      reinterpret_cast<struct id_index_event*>(calloc(1, event_size)));
  event->header.type = MaybeSwap32(PERF_RECORD_ID_INDEX);
  event->header.misc = 0;
  event->header.size = MaybeSwap16(static_cast<u16>(event_size));
  event->nr = MaybeSwap64(entries_.size());

  for (u64 i = 0; i < entries_.size(); ++i) {
    event->entries[i].id = MaybeSwap64(entries_[i].id);
    event->entries[i].idx = MaybeSwap64(entries_[i].idx);
    event->entries[i].cpu = MaybeSwap64(entries_[i].cpu);
    event->entries[i].tid = MaybeSwap64(entries_[i].tid);
  }

  const size_t pre_id_index_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(event.get()), event_size);
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_id_index_offset;
  CHECK_EQ(event_size, static_cast<u64>(written_event_size));
}

size_t ExampleThreadMapEvent::GetSize() const {
  return offsetof(struct thread_map_event, entries) +
         entries_.size() * sizeof(struct thread_map_event_entry);
}

void ExampleThreadMapEvent::WriteTo(std::ostream* out) const {
  static const int kMaxCommSize = sizeof(thread_map_event_entry{}.comm);
  const size_t event_size = GetSize();
  malloced_unique_ptr<thread_map_event> event(
      reinterpret_cast<struct thread_map_event*>(calloc(1, event_size)));
  event->header.type = MaybeSwap32(PERF_RECORD_THREAD_MAP);
  event->header.misc = 0;
  event->header.size = MaybeSwap16(static_cast<u16>(event_size));
  event->nr = MaybeSwap64(entries_.size());

  for (u64 i = 0; i < entries_.size(); ++i) {
    event->entries[i].pid = MaybeSwap64(entries_[i].pid);
    CHECK_LT(entries_[i].comm.size(), kMaxCommSize)
        << "command size should be less than " << kMaxCommSize
        << ", got the command: " << entries_[i].comm;
    snprintf(event->entries[i].comm, kMaxCommSize, "%s",
             entries_[i].comm.c_str());
  }

  const size_t pre_thread_map_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(event.get()), event_size);
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_thread_map_offset;
  CHECK_EQ(event_size, static_cast<u64>(written_event_size));
}

size_t ExampleStatConfigEvent::GetSize() const {
  return offsetof(struct stat_config_event, data) +
         data_.size() * sizeof(struct stat_config_event_entry);
}

void ExampleStatConfigEvent::WriteTo(std::ostream* out) const {
  const size_t event_size = GetSize();
  malloced_unique_ptr<stat_config_event> event(
      reinterpret_cast<struct stat_config_event*>(calloc(1, event_size)));
  event->header.type = MaybeSwap32(PERF_RECORD_STAT_CONFIG);
  event->header.misc = 0;
  event->header.size = MaybeSwap16(static_cast<u16>(event_size));
  event->nr = MaybeSwap64(data_.size());

  for (u64 i = 0; i < data_.size(); ++i) {
    event->data[i].tag = MaybeSwap64(data_[i].tag);
    event->data[i].val = MaybeSwap64(data_[i].val);
  }

  const size_t pre_stat_config_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(event.get()), event_size);
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_stat_config_offset;
  CHECK_EQ(event_size, static_cast<u64>(written_event_size));
}

size_t ExampleStatEvent::GetSize() const { return sizeof(struct stat_event); }

void ExampleStatEvent::WriteTo(std::ostream* out) const {
  const size_t event_size = GetSize();
  struct stat_event event = {
      .header =
          {
              .type = MaybeSwap32(PERF_RECORD_STAT),
              .misc = 0,
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .id = MaybeSwap64(id_),
      .cpu = MaybeSwap32(cpu_),
      .thread = MaybeSwap32(thread_),
  };
  event.val = MaybeSwap64(value_);
  event.ena = MaybeSwap64(enabled_);
  event.run = MaybeSwap64(running_);

  const size_t pre_stat_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(&event), event_size);
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_stat_offset;
  CHECK_EQ(event_size, static_cast<u64>(written_event_size));
}

size_t ExampleStatRoundEvent::GetSize() const {
  return sizeof(struct stat_round_event);
}

void ExampleStatRoundEvent::WriteTo(std::ostream* out) const {
  const size_t event_size = GetSize();
  struct stat_round_event event = {
      .header =
          {
              .type = MaybeSwap32(PERF_RECORD_STAT_ROUND),
              .misc = 0,
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .type = MaybeSwap64(type_),
      .time = MaybeSwap64(time_),
  };

  const size_t pre_stat_round_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(&event), event_size);
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_stat_round_offset;
  CHECK_EQ(event_size, static_cast<u64>(written_event_size));
}

size_t ExampleTimeConvEventSmall::GetSize() const {
  return offsetof(struct time_conv_event, time_cycles);
}

void ExampleTimeConvEventSmall::WriteTo(std::ostream* out) const {
  const size_t event_size = GetSize();
  struct time_conv_event event = {
      .header =
          {
              .type = MaybeSwap32(PERF_RECORD_TIME_CONV),
              .misc = 0,
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .time_shift = time_shift_,
      .time_mult = time_mult_,
      .time_zero = time_zero_,
  };

  const size_t pre_time_conv_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(&event), event_size);
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_time_conv_offset;
  CHECK_EQ(event_size, static_cast<u64>(written_event_size));
}

size_t ExampleTimeConvEvent::GetSize() const {
  return sizeof(struct time_conv_event);
}

void ExampleTimeConvEvent::WriteTo(std::ostream* out) const {
  const size_t event_size = GetSize();
  struct time_conv_event event = {
      .header =
          {
              .type = MaybeSwap32(PERF_RECORD_TIME_CONV),
              .misc = 0,
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .time_shift = time_shift_,
      .time_mult = time_mult_,
      .time_zero = time_zero_,
      .time_cycles = time_cycles_,
      .time_mask = time_mask_,
      .cap_user_time_zero = cap_user_time_zero_,
      .cap_user_time_short = cap_user_time_short_,
  };

  const size_t pre_time_conv_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(&event), event_size);
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_time_conv_offset;
  CHECK_EQ(event_size, static_cast<u64>(written_event_size));
}

size_t ExampleCgroupEvent::GetSize() const {
  return offsetof(struct cgroup_event, path) +
         GetUint64AlignedStringLength(path_.size()) +
         sample_id_.size();  // sample_id_all
}

void ExampleCgroupEvent::WriteTo(std::ostream* out) const {
  const size_t path_aligned_length = GetUint64AlignedStringLength(path_.size());
  const size_t event_size = GetSize();
  struct cgroup_event event = {
      .header =
          {
              .type = MaybeSwap32(PERF_RECORD_CGROUP),
              .misc = 0,
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .id = id_,
  };

  const size_t pre_cgroup_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(&event),
             offsetof(struct cgroup_event, path));
  *out << path_ << std::string(path_aligned_length - path_.size(), '\0');
  out->write(sample_id_.data(), sample_id_.size());
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_cgroup_offset;
  CHECK_EQ(event.header.size, static_cast<u64>(written_event_size));
}

size_t ExampleKsymbolEvent::GetSize() const {
  return offsetof(struct ksymbol_event, name) +
         GetUint64AlignedStringLength(name_.size()) +
         sample_id_.size();  // sample_id_all
}

void ExampleKsymbolEvent::WriteTo(std::ostream* out) const {
  const size_t name_aligned_length = GetUint64AlignedStringLength(name_.size());
  const size_t event_size = GetSize();
  struct ksymbol_event event = {
      .header =
          {
              .type = MaybeSwap32(PERF_RECORD_KSYMBOL),
              .misc = 0,
              .size = MaybeSwap16(static_cast<u16>(event_size)),
          },
      .addr = addr_,
      .len = len_,
      .ksym_type = static_cast<u16>(ksym_type_),
      .flags = static_cast<u16>(flags_),
  };

  const size_t pre_ksymbol_offset = out->tellp();
  out->write(reinterpret_cast<const char*>(&event),
             offsetof(struct ksymbol_event, name));
  *out << name_ << std::string(name_aligned_length - name_.size(), '\0');
  out->write(sample_id_.data(), sample_id_.size());
  const size_t written_event_size =
      static_cast<size_t>(out->tellp()) - pre_ksymbol_offset;
  CHECK_EQ(event.header.size, static_cast<u64>(written_event_size));
}

}  // namespace testing
}  // namespace quipper
