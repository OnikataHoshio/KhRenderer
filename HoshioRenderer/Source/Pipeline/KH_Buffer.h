#pragma once

#include "KH_Common.h"

template <typename T, GLenum Target = GL_SHADER_STORAGE_BUFFER>
class KH_Buffer {
public:
    KH_Buffer(unsigned int bindPoint = 0) : BindPoint(bindPoint), ID(0), Size(0) {}

    KH_Buffer(KH_Buffer&& other) noexcept : ID(other.ID), BindPoint(other.BindPoint), Size(other.Size) {
        other.ID = 0;
        other.Size = 0;
    }

    ~KH_Buffer() {
        if (ID != 0) glDeleteBuffers(1, &ID);
    }

    KH_Buffer(const KH_Buffer&) = delete;
    KH_Buffer& operator=(const KH_Buffer&) = delete;

    void SetData(const std::vector<T>& data, GLenum usage = GL_STATIC_DRAW) {
        UpdateBuffer(data.data(), data.size(), usage);
    }

    void SetData(const T* data, size_t count, GLenum usage = GL_STATIC_DRAW) {
        UpdateBuffer(data, count, usage);
    }

    void SetSingleData(const T& data, GLenum usage = GL_DYNAMIC_DRAW) {
        UpdateBuffer(&data, 1, usage);
    }

    void GetData(std::vector<T>& outData) const {
        outData.resize(Size);
        glBindBuffer(Target, ID);
        glGetBufferSubData(Target, 0, Size * sizeof(T), outData.data());
        glBindBuffer(Target, 0);
    }

    void Clear() const {
        if (ID == 0 || Size == 0) return;

        glBindBuffer(Target, ID);

        GLenum format = (sizeof(T) % 4 == 0) ? GL_R32UI : GL_R8UI;

        glClearBufferData(Target, format, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
        glBindBuffer(Target, 0);
    }

    void SetBindPoint(unsigned int BindPoint) {
        this->BindPoint = BindPoint;
    }

    void Bind(unsigned int BindPoint) {
        SetBindPoint(BindPoint);
        glBindBufferBase(Target, BindPoint, ID);
    }

    void Bind() const {
        glBindBufferBase(Target, BindPoint, ID);
    }

    void Unbind() const {
        glBindBufferBase(Target, BindPoint, 0);
    }

    unsigned int GetID() const { return ID; }
    size_t GetCount() const { return Size; }

private:
    unsigned int ID;
    unsigned int BindPoint;
    size_t Size;

    void UpdateBuffer(const T* data, size_t count, GLenum usage) {
        if (ID == 0) {
            glGenBuffers(1, &ID);
        }

        glBindBuffer(Target, ID);

        if (count != Size) {
            glBufferData(Target, count * sizeof(T), data, usage);
            Size = count;
        }
        else {
            glBufferSubData(Target, 0, count * sizeof(T), data);
        }

        glBindBuffer(Target, 0);
    }
};

template <typename T>
using KH_SSBO = KH_Buffer<T, GL_SHADER_STORAGE_BUFFER>;

template <typename T>
using KH_UBO = KH_Buffer<T, GL_UNIFORM_BUFFER>;
