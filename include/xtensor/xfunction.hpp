/***************************************************************************
* Copyright (c) 2016, Johan Mabille and Sylvain Corlay                     *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XFUNCTION_HPP
#define XFUNCTION_HPP

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <utility>

#include "xexpression.hpp"
#include "xiterator.hpp"
#include "xlayout.hpp"
#include "xutils.hpp"

namespace xt
{

    namespace detail
    {

        /********************
         * common_size_type *
         ********************/

        template <class... Args>
        struct common_size_type
        {
            using type = std::common_type_t<typename Args::size_type...>;
        };

        template <>
        struct common_size_type<>
        {
            using type = std::size_t;
        };

        template <class... Args>
        using common_size_type_t = typename common_size_type<Args...>::type;

        /**************************
         * common_difference type *
         **************************/

        template <class... Args>
        struct common_difference_type
        {
            using type = std::common_type_t<typename Args::difference_type...>;
        };

        template <>
        struct common_difference_type<>
        {
            using type = std::size_t;
        };

        template <class... Args>
        using common_difference_type_t = typename common_difference_type<Args...>::type;

        /*********************
         * common_value_type *
         *********************/

        template <class... Args>
        struct common_value_type
        {
            using type = std::common_type_t<xvalue_type_t<Args>...>;
        };

        template <class... Args>
        using common_value_type_t = typename common_value_type<Args...>::type;
    }

    template <class F, class R, class... CT>
    class xfunction_iterator;

    template <class F, class R, class... CT>
    class xfunction_stepper;

    /*************
     * xfunction *
     *************/

    /**
     * @class xfunction
     * @brief Multidimensional function operating on xexpression.
     *
     * Th xfunction class implements a multidimensional function
     * operating on xexpression.
     *
     * @tparam F the function type
     * @tparam R the return type of the function
     * @tparam CT the closure types for arguments of the function
     */
    template <class F, class R, class... CT>
    class xfunction : public xexpression<xfunction<F, R, CT...>>
    {

    public:

        using self_type = xfunction<F, R, CT...>;
        using functor_type = typename std::remove_reference<F>::type;

        using value_type = R;
        using reference = value_type;
        using const_reference = value_type;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using size_type = detail::common_size_type_t<std::decay_t<CT>...>;
        using difference_type = detail::common_difference_type_t<std::decay_t<CT>...>;

        using shape_type = promote_shape_t<typename std::decay_t<CT>::shape_type...>;

        using const_iterator = xfunction_iterator<F, R, CT...>;
        using iterator = const_iterator;

        using const_stepper = xfunction_stepper<F, R, CT...>;
        using stepper = const_stepper;

        using const_broadcast_iterator = xiterator<const_stepper, shape_type*>;
        using broadcast_iterator = const_broadcast_iterator;

        static constexpr xt::layout layout_type = compute_layout(std::decay_t<CT>::layout_type...);
        static constexpr bool contiguous_layout = and_c<std::decay_t<CT>::contiguous_layout...>::value;

        template <class Func>
        xfunction(Func&& f, CT... e) noexcept;

        size_type size() const noexcept;
        size_type dimension() const noexcept;
        const shape_type& shape() const;
        xt::layout layout() const noexcept;

        template <class... Args>
        const_reference operator()(Args... args) const;

        const_reference operator[](const xindex& index) const;
        const_reference operator[](size_type i) const;

        template <class It>
        const_reference element(It first, It last) const;

        template <class S>
        bool broadcast_shape(S& shape) const;

        template <class S>
        bool is_trivial_broadcast(const S& strides) const noexcept;

        const_iterator begin() const noexcept;
        const_iterator end() const noexcept;
        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

        const_broadcast_iterator xbegin() const noexcept;
        const_broadcast_iterator xend() const noexcept;
        const_broadcast_iterator cxbegin() const noexcept;
        const_broadcast_iterator cxend() const noexcept;

        template <class S>
        xiterator<const_stepper, S> xbegin(const S& shape) const noexcept;
        template <class S>
        xiterator<const_stepper, S> xend(const S& shape) const noexcept;
        template <class S>
        xiterator<const_stepper, S> cxbegin(const S& shape) const noexcept;
        template <class S>
        xiterator<const_stepper, S> cxend(const S& shape) const noexcept;

        template <class S>
        const_stepper stepper_begin(const S& shape) const noexcept;
        template <class S>
        const_stepper stepper_end(const S& shape) const noexcept;

    private:

        template <std::size_t... I>
        xt::layout layout_impl(std::index_sequence<I...>) const noexcept;

        template <std::size_t... I, class... Args>
        const_reference access_impl(std::index_sequence<I...>, Args... args) const;

        template <std::size_t... I, class It>
        const_reference element_access_impl(std::index_sequence<I...>, It first, It last) const;

        template <class Func, std::size_t... I>
        const_stepper build_stepper(Func&& f, std::index_sequence<I...>) const noexcept;

        template <class Func, std::size_t... I>
        const_iterator build_iterator(Func&& f, std::index_sequence<I...>) const noexcept;

        size_type compute_dimension() const noexcept;

        std::tuple<CT...> m_e;
        functor_type m_f;
        mutable shape_type m_shape;
        mutable bool m_shape_computed;

        friend class xfunction_iterator<F, R, CT...>;
        friend class xfunction_stepper<F, R, CT...>;
    };

    /**********************
     * xfunction_iterator *
     **********************/

    template <class F, class R, class... CT>
    class xfunction_iterator
    {

    public:

        using self_type = xfunction_iterator<F, R, CT...>;
        using functor_type = typename std::remove_reference<F>::type;
        using xfunction_type = xfunction<F, R, CT...>;

        using value_type = typename xfunction_type::value_type;
        using reference = typename xfunction_type::value_type;
        using pointer = typename xfunction_type::const_pointer;
        using difference_type = typename xfunction_type::difference_type;
        using iterator_category = std::forward_iterator_tag;

        template <class... It>
        xfunction_iterator(const xfunction_type* func, It&&... it) noexcept;

        self_type& operator++();
        self_type operator++(int);

        reference operator*() const;

        bool equal(const self_type& rhs) const;

    private:

        template <std::size_t... I>
        reference deref_impl(std::index_sequence<I...>) const;

        const xfunction_type* p_f;
        std::tuple<typename std::decay_t<CT>::const_iterator...> m_it;
    };

    template <class F, class R, class... CT>
    bool operator==(const xfunction_iterator<F, R, CT...>& it1,
                    const xfunction_iterator<F, R, CT...>& it2);

    template <class F, class R, class... CT>
    bool operator!=(const xfunction_iterator<F, R, CT...>& it1,
                    const xfunction_iterator<F, R, CT...>& it2);

    /*********************
     * xfunction_stepper *
     *********************/

    template <class F, class R, class... CT>
    class xfunction_stepper
    {

    public:

        using self_type = xfunction_stepper<F, R, CT...>;
        using functor_type = typename std::remove_reference<F>::type;
        using xfunction_type = xfunction<F, R, CT...>;

        using value_type = typename xfunction_type::value_type;
        using reference = typename xfunction_type::value_type;
        using pointer = typename xfunction_type::const_pointer;
        using size_type = typename xfunction_type::size_type;
        using difference_type = typename xfunction_type::difference_type;

        using shape_type = typename xfunction_type::shape_type;

        template <class... It>
        xfunction_stepper(const xfunction_type* func, It&&... it) noexcept;

        void step(size_type dim, size_type n = 1);
        void step_back(size_type dim, size_type n = 1);
        void reset(size_type dim);

        void to_end();

        reference operator*() const;

        bool equal(const self_type& rhs) const;

    private:

        template <std::size_t... I>
        reference deref_impl(std::index_sequence<I...>) const;

        const xfunction_type* p_f;
        std::tuple<typename std::decay_t<CT>::const_stepper...> m_it;
    };

    template <class F, class R, class... CT>
    bool operator==(const xfunction_stepper<F, R, CT...>& it1,
                    const xfunction_stepper<F, R, CT...>& it2);

    template <class F, class R, class... CT>
    bool operator!=(const xfunction_stepper<F, R, CT...>& it1,
                    const xfunction_stepper<F, R, CT...>& it2);

    /****************************
     * xfunction implementation *
     ****************************/

    /**
     * @name Constructor
     */
    //@{
    /**
     * Constructs an xfunction applying the specified function to the given
     * arguments.
     * @param f the function to apply
     * @param e the \ref xexpression arguments
     */
    template <class F, class R, class... CT>
    template <class Func>
    inline xfunction<F, R, CT...>::xfunction(Func&& f, CT... e) noexcept
        : m_e(e...), m_f(std::forward<Func>(f)), m_shape(make_sequence<shape_type>(0, size_type(1))),
          m_shape_computed(false)
    {
    }
    //@}

    /**
     * @name Size and shape
     */
    //@{
    /**
     * Returns the size of the expression.
     */
    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::size() const noexcept -> size_type
    {
        return compute_size(shape());
    }

    /**
     * Returns the number of dimensions of the function.
     */
    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::dimension() const noexcept -> size_type
    {
        size_type dimension = m_shape_computed ? m_shape.size() : compute_dimension();
        return dimension;
    }

    /**
     * Returns the shape of the xfunction.
     */
    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::shape() const -> const shape_type&
    {
        if (!m_shape_computed)
        {
            m_shape = make_sequence<shape_type>(compute_dimension(), size_type(1));
            broadcast_shape(m_shape);
            m_shape_computed = true;
        }
        return m_shape;
    }

    /**
     * Returns the layout of the xfunction.
     */
    template <class F, class R, class... CT>
    inline xt::layout xfunction<F, R, CT...>::layout() const noexcept
    {
        return layout_impl(std::make_index_sequence<sizeof...(CT)>());
    }
    //@}

    /**
     * @name Data
     */
    /**
     * Returns a constant reference to the element at the specified position in the function.
     * @param args a list of indices specifying the position in the function. Indices
     * must be unsigned integers, the number of indices should be equal or greater than
     * the number of dimensions of the function.
     */
    template <class F, class R, class... CT>
    template <class... Args>
    inline auto xfunction<F, R, CT...>::operator()(Args... args) const -> const_reference
    {
        return access_impl(std::make_index_sequence<sizeof...(CT)>(), args...);
    }

    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::operator[](const xindex& index) const -> const_reference
    {
        return element(index.cbegin(), index.cend());
    }

    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::operator[](size_type i) const -> const_reference
    {
        return operator()(i);
    }

    /**
     * Returns a constant reference to the element at the specified position in the function.
     * @param first iterator starting the sequence of indices
     * @param last iterator ending the sequence of indices
     * The number of indices in the squence should be equal to or greater
     * than the number of dimensions of the container.
     */
    template <class F, class R, class... CT>
    template <class It>
    inline auto xfunction<F, R, CT...>::element(It first, It last) const -> const_reference
    {
        return element_access_impl(std::make_index_sequence<sizeof...(CT)>(), first, last);
    }
    //@}

    /**
     * @name Broadcasting
     */
    //@{
    /**
     * Broadcast the shape of the function to the specified parameter.
     * @param shape the result shape
     * @return a boolean indicating whether the broadcasting is trivial
     */
    template <class F, class R, class... CT>
    template <class S>
    inline bool xfunction<F, R, CT...>::broadcast_shape(S& shape) const
    {
        // e.broadcast_shape must be evaluated even if b is false
        auto func = [&shape](bool b, auto&& e) { return e.broadcast_shape(shape) && b; };
        return accumulate(func, true, m_e);
    }

    /**
     * Compares the specified strides with those of the container to see whether
     * the broadcasting is trivial.
     * @return a boolean indicating whether the broadcasting is trivial
     */
    template <class F, class R, class... CT>
    template <class S>
    inline bool xfunction<F, R, CT...>::is_trivial_broadcast(const S& strides) const noexcept
    {
        auto func = [&strides](bool b, auto&& e) { return b && e.is_trivial_broadcast(strides); };
        return accumulate(func, true, m_e);
    }
    //@}

    /**
     * @name Iterators
     */
    /**
     * Returns an iterator to the first element of the buffer
     * containing the elements of the function.
     */
    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::begin() const noexcept -> const_iterator
    {
        auto f = [](const auto& e) noexcept { return e.cbegin(); };
        return build_iterator(f, std::make_index_sequence<sizeof...(CT)>());
    }

    /**
     * Returns a constant iterator to the element following the last
     * element of the buffer containing the elements of the function.
     */
    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::end() const noexcept -> const_iterator
    {
        auto f = [](const auto& e) noexcept { return e.cend(); };
        return build_iterator(f, std::make_index_sequence<sizeof...(CT)>());
    }

    /**
     * Returns a constant iterator to the first element of the buffer
     * containing the elements of the function.
     */
    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::cbegin() const noexcept -> const_iterator
    {
        return begin();
    }

    /**
     * Returns a constant iterator to the element following the last
     * element of the buffer containing the elements of the function.
     */
    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::cend() const noexcept -> const_iterator
    {
        return end();
    }
    //@}

    /**
     * @name Broadcast iterators
     */
    //@{
    /**
     * Returns a constant iterator to the first element of the function.
     */
    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::xbegin() const noexcept -> const_broadcast_iterator
    {
        return const_broadcast_iterator(stepper_begin(shape()), &shape());
    }

    /**
     * Returns a constant iterator to the element following the last element
     * of the function.
     */
    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::xend() const noexcept -> const_broadcast_iterator
    {
        return const_broadcast_iterator(stepper_end(shape()), &shape());
    }

    /**
     * Returns a constant iterator to the first element of the function.
     */
    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::cxbegin() const noexcept -> const_broadcast_iterator
    {
        return xbegin();
    }

    /**
     * Returns a constant iterator to the element following the last element
     * of the function.
     */
    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::cxend() const noexcept -> const_broadcast_iterator
    {
        return xend();
    }

    /**
     * Returns a constant iterator to the first element of the function. The
     * iteration is broadcasted to the specified shape.
     * @param shape the shape used for broadcasting
     */
    template <class F, class R, class... CT>
    template <class S>
    inline auto xfunction<F, R, CT...>::xbegin(const S& shape) const noexcept -> xiterator<const_stepper, S>
    {
        return xiterator<const_stepper, S>(stepper_begin(shape), shape);
    }

    /**
     * Returns a constant iterator to the element following the last element of the
     * function. The iteration is broadcasted to the specified shape.
     * @param shape the shape used for broadcasting
     */
    template <class F, class R, class... CT>
    template <class S>
    inline auto xfunction<F, R, CT...>::xend(const S& shape) const noexcept -> xiterator<const_stepper, S>
    {
        return xiterator<const_stepper, S>(stepper_end(shape), shape);
    }

    /**
     * Returns a constant iterator to the first element of the function. The
     * iteration is broadcasted to the specified shape.
     * @param shape the shape used for broadcasting
     */
    template <class F, class R, class... CT>
    template <class S>
    inline auto xfunction<F, R, CT...>::cxbegin(const S& shape) const noexcept -> xiterator<const_stepper, S>
    {
        return xbegin(shape);
    }

    /**
     * Returns a constant iterator to the element following the last element of the
     * function. The iteration is broadcasted to the specified shape.
     * @param shape the shape used for broadcasting
     */
    template <class F, class R, class... CT>
    template <class S>
    inline auto xfunction<F, R, CT...>::cxend(const S& shape) const noexcept -> xiterator<const_stepper, S>
    {
        return xend(shape);
    }
    //@}

    template <class F, class R, class... CT>
    template <class S>
    inline auto xfunction<F, R, CT...>::stepper_begin(const S& shape) const noexcept -> const_stepper
    {
        auto f = [&shape](const auto& e) noexcept { return e.stepper_begin(shape); };
        return build_stepper(f, std::make_index_sequence<sizeof...(CT)>());
    }

    template <class F, class R, class... CT>
    template <class S>
    inline auto xfunction<F, R, CT...>::stepper_end(const S& shape) const noexcept -> const_stepper
    {
        auto f = [&shape](const auto& e) noexcept { return e.stepper_end(shape); };
        return build_stepper(f, std::make_index_sequence<sizeof...(CT)>());
    }

    template <class F, class R, class... CT>
    template <std::size_t... I>
    inline xt::layout xfunction<F, R, CT...>::layout_impl(std::index_sequence<I...>) const noexcept
    {
        return compute_layout(std::get<I>(m_e).layout()...);
    }

    template <class F, class R, class... CT>
    template <std::size_t... I, class... Args>
    inline auto xfunction<F, R, CT...>::access_impl(std::index_sequence<I...>, Args... args) const -> const_reference
    {
        return m_f(detail::get_element(std::get<I>(m_e), args...)...);
    }

    template <class F, class R, class... CT>
    template <std::size_t... I, class It>
    inline auto xfunction<F, R, CT...>::element_access_impl(std::index_sequence<I...>, It first, It last) const -> const_reference
    {
        return m_f((std::get<I>(m_e).element(first, last))...);
    }

    template <class F, class R, class... CT>
    template <class Func, std::size_t... I>
    inline auto xfunction<F, R, CT...>::build_stepper(Func&& f, std::index_sequence<I...>) const noexcept -> const_stepper
    {
        return const_stepper(this, f(std::get<I>(m_e))...);
    }

    template <class F, class R, class... CT>
    template <class Func, std::size_t... I>
    inline auto xfunction<F, R, CT...>::build_iterator(Func&& f, std::index_sequence<I...>) const noexcept -> const_iterator
    {
        return const_iterator(this, f(std::get<I>(m_e))...);
    }

    template <class F, class R, class... CT>
    inline auto xfunction<F, R, CT...>::compute_dimension() const noexcept -> size_type
    {
        auto func = [](size_type d, auto&& e) noexcept { return std::max(d, e.dimension()); };
        return accumulate(func, size_type(0), m_e);
    }

    /*************************************
     * xfunction_iterator implementation *
     *************************************/

    template <class F, class R, class... CT>
    template <class... It>
    inline xfunction_iterator<F, R, CT...>::xfunction_iterator(const xfunction_type* func, It&&... it) noexcept
        : p_f(func), m_it(std::forward<It>(it)...)
    {
    }

    template <class F, class R, class... CT>
    inline auto xfunction_iterator<F, R, CT...>::operator++() -> self_type&
    {
        auto f = [](auto& it) { ++it; };
        for_each(f, m_it);
        return *this;
    }

    template <class F, class R, class... CT>
    inline auto xfunction_iterator<F, R, CT...>::operator++(int) -> self_type
    {
        self_type tmp(*this);
        auto f = [](auto& it) { ++it; };
        for_each(f, m_it);
        return tmp;
    }

    template <class F, class R, class... CT>
    inline auto xfunction_iterator<F, R, CT...>::operator*() const -> reference
    {
        return deref_impl(std::make_index_sequence<sizeof...(CT)>());
    }

    template <class F, class R, class... CT>
    inline bool xfunction_iterator<F, R, CT...>::equal(const self_type& rhs) const
    {
        return p_f == rhs.p_f && m_it == rhs.m_it;
    }

    template <class F, class R, class... CT>
    template <std::size_t... I>
    inline auto xfunction_iterator<F, R, CT...>::deref_impl(std::index_sequence<I...>) const -> reference
    {
        return (p_f->m_f)(*std::get<I>(m_it)...);
    }

    template <class F, class R, class... CT>
    inline bool operator==(const xfunction_iterator<F, R, CT...>& it1,
                           const xfunction_iterator<F, R, CT...>& it2)
    {
        return it1.equal(it2);
    }

    template <class F, class R, class... CT>
    inline bool operator!=(const xfunction_iterator<F, R, CT...>& it1,
                           const xfunction_iterator<F, R, CT...>& it2)
    {
        return !(it1.equal(it2));
    }

    /************************************
     * xfunction_stepper implementation *
     ************************************/

    template <class F, class R, class... CT>
    template <class... It>
    inline xfunction_stepper<F, R, CT...>::xfunction_stepper(const xfunction_type* func, It&&... it) noexcept
        : p_f(func), m_it(std::forward<It>(it)...)
    {
    }

    template <class F, class R, class... CT>
    inline void xfunction_stepper<F, R, CT...>::step(size_type dim, size_type n)
    {
        auto f = [dim, n](auto& it) { it.step(dim, n); };
        for_each(f, m_it);
    }

    template <class F, class R, class... CT>
    inline void xfunction_stepper<F, R, CT...>::step_back(size_type dim, size_type n)
    {
        auto f = [dim, n](auto& it) { it.step_back(dim, n); };
        for_each(f, m_it);
    }

    template <class F, class R, class... CT>
    inline void xfunction_stepper<F, R, CT...>::reset(size_type dim)
    {
        auto f = [dim](auto& it) { it.reset(dim); };
        for_each(f, m_it);
    }

    template <class F, class R, class... CT>
    inline void xfunction_stepper<F, R, CT...>::to_end()
    {
        auto f = [](auto& it) { it.to_end(); };
        for_each(f, m_it);
    }

    template <class F, class R, class... CT>
    inline auto xfunction_stepper<F, R, CT...>::operator*() const -> reference
    {
        return deref_impl(std::make_index_sequence<sizeof...(CT)>());
    }

    template <class F, class R, class... CT>
    inline bool xfunction_stepper<F, R, CT...>::equal(const self_type& rhs) const
    {
        return p_f == rhs.p_f && m_it == rhs.m_it;
    }

    template <class F, class R, class... CT>
    template <std::size_t... I>
    inline auto xfunction_stepper<F, R, CT...>::deref_impl(std::index_sequence<I...>) const -> reference
    {
        return (p_f->m_f)(*std::get<I>(m_it)...);
    }

    template <class F, class R, class... CT>
    inline bool operator==(const xfunction_stepper<F, R, CT...>& it1,
                           const xfunction_stepper<F, R, CT...>& it2)
    {
        return it1.equal(it2);
    }

    template <class F, class R, class... CT>
    inline bool operator!=(const xfunction_stepper<F, R, CT...>& it1,
                           const xfunction_stepper<F, R, CT...>& it2)
    {
        return !(it1.equal(it2));
    }
}

#endif
