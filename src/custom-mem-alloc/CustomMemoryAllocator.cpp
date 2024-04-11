#include "CustomMemoryAllocator.hpp"

#include "utils/logger/Logger.hpp"

#include <cassert>

CustomMemoryAllocator::CustomMemoryAllocator(Logger *logger, size_t poolSize)
    : _logger(logger), _poolSize(poolSize), _firstFreeList(std::make_unique<FreeList>()) {
  _firstFreeList->offset = 0;
  _firstFreeList->size   = poolSize;
  _test();
}

CustomMemoryAllocator::~CustomMemoryAllocator() = default;

void CustomMemoryAllocator::_test() {
  _logger->info("Running test for CustomMemoryAllocator");

  _logger->info("allocate 1 (10)");
  auto res1 = allocate(10);
  _logger->info("allocate 2 (20)");
  auto res2 = allocate(20);
  _logger->info("allocate 3 (10)");
  auto res3 = allocate(10);
  printStats();

  _logger->info("deallocate 1");
  deallocate(res1);
  printStats();
  _logger->info("deallocate 2");
  deallocate(res2);
  printStats();
  _logger->info("deallocate 3");
  deallocate(res3);
  printStats();

  // allocate 100
  _logger->info("allocate 4 (100)");
  auto res4 = allocate(100);
  printStats();

  // deallocate 4
  _logger->info("deallocate 4");
  deallocate(res4);
  printStats();

  // allocate 5 (50)
  _logger->info("allocate 5 (50)");
  auto res5 = allocate(50);
  printStats();

  _logger->info("Test for CustomMemoryAllocator finished");

  // quit the application
  exit(0);
}

// allocate using first-fit algorithm
CustomMemoryAllocationResult CustomMemoryAllocator::allocate(size_t size) {
  FreeList *current = _firstFreeList.get();
  if (current == nullptr) {
    _logger->error("no free memory available, allocation failed");
    return CustomMemoryAllocationResult(-1, 0);
  }

  do {
    if (current->size >= size) {
      size_t res = current->offset;
      // update the freelist to reflect the allocated memory
      current->offset += size;
      current->size -= size;

      // current freelist has been shrinked to 0, remove it
      if (current->size == 0) {
        _removeFreeList(current);
      }
      return CustomMemoryAllocationResult(res, size);
    }
    current = current->next.get();
  } while (current != nullptr);

  _logger->error("no free memory available, allocation failed");
  return CustomMemoryAllocationResult(-1, 0);
}

void CustomMemoryAllocator::deallocate(CustomMemoryAllocationResult allocToBeFreed) {
  FreeList *prev = nullptr;
  FreeList *next = _firstFreeList.get();

  do {
    if (allocToBeFreed.offset() < next->offset || next == nullptr) {
      _addFreeList(allocToBeFreed.offset(), allocToBeFreed.size(), prev, next);
      return;
    }
    prev = next;
    next = next->next.get();
  } while (next != nullptr);

  // if the code reaches here, it means that the allocated memory is at the end of the pool
  _addFreeList(allocToBeFreed.offset(), allocToBeFreed.size(), prev, next);
}

void CustomMemoryAllocator::_removeFreeList(FreeList *freeList) {
  FreeList *prev = freeList->prev;
  FreeList *next = freeList->next.get();

  if (next != nullptr) {
    next->prev = prev;
  }

  // this step also destroys the current freeList implicitly
  if (prev == nullptr) {
    _firstFreeList = std::move(freeList->next);
  } else {
    prev->next = std::move(freeList->next);
  }
}

void CustomMemoryAllocator::_addFreeList(size_t offset, size_t size, FreeList *prev,
                                         FreeList *next) {
  // merging cases are dealt with first, this avoids creating a new freeList
  // case 1: merge with prev and next
  if (prev != nullptr && next != nullptr) {
    if (prev->offset + prev->size == offset && offset + size == next->offset) {
      prev->size += size + next->size;
      _removeFreeList(next);
      return;
    }
  }

  // case 2: merge with prev
  if (prev != nullptr) {
    if (prev->offset + prev->size == offset) {
      prev->size += size;
      return;
    }
  }

  // case 3: merge with next
  if (next != nullptr) {
    if (offset + size == next->offset) {
      next->offset = offset;
      next->size += size;
      return;
    }
  }

  auto freeList    = std::make_unique<FreeList>();
  freeList->offset = offset;
  freeList->size   = size;

  // case 1: this is the first freeList
  if (prev == nullptr) {
    if (next != nullptr) {
      _firstFreeList->prev = freeList.get();
      freeList->next       = std::move(_firstFreeList);
    }
    _firstFreeList = std::move(freeList);
    return;
  }

  // case 2: has a previous one
  freeList->prev = prev;
  if (next != nullptr) {
    next->prev     = freeList.get();
    freeList->next = std::move(prev->next);
  }
  prev->next = std::move(freeList);
}

void CustomMemoryAllocator::printStats() const {
  FreeList *current = _firstFreeList.get();
  while (current != nullptr) {
    _logger->info("freeList: offset={}, size={}", current->offset, current->size);
    current = current->next.get();
  }

  // get the total size of the free memory
  size_t totalSize = 0;
  current          = _firstFreeList.get();
  while (current != nullptr) {
    totalSize += current->size;
    current = current->next.get();
  }
  size_t constexpr kMb = 1024 * 1024;
  _logger->info("total free memory size: {} ({} mb)", totalSize, totalSize / kMb);
}
