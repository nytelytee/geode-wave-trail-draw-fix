#include <Geode/Geode.hpp>
using namespace geode::prelude;

inline Result<> withProtectedMemory(void* address, size_t size, auto&& callback) {
  DWORD oldProtect;

  if (VirtualProtect(address, size, PAGE_READWRITE, &oldProtect)) {
    callback(address);
    VirtualProtect(address, size, oldProtect, &oldProtect);
  } else
    return Err(fmt::format("Failed to set memory protection: {}", GetLastError()));

  return Ok();
}

void inlHook(uintptr_t target, uintptr_t detour) {
  auto array = std::to_array<uint8_t>({
    0xff, 0x25, 0x00, 0x00, 0x00, 0x00,             // jmp [rip+0]
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // address
  });

  std::memcpy(array.data() + 6, &detour, sizeof(uintptr_t));

  auto res = withProtectedMemory(
    reinterpret_cast<void*>(target),
    array.size(),
    [&](void* mem){std::memcpy(mem, array.data(), array.size());}
  );

  if (!res) log::error("{}", res.unwrapErr());

}
$execute {
  inlHook(reinterpret_cast<uintptr_t>(&malloc), reinterpret_cast<uintptr_t>(&_malloc_base));
  inlHook(reinterpret_cast<uintptr_t>(&calloc), reinterpret_cast<uintptr_t>(&_calloc_base));
  inlHook(reinterpret_cast<uintptr_t>(&realloc), reinterpret_cast<uintptr_t>(&_realloc_base));
  inlHook(reinterpret_cast<uintptr_t>(&free), reinterpret_cast<uintptr_t>(&_free_base));
}
