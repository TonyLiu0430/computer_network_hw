#pragma once

#include <nlohmann/json.hpp>
#include <boost/pfr.hpp>
#include <iostream>
#include <source_location>
using json = nlohmann::json;

template<typename T, typename FIELD_TYPE, FIELD_TYPE T::*FIELD>
void foo() {
    std::cout << std::source_location::current().function_name() << std::endl;
}


template<typename T>
void serialize(const T && obj) {
    foo<T, boost::pfr::get<0>(obj)>(obj);
    // for(int i = 0; i < boost::pfr::tuple_size<my_struct>::value; i++ ) {
    //     //j[boost::pfr::get<0>(obj)] = boost::pfr::get<1>(obj);
    //     foo<T, boost::pfr::get<0>(obj)>(obj);
    // }
    return j;
}