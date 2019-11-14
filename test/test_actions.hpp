#pragma once
#include <catch.hpp>
#include <argparse.hpp>
#include <string_view>

TEST_CASE("Users can use default value inside actions", "[actions]") {
  argparse::ArgumentParser program("test");
  program.add_argument("input")
    .default_value("bar")
    .action([=](const std::string& value) {
      static const std::vector<std::string> choices = { "foo", "bar", "baz" };
      if (std::find(choices.begin(), choices.end(), value) != choices.end()) {
        return value;
      }
      return std::string{ "bar" };
    });

  program.parse_args({ "test", "fez" });
  REQUIRE(program.get("input") == "bar");
}

TEST_CASE("Users can add actions that return nothing", "[actions]") {
  argparse::ArgumentParser program("test");
  bool pressed = false;
  auto &arg = program.add_argument("button").action(
      [&](const std::string &) mutable { pressed = true; });

  REQUIRE_FALSE(pressed);

  SECTION("action performed") {
    program.parse_args({"test", "ignored"});
    REQUIRE(pressed);
  }

  SECTION("action performed and nothing overrides the default value") {
    arg.default_value(42);

    program.parse_args({"test", "ignored"});
    REQUIRE(pressed);
    REQUIRE(program.get<int>("button") == 42);
  }
}

class Image {
public:
  int w = 0, h = 0;

  void resize(std::string_view geometry) {
    std::stringstream s;
    s << geometry;
    s >> w;
    s.ignore();
    s >> h;
  }

  static auto create(int w, int h, std::string_view format) -> Image {
    auto factor = [=] {
      if (format == "720p")
        return std::min(1280. / w, 720. / h);
      else if (format == "1080p")
        return std::min(1920. / w, 1080. / h);
      else
        return 1.;
    }();

    return {static_cast<int>(w * factor), static_cast<int>(h * factor)};
  }
};

TEST_CASE("Users can bind arguments to actions", "[actions]") {
  argparse::ArgumentParser program("test");

  GIVEN("an default initialized object bounded by reference") {
    Image img;
    program.add_argument("--size").action(&Image::resize, std::ref(img));

    WHEN("provided no command-line arguments") {
      program.parse_args({"test"});

      THEN("the object is not updated") {
        REQUIRE(img.w == 0);
        REQUIRE(img.h == 0);
      }
    }

    WHEN("provided command-line arguments") {
      program.parse_args({"test", "--size", "320x98"});

      THEN("the object is updated accordingly") {
        REQUIRE(img.w == 320);
        REQUIRE(img.h == 98);
      }
    }
  }

  GIVEN("a command-line option waiting for the last argument in its action") {
    program.add_argument("format").action(Image::create, 400, 300);

    WHEN("provided such an argument") {
      program.parse_args({"test", "720p"});

      THEN("the option object is created as if providing all arguments") {
        auto img = program.get<Image>("format");

        REQUIRE(img.w == 960);
        REQUIRE(img.h == 720);
      }
    }

    WHEN("provided a different argument") {
      program.parse_args({"test", "1080p"});

      THEN("a different option object is created") {
        auto img = program.get<Image>("format");

        REQUIRE(img.w == 1440);
        REQUIRE(img.h == 1080);
      }
    }
  }
}
