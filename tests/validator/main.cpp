// Test runner for the validator/XML-reader suites. Running the executable with no arguments runs
// the whole doctest suite and exits non-zero on any failure (so CTest / CI can gate on it). The
// targets under test are pure XmlInspector/Bullet-math + pugixml + STL — no CommonLibSSE, no game.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
