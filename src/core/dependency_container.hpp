#pragma once

#include "interfaces.hpp"
#include <memory>
#include <mutex>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace TextSimilarity::Core {

class DependencyContainer final : public IDependencyContainer {
public:
  DependencyContainer() = default;
  ~DependencyContainer() override = default;

  // Non-copyable, non-movable (due to mutex)
  DependencyContainer(const DependencyContainer &) = delete;
  DependencyContainer &operator=(const DependencyContainer &) = delete;
  DependencyContainer(DependencyContainer &&) noexcept = delete;
  DependencyContainer &operator=(DependencyContainer &&) noexcept = delete;

private:
  std::shared_ptr<void> resolve_impl(const std::type_info &type) override {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::type_index type_index(type);

    // Check singletons first
    auto singleton_it = singletons_.find(type_index);
    if (singleton_it != singletons_.end()) {
      if (auto instance = singleton_it->second.lock()) {
        return instance;
      }
      // Expired singleton, recreate
      lock.unlock();
      std::unique_lock<std::shared_mutex> write_lock(mutex_);

      auto factory_it = singleton_factories_.find(type_index);
      if (factory_it != singleton_factories_.end()) {
        auto new_instance = factory_it->second();
        singletons_[type_index] = new_instance;
        return new_instance;
      }
    }

    // Check transients
    auto transient_it = transient_factories_.find(type_index);
    if (transient_it != transient_factories_.end()) {
      return transient_it->second();
    }

    throw std::runtime_error("Type not registered: " +
                             std::string(type.name()));
  }

  void register_singleton_impl(
      const std::type_info &type,
      std::function<std::shared_ptr<void>()> factory) override {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    std::type_index type_index(type);
    singleton_factories_[type_index] = std::move(factory);
  }

  void register_transient_impl(
      const std::type_info &type,
      std::function<std::shared_ptr<void>()> factory) override {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    std::type_index type_index(type);
    transient_factories_[type_index] = std::move(factory);
  }

  mutable std::shared_mutex mutex_;
  std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>>
      singleton_factories_;
  std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>>
      transient_factories_;
  std::unordered_map<std::type_index, std::weak_ptr<void>> singletons_;
};

// Factory function for creating configured container
[[nodiscard]] std::unique_ptr<IDependencyContainer>
create_dependency_container();

} // namespace TextSimilarity::Core