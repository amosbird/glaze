// Glaze Library
// For the license information refer to glaze.hpp

#pragma once

#include "glaze/core/opts.hpp"
#include "glaze/util/dump.hpp"
#include "glaze/binary/header.hpp"
#include "glaze/util/for_each.hpp"
#include "glaze/core/write.hpp"
#include "glaze/json/json_ptr.hpp"

namespace glaze
{
   namespace detail
   {      
      template <class T = void>
      struct to_binary {};
      
      template <>
      struct write<binary>
      {
         template <auto& Opts, class T, class B>
         static void op(T&& value, B&& b) {
            to_binary<std::decay_t<T>>::template op<Opts>(std::forward<T>(value), std::forward<B>(b));
         }
      };
      
      template <class T>
      requires (std::same_as<T, bool> || std::same_as<T, std::vector<bool>::reference> || std::same_as<T, std::vector<bool>::const_reference>)
      struct to_binary<T>
      {
         template <auto& Opts>
         static void op(const bool value, auto&& b) noexcept
         {
            if (value) {
               dump<static_cast<std::byte>(1)>(b);
            }
            else {
               dump<static_cast<std::byte>(0)>(b);
            }
         }
      };

      template <func_t T>
      struct to_binary<T>
      {
         template <auto& Opts>
         static void op(auto&& /*value*/, auto&& /*b*/) noexcept
         {}
      };
      
      void dump_type(auto&& value, auto&& b) noexcept
      {
         dump(std::as_bytes(std::span{ &value, 1 }), b);
      }
      
      void dump_int(size_t i, auto&& b)
      {
         if (i < 64) {
            dump_type(header8{ 0, static_cast<uint8_t>(i) }, b);
         }
         else if (i < 16384) {
            dump_type(header16{ 1, static_cast<uint16_t>(i) }, b);
         }
         else if (i < 1073741824) {
            dump_type(header32{ 2, static_cast<uint32_t>(i) }, b);
         }
         else if (i < 4611686018427387904) {
            dump_type(header64{ 3, i }, b);
         }
         else {
            throw std::runtime_error("size not supported");
         }
      }
      
      template <class T>
      requires num_t<T> || char_t<T>
      struct to_binary<T>
      {
         template <auto& Opts>
         static void op(auto&& value, auto&& b) noexcept
         {
            dump_type(value, b);
         }
      };
      
      template <str_t T>
      struct to_binary<T>
      {
         template <auto& Opts>
         static void op(auto&& value, auto&& b) noexcept
         {
            dump_int(value.size(), b);
            dump(std::as_bytes(std::span{ value.data(), value.size() }), b);
         }
      };
      
      template <array_t T>
      struct to_binary<T>
      {
         template <auto& Opts>
         static void op(auto&& value, auto&& b)
         {
            if constexpr (resizeable<T>) {
               dump_int(value.size(), b);
            }
            for (auto&& x : value) {
               write<binary>::op<Opts>(x, b);
            }
         }
      };
      
      template <map_t T>
      struct to_binary<T>
      {
         template <auto& Opts>
         static void op(auto&& value, auto&& b) noexcept
         {
            dump_int(value.size(), b);
            for (auto&& [k, v] : value) {
               write<binary>::op<Opts>(k, b);
               write<binary>::op<Opts>(v, b);
            }
         }
      };
      
      template <nullable_t T>
      struct to_binary<T>
      {
         template <auto& Opts>
         static void op(auto&& value, auto&& b) noexcept
         {
            if (value) {
               dump<static_cast<std::byte>(1)>(b);
               write<binary>::op<Opts>(*value, b);
            }
            else {
               dump<static_cast<std::byte>(0)>(b);
            }
         }
      };
      
      template <class T>
      requires glaze_object_t<T>
      struct to_binary<T>
      {
         template <auto& Opts>
         static void op(auto&& value, auto&& b) noexcept
         {
            using V = std::decay_t<T>;
            static constexpr auto N = std::tuple_size_v<meta_t<V>>;
            dump_int(N, b); // even though N is known at compile time in this case, it is not known for partial cases, so we still use a compressed integer
            
            for_each<N>([&](auto I) {
               static constexpr auto item = std::get<I>(meta_v<V>);
               dump_int(I, b); // dump the known key as an integer
               write<binary>::op<Opts>(value.*std::get<1>(item), b);
            });
         }
      };
   }
   
   template <class T, class Buffer>
   inline void write_binary(T&& value, Buffer&& buffer) {
      write<opts{.format = binary}>(std::forward<T>(value), std::forward<Buffer>(buffer));
   }
   
