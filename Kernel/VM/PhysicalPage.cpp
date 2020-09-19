/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <Kernel/Heap/kmalloc.h>
#include <Kernel/VM/MemoryManager.h>
#include <Kernel/VM/PhysicalPage.h>
#include <Kernel/VM/PhysicalRegion.h>

namespace Kernel {

NonnullRefPtr<PhysicalPage> PhysicalPage::create(PhysicalAddress paddr, bool supervisor, bool may_return_to_freelist)
{
    return adopt(*new PhysicalPage(paddr, supervisor, may_return_to_freelist));
}

PhysicalPage::PhysicalPage(PhysicalAddress paddr, bool supervisor, bool may_return_to_freelist)
    : m_may_return_to_freelist(may_return_to_freelist)
    , m_supervisor(supervisor)
    , m_paddr(paddr)
{
    m_swap_entry.clear();
}

void PhysicalPage::return_to_freelist()
{
    ASSERT(m_may_return_to_freelist);

    ASSERT((paddr().get() & ~PAGE_MASK) == 0);

    if (m_supervisor)
        MM.deallocate_supervisor_physical_page(*this);
    else
        MM.deallocate_user_physical_page(*this);

#ifdef MM_DEBUG
    dbg() << "MM: P" << String::format("%x", m_paddr.get()) << " released to freelist";
#endif
}

void PhysicalPage::make_eternal()
{
    // Pages are automatically added to the inactive list upon allocation.
    // But the shared zero/lazy allocation page should not be in any list,
    // so we need to remove those from these lists.
    ASSERT(!m_is_eternal);
    auto& region = MM.find_user_physical_region_for_physical_page(*this);
    region.remove_page_from_list(*this);
    m_is_eternal = true; // set *after* removing it from the list!
}

void PhysicalPage::was_accessed(bool mark_dirty)
{
    ASSERT(!m_supervisor);
    if (m_is_eternal)
        return;
    auto& region = MM.find_user_physical_region_for_physical_page(*this);
    region.add_page_to_active_list(*this);
    m_dirty |= mark_dirty;
}

}
