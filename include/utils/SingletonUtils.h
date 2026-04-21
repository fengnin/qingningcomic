#ifndef UTILS_SINGLETONUTILS_H
#define UTILS_SINGLETONUTILS_H

#include <QMutex>
#include <QMutexLocker>
#include <functional>

namespace SingletonUtils {

template<typename T>
struct SingletonDeleter
{
    static void destroy(T*& instance)
    {
        delete instance;
        instance = nullptr;
    }
};

template<typename T>
T* getOrCreate(T*& instance, QMutex& mutex, std::function<T*()> factory)
{
    if (instance) {
        return instance;
    }
    
    QMutexLocker locker(&mutex);
    if (!instance) {
        instance = factory();
    }
    return instance;
}

template<typename T>
void cleanup(T*& instance, QMutex& mutex)
{
    QMutexLocker locker(&mutex);
    if (instance) {
        SingletonDeleter<T>::destroy(instance);
    }
}

template<typename T>
T* getFromContainer(std::function<T*()> containerGetter)
{
    return containerGetter();
}

}

#define DECLARE_SINGLETON_MEMBERS(ClassName) \
    public: \
        static ClassName* instance(); \
        static void cleanup(); \
    private: \
        template<typename> friend struct SingletonUtils::SingletonDeleter; \
        static ClassName* m_instance; \
        static QMutex m_instanceMutex; \
        ClassName(const ClassName&) = delete; \
        ClassName& operator=(const ClassName&) = delete;

#define DEFINE_SINGLETON_STATIC_MEMBERS(ClassName) \
    ClassName* ClassName::m_instance = nullptr; \
    QMutex ClassName::m_instanceMutex;

#define IMPLEMENT_SINGLETON_SIMPLE(ClassName) \
    DECLARE_SINGLETON_MEMBERS(ClassName) \
    DEFINE_SINGLETON_STATIC_MEMBERS(ClassName) \
    inline ClassName* ClassName::instance() \
    { \
        return SingletonUtils::getOrCreate<ClassName>( \
            m_instance, m_instanceMutex, []() { return new ClassName(); }); \
    } \
    inline void ClassName::cleanup() \
    { \
        SingletonUtils::cleanup<ClassName>(m_instance, m_instanceMutex); \
    }

#define IMPLEMENT_SINGLETON_WITH_ARG(ClassName, ...) \
    DECLARE_SINGLETON_MEMBERS(ClassName) \
    DEFINE_SINGLETON_STATIC_MEMBERS(ClassName) \
    inline ClassName* ClassName::instance() \
    { \
        return SingletonUtils::getOrCreate<ClassName>( \
            m_instance, m_instanceMutex, []() { return new ClassName(__VA_ARGS__); }); \
    } \
    inline void ClassName::cleanup() \
    { \
        SingletonUtils::cleanup<ClassName>(m_instance, m_instanceMutex); \
    }

#define DEFINE_SINGLETON_INSTANCE_SIMPLE(ClassName) \
    DEFINE_SINGLETON_STATIC_MEMBERS(ClassName) \
    ClassName* ClassName::instance() \
    { \
        return SingletonUtils::getOrCreate<ClassName>( \
            m_instance, m_instanceMutex, []() { return new ClassName(); }); \
    } \
    void ClassName::cleanup() \
    { \
        SingletonUtils::cleanup<ClassName>(m_instance, m_instanceMutex); \
    }

#define DEFINE_SINGLETON_INSTANCE_WITH_ARG(ClassName, ...) \
    DEFINE_SINGLETON_STATIC_MEMBERS(ClassName) \
    ClassName* ClassName::instance() \
    { \
        return SingletonUtils::getOrCreate<ClassName>( \
            m_instance, m_instanceMutex, []() { return new ClassName(__VA_ARGS__); }); \
    } \
    void ClassName::cleanup() \
    { \
        SingletonUtils::cleanup<ClassName>(m_instance, m_instanceMutex); \
    }

#define DEFINE_SINGLETON_INSTANCE(ClassName, containerGetter) \
    ClassName* ClassName::instance() \
    { \
        ClassName* fromContainer = SingletonUtils::getFromContainer<ClassName>( \
            []() { return ServiceContainer::instance()->containerGetter(); }); \
        if (fromContainer) { \
            return fromContainer; \
        } \
        return SingletonUtils::getOrCreate<ClassName>( \
            m_instance, m_instanceMutex, []() { return new ClassName(); }); \
    } \
    void ClassName::cleanup() \
    { \
        SingletonUtils::cleanup<ClassName>(m_instance, m_instanceMutex); \
    }

#define DEFINE_SINGLETON_INSTANCE_SERVICE(ClassName, containerGetter, ...) \
    ClassName* ClassName::instance() \
    { \
        ClassName* fromContainer = SingletonUtils::getFromContainer<ClassName>( \
            []() { return ServiceContainer::instance()->containerGetter(); }); \
        if (fromContainer) { \
            return fromContainer; \
        } \
        return SingletonUtils::getOrCreate<ClassName>( \
            m_instance, m_instanceMutex, []() { return new ClassName(__VA_ARGS__); }); \
    } \
    void ClassName::cleanup() \
    { \
        SingletonUtils::cleanup<ClassName>(m_instance, m_instanceMutex); \
    }

// ============================================================================
// 兼容旧版宏 - 保持向后兼容
// ============================================================================

// 旧版简单单例声明宏（不禁止拷贝）
#define DECLARE_SINGLETON(Class) \
public: \
    static Class* instance(); \
    static void destroyInstance(); \
private: \
    static Class* m_instance; \
    static QMutex m_instanceMutex;

// 旧版简单单例实现宏
#define IMPLEMENT_SINGLETON(Class) \
Class* Class::m_instance = nullptr; \
QMutex Class::m_instanceMutex; \
Class* Class::instance() { \
    return SingletonUtils::getOrCreate<Class>( \
        m_instance, m_instanceMutex, []() { return new Class(); }); \
} \
void Class::destroyInstance() { \
    SingletonUtils::cleanup<Class>(m_instance, m_instanceMutex); \
}

// 旧版无锁单例声明宏
#define DECLARE_SINGLETON_NO_MUTEX(Class) \
public: \
    static Class* instance(); \
    static void destroyInstance(); \
private: \
    static Class* m_instance;

// 旧版无锁单例实现宏
#define IMPLEMENT_SINGLETON_NO_MUTEX(Class) \
Class* Class::m_instance = nullptr; \
Class* Class::instance() { \
    if (!m_instance) { \
        m_instance = new Class(); \
    } \
    return m_instance; \
} \
void Class::destroyInstance() { \
    if (m_instance) { \
        delete m_instance; \
        m_instance = nullptr; \
    } \
}

#endif // UTILS_SINGLETONUTILS_H
