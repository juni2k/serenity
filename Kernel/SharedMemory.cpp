#include <Kernel/SharedMemory.h>
#include <Kernel/VM/VMObject.h>
#include <Kernel/Lock.h>
#include <Kernel/Process.h>
#include <AK/HashMap.h>

Lockable<HashMap<String, RetainPtr<SharedMemory>>>& shared_memories()
{
    static Lockable<HashMap<String, RetainPtr<SharedMemory>>>* map;
    if (!map)
        map = new Lockable<HashMap<String, RetainPtr<SharedMemory>>>;
    return *map;
}

KResultOr<Retained<SharedMemory>> SharedMemory::open(const String& name, int flags, mode_t mode)
{
    LOCKER(shared_memories().lock());
    auto it = shared_memories().resource().find(name);
    if (it != shared_memories().resource().end()) {
        auto shared_memory = it->value;
        // FIXME: Improved access checking.
        if (shared_memory->uid() != current->process().uid())
            return KResult(-EACCES);
        return *shared_memory;
    }
    auto shared_memory = adopt(*new SharedMemory(name, current->process().uid(), current->process().gid(), mode));
    shared_memories().resource().set(name, shared_memory.ptr());
    return shared_memory;
}

KResult SharedMemory::unlink(const String& name)
{
    LOCKER(shared_memories().lock());
    auto it = shared_memories().resource().find(name);
    if (it == shared_memories().resource().end())
        return KResult(-ENOENT);
    shared_memories().resource().remove(it);
    return KSuccess;
}

SharedMemory::SharedMemory(const String& name, uid_t uid, gid_t gid, mode_t mode)
    : m_name(name)
    , m_uid(uid)
    , m_gid(gid)
    , m_mode(mode)
{
}

SharedMemory::~SharedMemory()
{
}

KResult SharedMemory::truncate(int length)
{
    if (!length) {
        m_vmo = nullptr;
        return KSuccess;
    }

    if (!m_vmo) {
        m_vmo = VMObject::create_anonymous(length);
        return KSuccess;
    }

    // FIXME: Support truncation.
    ASSERT_NOT_REACHED();
    return KResult(-ENOTIMPL);
}
