/*
 * Copyright (c) 2015-2019 Carnegie Mellon University and
 *         Los Alamos National Laboratory.
 * All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#pragma once

#include "deltafs_plfsio_builder.h"
#include "deltafs_plfsio_doublebuf.h"

namespace pdlfs {
namespace plfsio {

// Shared options object from which we obtain the thread pool for background
// compaction as well as other configurations.
struct DirOptions;

// Block type.
class ArrayBlockBuilder;

// Directly write data as formatted data blocks.
// Incoming key-value pairs are assumed to be fixed sized.
class BufferedBlockWriter : public DoubleBuffering {
 public:
  BufferedBlockWriter(const DirOptions& options, WritableFile* dst,
                      size_t buf_size);

  // REQUIRES: Finish() has NOT been called.
  // Insert data into the writer.
  Status Add(const Slice& k, const Slice& v);
  // Wait until there is no outstanding compactions.
  Status Wait();
  // Force an epoch flush.
  Status EpochFlush();
  // Force a compaction.
  Status Flush();
  // Sync data to storage.
  Status Sync();

  // Finalize the writer.
  Status Finish();

  ~BufferedBlockWriter();

 private:
  typedef ArrayBlockBuilder BlockBuf;
  const DirOptions& options_;
  WritableFile* const dst_;
  port::Mutex mu_;
  port::CondVar bg_cv_;
  const size_t buf_threshold_;  // Threshold for write buffer flush
  // Memory pre-reserved for each write buffer
  size_t buf_reserv_;

  friend class DoubleBuffering;
  Status Compact(void* buf);
  Status SyncBackend(bool close = false);
  void ScheduleCompaction();
  void Clear(void* buf) { static_cast<BlockBuf*>(buf)->Reset(); }
  void AddToBuffer(void* buf, const Slice& k, const Slice& v) {
    static_cast<BlockBuf*>(buf)->Add(k, v);
  }
  bool HasRoom(const void* buf, const Slice& k, const Slice& v) {
    return (static_cast<const BlockBuf*>(buf)->CurrentSizeEstimate() +
                k.size() + v.size() <=
            buf_threshold_);
  }
  bool IsEmpty(const void* buf) {
    return static_cast<const BlockBuf*>(buf)->empty();
  }

  static void BGWork(void*);

  BlockBuf bb0_;
  BlockBuf bb1_;
};

}  // namespace plfsio
}  // namespace pdlfs