// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "perf_test_files.h"

#include <vector>

namespace perf_test_files {

const std::vector<const char*>& GetPerfDataFiles() {
  static const std::vector<const char*>* files = new std::vector<const char*>{
      // The following perf data contains the following event types, as passed
      // to perf record via the -e option:
      // - cycles
      // - instructions
      // - cache-references
      // - cache-misses
      // - branches
      // - branch-misses
      //
      // Note: The test perf.data files were collected such that at least 95% of
      // the samples are mapped.

      // Obtained with "perf record -- echo > /dev/null"
      "perf.data.singleprocess-3.4",

      // Obtained with "perf record -a -- sleep $N", for N in {0, 1, 5}.
      "perf.data.systemwide.0-3.4",
#ifdef TEST_LARGE_PERF_DATA
      "perf.data.systemwide.1-3.4",
      "perf.data.systemwide.5-3.4",

      // Obtained with "perf record -a -- sleep $N", for N in {0, 1, 5}.
      // While in the background, this loop is running:
      //   while true; do ls > /dev/null; done
      "perf.data.busy.0-3.4",
      "perf.data.busy.1-3.4",
      "perf.data.busy.5-3.4",
#endif  // defined(TEST_LARGE_PERF_DATA)

      // Obtained with "perf record -a -- sleep 2"
      // While in the background, this loop is running:
      //   while true; do restart powerd; sleep .2; done
      "perf.data.forkexit-3.4",

#ifdef TEST_CALLGRAPH
      // Obtained with "perf record -a -g -- sleep 2"
      "perf.data.callgraph-3.4",
#endif

#ifdef TEST_CALLGRAPH
      // Obtained with "perf record -a -g -b -- sleep 2"
      "perf.data.callgraph_and_branch-3.4",
#endif

      // Obtained with "perf record -a -R -- sleep 2"
      "perf.data.raw-3.4",
#ifdef TEST_CALLGRAPH
      // Obtained with "perf record -a -R -g -b -- sleep 2"
      "perf.data.raw_callgraph_branch-3.4",
#endif

      // Data from other architectures.
      "perf.data.i686-3.4",   // 32-bit x86
      "perf.data.armv7-3.4",  // ARM v7

      // Same as above, obtained from a system running kernel v3.8.
      "perf.data.singleprocess-3.8",
      "perf.data.systemwide.0-3.8",
#ifdef TEST_LARGE_PERF_DATA
      "perf.data.systemwide.1-3.8",
      "perf.data.systemwide.5-3.8",
      "perf.data.busy.0-3.8",
      "perf.data.busy.1-3.8",
      "perf.data.busy.5-3.8",
#endif  // defined(TEST_LARGE_PERF_DATA)

      "perf.data.forkexit-3.8",
#ifdef TEST_CALLGRAPH
      "perf.data.callgraph-3.8",
#endif

#ifdef TEST_CALLGRAPH
      "perf.data.callgraph_and_branch-3.8",
#endif
      "perf.data.armv7.perf_3.14-3.8",  // ARM v7 obtained using perf 3.14.

      // Obtained from a system that uses NUMA topology.
      "perf.data.numatopology-3.2",

      /* Obtained to test GROUP_DESC feature
         Command:
            perf record -e "{cache-references,branch-misses}" -o \
            /tmp/perf.data.group_desc-4.14 -- echo "Hello, World!"
      */
      "perf.data.group_desc-4.14",

      /* Perf data that contains hardware and software events.
         Command:
            perf record -a -c 1000000 -e cycles,branch-misses,cpu-clock -- \
            sleep 2
         HW events are cycles and branch-misses, SW event is cpu-clock.
         This also tests non-consecutive event types.
      */
      "perf.data.hw_and_sw-3.4",

      // This test first mmap()s a DSO, then fork()s to copy the mapping to the
      // child and then modifies the mapping by mmap()ing a DSO on top of the
      // old one. It then records SAMPLEs events in the child. It ensures the
      // SAMPLEs in the child are attributed to the first DSO that was mmap()ed,
      // not the second one.
      "perf.data.remmap-3.2",

      // This is sample with a frequency higher than the max frequency, so it
      // has throttle and unthrottle events.
      "perf.data.throttle-3.8",

      /* Perf data that contains intel pt events from perf-4.14
         Command:
            perf record -e intel_pt// -e cycles -o /tmp/perf.data.intel_pt-4.14
            -- echo "Hello, World!"
      */
      "perf.data.intel_pt-4.14",

      /* Obtained with:
            "perf record -b -o /tmp/perf.data.branch-4.14 -- \
            echo "Hello, World!"
       */
      "perf.data.branch-4.14",

      /* Perf data that contains lost sample events from perf-4.4
         Command:
            perf record -e "{cycles:pp,instructions:pp,branch-instructions:pp}"\
            -P -c 20003 -o /tmp/perf.data.lost_samples-4.4 -- \
            echo "Hello, World!"
       */
      "perf.data.lost_samples-4.4",

      /* Perf data that contains switch and namespace samples.
         Command:
         perf record --switch-events --namespace -e cycles -o
         /tmp/perf.data.ctx_switch_namespaces-4.14 -- sleep 0.001
       */
      "perf.data.ctx_switch_namespaces-4.14",

      /* Perf data file with mmaps events containing the
         PERF_RECORD_MISC_PROC_MAP_PARSE_TIMEOUT misc bit.
         Command:
         perf record -e cycles -c 4000000 -p 9463 -o /tmp/perf.data \
         --proc-map-timeout=2 -- sleep 1
       */
      "perf.data.proc.map.timeout-3.18",

      /* Perf data file with branch sample events containing
       * PERF_SAMPLE_BRANCH_HW_INDEX. Generated with perf 5.15.
         Commands (running on Arm with CoreSight/ETM):
         perf record -e cs_etm/autofdo/u -a -- sleep 1
         perf inject --itrace=i1000il --strip -i perf.data \
         -o perf.data.branch_stack_hw_index
       */
      "perf.data.branch_stack_hw_index",
      /* Perf data file with weight struct. Generated with perf5.
      Command:
      remote_perf --duration=10 --command=record --sample_addresses --weight
      --pfm_events="MEM_TRANS_RETIRED.LOAD_LATENCY:ldlat=64:precise=2:mh:mg:pinned"
      --event_period=1009 --noinvoke_perf_report --host=oxco15 --use_perf5
      --output /tmp/perf.data
      */
      "perf.data.weight_struct",
      /* Perf data file with branch sample events for AMD Zen4 architecture.
         Command:
         perf5 record --event=rc4 -c 50 -b -o perf.data -- /usr/bin/lsattr
      */
      "perf.data.branch_stack_spec",
      /* Perf data file with hybrid topology recorded from Intel AlderLake
         microarchitecture.
         Command:
         perf record -e cycles:ppp -o perf.data -- sleep 1
      */
      "perf.data.hybrid_topology",
  };
  return *files;
}

const std::vector<const char*>& GetPerfPipedDataFiles() {
  static const std::vector<const char*>* files = new std::vector<const char*>{
      "perf.data.piped.target-3.4",
      "perf.data.piped.target.throttled-3.4",
      "perf.data.piped.target-3.8",

      /* Piped data that contains hardware and software events.
         Command:
            perf record -a -c 1000000 -e cycles,branch-misses,cpu-clock -o - \
            -- sleep 2
         HW events are cycles and branch-misses, SW event is cpu-clock.
       */
      "perf.data.piped.hw_and_sw-3.4",

      /* Perf data that contains intel pt events collected in piped mode from
         perf-4.14
         Command:
            perf record -e intel_pt// -e cycles -o - -- echo "Hello, World!" | \
            cat &> /tmp/perf.data.piped.intel_pt-4.14
       */
      "perf.data.piped.intel_pt-4.14",

      /* Perf data that contains lost sample events collected in piped mode from
         perf-4.4
         Command:
            perf record -e "{cycles:pp,instructions:pp,branch-instructions:pp}"\
            -P -c 20003 -o - -- echo "Hello, World!" | cat &> \
            /tmp/perf.data.piped.lost_samples-4.4
       */
      "perf.data.piped.lost_samples-4.4",

      /* Perf data contains PERF_RECORD_HEADER_FEATURE events generated in piped
         mode from perf 4.16.
         Command:
            /tmp/perf record -e "cycles" -o - -- echo Hello, World! | cat &> \
            /tmp/perf.data.piped.header_features-4.16
       */
      "perf.data.piped.header_features-4.16",

      /* Perf data from perf 4.14 containing no IDs in PERF_RECORD_HEADER_ATTR
         event.
         Command:
            perf record -e "cycles" -o - -- sleep 0.001 | cat &> \
            perf.data.piped.no_attr_ids-4.14
       */
      "perf.data.piped.no_attr_ids-4.14",

      /* Perf data that contains switch and namespace samples.
         Command:
         perf record --switch-events --namespace -e cycles -o - -- sleep 0.001 |
         cat &> /tmp/perf.data.pipedctx_switch_namespaces-4.14
       */
      "perf.data.piped.ctx_switch_namespaces-4.14",

      /* Perf data that contains a HEADER_GROUP_DESCR feature in the
       * PERF_RECORD_HEADER_FEATURE events, generated in piped mode from perf
       * 6.8.
       * Command:
       * perf record -e "{cycles,instructions}" -o - -- echo "Hello, World!" | \
       * cat &> /tmp/perf.data.piped.header_feautres_group_desc-6.8
       */
      "perf.data.piped.header_feautres_group_desc-6.8",

      /* Perf data that contains an aligned HEADER_PMU_MAPPINGS
       * PERF_RECORD_FEATURE, generated in piped mode from perf 6.12.
       * Command:
       * $ /tmp/perf record -e cycles -o - -- echo "Hello, World!" | \
       * cat &> /tmp/perf.data.piped.header_features_aligned-6.12
       */
      "perf.data.piped.header_features_aligned-6.12",
  };
  return *files;
}

const std::vector<const char*>& GetCorruptedPerfPipedDataFiles() {
  static const std::vector<const char*>* files = new std::vector<const char*>{
      // Has a SAMPLE event with size set to zero. Don't go into an infinite
      // loop.
      "perf.data.piped.corrupted.zero_size_sample-3.2",
  };
  return *files;
}

const std::vector<const char*>& GetPerfDataProtoFiles() {
  static const std::vector<const char*>* files = new std::vector<const char*>{
      "perf.callgraph.pb_text",
  };
  return *files;
}

}  // namespace perf_test_files
