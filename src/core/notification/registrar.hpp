// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>
#include <vector>

#include "core/notification/type.hpp"

class NotificationObserver;
class NotificationSource;

// Aids in registering for notifications and ensures that all registered
// notifications are unregistered when the class is destroyed.
//
// The intended use is that you make a NotificationRegistrar member in your
// class and use it to register your notifications instead of going through the
// notification service directly. It will automatically unregister them for
// you.
class NotificationRegistrar {
 public:
  // This class must not be derived from (we don't have a virtual destructor so
  // it won't work). Instead, use it as a member in your class.
  NotificationRegistrar();
  ~NotificationRegistrar();

  // Wrappers around NotificationService::[Add|Remove]Observer.
  void Add(NotificationObserver* observer,
           NotificationType type,
           const NotificationSource& source);
  void Remove(NotificationObserver* observer,
              NotificationType type,
              const NotificationSource& source);

  // Unregisters all notifications.
  void RemoveAll();

  // Returns true if no notifications are registered.
  bool IsEmpty() const;

  // Returns true if there is already a registered notification with the
  // specified details.
  bool IsRegistered(NotificationObserver* observer,
                    NotificationType type,
                    const NotificationSource& source);

 private:
  struct Record;

  // We keep registered notifications in a simple vector. This means we'll do
  // brute-force searches when removing them individually, but individual
  // removal is uncommon, and there will typically only be a couple of
  // notifications anyway.
  typedef std::vector<Record> RecordVector;

  // Lists all notifications we're currently registered for.
  RecordVector registered_;
};
