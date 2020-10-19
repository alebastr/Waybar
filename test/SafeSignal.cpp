#include <thread>

#include <catch2/catch.hpp>

#include "GlibTestsFixture.hpp"
#include "util/SafeSignal.hpp"

using namespace waybar;
/**
 * Basic sanity test for SafeSignal:
 * check that type deduction works, events are delivered and the order is right
 * Running this with -fsanitize=thread should not fail
 */
TEST_CASE_METHOD(GlibTestsFixture, "SafeSignal basic functionality", "[signal][thread][util]") {
  const int NUM_EVENTS = 100;
  int       count = 0;
  int       last_value = 0;

  SafeSignal<int, std::string> test_signal;

  const auto  main_tid = std::this_thread::get_id();
  std::thread producer;

  // timeout the test in 500ms
  Glib::signal_timeout().connect_once([]() { throw std::runtime_error("Test timed out"); }, 500);

  test_signal.connect([&](auto val, auto str) {
    static_assert(std::is_same<int, decltype(val)>::value);
    static_assert(std::is_same<std::string, decltype(str)>::value);
    // check that we're in the same thread as the main loop
    REQUIRE(std::this_thread::get_id() == main_tid);
    // check event order
    REQUIRE(val == last_value + 1);

    last_value = val;
    if (++count >= NUM_EVENTS) {
      this->quit();
    };
  });

  run([&]() {
    // check that events from the same thread are delivered
    test_signal.emit(1, "test");
    // can't assert here, dispatcher will process event on the next loop iteration

    // start another thread and generate events
    producer = std::thread([&]() {
      for (auto i = 2; i <= NUM_EVENTS; ++i) {
        test_signal.emit(i, "test");
      }
    });
  });
  producer.join();
  REQUIRE(count == NUM_EVENTS);
}
