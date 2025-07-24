#include "memory_pool.hpp"
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <malloc.h>
#else
#include <cstdlib>
#endif

// Check for macOS version
#ifdef __APPLE__
#include <Availability.h>
#endif

namespace TextSimilarity::Core {

// MemoryPool::Block Implementation

MemoryPool::Block::Block(size_t block_size) : size(block_size), offset(0) {
#ifdef _WIN32
  // Use _aligned_malloc on Windows
  memory = _aligned_malloc(block_size, alignof(std::max_align_t));
#elif defined(__APPLE__) && defined(__MAC_OS_X_VERSION_MIN_REQUIRED) &&        \
    __MAC_OS_X_VERSION_MIN_REQUIRED < 101500
  // Fallback for older macOS versions
  if (posix_memalign(&memory, alignof(std::max_align_t), block_size) != 0) {
    memory = nullptr;
  }
#elif defined(__cpp_aligned_new) && __cpp_aligned_new >= 201606L
  // Use std::aligned_alloc if available (C++17)
  memory = std::aligned_alloc(alignof(std::max_align_t), block_size);
#else
  // Fallback to posix_memalign for other systems
  if (posix_memalign(&memory, alignof(std::max_align_t), block_size) != 0) {
    memory = nullptr;
  }
#endif
  if (!memory) {
    throw std::bad_alloc();
  }
}

MemoryPool::Block::~Block() {
  if (memory) {
#ifdef _WIN32
    _aligned_free(memory);
#else
    std::free(memory);
#endif
  }
}

void *MemoryPool::Block::allocate(size_t alloc_size,
                                  size_t alignment) noexcept {
  // Calculate aligned offset
  size_t aligned_offset = (offset + alignment - 1) & ~(alignment - 1);

  if (aligned_offset + alloc_size > size) {
    return nullptr; // Not enough space
  }

  void *ptr = static_cast<char *>(memory) + aligned_offset;
  offset = aligned_offset + alloc_size;

  return ptr;
}

bool MemoryPool::Block::has_space(size_t alloc_size,
                                  size_t alignment) const noexcept {
  size_t aligned_offset = (offset + alignment - 1) & ~(alignment - 1);
  return aligned_offset + alloc_size <= size;
}

void MemoryPool::Block::reset() noexcept { offset = 0; }

// MemoryPool Implementation

MemoryPool::MemoryPool(size_t initial_size)
    : default_block_size_(initial_size), total_allocated_(0) {
  if (initial_size == 0) {
    throw std::invalid_argument("Block size must be greater than 0");
  }

  // Create initial block
  add_block(initial_size);
}

MemoryPool::~MemoryPool() = default;

void *MemoryPool::allocate(size_t size, size_t alignment) {
  if (size == 0) {
    return nullptr;
  }

  if (alignment == 0) {
    alignment = alignof(std::max_align_t);
  }

  // Ensure alignment is a power of 2
  if ((alignment & (alignment - 1)) != 0) {
    throw std::invalid_argument("Alignment must be a power of 2");
  }

  std::lock_guard<std::mutex> lock(mutex_);

  // Try to allocate from existing blocks
  for (auto &block : blocks_) {
    if (void *ptr = block->allocate(size, alignment)) {
      total_allocated_ += size;
      return ptr;
    }
  }

  // Need a new block
  size_t new_block_size =
      std::max(default_block_size_, align_size(size, alignment));
  add_block(new_block_size);

  // Try allocation again (should succeed now)
  void *ptr = blocks_.back()->allocate(size, alignment);
  if (!ptr) {
    throw std::bad_alloc();
  }

  total_allocated_ += size;
  return ptr;
}

void MemoryPool::deallocate(void *ptr, size_t size) noexcept {
  // Pool-based allocation doesn't support individual deallocation
  // Memory is reclaimed on reset() or destruction
  (void)ptr;
  (void)size;
}

void MemoryPool::reset() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto &block : blocks_) {
    block->reset();
  }

  total_allocated_ = 0;
}

size_t MemoryPool::get_allocated_size() const noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  return total_allocated_;
}

size_t MemoryPool::get_total_size() const noexcept {
  std::lock_guard<std::mutex> lock(mutex_);

  size_t total = 0;
  for (const auto &block : blocks_) {
    total += block->size;
  }

  return total;
}

double MemoryPool::get_utilization() const noexcept {
  std::lock_guard<std::mutex> lock(mutex_);

  size_t total_size = 0;
  size_t used_size = 0;

  for (const auto &block : blocks_) {
    total_size += block->size;
    used_size += block->offset;
  }

  return total_size > 0
             ? static_cast<double>(used_size) / static_cast<double>(total_size)
             : 0.0;
}

size_t MemoryPool::align_size(size_t size, size_t alignment) const noexcept {
  return (size + alignment - 1) & ~(alignment - 1);
}

void MemoryPool::add_block(size_t min_size) {
  try {
    blocks_.push_back(std::make_unique<Block>(min_size));
  } catch (const std::bad_alloc &) {
    // Clean up and rethrow
    throw;
  }
}

// ThreadLocalMemoryPool Implementation

thread_local std::unique_ptr<MemoryPool> ThreadLocalMemoryPool::local_pool_{
    nullptr};
std::mutex ThreadLocalMemoryPool::pools_mutex_;
std::vector<std::weak_ptr<MemoryPool>> ThreadLocalMemoryPool::all_pools_;

ThreadLocalMemoryPool::ThreadLocalMemoryPool(size_t block_size)
    : block_size_(block_size) {}

void *ThreadLocalMemoryPool::allocate(size_t size, size_t alignment) {
  if (!local_pool_) {
    local_pool_ = std::make_unique<MemoryPool>(block_size_);

    // Register this pool for global reset capability
    std::lock_guard<std::mutex> lock(pools_mutex_);
    all_pools_.emplace_back(std::weak_ptr<MemoryPool>{});
  }

  return local_pool_->allocate(size, alignment);
}

void ThreadLocalMemoryPool::deallocate(void *ptr, size_t size) noexcept {
  if (local_pool_) {
    local_pool_->deallocate(ptr, size);
  }
}

void ThreadLocalMemoryPool::reset() noexcept {
  if (local_pool_) {
    local_pool_->reset();
  }
}

void ThreadLocalMemoryPool::reset_all() noexcept {
  std::lock_guard<std::mutex> lock(pools_mutex_);

  // Clean up expired weak pointers and reset active ones
  all_pools_.erase(std::remove_if(all_pools_.begin(), all_pools_.end(),
                                  [](const std::weak_ptr<MemoryPool> &wp) {
                                    if (auto sp = wp.lock()) {
                                      sp->reset();
                                      return false;
                                    }
                                    return true; // Remove expired weak_ptr
                                  }),
                   all_pools_.end());
}

// Factory Functions

std::unique_ptr<IMemoryPool> create_memory_pool(size_t initial_size) {
  return std::make_unique<MemoryPool>(initial_size);
}

std::unique_ptr<IMemoryPool> create_thread_local_pool(size_t block_size) {
  return std::make_unique<ThreadLocalMemoryPool>(block_size);
}

} // namespace TextSimilarity::Core