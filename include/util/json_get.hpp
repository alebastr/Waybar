#pragma once

#include <json/json.h>

#include <cstdint>
#include <type_traits>
#include <utility>

namespace waybar::util {

/*
 * JSON deserialization helper that picks `from_json` implementations from the types' namespace.
 * The core concept here is a customization point - function that can be overloaded on user-defined
 * types in the user’s namespace and that is found by argument-dependent lookup
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4381.html
 *
 * Usage:
 * Define `void from_json(const Json::Value& j, YourType& v)` if you own the namespace and can add
 * functions to it. Define `namespace ::waybar::util { struct JsonDeserializer<Other::Type>... }` if
 * you don't have access to the `Other` namespace (i.e. std, Glib, etc...).
 *
 * YourType var = util::json_get(json_value);
 * YourType var; util::json_get_to(json_value, var);
 *
 * <magic>
 */
namespace detail {

/* Allow defining Class::from_json deserializers with access to private members */
template <typename JsonValueType, typename ValueType,
          typename = std::enable_if_t<
              std::is_member_function_pointer<decltype(&ValueType::from_json)>::value>>
void from_json(const JsonValueType& j, ValueType& v) {
  v.from_json(j);
}

struct from_json_impl {
  template <typename JsonValueType, typename ValueType>
  constexpr auto operator()(const JsonValueType& j, ValueType&& v) const
      -> decltype(from_json(j, std::forward<ValueType>(v))) {
    return from_json(j, std::forward<ValueType>(v));
  }
};
template <class T>
constexpr T static_const{};
}  // namespace detail
namespace {
constexpr auto const& from_json = detail::static_const<detail::from_json_impl>;
}

/*
 * An additional indirection via struct is needed for the cases when we can't simply put from_json
 * implementation into target type's namespace.
 * The default implementation will call the customization point defined above and let it use ADL to
 * find a deserializer method. Explicit namespace is needed to avoid possible recursion as the
 * struct method itself participates in the lookup.
 */
template <typename ValueType>
struct JsonDeserializer {
  template <typename JsonValueType, typename TargetType = ValueType>
  static auto from_json(JsonValueType&& val, TargetType& dst)
      -> decltype(::waybar::util::from_json(std::forward<JsonValueType>(val), dst), void()) {
    return ::waybar::util::from_json(std::forward<JsonValueType>(val), dst);
  }
};
/* </magic> */

template <typename ValueType>
ValueType& json_get_to(const Json::Value& val, ValueType& dst) {
  JsonDeserializer<ValueType>::from_json(std::forward<const Json::Value>(val), dst);
  return dst;
}

template <typename ValueType>
ValueType json_get(const Json::Value& val) {
  ValueType dst;
  json_get_to(std::forward<const Json::Value>(val), dst);
  return std::move(dst);
}

/*
 * Declare deserializers for common types
 */
#define DEFINE_JSONCPP_DESERIALIZER(ValueType, JsonCppType)     \
  template <>                                                   \
  struct JsonDeserializer<ValueType> {                          \
    static void from_json(const Json::Value& j, ValueType& v) { \
      if (j.is##JsonCppType()) {                                \
        v = j.as##JsonCppType();                                \
      }                                                         \
    }                                                           \
  }

DEFINE_JSONCPP_DESERIALIZER(bool, Bool);
DEFINE_JSONCPP_DESERIALIZER(int, Int);
DEFINE_JSONCPP_DESERIALIZER(int64_t, Int64);
DEFINE_JSONCPP_DESERIALIZER(unsigned int, UInt);
DEFINE_JSONCPP_DESERIALIZER(uint64_t, UInt64);
DEFINE_JSONCPP_DESERIALIZER(double, Double);
DEFINE_JSONCPP_DESERIALIZER(std::string, String);

#undef DEFINE_JSONCPP_DESERIALIZER

template <typename Key, typename Value>
struct JsonDeserializer<std::map<Key, Value>> {
  static void from_json(const Json::Value& src, std::map<Key, Value>& dst) {
    if (src.isObject()) {
      for (auto it = src.begin(); it != src.end(); it++) {
        ::waybar::util::json_get_to(*it, dst[it.key().asString()]);
      }
    }
  }
};

template <typename Value>
struct JsonDeserializer<std::vector<Value>> {
  static void from_json(const Json::Value& j, std::vector<Value>& v) {
    if (j.isArray()) {
      std::vector<Value> ret;
      ret.reserve(j.size());
      for (const auto& item : j) {
        ret.emplace_back(::waybar::util::json_get<Value>(item));
      }
      v = std::move(ret);
    }
  }
};

/* Useful for deserializing containers with Json::Value */
template <>
struct JsonDeserializer<Json::Value> {
  static void from_json(const Json::Value& src, Json::Value& dst) { dst = src; }
};

}  // namespace waybar::util
