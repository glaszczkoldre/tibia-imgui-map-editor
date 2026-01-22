#pragma once

#include <glad/glad.h>
#include <utility>

namespace MapEditor {
namespace Rendering {

/**
 * Traits for VAO (Vertex Array Object) resource management.
 */
struct VAOTraits {
    static void Generate(GLuint* id) { glGenVertexArrays(1, id); }
    static void Delete(GLuint* id) { 
        if (*id != 0) {
            glDeleteVertexArrays(1, id); 
            *id = 0; 
        }
    }
};

/**
 * Traits for Buffer objects (VBO/EBO/UBO) resource management.
 */
struct BufferTraits {
    static void Generate(GLuint* id) { glGenBuffers(1, id); }
    static void Delete(GLuint* id) { 
        if (*id != 0) {
            glDeleteBuffers(1, id); 
            *id = 0; 
        }
    }
};

/**
 * Traits for Framebuffer object resource management.
 */
struct FBOTraits {
    static void Generate(GLuint* id) { glGenFramebuffers(1, id); }
    static void Delete(GLuint* id) { 
        if (*id != 0) {
            glDeleteFramebuffers(1, id); 
            *id = 0; 
        }
    }
};

/**
 * Traits for Texture resource management.
 */
struct TextureTraits {
    static void Generate(GLuint* id) { glGenTextures(1, id); }
    static void Delete(GLuint* id) { 
        if (*id != 0) {
            glDeleteTextures(1, id); 
            *id = 0; 
        }
    }
};

/**
 * RAII wrapper template for OpenGL resources.
 * 
 * Provides automatic resource cleanup via RAII pattern.
 * Supports move semantics but not copy semantics.
 * 
 * Usage:
 *   VAOHandle vao;  // Auto-generates VAO
 *   glBindVertexArray(vao.get());
 *   // ... use VAO ...
 *   // Automatically deleted when vao goes out of scope
 */
template<typename Traits>
class GLHandle {
public:
    /**
     * Default constructor - generates a new resource.
     */
    GLHandle() { 
        Traits::Generate(&id_); 
    }
    
    /**
     * Construct from existing resource ID (takes ownership).
     */
    explicit GLHandle(GLuint existing_id) : id_(existing_id) {}
    
    /**
     * Destructor - releases the resource.
     */
    ~GLHandle() { 
        Traits::Delete(&id_);
    }
    
    /**
     * Move constructor - transfers ownership.
     */
    GLHandle(GLHandle&& other) noexcept : id_(other.id_) { 
        other.id_ = 0; 
    }
    
    /**
     * Move assignment - releases current and transfers ownership.
     */
    GLHandle& operator=(GLHandle&& other) noexcept {
        if (this != &other) {
            Traits::Delete(&id_);
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }
    
    // No copy semantics
    GLHandle(const GLHandle&) = delete;
    GLHandle& operator=(const GLHandle&) = delete;
    
    /**
     * Get the raw OpenGL handle.
     */
    GLuint get() const { return id_; }
    
    /**
     * Get pointer to the handle (for OpenGL functions that need GLuint*).
     */
    GLuint* ptr() { return &id_; }
    
    /**
     * Release ownership and reset to invalid state.
     */
    void reset() {
        Traits::Delete(&id_);
    }
    
    /**
     * Release ownership without deleting (caller takes ownership).
     */
    GLuint release() {
        GLuint tmp = id_;
        id_ = 0;
        return tmp;
    }
    
    /**
     * Check if handle is valid.
     */
    explicit operator bool() const { return id_ != 0; }
    
    /**
     * Check if handle is valid.
     */
    bool isValid() const { return id_ != 0; }
    
private:
    GLuint id_ = 0;
};

// Type aliases for common OpenGL resource types
using VAOHandle = GLHandle<VAOTraits>;
using VBOHandle = GLHandle<BufferTraits>;
using EBOHandle = GLHandle<BufferTraits>;
using UBOHandle = GLHandle<BufferTraits>;
using SSBOHandle = GLHandle<BufferTraits>;
using FBOHandle = GLHandle<FBOTraits>;
using TextureHandle = GLHandle<TextureTraits>;

/**
 * Deferred initialization variant - does NOT auto-generate.
 * Use when you need to delay resource creation.
 */
template<typename Traits>
class DeferredGLHandle {
public:
    DeferredGLHandle() = default;
    
    ~DeferredGLHandle() { 
        Traits::Delete(&id_);
    }
    
    DeferredGLHandle(DeferredGLHandle&& other) noexcept : id_(other.id_) { 
        other.id_ = 0; 
    }
    
    DeferredGLHandle& operator=(DeferredGLHandle&& other) noexcept {
        if (this != &other) {
            Traits::Delete(&id_);
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }
    
    DeferredGLHandle(const DeferredGLHandle&) = delete;
    DeferredGLHandle& operator=(const DeferredGLHandle&) = delete;
    
    void create() {
        if (id_ == 0) {
            Traits::Generate(&id_);
        }
    }
    
    GLuint get() const { return id_; }
    GLuint* ptr() { return &id_; }
    void reset() { Traits::Delete(&id_); }
    explicit operator bool() const { return id_ != 0; }
    bool isValid() const { return id_ != 0; }
    
private:
    GLuint id_ = 0;
};

using DeferredVAOHandle = DeferredGLHandle<VAOTraits>;
using DeferredVBOHandle = DeferredGLHandle<BufferTraits>;

} // namespace Rendering
} // namespace MapEditor
