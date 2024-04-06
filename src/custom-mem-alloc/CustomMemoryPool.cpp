#include "CustomMemoryPool.hpp"

#include "utils/logger/Logger.hpp"

#include <cassert>

CustomMemoryPool::CustomMemoryPool(Logger *logger, size_t poolSize)
    : _logger(logger), _poolSize(poolSize) {
  _firstFreeList         = new FreeList();
  _firstFreeList->offset = 0;
  _firstFreeList->size   = poolSize;
}

CustomMemoryPool::~CustomMemoryPool() {
  FreeList *current = _firstFreeList;
  while (current != nullptr) {
    FreeList *next = current->next;
    delete current;
    current = next;
  }
}

// void CustomMemoryPool::_test() {
//   auto offset1 = allocate(10);
//   auto offset2 = allocate(10);
//   auto offset3 = allocate(10);
//   auto offset4 = allocate(10);

//   deallocate(offset1.value(), 10);

//   deallocate(offset3.value(), 10);

//   auto offset5 = allocate(6);
//   auto offset6 = allocate(5);

//   print();
// }

std::optional<size_t> CustomMemoryPool::allocate(size_t size) {
  // allocate using first-fit algorithm
  FreeList *current = _firstFreeList;
  while (current != nullptr) {
    if (current->size >= size) {
      // allocate memory
      size_t offset = current->offset;
      current->offset += size;
      current->size -= size;

      if (current->size == 0) {
        _removeFreeList(current);
      }

      return offset;
    }
    current = current->next;
  }
  return std::nullopt;
}

void CustomMemoryPool::deallocate(size_t offset, size_t size) {
  assert(offset >= 0 && offset + size <= _poolSize);

  FreeList *prev = nullptr;
  FreeList *next = _firstFreeList;
  while (next != nullptr && next->offset < offset) {
    prev = next;
    next = next->next;
  }

  _createFreeList(offset, size, prev, next);
}

FreeList *CustomMemoryPool::_createFreeList(size_t offset, size_t size, FreeList *prev,
                                            FreeList *next) {
  FreeList *freeList = new FreeList();
  freeList->prev     = prev;
  freeList->next     = next;
  freeList->offset   = offset;
  freeList->size     = size;

  if (prev != nullptr) {
    prev->next = freeList;
    // merge if possible
    if (prev->offset + prev->size == freeList->offset) {
      prev->size += freeList->size;
      _removeFreeList(freeList);
      freeList = prev;
    }
  } else {
    _firstFreeList = freeList;
  }

  if (next != nullptr) {
    next->prev = freeList;
    // merge if possible
    if (freeList->offset + freeList->size == next->offset) {
      freeList->size += next->size;
      _removeFreeList(next);
    }
  }

  return freeList;
}

void CustomMemoryPool::_removeFreeList(FreeList *freeList) {
  if (freeList->prev != nullptr) {
    freeList->prev->next = freeList->next;
  }
  if (freeList->next != nullptr) {
    freeList->next->prev = freeList->prev;
  }
  if (freeList == _firstFreeList) {
    _firstFreeList = freeList->next;
  }
  delete freeList;
}

void CustomMemoryPool::printStats() const {
  FreeList *current = _firstFreeList;
  while (current != nullptr) {
    _logger->info("freeList: offset={}, size={}", current->offset, current->size);
    current = current->next;
  }

  // get the total size of the free memory
  size_t totalSize = 0;
  current          = _firstFreeList;
  while (current != nullptr) {
    totalSize += current->size;
    current = current->next;
  }
  size_t constexpr kMb = 1024 * 1024;
  _logger->info("total free memory size: {} mb", totalSize / kMb);
}
