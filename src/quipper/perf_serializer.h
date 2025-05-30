// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMIUMOS_WIDE_PROFILING_PERF_SERIALIZER_H_
#define CHROMIUMOS_WIDE_PROFILING_PERF_SERIALIZER_H_

#include <stdint.h>
#include <sys/types.h>

#include <map>
#include <memory>
#include <vector>

#include "compat/proto.h"
#include "perf_data_utils.h"

struct perf_event_attr;

namespace quipper {

struct ParsedEvent;
struct PerfFileAttr;
struct PerfGroupDescMetadata;
struct PerfPMUMappingsMetadata;
struct PerfNodeTopologyMetadata;
struct PerfCPUTopologyMetadata;
struct PerfEventStats;
struct PerfParserOptions;
struct PerfUint32Metadata;
struct PerfUint64Metadata;
struct PerfHybridTopologyMetadata;

class SampleInfoReader;

class PerfSerializer {
 public:
  PerfSerializer();
  ~PerfSerializer();
  PerfSerializer(const PerfSerializer&) = delete;
  PerfSerializer& operator=(const PerfSerializer&) = delete;

  // Returns true if the given event type is a kernel event and supported by
  // the PerfSerializer.
  static bool IsSupportedKernelEventType(uint32_t type);
  // Returns true if the given event type is an user event and supported by
  // the PerfSerializer.
  static bool IsSupportedUserEventType(uint32_t type);
  // Returns true if the given event type is a header event and supported by
  // the PerfSerializer.
  static bool IsSupportedHeaderEventType(uint32_t type);

  // Returns true if the given event type contains sample info data.
  bool ContainsSampleInfo(uint32_t type) const;

  // Calculates the event size ignoring event.header.size . Returns a non-zero
  // event size on success. Returns zero when the proto contains an unsupported
  //  perf event or sample info.
  size_t GetEventSize(const PerfDataProto_PerfEvent& event) const;

  // The following functions convert between raw perf data structures and their
  // equivalent PerfDataProto representations.
  bool SerializePerfFileAttr(
      const PerfFileAttr& perf_file_attr,
      PerfDataProto_PerfFileAttr* perf_file_attr_proto) const;
  bool DeserializePerfFileAttr(
      const PerfDataProto_PerfFileAttr& perf_file_attr_proto,
      PerfFileAttr* perf_file_attr) const;

  bool SerializePerfEventAttr(
      const perf_event_attr& perf_event_attr,
      PerfDataProto_PerfEventAttr* perf_event_attr_proto) const;
  bool DeserializePerfEventAttr(
      const PerfDataProto_PerfEventAttr& perf_event_attr_proto,
      perf_event_attr* perf_event_attr) const;

  bool SerializePerfEventType(
      const PerfFileAttr& event_attr,
      PerfDataProto_PerfEventType* event_type_proto) const;
  bool DeserializePerfEventType(
      const PerfDataProto_PerfEventType& event_type_proto,
      PerfFileAttr* event_attr) const;

  bool SerializeEvent(const malloced_unique_ptr<event_t>& event_ptr,
                      PerfDataProto_PerfEvent* event_proto) const;
  bool DeserializeEvent(const PerfDataProto_PerfEvent& event_proto,
                        malloced_unique_ptr<event_t>* event_ptr) const;

  bool SerializeEventHeader(const perf_event_header& header,
                            PerfDataProto_EventHeader* header_proto) const;
  bool DeserializeEventHeader(const PerfDataProto_EventHeader& header_proto,
                              perf_event_header* header) const;

  bool SerializeSampleEvent(const event_t& event,
                            PerfDataProto_SampleEvent* sample) const;
  bool DeserializeSampleEvent(const PerfDataProto_SampleEvent& sample,
                              event_t* event) const;

  bool SerializeMMapEvent(const event_t& event,
                          PerfDataProto_MMapEvent* sample) const;
  bool DeserializeMMapEvent(const PerfDataProto_MMapEvent& sample,
                            event_t* event) const;

  bool SerializeMMap2Event(const event_t& event,
                           PerfDataProto_MMapEvent* sample) const;
  bool DeserializeMMap2Event(const PerfDataProto_MMapEvent& sample,
                             event_t* event) const;

