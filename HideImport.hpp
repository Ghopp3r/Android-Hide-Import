#pragma once

#include "xdl/include/xdl.h"
#include <elf.h>
#include <cstring>
#include <link.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <stdexcept>

namespace HideImport {
    static std::unordered_map<std::string, void*> gHandleCache;
    static std::unordered_map<std::string, uintptr_t> gSymbolCache;
    static std::mutex gMutex;
    
    inline void* GetHandle(const std::string& lib) {
        auto it = gHandleCache.find(lib);
        if (it != gHandleCache.end()) 
            return it->second;
    
        void* h = nullptr;
        if (!lib.empty())
            h = xdl_open(lib.c_str(), XDL_DEFAULT);
    
        gHandleCache[lib] = h;
        return h;
    }
    
    inline uintptr_t GetSymbol(const std::string& lib, const std::string& sym) {
        const std::string key = lib + ':' + sym;
        {
            std::lock_guard lk(gMutex);
            if (auto it = gSymbolCache.find(key); it != gSymbolCache.end())
                return it->second;
        }
        void *handle = GetHandle(lib);
        if (!handle) return 0;
    
        size_t sz = 0;
        void  *addr = xdl_sym(handle, sym.c_str(), &sz);
        if (addr) {
            xdl_info_t info{};
            if (xdl_info(handle, XDL_DI_DLINFO, &info) == 0) {
                const ElfW(Phdr) *ph = info.dlpi_phdr;
                for(size_t i = 0; i < info.dlpi_phnum; ++i, ++ph) {
                    if (ph->p_type != PT_DYNAMIC) 
                        continue;
                    
                    auto dyn = reinterpret_cast<ElfW(Dyn)*>(reinterpret_cast<uintptr_t>(info.dli_fbase) + ph->p_vaddr);
                    ElfW(Sym) *dynSym = nullptr;
                    const char *strTab = nullptr;
                    size_t nSym = 0;
    
                    for (; dyn->d_tag != DT_NULL; ++dyn) {
                        if (dyn->d_tag == DT_SYMTAB)
                           dynSym = reinterpret_cast<ElfW(Sym)*>(reinterpret_cast<uintptr_t>(info.dli_fbase) + dyn->d_un.d_ptr);
                        else if (dyn->d_tag == DT_STRTAB)
                            strTab = reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(info.dli_fbase) + dyn->d_un.d_ptr);
                        else if (dyn->d_tag == DT_SYMENT)
                            nSym = dyn->d_un.d_val;
                    }
                    if (!dynSym || !strTab) break;
                    
                    for (size_t idx = 0; ; ++idx) {
                        const char *name = strTab + dynSym[idx].st_name;
                        if (strcmp(name, sym.c_str()) == 0) {
                            if (ELF_ST_TYPE(dynSym[idx].st_info) == STT_GNU_IFUNC) {
                                // IFUNC
                                auto selector = (uintptr_t(*)())addr;
                                addr = (void*)selector();
                            }
                            goto resolved;
                        }
                    }
                }
            }
        }
    
    resolved:
        uintptr_t res = reinterpret_cast<uintptr_t>(addr);
        { 
            std::lock_guard lk(gMutex);
            gSymbolCache[key] = res; 
        }
        return res;
    }
    
    inline bool IsHooked(void* fn) {
        #if defined(__aarch64__)
            const uint32_t* p = reinterpret_cast<uint32_t*>(fn);
            return ((p[0] & 0x9F000000u) == 0x90000000u) ||
                   ((p[0] & 0xFF000000u) == 0x58000000u && p[1] == 0xD61F0220u) ||
                   (p[0] == 0x14000000u);
        #elif defined(__arm__)
            const uint32_t* p = reinterpret_cast<uint32_t*>(fn);
            return ((p[0] & 0xFFFFF000u) == 0xE51FF000u) ||
                   ((p[0] & 0x0F7FF000u) == 0x051FF000u && p[1] == 0xE12FFF33u) ||
                   (p[0] == 0xE7F001F0u);
        #elif defined(__x86_64__)
            const uint8_t* b = reinterpret_cast<uint8_t*>(fn);
            return (b[0] == 0xE9) ||
                   (b[0] == 0xFF && b[1] == 0x25) ||
                   (b[0] == 0x48 && b[1] == 0xB8 && b[10] == 0xFF && b[11] == 0xE0) ||
                   (b[0] == 0x68 && b[5] == 0xC3);
        #elif defined(__i386__)
            const uint8_t* b = reinterpret_cast<uint8_t*>(fn);
            return (b[0] == 0xE9) ||
                   (b[0] == 0x68 && b[5] == 0xC3) ||
                   (b[0] == 0xFF && b[1] == 0x25);
        #else
            (void)fn;
            return false;
        #endif
    }
} // namespace HideImport

// helpers
#define HI_GET(lib, sym) HideImport::GetSymbol(lib, sym)
#define IS_FUNCTION_HOOKED(ptr) HideImport::IsHooked(reinterpret_cast<void*>(ptr))
