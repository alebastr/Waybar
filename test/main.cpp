#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include <catch2/catch_reporter_tap.hpp>
#include <glibmm.h>

int main(int argc, char* argv[]) {
  Glib::init();
  return Catch::Session().run(argc, argv);
}
