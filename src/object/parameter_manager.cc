// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#include "object/parameter_manager.h"

#include <stdexcept>

Scapegoat::Scapegoat() : root(nullptr) {}

Scapegoat::~Scapegoat() = default;

std::any const& Scapegoat::operator[](int key) const { return Get(key); }

std::any const& Scapegoat::Get(int key) const {
  const auto& ptr = Find(key, root);

  if (!ptr || !ptr->value.has_value()) {
    throw std::out_of_range("ScapegoatTree: Non-existent key " +
                            std::to_string(key));
  }
  return ptr->value;
}

void Scapegoat::Set(int key, std::any value) {
  Touch(key, std::move(value), root);
  CheckRebuild(key, root);
}

bool Scapegoat::Contains(int key) const {
  const auto& ptr = Find(key, root);
  return ptr && ptr->value.has_value();
}

void Scapegoat::Remove(int key) {
  Remove(key, root);
  CheckRebuild(key, root);
}

Scapegoat::Node::Node(key_t ikey, std::any ival)
    : key(ikey),
      value(std::move(ival)),
      tree_size(1),
      lch(nullptr),
      rch(nullptr) {}

Scapegoat::Node& Scapegoat::Node::Update() {
  tree_size = 1;
  if (lch)
    tree_size += lch->tree_size;
  if (rch)
    tree_size += rch->tree_size;
  return *this;
}

bool Scapegoat::Node::IsBalance() const {
  const size_t threshold = tree_size * Scapegoat::alpha;
  if (lch && lch->tree_size > threshold)
    return false;
  if (rch && rch->tree_size > threshold)
    return false;
  return true;
}

std::shared_ptr<Scapegoat::Node> Scapegoat::Find(
    key_t key,
    const std::shared_ptr<Node>& nowAt) const {
  if (!nowAt)
    return nullptr;
  if (key == nowAt->key)
    return nowAt;
  return Find(key, key < nowAt->key ? nowAt->lch : nowAt->rch);
}

std::shared_ptr<Scapegoat::Node>
Scapegoat::Touch(key_t key, value_t value, std::shared_ptr<Node>& nowAt) {
  if (!nowAt) {
    return nowAt = std::make_shared<Node>(key, std::move(value));
  }
  if (!nowAt.unique()) {
    nowAt = std::make_shared<Node>(*nowAt);
  }

  if (key == nowAt->key) {
    nowAt->value = std::move(value);
    return nowAt;
  } else {
    auto result = Touch(key, std::move(value),
                        key < nowAt->key ? nowAt->lch : nowAt->rch);
    nowAt->Update();
    return result;
  }
}

void Scapegoat::Remove(key_t key, std::shared_ptr<Node>& nowAt) {
  if (!nowAt)
    return;
  if (nowAt->key == key) {
    nowAt->value.reset();
    return;
  }

  Remove(key, key < nowAt->key ? nowAt->lch : nowAt->rch);
  nowAt->Update();
}

void Scapegoat::CheckRebuild(key_t key, std::shared_ptr<Node>& nowAt) {
  if (!nowAt || nowAt->tree_size < 16)
    return;
  if (!nowAt->IsBalance()) {
    nowAt = Rebuild(nowAt);
    return;
  }

  CheckRebuild(key, key < nowAt->key ? nowAt->lch : nowAt->rch);
}

std::shared_ptr<Scapegoat::Node> Scapegoat::Rebuild(
    std::shared_ptr<Node> root) {
  std::vector<std::pair<key_t, value_t>> nodes;
  nodes.reserve(root->tree_size);
  Collect(root, nodes);
  return Build(nodes, 0, nodes.size());
}

std::shared_ptr<Scapegoat::Node> Scapegoat::Build(
    const std::vector<std::pair<key_t, value_t>>& nodes,
    size_t fr,
    size_t to) {
  if (fr >= to)
    return nullptr;

  auto mid = (fr + to) >> 1;
  auto nowAt = std::make_shared<Node>(nodes[mid].first);
  nowAt->value = std::move(nodes[mid].second);
  nowAt->tree_size = to - fr;
  nowAt->lch = Build(nodes, fr, mid);
  nowAt->rch = Build(nodes, mid + 1, to);
  return nowAt;
}

void Scapegoat::Collect(std::shared_ptr<Node> nowAt,
                        std::vector<std::pair<key_t, value_t>>& container) {
  if (!nowAt)
    return;
  Collect(nowAt->lch, container);
  if (nowAt->value.has_value())
    container.emplace_back(std::make_pair(nowAt->key, nowAt->value));
  Collect(nowAt->rch, container);
}
