#ifndef _THALLIUM_GSL_SPAN
#define _THALLIUM_GSL_SPAN

#include <array>
#include <limits>
#include <type_traits>
#include <utility>

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

namespace gsl {

template <class ElementType>
class span {
  public:
    // constants and types
    using element_type = ElementType;
    using value_type = typename std::remove_cv<ElementType>::type;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = element_type*;
    using const_pointer = const element_type*;
    using reference = element_type&;
    using const_reference = const element_type&;
    using iterator = pointer;  // I am Lazy!
    using reverse_iterator = std::reverse_iterator<iterator>;

    // constructors, copy, and assignment
    constexpr span() noexcept;
    template <class It>
    constexpr span(It first, size_type count) : data_(first), size_(count) {}
    template <class It, class End>
    span(It first, End last) : data_(&(*first)) {
        if (first >= last) throw std::length_error("span");
        size_ = last - first;
    }
    template <size_t N>
    constexpr span(element_type (&arr)[N]) noexcept : data_(arr), size_(N) {}
    template <class T, size_t N>
    constexpr span(std::array<T, N>& arr) noexcept
        : data_(arr.data()), size_(N) {}
    template <class T, size_t N>
    constexpr span(const std::array<T, N>& arr) noexcept
        : data_(arr.data()), size_(N) {}
    constexpr span(span&& other) noexcept = default;
    constexpr span(const span& other) noexcept = default;

    ~span() noexcept = default;

    span& operator=(const span& other) noexcept = default;

    span<element_type> first(size_type count) const {
        if (count > size_) throw std::length_error("span");
        return span<element_type>(data_, count);
    }
    span<element_type> last(size_type count) const {
        if (count > size_) throw std::length_error("span");
        return span<element_type>(data_ + size_ - count, count);
    }
    span<element_type> subspan(size_type offset, size_type count) const {
        if (offset + count > size_) throw std::length_error("span");
        return span<element_type>(data_ + offset, count);
    }

    // observers
    constexpr size_type size() const noexcept { return size_; }
    constexpr size_type size_bytes() const noexcept {
        return size_ * sizeof(element_type);
    }
    constexpr bool empty() const noexcept { return size_ == 0; }

    // element access
    constexpr reference operator[](size_type idx) const { return data_[idx]; }
    constexpr reference front() const { return data_[0]; }
    constexpr reference back() const { return data_[size_ - 1]; }
    constexpr pointer data() const noexcept { return data_; }

    // iterator support
    constexpr iterator begin() const noexcept { return data_; }
    constexpr iterator end() const noexcept { return data_ + size_; }
    constexpr reverse_iterator rbegin() const noexcept {
        return data_ + size_ - 1;
    }
    constexpr reverse_iterator rend() const noexcept { return data_ - 1; }

  private:
    pointer data_;    // exposition only
    size_type size_;  // exposition only
};

template <class ElementType, size_t Extent>
class static_span : public span<ElementType> {
  public:
    using element_type = ElementType;
    using size_type = size_t;
    constexpr static size_t extent = Extent;
    constexpr static_span(ElementType (&arr)[extent]) noexcept
        : span<ElementType>(arr) {}
    constexpr static_span(std::array<ElementType, extent>& arr) noexcept
        : span<ElementType>(arr) {}
    constexpr static_span(const std::array<ElementType, extent>& arr) noexcept
        : span<ElementType>(arr) {}
    constexpr static_span(static_span&& other) noexcept = default;
    constexpr static_span(const static_span& other) noexcept = default;

    constexpr size_type size() const noexcept { return Extent; }
    constexpr size_type size_bytes() const noexcept {
        return Extent * sizeof(element_type);
    }
    constexpr bool empty() const noexcept { return Extent == 0; }
};

template <class T>
struct deref_type {
    using type =
        typename std::remove_reference<decltype(*std::declval<T>())>::type;
};

template <class T>
using deref_type_t = typename deref_type<T>::type;

template <class It>
constexpr span<deref_type_t<It>> make_span(It first, const size_t count) {
    return span<deref_type_t<It>>(first, count);
}

template <class It, class End>
constexpr span<deref_type_t<It>> make_span(It first, End last) {
    return span<deref_type_t<It>>(first, last);
}

template <class T, size_t N>
constexpr span<T> make_span(T (&arr)[N]) {
    return span<T>(arr);
}

// This is for vector and string, linear storage container
template <class Linear,
          class _T = typename Linear::value_type,  // test has value_type here
          class T = typename std::conditional<std::is_const<Linear>::value,
                                              const _T, _T>::type>
span<T> make_span(Linear& s) {
    T * start_ptr = nullptr;
    if (s.size() != 0) start_ptr = &s[0];
    return span<T>(start_ptr, s.size());
}

template <class T, size_t N>
constexpr static_span<T, N> make_static_span(T (&arr)[N]) {
    return static_span<T, N>(arr);
}

template <class T, size_t N>
constexpr static_span<T, N> make_static_span(std::array<T, N>& arr) {
    return static_span<T, N>(arr);
}

template <class T, size_t N>
constexpr static_span<const T, N> make_static_span(
    const std::array<T, N>& arr) {
    return static_span<const T, N>(arr);
}

}  // namespace gsl

_THALLIUM_END_NAMESPACE

#endif
