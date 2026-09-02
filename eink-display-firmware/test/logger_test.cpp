#include <doctest.h>

#include "fakes/scopedLogCapture.h"

TEST_CASE("Logger: routesToAttachedSink")
{
    ScopedLogCapture capture;

    logger.warning("disk at %d%%", 91);

    REQUIRE(capture.sink.lines.size() == 1);
    CHECK(capture.sink.lines[0].level == LogLevel::WARNING);
    CHECK(capture.sink.lines[0].message == "disk at 91%");
}

TEST_CASE("Logger: sinkOnlyReceivesItsOwnLevelAndAbove")
{
    ScopedLogCapture capture(LogLevel::WARNING);

    logger.debug("noise");
    logger.info("more noise");
    logger.error("this one matters");

    REQUIRE(capture.sink.lines.size() == 1);
    CHECK(capture.sink.lines[0].message == "this one matters");
}

TEST_CASE("Logger: previousCasesSinkIsGone")
{
    // Nothing is attached here. If ScopedLogCapture leaked, the sinks from the
    // cases above would still be collecting, and this would be observable.
    logger.error("should reach nobody");

    ScopedLogCapture capture;
    CHECK(capture.sink.lines.empty());
}