  bool SerializeCommEvent(const event_t& event,
                          PerfDataProto_CommEvent* sample) const;
  bool DeserializeCommEvent(const PerfDataProto_CommEvent& sample,
                            event_t* event) const;

  // These handle both fork and exit events, which use the same protobuf
  // message definition.
  bool SerializeForkExitEvent(const event_t& event,
                              PerfDataProto_ForkEvent* sample) const;
  bool DeserializeForkExitEvent(const PerfDataProto_ForkEvent& sample,
                                event_t* event) const;

  bool SerializeLostEvent(const event_t& event,
                          PerfDataProto_LostEvent* sample) const;
  bool DeserializeLostEvent(const PerfDataProto_LostEvent& sample,
                            event_t* event) const;

  bool SerializeThrottleEvent(const event_t& event,
                              PerfDataProto_ThrottleEvent* sample) const;
  bool DeserializeThrottleEvent(const PerfDataProto_ThrottleEvent& sample,
                                event_t* event) const;

  bool SerializeAuxEvent(const event_t& event,
                         PerfDataProto_AuxEvent* sample) const;
  bool DeserializeAuxEvent(const PerfDataProto_AuxEvent& sample,
                           event_t* event) const;

  bool SerializeItraceStartEvent(const event_t& event,
                                 PerfDataProto_ItraceStartEvent* sample) const;
  bool DeserializeItraceStartEvent(const PerfDataProto_ItraceStartEvent& sample,
                                   event_t* event) const;
  bool SerializeLostSamplesEvent(const event_t& event,
                                 PerfDataProto_LostSamplesEvent* sample) const;
  bool DeserializeLostSamplesEvent(const PerfDataProto_LostSamplesEvent& sample,
                                   event_t* event) const;
  bool SerializeContextSwitchEvent(
      const event_t& event, PerfDataProto_ContextSwitchEvent* sample) const;
  bool DeserializeContextSwitchEvent(
      const PerfDataProto_ContextSwitchEvent& sample, event_t* event) const;
  bool SerializeNamespacesEvent(const event_t& event,
                                PerfDataProto_NamespacesEvent* sample) const;
  bool DeserializeNamespacesEvent(const PerfDataProto_NamespacesEvent& sample,
                                  event_t* event) const;

  bool SerializeSampleInfo(const event_t& event,
                           PerfDataProto_SampleInfo* sample_info) const;
  bool DeserializeSampleInfo(const PerfDataProto_SampleInfo& sample,
                             event_t* event) const;

  bool SerializeTracingMetadata(const char* from, size_t size,
                                PerfDataProto* to) const;
  bool DeserializeTracingMetadata(const PerfDataProto& from,
                                  std::vector<char>* to) const;

  bool SerializeBuildIDEvent(const malloced_unique_ptr<build_id_event>& from,
                             PerfDataProto_PerfBuildID* to) const;
  bool DeserializeBuildIDEvent(const PerfDataProto_PerfBuildID& from,
                               malloced_unique_ptr<build_id_event>* to) const;

