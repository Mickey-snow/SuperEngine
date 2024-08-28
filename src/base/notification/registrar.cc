// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/notification/registrar.h"

#include <algorithm>

#include "base/notification/service.h"

struct NotificationRegistrar::Record {
  bool operator==(const Record& other) const;

  NotificationObserver* observer;
  NotificationType type;
  NotificationSource source;
};

bool NotificationRegistrar::Record::operator==(const Record& other) const {
  return observer == other.observer &&
         type == other.type &&
         source == other.source;
}

NotificationRegistrar::NotificationRegistrar() {
}

NotificationRegistrar::~NotificationRegistrar() {
  RemoveAll();
}

void NotificationRegistrar::Add(NotificationObserver* observer,
                                NotificationType type,
                                const NotificationSource& source) {
  Record record = { observer, type, source };
  registered_.push_back(record);

  NotificationService::current()->AddObserver(observer, type, source);
}

void NotificationRegistrar::Remove(NotificationObserver* observer,
                                   NotificationType type,
                                   const NotificationSource& source) {
  if (!IsRegistered(observer, type, source)) {
    return;
  }

  Record record = { observer, type, source };
  auto found = std::find(registered_.cbegin(), registered_.cend(), record);
  registered_.erase(found);

  // This can be NULL if our owner outlives the NotificationService, e.g. if our
  // owner is a Singleton.
  NotificationService* service = NotificationService::current();
  if (service)
    service->RemoveObserver(observer, type, source);
}

void NotificationRegistrar::RemoveAll() {
  // Early-exit if no registrations, to avoid calling
  // NotificationService::current.  If we've constructed an object with a
  // NotificationRegistrar member, but haven't actually used the notification
  // service, and we reach prgram exit, then calling current() below could try
  // to initialize the service's lazy TLS pointer during exit, which throws
  // wrenches at things.
  if (registered_.empty())
    return;

  // This can be NULL if our owner outlives the NotificationService, e.g. if our
  // owner is a Singleton.
  NotificationService* service = NotificationService::current();
  if (service) {
    for (size_t i = 0; i < registered_.size(); i++) {
      service->RemoveObserver(registered_[i].observer,
                              registered_[i].type,
                              registered_[i].source);
    }
  }
  registered_.clear();
}

bool NotificationRegistrar::IsEmpty() const {
  return registered_.empty();
}

bool NotificationRegistrar::IsRegistered(NotificationObserver* observer,
                                         NotificationType type,
                                         const NotificationSource& source) {
  Record record = { observer, type, source };
  return std::find(registered_.cbegin(), registered_.cend(), record) !=
      registered_.cend();
}
