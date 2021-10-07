// Copyright 2021, Autonomous Space Robotics Lab (ASRL)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * \file lockable.hpp
 * \brief
 * \details
 *
 * \author Autonomous Space Robotics Lab (ASRL)
 */
#pragma once

#include <mutex>
#include <shared_mutex>
#include <type_traits>

namespace vtr {
namespace common {

/// A lockable type that requires being locked for access
template <typename T>
struct Lockable {
 public:
  /// A locked thread-safe reference to the value
  template <typename R>
  struct LockedRef : public std::reference_wrapper<R> {
    LockedRef(std::recursive_mutex& mutex, R& ref)
        : lock(mutex, std::defer_lock), std::reference_wrapper<R>(ref) {
      lock.lock();
    }

   private:
    std::unique_lock<std::recursive_mutex> lock;
  };

  /// Constructors
  template <bool IDC = std::is_default_constructible<T>::value,
            typename std::enable_if<IDC, bool>::type = false>
  Lockable() {}
  Lockable(T&& val) { val_ = val; }
  Lockable(const Lockable& other) : val_(other.val_) {}
  Lockable(Lockable&& other) = default;

  Lockable& operator=(const Lockable& other) {
    this->val_ = other.val_;
    return *this;
  }
  Lockable& operator=(Lockable&& other) = default;

  /// Get a locked const reference to the value
  LockedRef<const T> locked() const { return {mutex_, val_}; }
  /// Get a locked reference to the value
  LockedRef<T> locked() { return {mutex_, val_}; }

 private:
  /// The value with controlled access
  T val_;
  /// The mutex that controls access to the value
  mutable std::recursive_mutex mutex_;
};

/// A lockable type that requires being locked for access
template <typename T>
struct SharedLockable {
 public:
  using MutexType = std::shared_mutex;
  using Ptr = std::shared_ptr<SharedLockable<T>>;

  /// A uniquely locked thread-safe reference to the value
  template <typename R>
  struct LockedRef : public std::reference_wrapper<R> {
    LockedRef(MutexType& mutex, R& ref)
        : lock(mutex, std::defer_lock), std::reference_wrapper<R>(ref) {
      lock.lock();
    }

   private:
    std::unique_lock<MutexType> lock;
  };

  /// A shared locked thread-safe reference to the value
  template <typename R>
  struct SharedLockedRef : public std::reference_wrapper<R> {
    SharedLockedRef(MutexType& mutex, R& ref)
        : lock(mutex, std::defer_lock), std::reference_wrapper<R>(ref) {
      lock.lock();
    }

   private:
    std::shared_lock<MutexType> lock;
  };

  /// Constructors
  template <class... Args>
  SharedLockable(Args&&... args) : val_(std::forward<Args>(args)...) {}

  /// Disable copy and move
  SharedLockable(const SharedLockable& other) = delete;
  SharedLockable(SharedLockable&& other) = delete;
  SharedLockable& operator=(const SharedLockable& other) = delete;
  SharedLockable& operator=(SharedLockable&& other) = delete;

  /// Get a locked const reference to the value
  LockedRef<const T> locked() const { return {mutex_, val_}; }
  /// Get a locked reference to the value
  LockedRef<T> locked() { return {mutex_, val_}; }

  /// Get a locked const reference to the value
  SharedLockedRef<const T> sharedLocked() const { return {mutex_, val_}; }
  /// Get a locked reference to the value
  SharedLockedRef<T> sharedLocked() { return {mutex_, val_}; }

  /** \brief Gets an unlocked const reference to the value. */
  std::reference_wrapper<const T> unlocked() const { return val_; }
  std::reference_wrapper<T> unlocked() { return val_; }

  /** \brief Gets a reference to the mutex. */
  MutexType& mutex() const { return std::ref(mutex_); }

 private:
  /// The value with controlled access
  T val_;
  /// The mutex that controls access to the value
  mutable MutexType mutex_;
};

}  // namespace common
}  // namespace vtr