  bool SerializeAuxtraceInfoEvent(
      const event_t& event, PerfDataProto_AuxtraceInfoEvent* sample) const;
  bool DeserializeAuxtraceInfoEvent(
      const PerfDataProto_AuxtraceInfoEvent& sample, event_t* event) const;
  bool SerializeAuxtraceEvent(const event_t& event,
                              PerfDataProto_AuxtraceEvent* sample) const;
  bool SerializeAuxtraceEventTraceData(const char* from, size_t size,
                                       PerfDataProto_AuxtraceEvent* to) const;
  bool DeserializeAuxtraceEvent(const PerfDataProto_AuxtraceEvent& sample,
                                event_t* event) const;
  bool DeserializeAuxtraceEventTraceData(
      const PerfDataProto_AuxtraceEvent& from, std::vector<char>* to) const;
  bool SerializeAuxtraceErrorEvent(
      const event_t& event, PerfDataProto_AuxtraceErrorEvent* sample) const;
  bool DeserializeAuxtraceErrorEvent(
      const PerfDataProto_AuxtraceErrorEvent& sample, event_t* event) const;
  bool SerializeIdIndexEvent(const event_t& event,
                             PerfDataProto_IdIndexEvent* id_index_event) const;
  bool DeserializeIdIndexEvent(const PerfDataProto_IdIndexEvent& id_index_event,
                               event_t* event) const;
  bool SerializeThreadMapEvent(const event_t& event,
                               PerfDataProto_ThreadMapEvent* sample) const;
  bool DeserializeThreadMapEvent(const PerfDataProto_ThreadMapEvent& sample,
                                 event_t* event) const;
  bool SerializeStatConfigEvent(const event_t& event,
                                PerfDataProto_StatConfigEvent* sample) const;
  bool DeserializeStatConfigEvent(const PerfDataProto_StatConfigEvent& sample,
                                  event_t* event) const;
  bool SerializeStatEvent(const event_t& event,
                          PerfDataProto_StatEvent* sample) const;
  bool DeserializeStatEvent(const PerfDataProto_StatEvent& sample,
                            event_t* event) const;
  bool SerializeStatRoundEvent(const event_t& event,
                               PerfDataProto_StatRoundEvent* sample) const;
  bool DeserializeStatRoundEvent(const PerfDataProto_StatRoundEvent& sample,
                                 event_t* event) const;
  bool SerializeTimeConvEvent(const event_t& event,
                              PerfDataProto_TimeConvEvent* sample) const;
  bool DeserializeTimeConvEvent(const PerfDataProto_TimeConvEvent& sample,
                                event_t* event) const;
  bool SerializeCgroupEvent(const event_t& event,
                            PerfDataProto_CgroupEvent* sample) const;
  bool DeserializeCgroupEvent(const PerfDataProto_CgroupEvent& sample,
                              event_t* event) const;
  bool SerializeKsymbolEvent(const event_t& event,
                             PerfDataProto_KsymbolEvent* sample) const;
  bool DeserializeKsymbolEvent(const PerfDataProto_KsymbolEvent& sample,
                               event_t* event) const;

  bool SerializeSingleUint32Metadata(
      const PerfUint32Metadata& metadata,
      PerfDataProto_PerfUint32Metadata* proto_metadata) const;
  bool DeserializeSingleUint32Metadata(
      const PerfDataProto_PerfUint32Metadata& proto_metadata,
      PerfUint32Metadata* metadata) const;

  bool SerializeSingleUint64Metadata(
      const PerfUint64Metadata& metadata,
      PerfDataProto_PerfUint64Metadata* proto_metadata) const;
  bool DeserializeSingleUint64Metadata(
      const PerfDataProto_PerfUint64Metadata& proto_metadata,
      PerfUint64Metadata* metadata) const;

  bool SerializeCPUTopologyMetadata(
      const PerfCPUTopologyMetadata& metadata,
      PerfDataProto_PerfCPUTopologyMetadata* proto_metadata) const;
  bool DeserializeCPUTopologyMetadata(
      const PerfDataProto_PerfCPUTopologyMetadata& proto_metadata,
      PerfCPUTopologyMetadata* metadata) const;

  bool SerializeNodeTopologyMetadata(
      const PerfNodeTopologyMetadata& metadata,
      PerfDataProto_PerfNodeTopologyMetadata* proto_metadata) const;
  bool DeserializeNodeTopologyMetadata(
      const PerfDataProto_PerfNodeTopologyMetadata& proto_metadata,
      PerfNodeTopologyMetadata* metadata) const;

  bool SerializePMUMappingsMetadata(
      const PerfPMUMappingsMetadata& metadata,
      PerfDataProto_PerfPMUMappingsMetadata* proto_metadata) const;
  bool DeserializePMUMappingsMetadata(
      const PerfDataProto_PerfPMUMappingsMetadata& proto_metadata,
      PerfPMUMappingsMetadata* metadata) const;

  bool SerializeGroupDescMetadata(
      const PerfGroupDescMetadata& metadata,
      PerfDataProto_PerfGroupDescMetadata* proto_metadata) const;
  bool DeserializeGroupDescMetadata(
      const PerfDataProto_PerfGroupDescMetadata& proto_metadata,
      PerfGroupDescMetadata* metadata) const;

