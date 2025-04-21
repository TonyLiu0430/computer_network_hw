#pragma once
#include <concepts>


namespace detail {

// template<typename T, template <typename> class U, typename ...Args> requires std::same_as<T, U<Args...>>
// consteval typename std::tuple_element<0, std::tuple<Args...>>::type get_inner_type(U<Args...>);


// template<typename T, template <typename> class U, typename ...Args> requires std::same_as<T, U<Args...>>
// struct inner {
// 	using type = std::tuple_element<0, std::tuple<Args...>>::type get_inner_type(U<Args...>);
// };




// template<typename T, template <typename> class U>
// using inner_t = typename inner<T, U>::type;

} // namespace detail