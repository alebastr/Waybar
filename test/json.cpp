#define CATCH_CONFIG_MAIN
#include "util/json.hpp"
#include "util/json_get.hpp"

#include <glibmm/ustring.h>

#include <catch2/catch.hpp>
#include <optional>

using waybar::util::json_get;
using waybar::util::json_get_to;

enum class Color { WHITE, RED, GREEN, BLUE, BLACK };

namespace ns {
/*
 * Check deserializer function lookup in a namespace
 */
struct NestedObj {
  std::string name;
  uint64_t    value;

  bool operator==(const NestedObj& other) const {
    return value == other.value && name == other.name;
  }
  bool operator!=(const NestedObj& other) const { return !(*this == other); }
};
void from_json(const Json::Value& j, NestedObj& o) {
  if (j.isObject()) {
    if (j.isMember("name")) {
      json_get_to(j["name"], o.name);
    }
    if (j.isMember("value")) {
      json_get_to(j["value"], o.value);
    }
  }
}

/*
 * Check deserializer class member lookup.
 * This should be able to deserialize private class members.
 * Note the default-constructible requirement.
 */
struct NestedObjWithPrivateData {
  NestedObjWithPrivateData() = default;
  NestedObjWithPrivateData(std::string n, unsigned v) : name{std::move(n)}, value{v} {};

  bool operator==(const NestedObjWithPrivateData& other) const {
    return value == other.value && name == other.name;
  }

  void from_json(const Json::Value& j) {
    if (j.isObject()) {
      if (j.isMember("name")) {
        json_get_to(j["name"], name);
      }
      if (j.isMember("value")) {
        json_get_to(j["value"], value);
      }
    }
  }

 private:
  std::string name;
  unsigned    value = 0;
};
}  // namespace ns

struct TestData {
  bool                         boolean;
  int                          integer;
  double                       number;
  std::vector<int>             array;
  std::map<std::string, Color> colors;
  std::vector<ns::NestedObj>   objects;
};

namespace waybar::util {

template <typename T>
struct JsonDeserializer<std::optional<T>> {
  static void from_json(const Json::Value& j, std::optional<T>& o) {
    o = std::nullopt;
    if (!j.isNull()) {
      o = json_get<T>(j);
    }
  }
};
}  // namespace waybar::util

void from_json(const Json::Value& j, TestData& t) {
  if (j.isObject()) {
    if (j.isMember("boolean")) {
      json_get_to(j["boolean"], t.boolean);
    }
    if (j.isMember("integer")) {
      json_get_to(j["integer"], t.integer);
    }
    if (j.isMember("number")) {
      json_get_to(j["number"], t.number);
    }
    if (j.isMember("colors")) {
      json_get_to(j["colors"], t.colors);
    }
    if (j.isMember("array")) {
      json_get_to(j["array"], t.array);
    }
    if (j.isMember("objects")) {
      json_get_to(j["objects"], t.objects);
    }
  }
}

void from_json(const Json::Value& j, Color& c) {
  Glib::ustring str = json_get<std::string>(j);
  str = str.lowercase();
  if (str == "white") {
    c = Color::WHITE;
  } else if (str == "red") {
    c = Color::RED;
  } else if (str == "green") {
    c = Color::GREEN;
  } else if (str == "blue") {
    c = Color::BLUE;
  } else if (str == "black") {
    c = Color::BLACK;
  }
}

TEST_CASE("Deserialize object with from_json in the namespace", "[json]") {
  using namespace waybar::util;
  JsonParser p;

  auto j = p.parse(R"(
    {
      "name": "test",
      "value": 42
    }
  )");
  auto v = json_get<ns::NestedObj>(j);
  REQUIRE(v == ns::NestedObj{"test", 42});
}

TEST_CASE("Deserialize object with from_json member", "[json]") {
  using namespace waybar::util;
  JsonParser p;

  auto j = p.parse(R"(
    {
      "name": "test",
      "value": 42
    }
  )");
  auto v = json_get<ns::NestedObjWithPrivateData>(j);
  REQUIRE(v == ns::NestedObjWithPrivateData{"test", 42});
}

TEST_CASE("Deserialize STL containers", "[json]") {
  using namespace waybar::util;
  JsonParser p;

  auto m = json_get<std::map<std::string, Color>>(p.parse(R"(
    {
      "first": "red",
      "second": "green",
      "third": "blue"
    }
  )"));
  REQUIRE(m.size() == 3);
  REQUIRE(m["first"] == Color::RED);
  REQUIRE(m["second"] == Color::GREEN);
  REQUIRE(m["third"] == Color::BLUE);

  auto v = json_get<std::vector<ns::NestedObj>>(p.parse(R"(
    [
      { "name": "one", "value": 1 },
      { "name": "two", "value": 2},
      { "name": "three",  "value": 3 }
    ]
  )"));
  REQUIRE_THAT(
      v, Catch::Matchers::Equals(std::vector<ns::NestedObj>{{"one", 1}, {"two", 2}, {"three", 3}}));

  struct TestData {
    std::optional<ns::NestedObj> opt;

    void from_json(const Json::Value& j) { json_get_to(j["opt"], opt); }
  };

  auto o = json_get<TestData>(p.parse("{}"));
  REQUIRE(o.opt == std::nullopt);

  o = json_get<TestData>(p.parse(R"(
    {
      "opt": null
    }
  )"));
  REQUIRE(o.opt == std::nullopt);

  o = json_get<TestData>(p.parse(R"(
    {
      "opt": {
        "name": "one",
        "value": 1
      }
    }
  )"));
  REQUIRE(o.opt == ns::NestedObj{"one", 1});
}

TEST_CASE("Deserialize complex user-defined object", "[json]") {
  using namespace waybar::util;
  JsonParser  p;
  Json::Value val = p.parse(R"(
  {
    "boolean": true,
    "integer": 42,
    "number": 42.05,
    "array": [1, 2, 3, 4, 5],
    "colors": {
      "first": "red",
      "second": "green",
      "third": "blue"
    },
    "objects": [
      { "name": "one", "value": 1 },
      { "name": "two", "value": 2},
      { "name": "three",  "value": 3 }
    ]
   }
  )");
  auto        obj = json_get<TestData>(val);

  REQUIRE(obj.boolean);
  REQUIRE(obj.integer == 42);
  REQUIRE(obj.number == Approx(42.05));
  REQUIRE_THAT(obj.array, Catch::Matchers::Equals(std::vector<int>{1, 2, 3, 4, 5}));
  REQUIRE(obj.colors.size() == 3);
  REQUIRE(obj.colors["first"] == Color::RED);
  REQUIRE(obj.colors["second"] == Color::GREEN);
  REQUIRE(obj.colors["third"] == Color::BLUE);
  REQUIRE_THAT(
      obj.objects,
      Catch::Matchers::Equals(std::vector<ns::NestedObj>{{"one", 1}, {"two", 2}, {"three", 3}}));
}