   template <auto& Partial, opts Opts, size_t Depth = 0, class T, class Buffer>
   requires nano::ranges::input_range<Buffer> && (sizeof(nano::ranges::range_value_t<Buffer>) == sizeof(char))
   inline void write(T&& value, Buffer& buffer) noexcept
   {
      if constexpr (std::same_as<Buffer, std::string> || std::same_as<Buffer, std::vector<std::byte>>) {
         using P = std::decay_t<decltype(Partial)>;
         static constexpr auto N = std::tuple_size_v<P>;
         static constexpr auto key_to_int = detail::make_key_int_map<T>();
         
         detail::dump_int(N, buffer); // write out the number of elements
         
         static constexpr auto sorted = (Depth == 0) ? sort_json_ptrs(Partial) : Partial;
         
         
      }
   }
   
   /*template <auto& Partial, opts Opts, class T, class Buffer>
   requires nano::ranges::input_range<Buffer> && (sizeof(nano::ranges::range_value_t<Buffer>) == sizeof(char))
   inline void write(T&& value, Buffer& buffer) noexcept
   {
      if constexpr (std::same_as<Buffer, std::string> || std::same_as<Buffer, std::vector<std::byte>>) {
         
         using P = std::decay_t<decltype(Partial)>;
         static constexpr auto N = std::tuple_size_v<P>;
         static constexpr auto key_to_int = detail::make_key_int_map<T>();
         
         detail::dump_int(N, buffer); // write out the number of elements
         
         static constexpr auto sorted = sort_json_ptrs(Partial);
         
         // for every path
         for_each<N>([&](auto i) {
            static constexpr auto path = std::get<i>(sorted);
            static constexpr auto keys = split_json_ptr<path>();
            
            static constexpr auto KeysRest = std::tuple_size_v<decltype(keys)> - 1;
            
            if constexpr (i == 0) {
               detail::dump_int(key_to_int.find(std::get<0>(keys))->second, buffer);
            }
            else {
               static constexpr auto prev_path = std::get<i - 1>(sorted);
               static constexpr auto prev_keys = split_json_ptr<prev_path>();
               static constexpr auto key = std::get<0>(keys);
               
               if constexpr (key != std::get<0>(prev_keys)) {
                  detail::dump_int(key_to_int.find(key)->second, buffer);
               }
            }
            
            static constexpr auto frozen_map = detail::make_map<std::decay_t<T>>();
            
            static constexpr auto member_it = frozen_map.find(std::get<0>(keys));
            if constexpr (member_it != frozen_map.end()) {
               static constexpr auto member_ptr = std::get<member_it->second.index()>(member_it->second);
               
               if constexpr (KeysRest == 0) {
                  write<opts{.format = binary}>(value.*member_ptr, buffer);
               }
               else {
                  static constexpr auto Index = i; // lambda cannot capture i
                  
                  // recursing into each path
                  for_each_value<KeysRest>([&](auto I, auto&& value) {
                     static constexpr auto key = std::get<I + 1>(keys);
                     
                     using V = std::decay_t<decltype(value)>;
                     static constexpr auto frozen_map = detail::make_map<V>();
                     
                     static constexpr auto k_i_map = detail::make_key_int_map<V>();
                     
                     if constexpr (Index == 0) {
                        detail::dump_int(k_i_map.find(key)->second, buffer);
                     }
                     else {
                        static constexpr auto prev_path = std::get<Index - 1>(sorted);
                        static constexpr auto prev_keys = split_json_ptr<prev_path>();
                        static constexpr auto prev_depth = std::tuple_size_v<decltype(prev_keys)>;
                        
                        // dump key if not already written
                        if constexpr ((I + 1) < prev_depth) {
                           static constexpr auto prev_key = std::get<I + 1>(prev_keys);
                           if constexpr (key != prev_key) {
                              detail::dump_int(k_i_map.find(key).second, buffer);
                           }
                        }
                        else {
                           // dump key if deeper than previous
                           detail::dump_int(k_i_map.find(key)->second, buffer);
                        }
                     }
                     
                     if constexpr (I == (KeysRest - 1)) {
                        write<opts{.format = binary}>(value, buffer);
                     }
                     
                     return frozen_map.find(key)->second;
                  }, value.*member_ptr);
               }
            }
         });
      }
      else {
      }
   }*/
   
   template <auto& Partial, class T, class Buffer>
   inline void write_binary(T&& value, Buffer&& buffer) {
      write<Partial, opts{}>(std::forward<T>(value), std::forward<Buffer>(buffer));
   }
}