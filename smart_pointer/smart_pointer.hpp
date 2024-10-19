#pragma once
#include <memory>

struct BaseControlBlock {
  size_t shared_count;
  size_t weak_count;
  BaseControlBlock(size_t shared_count, size_t weak_count)
      : shared_count(shared_count), weak_count(weak_count) {}
  virtual void destroy_object() {}
  virtual void destroy_and_deallocate_block(BaseControlBlock* block) {
    BaseControlBlock* ptr = block;
  }
  virtual ~BaseControlBlock() = default;
};

template <typename T>
class SharedPtr {
  template <typename Y, typename Alloc, typename... Args>
  friend SharedPtr<Y> AllocateShared(Alloc alloc, Args&&... args);

  template <typename Y, typename... Args>
  friend SharedPtr<Y> MakeShared(Args&&... args);

  template <typename U>
  friend class SharedPtr;

  template <typename U>
  friend class WeakPtr;

 public:
  SharedPtr() : block_(nullptr), object_(nullptr) {}

  template <typename Y, typename Deleter = std::default_delete<Y>,
      typename Allocator = std::allocator<Y>>
  SharedPtr(Y* ptr, Deleter del = std::default_delete<Y>(),
            Allocator alloc = std::allocator<Y>()) {
    using alloc_traits = std::allocator_traits<Allocator>;
    using control_block = ControlBlockRegular<Y, Deleter, Allocator>;
    using alloc_control_block =
        typename alloc_traits::template rebind_alloc<control_block>;
    using control_block_traits = std::allocator_traits<alloc_control_block>;
    alloc_control_block alloc_control_bl = alloc;
    control_block* conblockptr =
        control_block_traits::allocate(alloc_control_bl, 1);
    control_block_traits::construct(alloc_control_bl, conblockptr, 1, 0, ptr,
                                    del, alloc);
    block_ = conblockptr;
    object_ = ptr;
  }

  SharedPtr(std::nullptr_t) : block_(nullptr), object_(nullptr) {}

  template <typename U>
  SharedPtr(const SharedPtr<U>& shared)
      : block_(shared.block_), object_(shared.object_) {
    ++block_->shared_count;
  }

  SharedPtr(const SharedPtr& shared)
      : block_(shared.block_), object_(shared.object_) {
    ++block_->shared_count;
  }

  SharedPtr(SharedPtr&& shared)
      : block_(shared.block_), object_(shared.object_) {
    shared.block_ = nullptr;
    shared.object_ = nullptr;
  }

  template <typename U>
  SharedPtr& operator=(const SharedPtr<U>& other) {
    SharedPtr<T> copy = other;
    std::swap(block_, copy.block_);
    std::swap(object_, copy.object_);
    return *this;
  }

  SharedPtr& operator=(const SharedPtr& other) {
    SharedPtr<T> copy = other;
    std::swap(block_, copy.block_);
    std::swap(object_, copy.object_);
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) {
    SharedPtr copy = std::move(*this);
    block_ = other.block_;
    object_ = other.object_;
    other.block_ = nullptr;
    other.object_ = nullptr;
    return *this;
  }

  ~SharedPtr() {
    if (block_ == nullptr && object_ == nullptr) {
      return;
    }
    --block_->shared_count;
    if (block_->shared_count != 0) {
      return;
    }
    if (block_->weak_count != 0) {
      block_->destroy_object();
      return;
    }
    block_->destroy_object();
    block_->destroy_and_deallocate_block(block_);
  }

  size_t use_count() const noexcept {
    if (block_ != nullptr) {
      return block_->shared_count;
    }
    return 0;
  }

  T* get() const noexcept { return object_; }

  T& operator*() const noexcept { return *get(); }

  T* operator->() const noexcept { return get(); }

  void reset() noexcept {
    SharedPtr shared;
    shared.block_ = block_;
    shared.object_ = object_;
    block_ = nullptr;
    object_ = nullptr;
  }

 private:
  template <typename U, typename Deleter = std::default_delete<U>,
      typename Allocator = std::allocator<U>>
  struct ControlBlockRegular : public BaseControlBlock {
    using alloc_traits = std::allocator_traits<Allocator>;
    using control_block = ControlBlockRegular<U, Deleter, Allocator>;
    using alloc_control_block =
        typename alloc_traits::template rebind_alloc<control_block>;
    using control_block_traits = std::allocator_traits<alloc_control_block>;

    U* ptr;
    Deleter del;
    Allocator alloc;

