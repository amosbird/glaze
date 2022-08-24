// Glaze Library
// For the license information refer to glaze.hpp

#pragma once

#include "glaze/api/hash.hpp"
#include "glaze/util/string_view.hpp"

#include <array>

#define GLAZE_SPECIALIZE(type, major, minor, revision) \
template <> \
struct glaze::name_t<type> { \
   static constexpr std::string_view value = #type; \
}; \
template <> \
struct glaze::version_t<type> { \
   static constexpr version_type value = { major, minor, revision }; \
};

namespace glaze
{
   using version_type = std::array<uint32_t, 3>;
   
   template <class T>
   struct version_t {
      static constexpr version_type value = {0,0,0};
   };
   
   template <class T>
   inline constexpr version_type version = version_t<T>::value;

   template <class T>
   struct trait
   {
      using sv = std::string_view;
      static constexpr sv type_name_unhashed = name<T>;
      static constexpr sv type_name_hash = hash128_v<type_name_unhashed>; // must hash for consistent length
      
      static constexpr sv type_size_hash = hash128_v<int_to_sv_v<size_t, sizeof(T)>>; // must hash for consistent length
      
      static constexpr sv major_version = hash128_i_v<version<T>[0]>; // must hash for consistent length
      static constexpr sv minor_version = hash128_i_v<version<T>[1]>; // must hash for consistent length
      static constexpr sv revision = hash128_i_v<version<T>[2]>; // must hash for consistent length
      
#define std_trait(x) static constexpr sv x = to_sv<std::x##_v<T>>()
      std_trait(is_trivial);
      std_trait(is_standard_layout);
      
      std_trait(is_default_constructible);
      std_trait(is_trivially_default_constructible);
      std_trait(is_nothrow_default_constructible);
      
      std_trait(is_trivially_copyable);
      
      std_trait(is_move_constructible);
      std_trait(is_trivially_move_constructible);
      std_trait(is_nothrow_move_constructible);
      
      std_trait(is_destructible);
      std_trait(is_trivially_destructible);
      std_trait(is_nothrow_destructible);
      
      std_trait(has_unique_object_representations);
      
      std_trait(is_polymorphic);
      std_trait(has_virtual_destructor);
      std_trait(is_aggregate);
#undef std_trait
      
      //static constexpr sv cplusplus = empty_if<std::is_standard_layout_v<T>>(to_sv<__cplusplus>());
      
#ifdef __clang__
      static constexpr sv clang = "clang";
#endif
#ifdef __GNUC__
      static constexpr sv gnuc = "gnuc";
#endif
#ifdef _MSC_VER
      static constexpr sv msvc = "msvc";
#endif
      
      static constexpr sv blank = ""; // to end possible macros
      
      static constexpr sv to_hash = detail::join_v<
      type_name_hash,
      
      type_size_hash,
      
      major_version,
      minor_version,
      revision,
      
      is_trivial,
      is_standard_layout,
      
      is_default_constructible,
      is_trivially_default_constructible,
      is_nothrow_default_constructible,
      
      is_trivially_copyable,
      
      is_move_constructible,
      is_trivially_move_constructible,
      is_nothrow_move_constructible,
      
      is_destructible,
      is_trivially_destructible,
      is_nothrow_destructible,
      
      has_unique_object_representations,
      
      is_polymorphic,
      has_virtual_destructor,
      is_aggregate,
      
#ifdef __clang__
      clang,
#endif
#ifdef __GNUC__
      gnuc,
#endif
#ifdef _MSC_VER
      msvc,
#endif
      blank
      >;
      
   private:
      static constexpr sv v = "v";
      static constexpr sv comma = ",";
   public:
      static constexpr sv version_sv = detail::join_v<v, major_version, comma, minor_version, comma, revision>;
      static constexpr version_type version = ::glaze::version<T>;
      
      static constexpr sv hash = hash128_v<to_hash>;
   };
   
   template <class T>
   consteval auto hash() {
      return trait<T>::hash;
   }
}