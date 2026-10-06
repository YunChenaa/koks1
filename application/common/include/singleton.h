/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-12 17:51:00
 * @LastEditors: 洪照
 * @LastEditTime: 2024-12-12 17:51:02
 * @FilePath: /sense-koks/application/common/include/singleton.h
 */
#pragma onece
#include <memory>

template <typename T>
class singleton
{
    private:
    /* data */
    public:
    static T& GetInstance()
    {
        static const std::unique_ptr<T> instance{ new T() };
        return *instance;
    }

    explicit singleton(T&&) = delete;
    explicit singleton(const T&) = delete;
    void operator=(const T&) = delete;

    protected:
    singleton(/* args */) = default;
};