#include "CustomMemoryAllocator.hpp"

#include "utils/logger/Logger.hpp"

#include <cassert>

#include <random>

static size_t constexpr kMb = 1024 * 1024;

CustomMemoryAllocator::CustomMemoryAllocator(Logger *logger, size_t poolSize)
    : _logger(logger), _poolSize(poolSize), _firstFreeList(std::make_unique<FreeList>()) {
  _firstFreeList->offset = 0;
  _firstFreeList->size   = poolSize;

  // uncomment this line to run the test
  // _test();
}

CustomMemoryAllocator::~CustomMemoryAllocator() = default;

void CustomMemoryAllocator::_test() {
  _logger->info("Running test for CustomMemoryAllocator");

  // create random device
  std::random_device rd;
  std::mt19937 gen(rd());

  double constexpr kBufferLowerBound = 1.0;
  double constexpr kBufferUpperBound = 3.0;
  size_t constexpr kInitialChunkSize = 100;
  double constexpr kBufferGrowSpeed  = 0.0001;

  std::uniform_real<double> chunkBufferSizeDis(kBufferLowerBound, kBufferUpperBound);

  // this is for selecting a random chunk
  std::uniform_int_distribution<size_t> chunkSelectionDis(0, kInitialChunkSize - 1);

  std::vector<CustomMemoryAllocationResult> chunks{};
  for (int i = 0; i < kInitialChunkSize; ++i) {
    size_t usingBufferSize = static_cast<size_t>(chunkBufferSizeDis(gen) * kMb);
    chunks.push_back(allocate(usingBufferSize));
  }

  int constexpr kIt = 100000000;
  for (int i = 0; i < kIt; ++i) {
    size_t selectedChunk = chunkSelectionDis(gen);

    CustomMemoryAllocationResult &chunk = chunks[selectedChunk];

    double chunkSizeMb = static_cast<double>(chunk.size()) / kMb;
    double bufferSize =
        chunkSizeMb < kBufferUpperBound ? chunkSizeMb + kBufferGrowSpeed : chunkBufferSizeDis(gen);

    deallocate(chunk);

    size_t usingBufferSize = static_cast<size_t>(bufferSize * kMb);
    chunk                  = allocate(usingBufferSize);
  }

  printStats();

  // quit the application
  exit(0);
}

// allocate using first-fit algorithm
CustomMemoryAllocationResult CustomMemoryAllocator::allocate(size_t size) {
  FreeList *current = _firstFreeList.get();
  if (current == nullptr) {
    _logger->error("no free memory available, allocation failed");
    exit(0);
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
  exit(0);
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

void CustomMemoryAllocator::freeAll() {
  _firstFreeList         = std::make_unique<FreeList>();
  _firstFreeList->offset = 0;
  _firstFreeList->size   = _poolSize;
  _logger->info("all memory has been freed");

  printStats();
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