  bool SerializeHybridTopologyMetadata(
      const PerfHybridTopologyMetadata& metadata,
      PerfDataProto_PerfHybridTopologyMetadata* proto_metadata) const;
  bool DeserializeHybridTopologyMetadata(
      const PerfDataProto_PerfHybridTopologyMetadata& proto_metadata,
      PerfHybridTopologyMetadata* metadata) const;

  static void SerializeParserStats(const PerfEventStats& stats,
                                   PerfDataProto* perf_data_proto);
  static void DeserializeParserStats(const PerfDataProto& perf_data_proto,
                                     PerfEventStats* stats);

  // Instantiate a new PerfSampleReader with the given attr type. If an old one
  // exists for that attr type, it is discarded.
  bool CreateSampleInfoReader(const PerfFileAttr& event_attr,
                              bool read_cross_endian);

  bool SampleInfoReaderAvailable() const {
    return !sample_info_reader_map_.empty();
  }

 private:
  // Special values for the event/other_event_id_pos_ fields.
  enum EventIdPosition {
    Uninitialized = -2,
    NotPresent = -1,
  };

  // Given a perf_event_attr, determines the offset of the ID field within an
  // event, relative to the start of sample info within an event. Returns true
  // when all attrs have the same ID field offset. When the ID field offset from
  // |attr| is inconsistent with the ID field offsets from other attrs, doesn't
  // update the event id position and returns false.
  bool UpdateEventIdPositions(const struct perf_event_attr& attr);

  // Find the event id in the event, and returns the corresponding
  // SampleInfoReader. Returns nullptr if a SampleInfoReader could not be found.
  const SampleInfoReader* GetSampleInfoReaderForEvent(
      const event_t& event) const;

  // Returns the SampleInfoReader associated with the given perf event ID, or
  // nullptr if none exists. |id| == 0 means there is no attr ID for each event
  // that associates it with a particular SampleInfoReader, in which case the
  // first available SampleInfoReader is returned.
  const SampleInfoReader* GetSampleInfoReaderForId(uint64_t id) const;

  // Reads the sample info fields from |event| into |sample_info|. If more than
  // one type of perf event attr is present, will pick the correct one. Also
  // returns a bitfield of available sample info fields for the attr, in
  // |sample_type|.
  // Returns true if successfully read.
  bool ReadPerfSampleInfoAndType(const event_t& event, perf_sample* sample_info,
                                 uint64_t* sample_type) const;

  bool SerializeKernelEvent(const event_t& event,
                            PerfDataProto_PerfEvent* event_proto) const;
  bool SerializeUserEvent(const event_t& event,
                          PerfDataProto_PerfEvent* event_proto) const;

  bool DeserializeKernelEvent(const PerfDataProto_PerfEvent& event_proto,
                              event_t* event) const;
  bool DeserializeUserEvent(const PerfDataProto_PerfEvent& event_proto,
                            event_t* event) const;

  void GetPerfSampleInfo(const PerfDataProto_SampleInfo& sample,
                         perf_sample* sample_info) const;

  void GetPerfSampleInfo(const PerfDataProto_SampleEvent& sample,
                         perf_sample* sample_info) const;

  // For SAMPLE events, the position of the sample id,
  // Or EventIdPosition::NotPresent if neither PERF_SAMPLE_ID(ENTIFIER) are set.
  // (Corresponds to evsel->id_pos in perf)
  ssize_t sample_event_id_pos_ = EventIdPosition::Uninitialized;
  // For non-SAMPLE events, the position of the sample id, counting backwards
  // from the end of the event.
  // Or EventIdPosition::NotPresent if neither PERF_SAMPLE_ID(ENTIFIER) are set.
  // (Corresponds to evsel->is_pos in perf)
  ssize_t other_event_id_pos_ = EventIdPosition::Uninitialized;

  // For each perf event attr ID, there is a SampleInfoReader to read events of
  // the associated perf attr type.
  std::map<uint64_t, std::unique_ptr<SampleInfoReader>> sample_info_reader_map_;
};

}  // namespace quipper

#endif  // CHROMIUMOS_WIDE_PROFILING_PERF_SERIALIZER_H_
