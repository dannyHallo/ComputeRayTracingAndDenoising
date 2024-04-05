#pragma once

#include <optional>

struct FreeList;
struct FreeList {
  FreeList *prev = nullptr;
  FreeList *next = nullptr;
  size_t offset  = 0;
  size_t size    = 0;
};

class Logger;

class CustomMemoryPool {
public:
  CustomMemoryPool(Logger *logger, size_t poolSize);
  ~CustomMemoryPool();

  // disable copy and move
  CustomMemoryPool(const CustomMemoryPool &)            = delete;
  CustomMemoryPool &operator=(const CustomMemoryPool &) = delete;
  CustomMemoryPool(CustomMemoryPool &&)                 = delete;
  CustomMemoryPool &operator=(CustomMemoryPool &&)      = delete;

  // allocate memory from the pool using first-fit algorithm, returns the starting address (aka.
  // offset) of the allocated memory
  std::optional<size_t> allocate(size_t size);

  // deallocate memory from the pool using the address of the allocated memory
  void deallocate(size_t offset, size_t size);

  void print() const;

private:
  Logger *_logger;

  size_t _poolSize;
  FreeList *_firstFreeList = nullptr;

  // void _test();

  FreeList *_createFreeList(size_t offset, size_t size, FreeList *prev = nullptr,
                            FreeList *next = nullptr);

  void _removeFreeList(FreeList *freeList);
};
