// Copyright Tobias Hermann 2015.
// https://github.com/Dobiasd/FunctionalPlus
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "container_properties.h"
#include "filter.h"
#include "numeric.h"
#include "transform.h"
#include "composition.h"

namespace fplus
{

// generate(f, 3) == [f(), f(), f()]
template <typename ContainerOut, typename F>
ContainerOut generate(F f, std::size_t amount)
{
    static_assert(utils::function_traits<F>::arity == 0, "Wrong arity.");
    ContainerOut ys;
    prepare_container(ys, amount);
    auto it = get_back_inserter<ContainerOut>(ys);
    for (std::size_t i = 0; i < amount; ++i)
    {
        *it = f();
    }
    return ys;
}

// generate_by_idx(f, 3) == [f(0), f(1), f(2)]
template <typename ContainerOut, typename F>
ContainerOut generate_by_idx(F f, std::size_t amount)
{
    static_assert(utils::function_traits<F>::arity == 1, "Wrong arity.");
    typedef typename utils::function_traits<F>::template arg<0>::type FIn;
    static_assert(std::is_convertible<std::size_t, FIn>::value, "Function does not take std::size_t or compatible type.");
    ContainerOut ys;
    prepare_container(ys, amount);
    auto it = get_back_inserter<ContainerOut>(ys);
    for (std::size_t i = 0; i < amount; ++i)
    {
        *it = f(i);
    }
    return ys;
}

// repeat(3, [1, 2]) == [1, 2, 1, 2, 1, 2]
template <typename Container>
Container repeat(size_t n, const Container& xs)
{
    std::vector<Container> xss(n, xs);
    return concat(xss);
}

// replicate(3, [1]) == [1, 1, 1]
template <typename ContainerOut>
ContainerOut replicate(size_t n, const typename ContainerOut::value_type& x)
{
    return ContainerOut(n, x);
}

// infixes(3, [1,2,3,4,5,6]) == [[1,2,3], [2,3,4], [3,4,5], [4,5,6]]
template <typename ContainerIn,
    typename ContainerOut = std::vector<ContainerIn>>
ContainerOut infixes(std::size_t length, const ContainerIn& xs)
{
    assert(length > 0);
    static_assert(std::is_convertible<ContainerIn, typename ContainerOut::value_type>::value, "ContainerOut can not take values of type ContainerIn as elements.");
    ContainerOut result;
    if (size_of_cont(xs) < length)
        return result;
    prepare_container(result, size_of_cont(xs) - length);
    auto itOut = get_back_inserter(result);
    for (std::size_t idx = 0; idx <= size_of_cont(xs) - length; ++idx)
    {
        *itOut = get_range(idx, idx + length, xs);
    }
    return result;
}

namespace {

template <typename T>
std::vector<std::vector<T>>
    product_idxs_helper(
        const std::vector<T>& xs,
        const std::vector<std::vector<T>> & acc, std::size_t reps_left)
{
    if (reps_left == 1)
        return acc;
    typedef std::vector<std::vector<T>> result_t;
    result_t result;
    for (const std::vector<T>& a : acc)
    {
        auto ys = replicate<result_t>(size_of_cont(xs), a);
        for (std::size_t i = 0; i < ys.size(); ++i)
        {
            ys[i].push_back(xs[i]);
        }
        result = append(result, ys);
    }
    return product_idxs_helper(xs, result, reps_left - 1);
}

template <typename ContainerIn,
    typename T = typename ContainerIn::value_type,
    typename ContainerOut = std::vector<std::vector<T>>>
ContainerOut product_idxs(std::size_t power, const ContainerIn& xs)
{
    static_assert(std::is_same<T, std::size_t>::value, "T must be std::size_t");
    typedef std::vector<std::size_t> result_t;
    auto elem_to_vec = [](const T& x) { return result_t(1, x); };
    auto singletons = transform(elem_to_vec, xs);
    return product_idxs_helper(xs, singletons, power);
}

} // anonymous namespace

// product(2, "ABCD") == AA AB AC AD BA BB BC BD CA CB CC CD DA DB DC DD
template <typename ContainerIn,
    typename T = typename ContainerIn::value_type,
    typename ContainerOut = std::vector<ContainerIn>>
ContainerOut product(std::size_t power, const ContainerIn& xs_in)
{
    std::vector<T> xs = convert_container<std::vector<T>>(xs_in);
    auto idxs = all_idxs(xs);
    auto result_idxss = product_idxs(power, idxs);
    typedef typename ContainerOut::value_type ContainerOutInner;
    auto to_result_cont = [&](const std::vector<std::size_t>& idxs)
    {
        return convert_container_and_elems<ContainerOutInner>(
            elems_at_idxs(idxs, xs));
    };
    return transform(to_result_cont, result_idxss);
}

// permutations(2, "ABCD") == AB AC AD BA BC BD CA CB CD DA DB DC
template <typename ContainerIn,
    typename T = typename ContainerIn::value_type,
    typename ContainerOut = std::vector<ContainerIn>>
ContainerOut permutations(std::size_t power, const ContainerIn& xs_in)
{
    std::vector<T> xs = convert_container<std::vector<T>>(xs_in);
    auto idxs = all_idxs(xs);
    typedef std::vector<std::size_t> idx_vec;
    auto result_idxss = keep_if(all_unique<idx_vec>,
        product_idxs(power, idxs));
    typedef typename ContainerOut::value_type ContainerOutInner;
    auto to_result_cont = [&](const std::vector<std::size_t>& idxs)
    {
        return convert_container_and_elems<ContainerOutInner>(
            elems_at_idxs(idxs, xs));
    };
    return transform(to_result_cont, result_idxss);
}

// combinations(2, "ABCD") == AB AC AD BC BD CD
template <typename ContainerIn,
    typename T = typename ContainerIn::value_type,
    typename ContainerOut = std::vector<ContainerIn>>
ContainerOut combinations(std::size_t power, const ContainerIn& xs_in)
{
    std::vector<T> xs = convert_container<std::vector<T>>(xs_in);
    auto idxs = all_idxs(xs);
    typedef std::vector<std::size_t> idx_vec;
    auto result_idxss = keep_if(is_strictly_sorted<idx_vec>,
        product_idxs(power, idxs));
    typedef typename ContainerOut::value_type ContainerOutInner;
    auto to_result_cont = [&](const std::vector<std::size_t>& idxs)
    {
        return convert_container_and_elems<ContainerOutInner>(
            elems_at_idxs(idxs, xs));
    };
    return transform(to_result_cont, result_idxss);
}

// combinations_with_replacement(2, "ABCD") == AA AB AC AD BB BC BD CC CD DD
template <typename ContainerIn,
    typename T = typename ContainerIn::value_type,
    typename ContainerOut = std::vector<ContainerIn>>
ContainerOut combinations_with_replacement(std::size_t power, const ContainerIn& xs_in)
{
    std::vector<T> xs = convert_container<std::vector<T>>(xs_in);
    auto idxs = all_idxs(xs);
    typedef std::vector<std::size_t> idx_vec;
    auto result_idxss = keep_if(is_sorted<idx_vec>,
        product_idxs(power, idxs));
    typedef typename ContainerOut::value_type ContainerOutInner;
    auto to_result_cont = [&](const std::vector<std::size_t>& idxs)
    {
        return convert_container_and_elems<ContainerOutInner>(
            elems_at_idxs(idxs, xs));
    };
    return transform(to_result_cont, result_idxss);
}


//fill_left(0, 6, [1,2,3,4]) == [0,0,1,2,3,4]
template <typename Container,
        typename T = typename Container::value_type>
Container fill_left(const T& x, std::size_t min_size, const Container& xs)
{
    if (min_size <= size_of_cont(xs))
        return xs;
    return append(replicate<Container>(min_size - size_of_cont(xs), x), xs);
}

//fill_right(0, 6, [1,2,3,4]) == [1,2,3,4,0,0]
template <typename Container,
        typename T = typename Container::value_type>
Container fill_right(const T& x, std::size_t min_size, const Container& xs)
{
    if (min_size <= size_of_cont(xs))
        return xs;
    return append(xs, replicate<Container>(min_size - size_of_cont(xs), x));
}

} // namespace fplus