#pragma once

#include "interfaces.hpp"
#include <cstdlib>
#include <memory>
#include <mutex>
#include <new>
#include <vector>

namespace TextSimilarity::Core {

// Memory pool for algorithm computations
class MemoryPool final : public IMemoryPool {
public:
  explicit MemoryPool(size_t initial_size = 1024 * 1024); // 1MB default
  ~MemoryPool() override;

  // Non-copyable, non-movable (due to mutex)
  MemoryPool(const MemoryPool &) = delete;
  MemoryPool &operator=(const MemoryPool &) = delete;
  MemoryPool(MemoryPool &&) = delete;
  MemoryPool &operator=(MemoryPool &&) = delete;

  [[nodiscard]] void *
  allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override;
  void deallocate(void *ptr, size_t size) noexcept override;
  void reset() noexcept override;

  // Performance metrics
  [[nodiscard]] size_t get_allocated_size() const noexcept;
  [[nodiscard]] size_t get_total_size() const noexcept;
  [[nodiscard]] double get_utilization() const noexcept;

private:
  struct Block {
    void *memory;
    size_t size;
    size_t offset;

    Block(size_t block_size);
    ~Block();

    [[nodiscard]] void *allocate(size_t size, size_t alignment) noexcept;
    [[nodiscard]] bool has_space(size_t size, size_t alignment) const noexcept;
    void reset() noexcept;
  };

  mutable std::mutex mutex_;
  std::vector<std::unique_ptr<Block>> blocks_;
  size_t default_block_size_;
  size_t total_allocated_;

  [[nodiscard]] size_t align_size(size_t size, size_t alignment) const noexcept;
  void add_block(size_t min_size);
};

// Thread-local memory pool for performance
class ThreadLocalMemoryPool final : public IMemoryPool {
public:
  explicit ThreadLocalMemoryPool(size_t block_size = 64 * 1024); // 64KB default
  ~ThreadLocalMemoryPool() override = default;

  [[nodiscard]] void *
  allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override;
  void deallocate(void *ptr, size_t size) noexcept override;
  void reset() noexcept override;

  // Thread-safe reset for all thread-local pools
  static void reset_all() noexcept;

private:
  size_t block_size_;

  static thread_local std::unique_ptr<MemoryPool> local_pool_;
  static std::mutex pools_mutex_;
  static std::vector<std::weak_ptr<MemoryPool>> all_pools_;
};

// RAII wrapper for scoped memory management
template <typename T> class ScopedPoolAllocator {
public:
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  template <typename U> struct rebind {
    using other = ScopedPoolAllocator<U>;
  };

  explicit ScopedPoolAllocator(std::shared_ptr<IMemoryPool> pool)
      : pool_(std::move(pool)) {}

  template <typename U>
  ScopedPoolAllocator(const ScopedPoolAllocator<U> &other)
      : pool_(other.pool_) {}

  [[nodiscard]] pointer allocate(size_type n) {
    if (n > std::numeric_limits<size_type>::max() / sizeof(T)) {
      throw std::bad_alloc();
    }

    void *ptr = pool_->allocate(n * sizeof(T), alignof(T));
    if (!ptr) {
      throw std::bad_alloc();
    }

    return static_cast<pointer>(ptr);
  }

  void deallocate(pointer ptr, size_type n) noexcept {
    pool_->deallocate(ptr, n * sizeof(T));
  }

  template <typename... Args> void construct(pointer ptr, Args &&...args) {
    ::new (static_cast<void *>(ptr)) T(std::forward<Args>(args)...);
  }

  void destroy(pointer ptr) { ptr->~T(); }

  [[nodiscard]] bool
  operator==(const ScopedPoolAllocator &other) const noexcept {
    return pool_ == other.pool_;
  }

  [[nodiscard]] bool
  operator!=(const ScopedPoolAllocator &other) const noexcept {
    return !(*this == other);
  }

  std::shared_ptr<IMemoryPool> pool_;
};

// Convenient type aliases
template <typename T> using PoolVector = std::vector<T, ScopedPoolAllocator<T>>;

template <typename K, typename V>
using PoolMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>,
                                   ScopedPoolAllocator<std::pair<const K, V>>>;

// Factory functions
[[nodiscard]] std::unique_ptr<IMemoryPool>
create_memory_pool(size_t initial_size = 1024 * 1024);
[[nodiscard]] std::unique_ptr<IMemoryPool>
create_thread_local_pool(size_t block_size = 64 * 1024);

} // namespace TextSimilarity::Core