    ControlBlockRegular(size_t shared_count, size_t weak_count, U* ptr,
                        Deleter del, Allocator alloc)
        : BaseControlBlock(shared_count, weak_count),
          ptr(ptr),
          del(del),
          alloc(alloc) {}

    void destroy_object() override { del(ptr); }

    void destroy_and_deallocate_block(BaseControlBlock* block) override {
      alloc_control_block alloc_control_bl = alloc;
      DestroyDeallocateBlock<control_block*, alloc_control_block>()(
          static_cast<control_block*>(block), alloc_control_bl);
    }

    ~ControlBlockRegular() override {}
  };

  template <typename U, typename Allocator = std::allocator<U>>
  struct ControlBlockMakeShared : BaseControlBlock {
    using alloc_traits = std::allocator_traits<Allocator>;
    using control_block = ControlBlockMakeShared<U, Allocator>;
    using alloc_control_block =
        typename alloc_traits::template rebind_alloc<control_block>;
    using control_block_traits = std::allocator_traits<alloc_control_block>;

    alignas(U) char ptr_in_char[sizeof(U)];
    U* ptr;
    Allocator alloc;

    template <typename... Args>
    ControlBlockMakeShared(Allocator alloc, Args&&... args)
        : BaseControlBlock(1, 0), ptr(nullptr), alloc(alloc) {
      ptr = reinterpret_cast<U*>(ptr_in_char);
      ::new (ptr) U(std::forward<Args>(args)...);
    }

    void destroy_object() override { ptr->~U(); }

    void destroy_and_deallocate_block(BaseControlBlock* block) override {
      alloc_control_block alloc_control_bl = alloc;
      DestroyDeallocateBlock<control_block*, alloc_control_block>()(
          static_cast<control_block*>(block), alloc_control_bl);
    }

    ~ControlBlockMakeShared() override {}
  };

  template <typename ControlBlockToDelete, typename AllocControlBlock>
  struct DestroyDeallocateBlock {
    void operator()(ControlBlockToDelete block, AllocControlBlock alloc) {
      using control_block_traits = std::allocator_traits<AllocControlBlock>;
      control_block_traits::destroy(alloc, block);
      control_block_traits::deallocate(alloc, block, 1);
    }
  };

  template <typename U, typename Allocator>
  SharedPtr(ControlBlockMakeShared<U, Allocator>* cblock)
      : block_(cblock), object_(cblock->ptr) {}

  SharedPtr(BaseControlBlock* cblock, T* object)
      : block_(cblock), object_(object) {
    ++block_->shared_count;
  }

  BaseControlBlock* block_;
  T* object_;
};

template <typename T>
class WeakPtr {
 public:
  WeakPtr() : block_(nullptr), object_(nullptr) {}

  WeakPtr(const WeakPtr& other) : block_(other.block_), object_(other.object_) {
    ++block_->weak_count;
  }

  WeakPtr(const SharedPtr<T>& other)
      : block_(other.block_), object_(other.object_) {
    ++block_->weak_count;
  }

  WeakPtr(WeakPtr&& other) : block_(other.block_), object_(other.object_) {
    other.block_ = nullptr;
    other.object_ = nullptr;
  }

  WeakPtr& operator=(const WeakPtr& other) {
    WeakPtr copy = other;
    std::swap(object_, copy.object_);
    std::swap(block_, copy.block_);
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) {
    WeakPtr copy = std::move(*this);
    std::swap(object_, other.object_);
    std::swap(block_, other.block_);
    return *this;
  }

  ~WeakPtr() {
    if (block_ == nullptr && object_ == nullptr) {
      return;
    }
    --block_->weak_count;
    if (block_->shared_count != 0 || block_->weak_count != 0) {
      return;
    }
    block_->destroy_and_deallocate_block(block_);
  }

  bool expired() { return (block_->shared_count == 0); }

  SharedPtr<T> lock() { return SharedPtr<T>(block_, object_); }

 private:
  BaseControlBlock* block_;
  T* object_;
};

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> AllocateShared(Alloc alloc, Args&&... args) {
  using alloc_traits = std::allocator_traits<Alloc>;
  using control_block =
      typename SharedPtr<T>::template ControlBlockMakeShared<T, Alloc>;
  using alloc_control_block =
      typename alloc_traits::template rebind_alloc<control_block>;
  using control_block_traits = std::allocator_traits<alloc_control_block>;

  alloc_control_block alloc_control_bl = alloc;
  control_block* block = control_block_traits::allocate(alloc_control_bl, 1);

  control_block_traits::construct(alloc_control_bl, block, alloc,
                                  std::forward<Args>(args)...);
  return SharedPtr<T>(block);
}

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
  return AllocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}