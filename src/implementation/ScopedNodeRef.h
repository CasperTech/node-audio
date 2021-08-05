#pragma once

#include <napi.h>

namespace CasperTech
{
    template<class T>
    class ScopedNodeRef
    {
        public:
            explicit ScopedNodeRef(Napi::Reference<T>* ref);

            ~ScopedNodeRef();

        private:
            Napi::Reference<T>* _ref;
    };

    template<class T>
    ScopedNodeRef<T>::ScopedNodeRef(Napi::Reference<T>* ref)
            : _ref(ref)
    {
        _ref->Ref();
    }

    template<class T>
    ScopedNodeRef<T>::~ScopedNodeRef()
    {
        _ref->Unref();
    }
}