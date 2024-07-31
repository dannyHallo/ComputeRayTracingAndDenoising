#pragma once

#include <memory>

struct FreeList {
  FreeList *prev                 = nullptr;
  std::unique_ptr<FreeList> next = nullptr;
  size_t offset                  = 0;
  size_t size                    = 0;
};

class CustomMemoryAllocationResult {
public:
  CustomMemoryAllocationResult() : _offset(0), _size(0) {}
  CustomMemoryAllocationResult(size_t offset, size_t size) : _offset(offset), _size(size) {}
  size_t offset() const { return _offset; }
  size_t size() const { return _size; }

private:
  size_t _offset;
  size_t _size;
};

class Logger;

class CustomMemoryAllocator {
public:
  CustomMemoryAllocator(Logger *logger, size_t poolSize);
  ~CustomMemoryAllocator();

  // disable copy and move
  CustomMemoryAllocator(const CustomMemoryAllocator &)            = delete;
  CustomMemoryAllocator &operator=(const CustomMemoryAllocator &) = delete;
  CustomMemoryAllocator(CustomMemoryAllocator &&)                 = delete;
  CustomMemoryAllocator &operator=(CustomMemoryAllocator &&)      = delete;

  // allocate memory from the pool using first-fit algorithm, returns the starting address (aka.
  // offset) of the allocated memory
  CustomMemoryAllocationResult allocate(size_t size);

  // deallocate memory from the pool using the address of the allocated memory
  void deallocate(CustomMemoryAllocationResult allocToBeFreed);

  void freeAll();

  void printStats() const;

private:
  Logger *_logger;

  size_t _poolSize;
  std::unique_ptr<FreeList> _firstFreeList = nullptr;

  void _test();

  std::unique_ptr<FreeList> &_getUniquePtr(FreeList *freeList);

  void _removeFreeList(FreeList *freeList);

  void _addFreeList(size_t offset, size_t size, FreeList *prev = nullptr, FreeList *next = nullptr);
};
