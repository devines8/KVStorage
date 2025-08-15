#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

template <typename Clock = std::chrono::system_clock> class KVStorage {
public:
  struct Record {
    std::string value;
    std::optional<typename Clock::time_point> expiry;
  };

  explicit KVStorage(
      std::span<std::tuple<std::string, std::string, uint32_t>> entries,
      Clock clock = Clock{})
      : clock_(clock) {
    for (auto &[key, value, ttl] : entries) {
      set(key, value, ttl);
    }
  }

  void set(std::string key, std::string value, uint32_t ttl = 0) {
    std::unique_lock l(mutex_);
    auto expiry = (ttl != 0)
                      ? std::optional{clock_.now() + std::chrono::seconds(ttl)}
                      : std::nullopt;

    records_[key] = {std::move(value), expiry};
    if (expiry) {
      expiry_queue_.insert({*expiry, key});
    }
  }

  bool remove(std::string_view key) {
    std::unique_lock l(mutex_);
    auto it = records_.find(std::string(key));
    if (it == end(records_)) {
      return false;
    }

    if (it->second.expiry) {
      expiry_queue_.erase({*it->second.expiry, it->first});
    }
    records_.erase(it);
    return true;
  }

  std::optional<std::string> get(std::string_view key) const {
    std::shared_lock l(mutex_);
    auto it = records_.find(std::string(key));
    if (it == end(records_)) {
      return std::nullopt;
    }

    auto &[value, expire] = it->second;
    if (expire && *expire <= clock_.now()) {
      return std::nullopt;
    }
    return value;
  }

  std::vector<std::pair<std::string, std::string>>
  getManySorted(std::string_view key, uint32_t count) const {
    std::shared_lock l(mutex_);
    std::vector<std::pair<std::string, std::string>> result;
    auto it = records_.lower_bound(std::string(key));

    if (it != records_.end() && it->first == key) {
      ++it;
    }

    while (it != end(records_) && result.size() < count) {
      if (!it->second.expiry || *it->second.expiry > clock_.now()) {
        result.emplace_back(it->first, it->second.value);
      }
      ++it;
    }
    return result;
  }

  std::optional<std::pair<std::string, std::string>> removeOneExpiredEntry() {
    std::unique_lock l(mutex_);
    auto it = begin(expiry_queue_);
    if (it != end(expiry_queue_) && it->expiry <= clock_.now()) {
      auto value = records_[it->key].value;
      records_.erase(it->key);
      expiry_queue_.erase(it);
      return std::make_pair(it->key, value);
    }
    return std::nullopt;
  }

private:
  struct ExpiryEntry {
    typename Clock::time_point expiry;
    std::string key;

    bool operator<(const ExpiryEntry &other) const {
      return std::tie(expiry, key) < std::tie(other.expiry, other.key);
    }
  };

  mutable std::shared_mutex mutex_;
  std::map<std::string, Record> records_;
  std::set<ExpiryEntry> expiry_queue_;
  Clock clock_;
};
