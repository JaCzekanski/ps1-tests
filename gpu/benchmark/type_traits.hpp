#pragma once

template<typename T, typename U>
struct is_same {
    static const bool value = false; 
};

template<typename T>
struct is_same<T,T> { 
   static const bool value = true; 
};

#define IS_SAME(x,y) is_same<x, y>::value